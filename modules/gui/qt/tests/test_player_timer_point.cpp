// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "player/player_timer_point.hpp"
#include <QTest>
#include <limits>

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
    void finiteAndUnknownDurationKeepTheEngineClock()
    {
        auto point = vlc::qt::invalidPlayerTimerPoint();
        point.length = VLC_TICK_FROM_SEC(40);
        const auto time = VLC_TICK_0 + VLC_TICK_FROM_SEC(10);
        QCOMPARE(vlc::qt::playerTimelineTime(point, time, .25, true), time);
        QCOMPARE(vlc::qt::playerTimelineTime(point, time, .25, false), time);
        point.length = VLC_TICK_INVALID;
        QCOMPARE(vlc::qt::playerTimelineTime(point, time, .25, false), time);
        QCOMPARE(vlc::qt::playerTimelineTime(point, VLC_TICK_INVALID, .25, false), VLC_TICK_INVALID);
    }
    void rollingWindowIsNotAnAbsoluteStreamTimestamp()
    {
        vlc_player_timer_point point = {.75, 1.,
            VLC_TICK_0 + VLC_TICK_FROM_SEC(10 * 3600), VLC_TICK_FROM_SEC(300),
            true, VLC_TICK_FROM_SEC(100)};
        QCOMPARE(vlc::qt::playerTimelineTime(point, point.ts, point.position, true),
                 VLC_TICK_0 + VLC_TICK_FROM_SEC(225));
        QCOMPARE(point.ts, VLC_TICK_0 + VLC_TICK_FROM_SEC(10 * 3600));
        QCOMPARE(vlc::qt::playerTimelineTime(point, point.ts, 0., true), VLC_TICK_0);
        QCOMPARE(vlc::qt::playerTimelineTime(point, point.ts, 1., true),
                 VLC_TICK_0 + VLC_TICK_FROM_SEC(300));
    }
    void unusableLiveRangesStayUnknown_data()
    {
        QTest::addColumn<bool>("seekable");
        QTest::addColumn<qlonglong>("length");
        QTest::addColumn<double>("position");
        QTest::newRow("unseekable") << false << qlonglong(VLC_TICK_FROM_SEC(300)) << .5;
        QTest::newRow("unknown-range") << true << qlonglong(VLC_TICK_INVALID) << .5;
        QTest::newRow("negative-range") << true << qlonglong(-1) << .5;
        QTest::newRow("nan") << true << qlonglong(VLC_TICK_FROM_SEC(300)) << std::numeric_limits<double>::quiet_NaN();
        QTest::newRow("infinity") << true << qlonglong(VLC_TICK_FROM_SEC(300)) << std::numeric_limits<double>::infinity();
        QTest::newRow("before-window") << true << qlonglong(VLC_TICK_FROM_SEC(300)) << -.1;
        QTest::newRow("after-window") << true << qlonglong(VLC_TICK_FROM_SEC(300)) << 1.1;
        QTest::newRow("overflow") << true << qlonglong(std::numeric_limits<vlc_tick_t>::max()) << 1.;
    }
    void unusableLiveRangesStayUnknown()
    {
        QFETCH(bool, seekable);
        QFETCH(qlonglong, length);
        QFETCH(double, position);
        auto point = vlc::qt::invalidPlayerTimerPoint();
        point.live = true;
        point.length = length;
        QCOMPARE(vlc::qt::playerTimelineTime(point, VLC_TICK_FROM_SEC(36000), position, seekable), VLC_TICK_INVALID);
    }
    void livePauseResumeAndRollingUpdatesKeepSeparateClocks()
    {
        vlc_player_timer_point point = {.75, 1.,
            VLC_TICK_0 + VLC_TICK_FROM_SEC(36000), VLC_TICK_FROM_SEC(300),
            true, VLC_TICK_MAX};
        vlc_tick_t time = VLC_TICK_INVALID;
        double position = -1.;
        QCOMPARE(vlc::qt::interpolatePlayerTimerPoint(point, VLC_TICK_FROM_SEC(100), &time, &position), VLC_SUCCESS);
        QCOMPARE(time, point.ts);
        QCOMPARE(vlc::qt::playerTimelineTime(point, time, position, true), VLC_TICK_0 + VLC_TICK_FROM_SEC(225));
        point.system_date = VLC_TICK_FROM_SEC(100);
        QCOMPARE(vlc::qt::interpolatePlayerTimerPoint(point, VLC_TICK_FROM_SEC(104), &time, &position), VLC_SUCCESS);
        QCOMPARE(time, point.ts + VLC_TICK_FROM_SEC(4));
        QCOMPARE(position, .75); // Core does not invent live-window movement.
        point.position = .8;
        point.length = VLC_TICK_FROM_SEC(200); // The advertised window moved.
        QCOMPARE(vlc::qt::playerTimelineTime(point, time, point.position, true), VLC_TICK_0 + VLC_TICK_FROM_SEC(160));
        point = vlc::qt::invalidPlayerTimerPoint();
        QCOMPARE(vlc::qt::playerTimelineTime(point, VLC_TICK_0, 0., true), VLC_TICK_0);
    }
    void seekRequestsKeepTheCorrectClock_data()
    {
        QTest::addColumn<bool>("live");
        QTest::addColumn<double>("position");
        QTest::addColumn<qlonglong>("requested");
        QTest::addColumn<qlonglong>("expected");
        QTest::newRow("finite-zero") << false << 0. << qlonglong(0) << qlonglong(VLC_TICK_0);
        QTest::newRow("finite-position") << false << .5 << qlonglong(5000000) << qlonglong(5000000 + VLC_TICK_0);
        QTest::newRow("finite-time") << false << -1. << qlonglong(5000000) << qlonglong(5000000 + VLC_TICK_0);
        QTest::newRow("live-position") << true << .5 << qlonglong(5000000) << qlonglong(VLC_TICK_INVALID);
        QTest::newRow("live-zero") << true << 0. << qlonglong(0) << qlonglong(VLC_TICK_INVALID);
        QTest::newRow("live-time") << true << -1. << qlonglong(36005000000) << qlonglong(36005000000 + VLC_TICK_0);
        QTest::newRow("unknown-time") << false << -1. << qlonglong(VLC_TICK_INVALID) << qlonglong(36000000001);
        QTest::newRow("overflow") << false << -1. << qlonglong(std::numeric_limits<vlc_tick_t>::max()) << qlonglong(36000000001);
    }
    void seekRequestsKeepTheCorrectClock()
    {
        QFETCH(bool, live);
        QFETCH(double, position);
        QFETCH(qlonglong, requested);
        QFETCH(qlonglong, expected);
        auto point = vlc::qt::invalidPlayerTimerPoint();
        point.live = live;
        point.position = position;
        point.ts = requested;
        point.length = VLC_TICK_FROM_SEC(10);
        QCOMPARE(vlc::qt::playerSeekTime(point, VLC_TICK_0 + VLC_TICK_FROM_SEC(36000)), vlc_tick_t(expected));
    }
};

QTEST_GUILESS_MAIN(TestPlayerTimerPoint)
#include "test_player_timer_point.moc"
