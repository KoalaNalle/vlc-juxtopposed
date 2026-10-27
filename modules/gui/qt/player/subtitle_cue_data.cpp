// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "subtitle_cue_data.hpp"
#include "qt.hpp"
#include "util/filterchain.hpp"
#include <vlc_cxx_helpers.hpp>
using vlc_player_locker = vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock>;
#include <vlc_vout.h>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringDecoder>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr qsizetype maxBytes = 2 * 1024 * 1024;
constexpr qsizetype maxCues = 20000;
bool canceled(const SubtitleCueCancellation &token) { return token && token->load(); }
bool localSource(const QUrl &uri)
{
    const auto path = uri.toLocalFile();
    const auto separator = [](QChar c) { return c == '/' || c == '\\'; };
    if (!uri.isValid() || !uri.isLocalFile() || !uri.host().isEmpty()
        || path.isEmpty() || path.contains(QChar::Null)
        || !uri.query().isEmpty() || !uri.fragment().isEmpty()
        || (path.size() >= 2 && separator(path[0]) && separator(path[1])))
        return false;
    // Check the raw decoded prefix before Qt can strip an extended Windows
    // device prefix; reject all twin/mixed slash forms. Qt calls C:relative and
    // bare C: absolute on Windows, so additionally require the drive root.
    const auto normalized = QDir::fromNativeSeparators(path);
    if (normalized.size() >= 2 && normalized[0].isLetter() && normalized[1] == ':'
        && (normalized.size() < 3 || normalized[2] != '/'))
        return false;
    return !normalized.startsWith("//") && QDir::isAbsolutePath(normalized);
}
bool timestamp(const QRegularExpressionMatch &match, int first, vlc_tick_t &value)
{
    const auto hours = match.captured(first).toLongLong();
    const auto minutes = match.captured(first + 1).toInt();
    const auto seconds = match.captured(first + 2).toInt();
    const auto millis = match.captured(first + 3).toInt();
    if (hours > 9999 || minutes >= 60 || seconds >= 60) return false;
    value = VLC_TICK_FROM_MS(((hours * 60 + minutes) * 60 + seconds) * 1000 + millis);
    return true;
}
}

SubtitleCueReadResult parseExternalSrtCues(const QByteArray &bytes, const SubtitleCueCancellation &token)
{
    SubtitleCueReadResult result;
    if (canceled(token)) { result.canceled = true; return result; }
    if (bytes.size() > maxBytes) { result.error = QStringLiteral("Subtitle timeline supports SRT files up to 2 MB."); return result; }
    QStringDecoder decoder(QStringDecoder::Utf8, QStringConverter::Flag::Stateless);
    QString text = decoder.decode(bytes);
    if (decoder.hasError() || text.contains(QChar::Null))
    { result.error = QStringLiteral("Subtitle timeline supports UTF-8 SRT text."); return result; }
    if (text.startsWith(QChar(0xfeff))) text.remove(0, 1);
    text.replace("\r\n", "\n");
    text.replace('\r', '\n');
    if (text.trimmed().isEmpty()) return result;
    const auto blocks = text.split(QRegularExpression("\n[ \\t]*\n+"), Qt::SkipEmptyParts);
    const QRegularExpression timing(QStringLiteral("^([0-9]{1,4}):([0-9]{2}):([0-9]{2})[,\\.]([0-9]{3})[ \\t]+-->[ \\t]+([0-9]{1,4}):([0-9]{2}):([0-9]{2})[,\\.]([0-9]{3})(?:[ \\t]+.*)?$"));
    const QRegularExpression index(QStringLiteral("^[0-9]+$"));
    for (const auto &block : blocks)
    {
        if (canceled(token)) { result.cues.clear(); result.canceled = true; return result; }
        const auto lines = block.trimmed().split('\n');
        if (lines.isEmpty()) continue;
        const int line = index.match(lines[0].trimmed()).hasMatch() ? 1 : 0;
        const auto match = line < lines.size() ? timing.match(lines[line].trimmed()) : QRegularExpressionMatch{};
        SubtitleSourceCue cue;
        if (!match.hasMatch() || !timestamp(match, 1, cue.start) || !timestamp(match, 5, cue.end)
            || cue.end <= cue.start || line + 1 >= lines.size())
        {
            result.cues.clear();
            result.error = QStringLiteral("Could not read valid SRT cue timings. Playback subtitles are unchanged.");
            return result;
        }
        cue.text = lines.mid(line + 1).join('\n');
        result.cues.append(std::move(cue));
        if (result.cues.size() > maxCues)
        {
            result.cues.clear();
            result.error = QStringLiteral("Subtitle timeline supports at most 20,000 cues.");
            return result;
        }
    }
    std::stable_sort(result.cues.begin(), result.cues.end(), [](const auto &a, const auto &b) { return a.start < b.start; });
    return result;
}

SubtitleCueReadResult readExternalSrtCues(const QUrl &uri, const SubtitleCueCancellation &token)
{
    if (canceled(token)) return {{}, {}, true};
    if (!localSource(uri) || QFileInfo(uri.toLocalFile()).suffix().compare("srt", Qt::CaseInsensitive) != 0)
        return {{}, QStringLiteral("Subtitle timeline supports a selected local SRT file."), false};
    if (!QFileInfo(uri.toLocalFile()).isFile())
        return {{}, QStringLiteral("Could not open a regular SRT file."), false};
    QFile file(uri.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) return {{}, QStringLiteral("Could not open the selected SRT file."), false};
    if (file.size() > maxBytes) return {{}, QStringLiteral("Subtitle timeline supports SRT files up to 2 MB."), false};
    QByteArray bytes;
    while (!file.atEnd())
    {
        if (canceled(token)) return {{}, {}, true};
        const auto chunk = file.read(16384);
        if (chunk.isEmpty() && file.error() != QFile::NoError)
            return {{}, QStringLiteral("Could not read the selected SRT file."), false};
        bytes += chunk;
        if (bytes.size() > maxBytes) return {{}, QStringLiteral("Subtitle timeline supports SRT files up to 2 MB."), false};
    }
    return parseExternalSrtCues(bytes, token);
}

SubtitleCueSource selectedExternalSrtSource(vlc_player_t *player)
{
    if (!player) return selectedExternalSrtSourceLocked(nullptr);
    vlc_player_locker guard(player);
    return selectedExternalSrtSourceLocked(player);
}

SubtitleCueSource selectedExternalSrtSourceLocked(vlc_player_t *player)
{
    SubtitleCueSource source;
    if (!player) { source.status = QStringLiteral("Subtitle timeline is unavailable."); return source; }
    auto *media = vlc_player_GetCurrentMedia(player);
    if (!media) { source.status = QStringLiteral("Open a video with an external SRT subtitle file."); return source; }
    source.mediaToken = reinterpret_cast<quintptr>(media);
    auto *uri = input_item_GetURI(media);
    source.mediaUri = uri ? QByteArray(uri) : QByteArray{};
    free(uri);
    source.position = vlc_player_GetTime(player);
    source.length = vlc_player_GetLength(player);
    if (source.position == VLC_TICK_INVALID) source.position = 0;
    if (source.length == VLC_TICK_INVALID) source.length = 0;
    source.canSeek = vlc_player_IsStarted(player) && vlc_player_CanSeek(player)
        && vlc_player_GetError(player) == VLC_PLAYER_ERROR_NONE;
    const vlc_player_track *selected = nullptr;
    size_t selectedCount = 0;
    const auto count = vlc_player_GetTrackCount(player, SPU_ES);
    if (count > 512) { source.status = QStringLiteral("This subtitle source has too many tracks to display."); return source; }
    for (size_t i = 0; i < count; ++i)
    {
        const auto *track = vlc_player_GetTrackAt(player, SPU_ES, i);
        if (track->selected) { selected = track; ++selectedCount; }
    }
    if (selectedCount != 1)
    { source.status = QStringLiteral("Select one external SRT subtitle track to see its timeline."); return source; }
    source.trackId = vlc_es_id_GetStrId(selected->es_id);
    const auto slash = source.trackId.indexOf('/');
    if (slash != 32 || !source.trackId.mid(slash).startsWith("/spu/")
        || (selected->fmt.i_codec != VLC_CODEC_SUBT && selected->fmt.i_codec != VLC_CODEC_TEXT))
    { source.status = QStringLiteral("Timeline is unavailable for embedded or image subtitles."); return source; }
    vlc_vout_order order = VLC_VOUT_ORDER_NONE;
    auto *vout = vlc_player_GetEsIdVout(player, selected->es_id, &order);
    if (!vout && order == VLC_VOUT_ORDER_PRIMARY)
    {
        // A selected subtitle has an order before its first cue creates an SPU
        // output. Use the actual primary video's output to inspect its filters
        // while paused before that cue; never substitute the idle output.
        const auto *video = vlc_player_GetSelectedTrack(player, VIDEO_ES);
        vlc_vout_order videoOrder = VLC_VOUT_ORDER_NONE;
        if (video)
            vout = vlc_player_GetEsIdVout(player, video->es_id, &videoOrder);
        if (videoOrder != VLC_VOUT_ORDER_PRIMARY) vout = nullptr;
    }
    if (!vout || order != VLC_VOUT_ORDER_PRIMARY)
    { source.status = QStringLiteral("Subtitle timeline is waiting for the primary video output."); return source; }
    for (const auto &filter : vlc::qt::filterChainEntries(vout, "sub-filter"))
        if (filter.name == "subsdelay")
        { source.status = QStringLiteral("Disable subtitle duration effects to view accurate cue timings."); return source; }
    const auto fps = vlc_player_GetAssociatedSubsFPS(player);
    if (!std::isfinite(fps) || fps > 1.f)
    { source.status = QStringLiteral("Timeline is unavailable while subtitle FPS correction is enabled."); return source; }
    source.delay = vlc_player_GetEsIdDelay(player, selected->es_id);
    if (source.delay == INT64_MAX) source.delay = vlc_player_GetSubtitleDelay(player);
    if (source.delay < -VLC_TICK_FROM_SEC(30 * 24 * 60 * 60) || source.delay > VLC_TICK_FROM_SEC(30 * 24 * 60 * 60))
    { source.status = QStringLiteral("Subtitle delay is too large to display on the timeline."); return source; }

    // InputSourceNew hashes the complete, exact UTF-8 slave URL. Hash before
    // QUrl normalization; file-name matching would confuse different sources.
    vlc_mutex_lock(&media->lock);
    const auto prefix = source.trackId.left(32);
    int matches = 0;
    if (media->i_slaves <= 256)
        for (int i = 0; i < media->i_slaves; ++i)
        {
            const auto *slave = media->pp_slaves[i];
            if (!slave || slave->i_type != SLAVE_TYPE_SPU || !slave->psz_uri[0]) continue;
            const QByteArray exact(slave->psz_uri);
            if (QCryptographicHash::hash(exact, QCryptographicHash::Md5).toHex() == prefix)
            { source.exactUri = exact; ++matches; }
        }
    vlc_mutex_unlock(&media->lock);
    if (matches != 1)
    { source.status = QStringLiteral("The selected subtitle source could not be identified reliably."); return source; }
    source.uri = QUrl::fromEncoded(source.exactUri, QUrl::StrictMode);
    if (!localSource(source.uri) || QFileInfo(source.uri.toLocalFile()).suffix().compare("srt", Qt::CaseInsensitive) != 0)
    { source.status = QStringLiteral("Subtitle timeline supports a selected local SRT file."); return source; }
    source.available = true;
    return source;
}
