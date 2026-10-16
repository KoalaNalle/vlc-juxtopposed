// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "audio_waveform.hpp"
#include "util/soutchain.hpp"
#include <vlc_input_item.h>
#include <vlc_player.h>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QThread>
#include <QtEndian>
#include <algorithm>
#include <cmath>

namespace {
constexpr int SampleRate = 8000;
constexpr int PeakCount = 300;
constexpr qint64 MaximumPcmBytes = 128 * 1024 * 1024;

QString fileKey(const QString &path)
{
    const QFileInfo info(path);
    return QString::number(info.size()) + ':'
         + QString::number(info.lastModified().toMSecsSinceEpoch()) + ':' + path;
}

struct DecodeState {
    std::atomic_bool started{false};
    std::atomic_bool done{false};
    std::atomic_bool failed{false};
};
const vlc_player_cbs DecodeCallbacks = [] {
    vlc_player_cbs callbacks{};
    callbacks.on_state_changed = [](vlc_player_t *, vlc_player_state state, void *data) {
        auto *status = static_cast<DecodeState *>(data);
        if (state == VLC_PLAYER_STATE_STARTED)
            status->started = true;
        if (state == VLC_PLAYER_STATE_STOPPED && status->started)
            status->done = true;
    };
    callbacks.on_error_changed = [](vlc_player_t *, vlc_player_error error, void *data) {
        if (error != VLC_PLAYER_ERROR_NONE)
            static_cast<DecodeState *>(data)->failed = true;
    };
    return callbacks;
}();
}

AudioWaveform::AudioWaveform(vlc_object_t *parent, QObject *objectParent)
    : QObject(objectParent), m_parent(parent)
{
    m_pool.setMaxThreadCount(1);
}

AudioWaveform::~AudioWaveform()
{
    if (m_cancel) *m_cancel = true;
    m_pool.clear();
    // The native parent must remain alive until this worker releases its player.
    m_pool.waitForDone();
}

void AudioWaveform::setSource(const QUrl &source)
{
    if (m_source == source) return;
    m_source = source;
    restart();
    emit sourceChanged();
}

void AudioWaveform::setEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    restart();
    emit enabledChanged();
}

void AudioWaveform::restart()
{
    const auto generation = ++m_generation;
    if (m_cancel) *m_cancel = true;
    m_pool.clear(); // Keep only the latest request behind a cancelling decode.
    m_result = {};
    m_loading = false;
    if (!m_enabled || m_source.isEmpty()) { emit stateChanged(); return; }
    const QFileInfo info(m_source.toLocalFile());
    if (!m_source.isLocalFile() || !m_source.host().isEmpty()
        || !info.isFile() || !info.isReadable())
    {
        m_result.error = tr("Waveform is available for readable local audio files.");
        emit stateChanged();
        return;
    }
    const auto path = info.canonicalFilePath();
    const auto key = fileKey(path);
    if (m_cache.contains(key))
    {
        m_result = m_cache.value(key);
        emit stateChanged();
        return;
    }
    m_cancel = std::make_shared<std::atomic_bool>(false);
    const auto cancel = m_cancel;
    m_loading = true;
    emit stateChanged();
    m_pool.start([this, path, key, generation, cancel] {
        auto result = analyze(m_parent, path, cancel);
        if (*cancel) return;
        if (fileKey(path) != key)
            result = {{}, 0, tr("The audio file changed during waveform analysis.")};
        QMetaObject::invokeMethod(this, [this, key, generation, result = std::move(result)] {
            if (generation != m_generation) return;
            m_loading = false;
            m_result = result;
            if (!result.peaks.isEmpty())
            {
                if (m_cache.size() >= 8) m_cache.erase(m_cache.begin());
                m_cache.insert(key, result);
            }
            emit stateChanged();
        }, Qt::QueuedConnection);
    });
}

AudioWaveform::Result AudioWaveform::analyze(vlc_object_t *parent,
                                           const QString &path,
                                           const Cancellation &cancel)
{
    Result result;
    if (*cancel) return result;
    QTemporaryDir directory;
    if (!directory.isValid())
        return {{}, 0, tr("Could not create temporary waveform storage.")};
    const QString output = directory.filePath("audio.pcm");
    SoutChain chain("#");
    auto &transcode = chain.begin("transcode");
    transcode.option("acodec", "s16l", false);
    transcode.option("channels", "1", false);
    transcode.option("samplerate", QString::number(SampleRate), false);
    SoutModule access("file");
    access.option("no-overwrite");
    auto &sink = chain.begin("std");
    sink.option("access", access);
    sink.option("mux", "raw", false);
    sink.option("dst", output, true);
    const auto uri = QUrl::fromLocalFile(path).toEncoded();
    auto *media = input_item_New(uri.constData(), nullptr);
    if (!media) return {{}, 0, tr("Could not open audio for waveform analysis.")};
    // Fresh media deliberately has no source item's sout/start/stop options.
    const auto sout = (":sout=" + chain.to_string()).toUtf8();
    input_item_AddOption(media, sout.constData(), VLC_INPUT_OPTION_TRUSTED);
    for (const char *option : {":no-sout-keep", ":no-sout-video", ":no-sout-spu",
                              ":no-video", ":no-spu", ":no-sub-autodetect-file",
                              ":audio", ":audio-filter=", ":audio-track=0", ":rate=1",
                              ":no-play-and-pause",
                              ":start-time=0", ":stop-time=0", ":input-repeat=0"})
        input_item_AddOption(media, option, VLC_INPUT_OPTION_TRUSTED);

    auto *player = vlc_player_New(parent, VLC_PLAYER_LOCK_NORMAL);
    if (!player) { input_item_Release(media); return {{}, 0, tr("Could not start waveform analysis.")}; }
    DecodeState state;
    vlc_player_Lock(player);
    auto *listener = vlc_player_AddListener(player, &DecodeCallbacks, &state);
    vlc_player_SetStartPaused(player, false);
    vlc_player_SetPauseOnCork(player, false);
    const bool started = listener && vlc_player_SetCurrentMedia(player, media) == VLC_SUCCESS
                                 && vlc_player_Start(player) == VLC_SUCCESS;
    vlc_player_Unlock(player);
    input_item_Release(media);
    QElapsedTimer timer;
    timer.start();
    while (started && !state.done && !state.failed && !*cancel)
    {
        if (timer.elapsed() > 120000 || QFileInfo(output).size() > MaximumPcmBytes)
        {
            result.error = tr("Audio exceeds the waveform analysis limit.");
            break;
        }
        QThread::msleep(20);
    }
    vlc_player_Lock(player);
    vlc_player_Stop(player);
    if (listener) vlc_player_RemoveListener(player, listener);
    vlc_player_Unlock(player);
    vlc_player_Delete(player); // On the worker; waits until the PCM file closes.
    if (*cancel) return {};
    if (!started || state.failed)
        return {{}, 0, tr("Could not decode this audio file for a waveform.")};
    if (!result.error.isEmpty()) return result;

    QFile pcm(output);
    if (!pcm.open(QIODevice::ReadOnly) || pcm.size() <= 0 || pcm.size() % 2 != 0)
        return {{}, 0, tr("This file has no decodable audio waveform.")};
    if (pcm.size() > MaximumPcmBytes)
        return {{}, 0, tr("Audio exceeds the waveform analysis limit.")};
    const qint64 samples = pcm.size() / 2;
    const int count = std::min<qint64>(PeakCount, samples);
    QVector<float> peaks(count, 0.f);
    qint64 index = 0;
    while (!pcm.atEnd() && !*cancel)
    {
        const auto bytes = pcm.read(65536);
        if (bytes.isEmpty() || bytes.size() % 2 != 0)
            return {{}, 0, tr("Could not read the decoded waveform.")};
        for (qsizetype i = 0; i < bytes.size(); i += 2, ++index)
        {
            const auto sample = qFromLittleEndian<qint16>(bytes.constData() + i);
            const int bin = index * count / samples;
            peaks[bin] = std::max(peaks[bin], std::abs(int(sample)) / 32768.f);
        }
    }
    if (*cancel) return {};
    result.duration = double(samples) / SampleRate;
    for (float peak : peaks) result.peaks.append(peak);
    return result;
}
