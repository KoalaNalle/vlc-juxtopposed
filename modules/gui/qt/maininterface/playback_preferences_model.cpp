// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "playback_preferences_model.hpp"
#include "qt.hpp"
#include "util/checked_preferences_store.hpp"
#include <vlc_configuration.h>
#include <vlc_iso_lang.h>
#include <vlc_playlist.h>
#include <vlc_player.h>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QPointer>
#include <QRegularExpression>
#include <cmath>
#include <climits>

namespace {
using vlc_player_locker = vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock>;
using vlc_playlist_locker = vlc_locker<vlc_playlist_t, vlc_playlist_Lock, vlc_playlist_Unlock>;
struct Option { const char *key, *native; int type; };
constexpr Option options[] = {
    {"continuePlayback", "restore-playback-pos", VLC_VAR_INTEGER},
    {"startPlayback", "start-paused", VLC_VAR_BOOL},
    {"defaultPlaybackSpeed", "rate", VLC_VAR_FLOAT},
    {"pauseOtherAudio", "playlist-cork", VLC_VAR_BOOL},
    {"preferredVideoQuality", "preferred-resolution", VLC_VAR_INTEGER},
    {"preferredMenuLanguage", "menu-language", VLC_VAR_STRING},
    {"defaultAudioTrack", "audio-track", VLC_VAR_INTEGER},
    {"defaultSubtitleTrack", "sub-track", VLC_VAR_INTEGER},
    {"repeatMode", "repeat", VLC_VAR_BOOL},
    {"shuffle", "random", VLC_VAR_BOOL},
    {"stopAfterEachItem", "play-and-stop", VLC_VAR_BOOL},
    {"finishAction", "play-and-exit", VLC_VAR_BOOL},
    {"preciseSeeking", "input-fast-seek", VLC_VAR_BOOL},
    {"defaultOpticalDrive", "dvd", VLC_VAR_STRING},
    {"aviRepair", "avi-index", VLC_VAR_INTEGER},
    {"mkvPreload", "mkv-preload-local-dir", VLC_VAR_BOOL},
    {"timeShiftFolder", "input-timeshift-path", VLC_VAR_STRING},
    {"timeShiftSize", "input-timeshift-granularity", VLC_VAR_INTEGER}
};
QString nativeText(const char *name)
{
    if (config_GetType(name) != VLC_VAR_STRING) return {};
    char *raw = config_GetPsz(name);
    const auto value = QString::fromUtf8(raw ? raw : ""); free(raw); return value;
}
bool integral(const QVariant &input, qlonglong lo, qlonglong hi, qlonglong &out)
{
    switch (input.typeId()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong: case QMetaType::ULongLong:
    case QMetaType::Float: case QMetaType::Double: break;
    default: return false;
    }
    const auto value = input.toDouble();
    if (!std::isfinite(value) || std::floor(value) != value || value < lo || value > hi) return false;
    out = qlonglong(value); return true;
}
bool numericRate(const QVariant &input, float &out)
{
    switch (input.typeId()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong: case QMetaType::ULongLong:
    case QMetaType::Float: case QMetaType::Double: break;
    default: return false;
    }
    const auto value = input.toDouble();
    if (!std::isfinite(value) || value < .25 || value > 2. || std::floor(value * 4.) != value * 4.) return false;
    out = float(value); return true;
}
bool validText(const QString &value)
{
    if (QString::fromUtf8(value.toUtf8()) != value || value.toUtf8().size() > 32760) return false;
    for (const auto ch : value) if (ch.unicode() < 32 || ch.unicode() == 127) return false;
    return true;
}
QVariantList integerChoices(const char *name)
{
    if (config_GetType(name) != VLC_VAR_INTEGER) return {};
    int64_t *values = nullptr; char **names = nullptr;
    const auto count = config_GetIntChoices(name, &values, &names);
    QVariantList result;
    for (ssize_t i = 0; i < count; ++i) {
        result.append(QVariantMap{{QStringLiteral("value"), qlonglong(values[i])},
            {QStringLiteral("name"), QString::fromUtf8(names[i] ? names[i] : "")}});
        free(names[i]);
    }
    free(names); free(values); return result;
}
bool listed(const QVariantList &choices, qlonglong value)
{
    for (const auto &row : choices) if (row.toMap().value(QStringLiteral("value")).toLongLong() == value) return true;
    return false;
}
bool validFolder(const QString &value)
{
    if (value.isEmpty()) return true; // Native system temporary directory.
    const QFileInfo info(value);
    return validText(value) && !value.contains(QStringLiteral("://")) && info.isAbsolute()
        && info.isDir() && info.isReadable() && info.isWritable();
}
QVariantList languages()
{
    QVariantList result { QVariantMap{{QStringLiteral("value"), QString{}}, {QStringLiteral("name"), QObject::tr("Automatic")}},
        QVariantMap{{QStringLiteral("value"), QStringLiteral("any")}, {QStringLiteral("name"), QObject::tr("Any language")}} };
    for (char a = 'a'; a <= 'z'; ++a) for (char b = 'a'; b <= 'z'; ++b) {
        const char code[] = {a, b, 0};
        const auto language = vlc_find_iso639(code, false);
        if (!language || QByteArray(language->psz_iso639_1) != code) continue;
        result.append(QVariantMap{{QStringLiteral("value"), QString::fromLatin1(code)},
            {QStringLiteral("name"), QString::fromUtf8(language->psz_eng_name)}});
    }
    return result;
}
}

PlaybackPreferencesModel::PlaybackPreferencesModel(qt_intf_t *intf, QObject *parent)
    : PlaybackPreferencesModel(intf, Consumers{}, parent) {}
PlaybackPreferencesModel::PlaybackPreferencesModel(qt_intf_t *intf, Consumers consumers, QObject *parent)
    : QObject(parent), m_intf(intf), m_consumers(consumers)
{
    m_choices.insert(QStringLiteral("continuePlayback"), integerChoices("restore-playback-pos"));
    m_choices.insert(QStringLiteral("preferredVideoQuality"), integerChoices("preferred-resolution"));
    m_choices.insert(QStringLiteral("aviRepair"), integerChoices("avi-index"));
    m_choices.insert(QStringLiteral("preferredMenuLanguage"), languages());
    m_choices.insert(QStringLiteral("repeatMode"), QVariantList{
        QVariantMap{{QStringLiteral("value"), 0}, {QStringLiteral("name"), tr("Off")}},
        QVariantMap{{QStringLiteral("value"), 1}, {QStringLiteral("name"), tr("One item")}},
        QVariantMap{{QStringLiteral("value"), 2}, {QStringLiteral("name"), tr("All items")}}});
    m_choices.insert(QStringLiteral("finishAction"), QVariantList{
        QVariantMap{{QStringLiteral("value"), 0}, {QStringLiteral("name"), tr("Stop")}},
        QVariantMap{{QStringLiteral("value"), 1}, {QStringLiteral("name"), tr("Quit VLC")}}});
    m_startupRate = config_GetType("rate") == VLC_VAR_FLOAT ? config_GetFloat("rate") : 1.f;
    m_startupCork = config_GetType("playlist-cork") == VLC_VAR_BOOL && config_GetInt("playlist-cork") != 0;
    refresh();
}
PlaybackPreferencesModel::~PlaybackPreferencesModel() = default;
void PlaybackPreferencesModel::setError(const QString &error)
{ if (error != m_error) { m_error = error; emit lastErrorChanged(); } }
bool PlaybackPreferencesModel::fail(const QString &error) { setError(error); return false; }
void PlaybackPreferencesModel::refresh()
{
    QVariantMap values, caps;
    for (const auto &option : options) {
        const auto key = QString::fromLatin1(option.key);
        const bool present = config_GetType(option.native) == option.type;
        caps[key] = m_intf && present;
        if (option.type == VLC_VAR_STRING) values[key] = nativeText(option.native);
        else if (option.type == VLC_VAR_FLOAT) values[key] = present ? config_GetFloat(option.native) : 1.f;
        else if (option.type == VLC_VAR_BOOL) values[key] = present && config_GetInt(option.native) != 0;
        else values[key] = qlonglong(present ? config_GetInt(option.native) : -1);
    }
    values[QStringLiteral("startPlayback")] = !values.value(QStringLiteral("startPlayback")).toBool();
    values[QStringLiteral("preciseSeeking")] = !values.value(QStringLiteral("preciseSeeking")).toBool();
    values[QStringLiteral("repeatMode")] = config_GetInt("repeat") ? 1 : config_GetInt("loop") ? 2 : 0;
    values[QStringLiteral("finishAction")] = config_GetInt("play-and-exit") ? 1 : 0;
    values[QStringLiteral("showThumbnails")] = false;
    caps[QStringLiteral("showThumbnails")] = false; // No real timeline thumbnail renderer/provider.
    caps[QStringLiteral("preciseSeeking")] = caps.value(QStringLiteral("preciseSeeking")).toBool() && m_consumers.preciseSeeking;
    caps[QStringLiteral("repeatMode")] = caps.value(QStringLiteral("repeatMode")).toBool()
        && config_GetType("loop") == VLC_VAR_BOOL && m_intf && m_intf->p_playlist;
    caps[QStringLiteral("shuffle")] = caps.value(QStringLiteral("shuffle")).toBool() && m_intf && m_intf->p_playlist;
    for (const auto &key : {"startPlayback", "pauseOtherAudio"})
        caps[QString::fromLatin1(key)] = caps.value(QString::fromLatin1(key)).toBool() && m_intf && m_intf->p_player;
    caps[QStringLiteral("finishAction")] = caps.value(QStringLiteral("finishAction")).toBool() && m_intf && m_intf->p_playlist;
    caps[QStringLiteral("stopAfterEachItem")] = caps.value(QStringLiteral("stopAfterEachItem")).toBool()
        && m_intf && m_intf->p_playlist && values.value(QStringLiteral("finishAction")).toInt() == 0;
    caps[QStringLiteral("defaultAudioTrack")] = caps.value(QStringLiteral("defaultAudioTrack")).toBool()
        && config_GetType("audio-track-id") == VLC_VAR_STRING;
    caps[QStringLiteral("defaultSubtitleTrack")] = caps.value(QStringLiteral("defaultSubtitleTrack")).toBool()
        && config_GetType("sub-track-id") == VLC_VAR_STRING;
    caps[QStringLiteral("defaultOpticalDrive")] = caps.value(QStringLiteral("defaultOpticalDrive")).toBool()
        && config_GetType("vcd") == VLC_VAR_STRING;
    if (nativeText("dvd") != nativeText("vcd") || (config_GetType("cd-audio") == VLC_VAR_STRING
        && nativeText("dvd") != nativeText("cd-audio"))) values[QStringLiteral("defaultOpticalDrive")] = QVariant{};
    for (const auto &key : {"continuePlayback", "preferredVideoQuality", "aviRepair"})
        caps[QString::fromLatin1(key)] = caps.value(QString::fromLatin1(key)).toBool()
            && !m_choices.value(QString::fromLatin1(key)).toList().isEmpty();
    if (caps != m_capabilities) { m_capabilities = caps; emit capabilitiesChanged(); }
    if (values != m_values) { m_values = values; emit valuesChanged(); }
    const bool restart = (config_GetType("rate") == VLC_VAR_FLOAT && config_GetFloat("rate") != m_startupRate)
        || (config_GetType("playlist-cork") == VLC_VAR_BOOL && (config_GetInt("playlist-cork") != 0) != m_startupCork);
    if (restart != m_restartRequired) { m_restartRequired = restart; emit restartRequiredChanged(); }
}
QVariantMap PlaybackPreferencesModel::applyModes() const
{
    QVariantMap modes;
    for (const auto &option : options) modes[QString::fromLatin1(option.key)] = QStringLiteral("nextInput");
    for (const auto &key : {"startPlayback", "repeatMode", "shuffle", "stopAfterEachItem", "finishAction", "preciseSeeking"})
        modes[QString::fromLatin1(key)] = QStringLiteral("live");
    modes[QStringLiteral("defaultPlaybackSpeed")] = QStringLiteral("restart");
    modes[QStringLiteral("pauseOtherAudio")] = QStringLiteral("restart");
    modes[QStringLiteral("showThumbnails")] = QStringLiteral("unavailable");
    return modes;
}
QVariantMap PlaybackPreferencesModel::supportNotes() const
{
    return {{QStringLiteral("continuePlayback"), tr("Uses saved video positions in the media library. Remembered positions must be enabled; matching Privacy exclusions can prevent restore. Ask is preserved as a distinct choice.")},
        {QStringLiteral("startPlayback"), tr("Applies to subsequent media starts; it does not pause or resume the current item. Command-line queue autostart is a separate setting.")},
        {QStringLiteral("defaultPlaybackSpeed"), tr("The saved default takes effect after restart. The current session's speed controls and remembered per-media playback states can override it.")},
        {QStringLiteral("pauseOtherAudio"), tr("After restart, pauses and resumes for audio communication/ducking events reported by the audio backend. It does not detect every other application's ordinary audio.")},
        {QStringLiteral("defaultAudioTrack"), tr("Zero-based stream index; -1 means automatic. Choosing an index clears a saved audio-track ID override. Current media tracks are unchanged.")},
        {QStringLiteral("defaultSubtitleTrack"), tr("Zero-based stream index; -1 means automatic. Choosing an index clears a saved subtitle-track ID override. This does not disable subtitles.")},
        {QStringLiteral("preferredMenuLanguage"), tr("DVD/Blu-ray menu language, using native ISO language codes. Existing multi-language lists are preserved until an explicit choice.")},
        {QStringLiteral("preferredVideoQuality"), tr("Preferred maximum height when a source offers multiple renditions; this does not upscale or change every local video.")},
        {QStringLiteral("finishAction"), tr("Quit VLC uses native play-and-exit, which also quits after a manual Stop. It overrides Stop after each item while selected; no sleep or shutdown action is implemented.")},
        {QStringLiteral("stopAfterEachItem"), tr("Stops before advancing to the next queued item. Native Quit VLC takes precedence; choose Stop as the finish action to edit this option.")},
        {QStringLiteral("preciseSeeking"), tr("Requires the paired native Qt timeline/time-seek consumer. Without that linked consumer this control is unavailable.")},
        {QStringLiteral("showThumbnails"), tr("The timeline has no implemented thumbnail preview provider; this control is unavailable.")},
        {QStringLiteral("defaultOpticalDrive"), tr("Sets the shared DVD/VCD/audio-CD default. A mixed existing set remains unselected until you explicitly choose a path; empty uses native defaults.")},
        {QStringLiteral("timeShiftFolder"), tr("Existing writable local directory for future time-shift temporary files; empty uses the system temporary directory.")},
        {QStringLiteral("timeShiftSize"), tr("Maximum bytes per native temporary-file chunk, not a total buffer limit. -1 uses the native 50 MiB chunk default; explicit sizes range from 1 MiB to INT_MAX bytes.")}};
}
void PlaybackPreferencesModel::reload() { setError({}); refresh(); if (!m_intf) setError(tr("The native Playback preferences service is unavailable.")); }
bool PlaybackPreferencesModel::saveRate(float value)
{
    // The immutable shared store deliberately handles only bool/int/text.
    // This single native FLOAT option has a separately bounded transaction.
    const float before = config_GetFloat("rate");
    config_PutFloat("rate", value);
    if (config_GetFloat("rate") != value) { config_PutFloat("rate", before); return fail(tr("The native default speed could not be updated.")); }
    QFile file(CheckedPreferencesStore(m_intf).configFile());
    bool saved = config_SaveConfigFile(m_intf) == 0 && file.open(QIODevice::ReadOnly);
    QByteArray bytes;
    if (saved) { bytes = file.readAll(); saved = file.error() == QFile::NoError; file.close(); }
    bool found = false;
    if (saved) for (auto line : bytes.split('\n')) {
        if (line.endsWith('\r')) line.chop(1);
        if (line.startsWith('#')) line.remove(0, 1);
        if (!line.startsWith("rate=")) continue;
        bool valid = false; const auto actual = line.mid(5).toDouble(&valid);
        found = valid && actual == double(value); if (found) break;
    }
    if (!saved || !found) { config_PutFloat("rate", before); return fail(tr("The default playback speed could not be saved. The previous in-memory value was restored.")); }
    return true;
}
void PlaybackPreferencesModel::applyLive(const QString &key)
{
    if (m_intf->p_player && key == QStringLiteral("startPlayback")) {
        vlc_player_locker lock{m_intf->p_player};
        vlc_player_SetStartPaused(m_intf->p_player, config_GetInt("start-paused") != 0);
    }
    if (!m_intf->p_playlist) return;
    vlc_playlist_locker lock{m_intf->p_playlist};
    if (key == QStringLiteral("repeatMode")) vlc_playlist_SetPlaybackRepeat(m_intf->p_playlist,
        config_GetInt("repeat") ? VLC_PLAYLIST_PLAYBACK_REPEAT_CURRENT : config_GetInt("loop") ? VLC_PLAYLIST_PLAYBACK_REPEAT_ALL : VLC_PLAYLIST_PLAYBACK_REPEAT_NONE);
    else if (key == QStringLiteral("shuffle")) vlc_playlist_SetPlaybackOrder(m_intf->p_playlist,
        config_GetInt("random") ? VLC_PLAYLIST_PLAYBACK_ORDER_RANDOM : VLC_PLAYLIST_PLAYBACK_ORDER_NORMAL);
    else if (key == QStringLiteral("finishAction") || key == QStringLiteral("stopAfterEachItem")) vlc_playlist_SetMediaStoppedAction(m_intf->p_playlist,
        config_GetInt("play-and-exit") ? VLC_PLAYLIST_MEDIA_STOPPED_EXIT : config_GetInt("play-and-stop") ? VLC_PLAYLIST_MEDIA_STOPPED_STOP : VLC_PLAYLIST_MEDIA_STOPPED_CONTINUE);
}
bool PlaybackPreferencesModel::setValue(const QString &key, const QVariant &input)
{
    setError({});
    if (m_pickerActive) return fail(tr("Finish choosing the time-shift folder before changing Playback preferences."));
    const Option *option = nullptr;
    for (const auto &candidate : options) if (key == QString::fromLatin1(candidate.key)) { option = &candidate; break; }
    if (!option) return fail(key == QStringLiteral("showThumbnails") ? tr("Timeline thumbnail previews are not implemented.") : tr("Unknown Playback preference."));
    if (!m_intf) return fail(tr("The native Playback preferences service is unavailable."));
    refresh();
    if (!m_capabilities.value(key).toBool()) return fail(tr("This Playback preference is unavailable in this build or session."));
    QVariantMap changes;
    if (key == QStringLiteral("defaultPlaybackSpeed")) {
        float value;
        if (!numericRate(input, value)) return fail(tr("Choose a speed from 0.25 to 2 in steps of 0.25."));
        if (config_GetFloat("rate") == value) return true;
        if (!saveRate(value)) return false;
        refresh(); return true;
    }
    if (key == QStringLiteral("repeatMode") || key == QStringLiteral("finishAction") || option->type == VLC_VAR_INTEGER) {
        qlonglong value;
        qlonglong lo = 0, hi = INT_MAX;
        if (key == QStringLiteral("defaultAudioTrack") || key == QStringLiteral("defaultSubtitleTrack") || key == QStringLiteral("timeShiftSize")
            || key == QStringLiteral("preferredVideoQuality")) lo = -1;
        if (!integral(input, lo, hi, value)) return fail(tr("Enter a whole number in the supported range."));
        if (m_choices.contains(key) && !listed(m_choices.value(key).toList(), value)) return fail(tr("Choose one of this build's supported native values."));
        if (key == QStringLiteral("timeShiftSize") && value != -1 && value < 1024 * 1024) return fail(tr("Time-shift chunks must be at least 1 MiB, or -1 for the native default."));
        if (key == QStringLiteral("repeatMode")) {
            changes[QStringLiteral("repeat")] = value == VLC_PLAYLIST_PLAYBACK_REPEAT_CURRENT;
            changes[QStringLiteral("loop")] = value == VLC_PLAYLIST_PLAYBACK_REPEAT_ALL;
        } else if (key == QStringLiteral("finishAction")) changes[QStringLiteral("play-and-exit")] = value == 1;
        else changes[QString::fromLatin1(option->native)] = value;
        if (key == QStringLiteral("defaultAudioTrack")) changes[QStringLiteral("audio-track-id")] = QString{};
        if (key == QStringLiteral("defaultSubtitleTrack")) changes[QStringLiteral("sub-track-id")] = QString{};
    } else if (option->type == VLC_VAR_BOOL) {
        if (input.typeId() != QMetaType::Bool) return fail(tr("This preference requires an on or off value."));
        const bool invert = key == QStringLiteral("startPlayback") || key == QStringLiteral("preciseSeeking");
        changes[QString::fromLatin1(option->native)] = invert ? !input.toBool() : input.toBool();
    } else {
        if (input.typeId() != QMetaType::QString || !validText(input.toString())) return fail(tr("Enter valid text without control characters."));
        const auto value = input.toString();
        if (key == QStringLiteral("preferredMenuLanguage")) {
            bool found = false;
            for (const auto &row : m_choices.value(key).toList()) if (row.toMap().value(QStringLiteral("value")).toString() == value) { found = true; break; }
            if (!found) return fail(tr("Choose a supported native menu language code."));
        } else if (key == QStringLiteral("timeShiftFolder") && !validFolder(value)) return fail(tr("Choose an existing writable local time-shift folder, or leave it empty."));
        else if (key == QStringLiteral("defaultOpticalDrive")) {
            if (!value.isEmpty() && (value.contains(QStringLiteral("://")) || !QFileInfo(value).isAbsolute())) return fail(tr("Enter an absolute local optical-device path, or leave it empty."));
            changes[QStringLiteral("vcd")] = value;
            if (config_GetType("cd-audio") == VLC_VAR_STRING) changes[QStringLiteral("cd-audio")] = value;
        }
        changes[QString::fromLatin1(option->native)] = value;
    }
    bool unchanged = true;
    for (auto it = changes.cbegin(); it != changes.cend(); ++it) {
        const auto native = it.key().toUtf8();
        unchanged &= it.value().typeId() == QMetaType::QString ? nativeText(native.constData()) == it.value().toString()
            : config_GetInt(native.constData()) == it.value().toLongLong();
    }
    if (!unchanged) {
        QString error;
        if (!CheckedPreferencesStore(m_intf).saveNative(changes, error)) return fail(error);
    }
    // An explicit accepted no-op also reapplies current saved runtime policy.
    applyLive(key); refresh(); return true;
}
bool PlaybackPreferencesModel::chooseTimeShiftFolder()
{
    setError({});
    if (m_pickerActive) return fail(tr("A time-shift folder chooser is already open."));
    refresh();
    if (!m_intf || !m_capabilities.value(QStringLiteral("timeShiftFolder")).toBool()) return fail(tr("The native time-shift folder preference is unavailable."));
    if (!qobject_cast<QApplication *>(QCoreApplication::instance())) return fail(tr("The native folder chooser is unavailable in this session."));
    m_pickerActive = true; emit pickerActiveChanged();
    QPointer<PlaybackPreferencesModel> lifetime(this);
    const auto selected = QFileDialog::getExistingDirectory(nullptr, tr("Choose time-shift buffer folder"), m_values.value(QStringLiteral("timeShiftFolder")).toString());
    if (!lifetime) return false;
    m_pickerActive = false; emit pickerActiveChanged();
    if (selected.isEmpty()) return true;
    return setValue(QStringLiteral("timeShiftFolder"), selected);
}
