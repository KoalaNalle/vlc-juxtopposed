// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#pragma once
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_player.h>
#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QVector>
#include <atomic>
#include <memory>

struct SubtitleSourceCue
{
    vlc_tick_t start = 0, end = 0; // Native microsecond ticks, never milliseconds.
    QString text;
};
struct SubtitleCueReadResult
{
    QVector<SubtitleSourceCue> cues;
    QString error;
    bool canceled = false;
};
struct SubtitleCueSource
{
    QUrl uri;
    QByteArray exactUri, trackId, mediaUri;
    quintptr mediaToken = 0;
    vlc_tick_t delay = 0, position = 0, length = 0;
    QString status;
    bool available = false;
    bool canSeek = false;
};
using SubtitleCueCancellation = std::shared_ptr<std::atomic_bool>;
SubtitleCueReadResult readExternalSrtCues(const QUrl &, const SubtitleCueCancellation &);
SubtitleCueReadResult parseExternalSrtCues(const QByteArray &, const SubtitleCueCancellation &);
// Uses the documented exact slave-URI MD5 prefix; returns no guessed source.
// Locks the native player and current item only while copying bounded metadata.
SubtitleCueSource selectedExternalSrtSource(vlc_player_t *);
// Same snapshot without acquiring the player lock. Callers must already hold
// that lock through any compound validation/action; supports normal locks.
SubtitleCueSource selectedExternalSrtSourceLocked(vlc_player_t *);
