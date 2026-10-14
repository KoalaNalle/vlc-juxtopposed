// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "player/player_timer_point.hpp"
#include <QTest>

class TestPlayerTimerPoint : public QObject
{
    Q_OBJECT
private slots:
    void pausedBeforeAnyPointDoesNotPublishTime()
    {
        const auto point = vlc::qt::invalidPlayerTimerPoint();
        vlc_tick_t time = VLC_TICK_0;
        double position = 0.0;
        for (int pause = 1; pause <= 3; ++pause)
        {
            QCOMPARE(vlc::qt::interpolatePlayerTimerPoint(point,
                VLC_TICK_FROM_SEC(100 + pause), &time, &position), VLC_EGENERIC);
            QCOMPARE(time, VLC_TICK_0);
            QCOMPARE(position, 0.0);
        }
    }
    void missingTimestampAndDateDoNotPublishTime()
    {
        auto point = vlc::qt::invalidPlayerTimerPoint();
        point.ts = VLC_TICK_0;
        vlc_tick_t time = VLC_TICK_FROM_SEC(7);
        double position = 0.25;
        QCOMPARE(vlc::qt::interpolatePlayerTimerPoint(point,
            VLC_TICK_FROM_SEC(100), &time, &position), VLC_EGENERIC);
        QCOMPARE(time, VLC_TICK_FROM_SEC(7));
        QCOMPARE(position, 0.25);
        point.ts = VLC_TICK_INVALID;
        point.system_date = VLC_TICK_MAX;
        QCOMPARE(vlc::qt::interpolatePlayerTimerPoint(point,
            VLC_TICK_FROM_SEC(100), &time, &position), VLC_EGENERIC);
        QCOMPARE(time, VLC_TICK_FROM_SEC(7));
        QCOMPARE(position, 0.25);
    }
    void actualPausedAndResumedPointInterpolation()
    {
        vlc_player_timer_point point = {0.25, 1.0,
            VLC_TICK_0 + VLC_TICK_FROM_SEC(10), VLC_TICK_FROM_SEC(40),
            false, VLC_TICK_MAX};
        vlc_tick_t time = VLC_TICK_INVALID;
        double position = -1.0;
        QCOMPARE(vlc::qt::interpolatePlayerTimerPoint(point,
            VLC_TICK_FROM_SEC(100), &time, &position), VLC_SUCCESS);
        QCOMPARE(time, VLC_TICK_0 + VLC_TICK_FROM_SEC(10));
        QCOMPARE(position, 0.25);

        point.system_date = VLC_TICK_FROM_SEC(100);
        QCOMPARE(vlc::qt::interpolatePlayerTimerPoint(point,
            VLC_TICK_FROM_SEC(102), &time, &position), VLC_SUCCESS);
        QCOMPARE(time, VLC_TICK_0 + VLC_TICK_FROM_SEC(12));
        QCOMPARE(position, 0.30);
    }
    void mediaSwitchInvalidatesPreviousClock()
    {
        vlc_player_timer_point point = {0.75, 1.0,
            VLC_TICK_0 + VLC_TICK_FROM_SEC(30), VLC_TICK_FROM_SEC(40),
            false, VLC_TICK_FROM_SEC(100)};
        vlc_tick_t time = VLC_TICK_0;
        double position = 0.0;
        point = vlc::qt::invalidPlayerTimerPoint();
        QCOMPARE(vlc::qt::interpolatePlayerTimerPoint(point,
            VLC_TICK_FROM_SEC(104), &time, &position), VLC_EGENERIC);
        QCOMPARE(time, VLC_TICK_0);
        QCOMPARE(position, 0.0);

        point = {0.0, 1.0, VLC_TICK_0, VLC_TICK_FROM_SEC(30),
                 false, VLC_TICK_MAX};
        QCOMPARE(vlc::qt::interpolatePlayerTimerPoint(point,
            VLC_TICK_FROM_SEC(105), &time, &position), VLC_SUCCESS);
        QCOMPARE(time, VLC_TICK_0);
        QCOMPARE(position, 0.0);
    }
};

QTEST_GUILESS_MAIN(TestPlayerTimerPoint)
#include "test_player_timer_point.moc"
