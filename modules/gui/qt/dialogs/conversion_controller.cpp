// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "conversion_controller.hpp"
#include "util/soutchain.hpp"
#include <vlc_input_item.h>
#include <vlc_player.h>
#include <vlc_configuration.h>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTemporaryDir>
#include <QThread>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>
#ifdef _WIN32
# include <qt_windows.h>
#else
# include <unistd.h>
#endif

namespace {
constexpr qint64 MaximumSourceBytes = 512ll * 1024 * 1024;
constexpr qint64 MaximumOutputBytes = 512ll * 1024 * 1024;
constexpr qint64 MaximumValidationBytes = 64ll * 1024 * 1024;
constexpr qint64 MaximumStageMs = 120000;
constexpr vlc_tick_t MaximumDuration = VLC_TICK_FROM_SEC(3600);

bool localPath(const QUrl &url)
{
    // Reject remote hosts and Windows device/UNC paths before QFileInfo does IO.
    const QString path = url.toLocalFile();
    const auto separator = [](QChar c) { return c == '/' || c == '\\'; };
    if (!url.isValid() || !url.isLocalFile() || !url.host().isEmpty()
        || !url.query().isEmpty() || !url.fragment().isEmpty()
        || path.isEmpty() || path.contains(QChar::Null)
        || (path.size() >= 2 && separator(path[0]) && separator(path[1])))
        return false;
    const auto normalized = QDir::fromNativeSeparators(path);
    if (normalized.size() >= 2 && normalized[0].isLetter() && normalized[1] == ':'
        && (normalized.size() < 3 || normalized[2] != '/'))
        return false; // Windows drive-relative C:filename/bare C: are not rooted
    return !normalized.startsWith("//") && QDir::isAbsolutePath(normalized);
}

QString fileKey(const QString &path)
{
    const QFileInfo info(path);
    return QString::number(info.size()) + ':'
        + QString::number(info.lastModified().toMSecsSinceEpoch());
}

bool publishNoReplace(const QString &from, const QString &to)
{
    // QFile::rename may fall back to copying. Both paths are on the same local
    // filesystem; use an OS operation that never replaces a racing destination.
#ifdef _WIN32
    return MoveFileW(reinterpret_cast<LPCWSTR>(from.utf16()),
                     reinterpret_cast<LPCWSTR>(to.utf16())) != 0;
#else
    const auto source = QFile::encodeName(from), destination = QFile::encodeName(to);
    if (::link(source.constData(), destination.constData()) != 0) return false;
    ::unlink(source.constData());
    return true;
#endif
}

struct NativeState {
    std::atomic_bool started{false}, stopped{false}, failed{false};
    std::atomic_int reason{-1};
    std::atomic<vlc_tick_t> time{VLC_TICK_INVALID}, length{VLC_TICK_INVALID};
};
const vlc_player_cbs Callbacks = [] {
    vlc_player_cbs callbacks{};
    callbacks.on_state_changed = [](vlc_player_t *, vlc_player_state state, void *data) {
        auto &status = *static_cast<NativeState *>(data);
        if (state == VLC_PLAYER_STATE_STARTED) status.started = true;
        if (state == VLC_PLAYER_STATE_STOPPED && status.started) status.stopped = true;
    };
    callbacks.on_error_changed = [](vlc_player_t *, vlc_player_error error, void *data) {
        if (error != VLC_PLAYER_ERROR_NONE) static_cast<NativeState *>(data)->failed = true;
    };
    callbacks.on_stopping_current_media = [](vlc_player_t *, input_item_t *,
            vlc_player_media_stopping_reason reason, void *data) {
        int unset = -1;
        static_cast<NativeState *>(data)->reason.compare_exchange_strong(unset, reason);
    };
    callbacks.on_position_changed = [](vlc_player_t *, vlc_tick_t time, double, void *data) {
        static_cast<NativeState *>(data)->time = time;
    };
    callbacks.on_length_changed = [](vlc_player_t *, vlc_tick_t length, void *data) {
        static_cast<NativeState *>(data)->length = length;
    };
    return callbacks;
}();

QString chain(const QString &output, bool validate)
{
    SoutChain sout("#");
    auto &transcode = sout.begin("transcode");
    // These are the native built-in Audio - FLAC profile's codec/channel/rate
    // values, not an interpretation of arbitrary user-saved codec strings.
    transcode.option("acodec", validate ? "s16l" : "flac", false);
    transcode.option("channels", validate ? "1" : "2", false);
    transcode.option("samplerate", validate ? "8000" : "44100", false);
    if (!validate) transcode.option("ab", "128", false);
    SoutModule access("file");
    access.option("no-overwrite");
    access.option("no-format"); // Both owned stage paths are literal, never strftime templates.
    auto &sink = sout.begin("std");
    sink.option("access", access);
    sink.option("mux", validate ? "raw" : "flac", false);
    // false asks SoutChain to perform native string escaping. true would mean
    // the value was already escaped and would break quoted destination paths.
    sink.option("dst", output, false);
    return sout.to_string();
}
}

struct ConversionController::Job {
    QString preferredEncoder = QStringLiteral("any");
    std::atomic_bool cancel{false}, nativeActive{false};
    std::atomic<State> phase{Starting};
    std::atomic<vlc_tick_t> time{VLC_TICK_INVALID}, length{VLC_TICK_INVALID};
    std::mutex publication;
    bool published = false; // guarded by publication, including cancel()
};
struct ConversionController::Result {
    State state = Failed;
    QString error;
    QUrl output;
    qint64 durationMs = 0;
};

ConversionController::ConversionController(vlc_object_t *parent, QObject *objectParent)
    : QObject(objectParent), m_parent(parent)
{
    m_pool.setMaxThreadCount(1);
    m_poll.setInterval(30);
    connect(&m_poll, &QTimer::timeout, this, &ConversionController::poll);
}

ConversionController::~ConversionController()
{
    cancel();
    m_poll.stop();
    m_pool.waitForDone(); // parent and callback storage remain alive through Delete
}

bool ConversionController::busy() const
{
    return m_state == Starting || m_state == Encoding || m_state == Validating
        || m_state == Publishing || m_state == Cancelling;
}

bool ConversionController::nativeInputActive() const
{
    return m_job && m_job->nativeActive;
}

QString ConversionController::validate(QString *source, QString *destination) const
{
    if (!localPath(m_source) || !localPath(m_destination))
        return tr("Choose a local audio source and a local FLAC destination.");
    const QFileInfo input(m_source.toLocalFile()), output(m_destination.toLocalFile());
    static const QSet<QString> extensions {"wav", "flac", "mp3", "ogg", "opus", "m4a", "aac", "aiff", "aif", "wma"};
    if (!input.isFile() || !input.isReadable() || input.size() <= 0
        || !extensions.contains(input.suffix().toLower()))
        return tr("Choose a readable local audio file in a supported format.");
    if (input.size() > MaximumSourceBytes)
        return tr("This conversion supports audio sources up to 512 MiB.");
    if (output.suffix().compare("flac", Qt::CaseInsensitive) != 0)
        return tr("The destination must have a .flac extension.");
    const QFileInfo directory(output.absolutePath());
    if (!directory.isDir() || !directory.isWritable())
        return tr("Choose an existing writable destination folder.");
    const QString inputPath = input.canonicalFilePath();
    const QString directoryPath = directory.canonicalFilePath();
    // QFileInfo may have cached isFile/isDir before a concurrent deletion.
    // QDir("") would otherwise turn the result into a relative cwd pathname.
    if (inputPath.isEmpty() || directoryPath.isEmpty()
        || !QDir::isAbsolutePath(inputPath) || !QDir::isAbsolutePath(directoryPath))
        return tr("The source or destination folder is no longer available.");
    const QString outputPath = QDir(directoryPath).filePath(output.fileName());
    if (inputPath.startsWith("//") || inputPath.startsWith("\\\\")
        || outputPath.startsWith("//") || outputPath.startsWith("\\\\"))
        return tr("Network paths are unavailable for this conversion.");
#ifdef _WIN32
    const bool same = inputPath.compare(outputPath, Qt::CaseInsensitive) == 0;
#else
    const bool same = inputPath == outputPath;
#endif
    if (same) return tr("The destination must differ from the source.");
    // isSymLink catches dangling links, which exists() deliberately omits.
    if (output.exists() || output.isSymLink())
        return tr("The destination already exists. Choose a new filename.");
    if (source) *source = inputPath;
    if (destination) *destination = outputPath;
    return {};
}

bool ConversionController::canStart() const
{
    return !busy() && !m_source.isEmpty() && !m_destination.isEmpty() && validate().isEmpty();
}

void ConversionController::setSourceUrl(const QUrl &source)
{
    if (busy() || source == m_source) return;
    m_source = source; reset();
}

void ConversionController::setDestinationUrl(const QUrl &destination)
{
    if (busy() || destination == m_destination) return;
    m_destination = destination; reset();
}

void ConversionController::reset()
{
    if (busy()) return;
    ++m_generation;
    m_job.reset();
    m_state = Idle; m_output = QUrl{}; m_error.clear();
    m_progress = -1; m_time = 0; m_duration = 0;
    emit changed();
}

bool ConversionController::start()
{
    if (busy()) return false;
    QString source, destination;
    const auto error = validate(&source, &destination);
    if (!error.isEmpty()) {
        m_state = Failed; m_error = error; m_output = QUrl{}; emit changed(); return false;
    }
    ++m_generation;
    m_output = QUrl{}; m_error.clear(); m_progress = -1; m_time = 0; m_duration = 0;
    const auto generation = m_generation;
    const auto job = m_job = std::make_shared<Job>();
    if (m_parent && config_GetType("sout-transcode-preferred-encoder") == VLC_VAR_STRING) {
        char *preferred = var_InheritString(m_parent, "sout-transcode-preferred-encoder");
        if (preferred && *preferred) job->preferredEncoder = QString::fromUtf8(preferred);
        free(preferred);
    }
    m_state = Starting; emit changed(); m_poll.start();
    m_pool.start([this, source, destination, generation, job] {
        auto result = convert(source, destination, job);
        QMetaObject::invokeMethod(this, [this, generation, result = std::move(result)] {
            if (generation != m_generation) return;
            poll(); m_poll.stop();
            m_state = result.state; m_error = result.error; m_output = result.output;
            if (result.durationMs > 0) m_duration = result.durationMs;
            if (m_state == Succeeded) { m_progress = 1; m_time = m_duration; }
            emit changed();
        }, Qt::QueuedConnection);
    });
    return true;
}

void ConversionController::cancel()
{
    if (!busy() || !m_job) return;
    {
        std::lock_guard<std::mutex> guard(m_job->publication);
        if (m_job->published) return; // publication already won; completion is queued
        m_job->cancel = true;
    }
    m_state = Cancelling;
    emit changed();
}

void ConversionController::poll()
{
    if (!m_job) return;
    if (m_state != Cancelling) m_state = m_job->phase;
    const auto time = m_job->time.load(), length = m_job->length.load();
    m_time = time == VLC_TICK_INVALID ? 0 : MS_FROM_VLC_TICK(time);
    m_duration = length == VLC_TICK_INVALID ? 0 : MS_FROM_VLC_TICK(length);
    m_progress = length > 0 && time != VLC_TICK_INVALID
        ? std::clamp(double(time) / length, 0., .999) : -1;
    emit changed();
}

ConversionController::Result ConversionController::convert(const QString &source,
        const QString &destination, const std::shared_ptr<Job> &job)
{
    const auto cancelled = [] { return Result{Cancelled, {}, {}, 0}; };
    if (job->cancel) return cancelled();
    const auto sourceKey = fileKey(source);
    QTemporaryDir directory(QFileInfo(destination).absolutePath() + "/.vlc-convert-XXXXXX");
    if (!directory.isValid()) return {Failed, tr("Could not create conversion storage in the destination folder.")};
    const auto encoded = directory.filePath("audio.flac"), decoded = directory.filePath("validation.pcm");

    // This lambda owns all player/listener/media resources on this worker. No
    // callback touches QObject or obtains an external mutex. All native calls
    // requiring the player lock occur while locked; Delete occurs unlocked.
    auto stage = [&](const QString &input, const QString &output, bool validating) -> QString {
        NativeState status;
        const auto uri = QUrl::fromLocalFile(input).toEncoded();
        input_item_t *media = input_item_New(uri.constData(), nullptr);
        if (!media) return tr("Could not allocate a conversion source.");
        const auto sout = (QString(":sout=") + chain(output, validating)).toUtf8();
        bool options = input_item_AddOption(media, sout.constData(), VLC_INPUT_OPTION_TRUSTED) == VLC_SUCCESS;
        // A fresh input has no primary-item options; override inherited input
        // start/stop/repeat/filter state and disable all playback/display output.
        for (const char *option : {":no-sout-keep", ":sout-audio", ":no-sout-video", ":no-sout-spu",
                                  ":no-sout-all", ":sout-transcode-afilter=", ":sout-transcode-aenc=none",
                                  ":audio", ":no-video", ":no-spu", ":no-sub-autodetect-file",
                                  ":audio-filter=", ":demux=", ":demux-filter=", ":stream-filter=",
                                  ":input-slave=", ":sub-file=", ":audio-track-id=", ":audio-track=0",
                                  ":rate=1", ":no-play-and-pause",
                                  ":restore-playback-pos=0", ":no-restore-playback-states", ":no-save-recentplay",
                                  ":start-time=0", ":stop-time=0", ":run-time=0", ":input-repeat=0"})
            options &= input_item_AddOption(media, option, VLC_INPUT_OPTION_TRUSTED) == VLC_SUCCESS;
        // The public player type is opaque. Own a real VLC configuration
        // object beneath the borrowed interface/native test parent instead of
        // casting player storage or changing unrelated playback preferences.
        const auto deleteConfiguration = [](vlc_object_t *object) { vlc_object_delete(object); };
        std::unique_ptr<vlc_object_t, decltype(deleteConfiguration)> configuration(
            options && m_parent ? static_cast<vlc_object_t *>(vlc_object_create(m_parent, sizeof(vlc_object_t))) : nullptr,
            deleteConfiguration);
        if (!configuration)
        { input_item_Release(media); return tr("Could not allocate the independent conversion configuration."); }
        // Empty string input-item options are ignored by var_OptionParse.
        // Define them on the owned parent so input/player/resource inheritance
        // stops here, before any independent player or input is created.
        bool preferences = var_Create(configuration.get(), "sout-transcode-aenc", VLC_VAR_STRING) == VLC_SUCCESS
            && var_SetString(configuration.get(), "sout-transcode-aenc", "none") == VLC_SUCCESS;
        // Keep this job's encoder policy through its native stages. Internal
        // PCM validation uses ordinary selection independently of that policy.
        const auto preferred = (validating ? QStringLiteral("any") : job->preferredEncoder).toUtf8();
        preferences = preferences
            && var_Create(configuration.get(), "sout-transcode-preferred-encoder", VLC_VAR_STRING) == VLC_SUCCESS
            && var_SetString(configuration.get(), "sout-transcode-preferred-encoder", preferred.constData()) == VLC_SUCCESS;
        for (const char *name : {"sout-transcode-afilter", "audio-filter", "demux",
                                 "demux-filter", "stream-filter", "input-slave",
                                 "sub-file", "audio-track-id"})
            preferences = preferences
                && var_Create(configuration.get(), name, VLC_VAR_STRING) == VLC_SUCCESS
                && var_SetString(configuration.get(), name, "") == VLC_SUCCESS;
        auto *player = preferences ? vlc_player_New(configuration.get(), VLC_PLAYER_LOCK_NORMAL) : nullptr;
        if (!player) { input_item_Release(media); return tr("Could not allocate the independent conversion player."); }
        vlc_player_Lock(player);
        auto *listener = vlc_player_AddListener(player, &Callbacks, &status);
        vlc_player_SetStartPaused(player, false);
        vlc_player_SetPauseOnCork(player, false);
        vlc_player_SetPlayAndPause(player, false);
        vlc_player_SetRepeatCount(player, 0);
        vlc_player_ChangeRate(player, 1.f);
        const bool started = preferences && listener && vlc_player_SetCurrentMedia(player, media) == VLC_SUCCESS
            && vlc_player_Start(player) == VLC_SUCCESS;
        vlc_player_Unlock(player);
        input_item_Release(media);
        job->phase = validating ? Validating : Encoding;
        QElapsedTimer timer; timer.start();
        QString error;
        while (started && !status.stopped && !status.failed && !job->cancel) {
            job->nativeActive = status.started.load();
            if (!validating) { job->time = status.time.load(); job->length = status.length.load(); }
            if (timer.elapsed() > MaximumStageMs) { error = tr("Conversion exceeded its two-minute processing limit."); break; }
            if (status.length > MaximumDuration) { error = tr("This conversion supports audio up to one hour long."); break; }
            if (QFileInfo(output).size() > (validating ? MaximumValidationBytes : MaximumOutputBytes)) {
                error = tr("Conversion exceeded its temporary output limit."); break;
            }
            QThread::msleep(5);
        }
        if (!validating) { job->time = status.time.load(); job->length = status.length.load(); }
        vlc_player_Lock(player);
        vlc_player_Stop(player);
        if (listener) vlc_player_RemoveListener(player, listener);
        vlc_player_Unlock(player);
        vlc_player_Delete(player); // synchronously drains/closes this stage's output
        configuration.reset(); // The owned parent outlives every player/resource callback.
        job->nativeActive = false;
        if (job->cancel) return {};
        if (!error.isEmpty()) return error;
        if (!started || status.failed || status.reason != VLC_PLAYER_MEDIA_STOPPING_EOS)
            return tr("The audio could not be converted. Check the source and available codecs.");
        return {};
    };

    QString error = stage(source, encoded, false);
    if (job->cancel) return cancelled();
    if (!error.isEmpty()) return {Failed, error};
    if (QFileInfo(encoded).size() <= 4 || QFileInfo(encoded).size() > MaximumOutputBytes)
        return {Failed, tr("Conversion did not produce a complete FLAC file.")};
    error = stage(encoded, decoded, true);
    if (job->cancel) return cancelled();
    if (!error.isEmpty()) return {Failed, tr("The converted file could not be decoded: %1").arg(error)};
    const auto bytes = QFileInfo(decoded).size();
    if (bytes <= 0 || bytes % 2 || bytes > MaximumValidationBytes)
        return {Failed, tr("The converted file contains no complete decodable audio.")};
    const auto length = job->length.load();
    const auto decodedDuration = VLC_TICK_FROM_MS(bytes * 1000 / (8000 * 2));
    if (decodedDuration > MaximumDuration)
        return {Failed, tr("This conversion supports audio up to one hour long.")};
    // Source duration can be unknown. When it is known, reject a materially
    // truncated file rather than treating a tiny playable prefix as success.
    if (length > 0 && std::abs(double(decodedDuration - length)) > std::max<double>(VLC_TICK_FROM_MS(150), length * .01))
        return {Failed, tr("The converted audio duration differs from the source.")};
    if (fileKey(source) != sourceKey)
        return {Failed, tr("The source changed during conversion. Try again with an unchanged file.")};
    job->phase = Publishing;
    {
        std::lock_guard<std::mutex> guard(job->publication);
        if (job->cancel) return cancelled();
        if (!publishNoReplace(encoded, destination))
            return {Failed, tr("Could not publish the result. The destination may already exist or be unwritable.")};
        job->published = true;
    }
    return {Succeeded, {}, QUrl::fromLocalFile(destination), MS_FROM_VLC_TICK(decodedDuration)};
}
