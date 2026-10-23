// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_player.h>
#include <QTest>
extern "C" unsigned vlc_test_pending_seek_point(vlc_tick_t,bool,vlc_tick_t,bool,
    vlc_tick_t,double,double,vlc_player_timer_point *);
class TestPlayerSeekTimerPoint : public QObject
{
    Q_OBJECT
private slots:
    void pendingPoint_data()
    {
        QTest::addColumn<qlonglong>("inputLength"); QTest::addColumn<bool>("inputLive");
        QTest::addColumn<qlonglong>("outputLength"); QTest::addColumn<bool>("outputLive");
        QTest::addColumn<qlonglong>("time"); QTest::addColumn<double>("position");
        QTest::addColumn<double>("previousPosition"); QTest::addColumn<qlonglong>("expectedTime");
        QTest::addColumn<double>("expectedPosition");
        const qlonglong second=CLOCK_FREQ;
        QTest::newRow("initial-percentage") << 30*second << false << qlonglong(0) << false << qlonglong(0) << .3 << -1. << 9*second << .3;
        QTest::newRow("initial-time") << 30*second << false << qlonglong(0) << false << 9*second << -1. << -1. << 9*second << .3;
        QTest::newRow("initial-zero") << 30*second << false << qlonglong(0) << false << qlonglong(0) << 0. << .3 << qlonglong(0) << 0.;
        QTest::newRow("existing-output-percentage") << 30*second << false << 30*second << false << qlonglong(0) << .3 << -1. << 9*second << .3;
        QTest::newRow("existing-output-time") << 30*second << false << 30*second << false << 9*second << -1. << .6 << 9*second << .3;
        QTest::newRow("replacement-duration") << 10*second << false << 30*second << false << qlonglong(0) << .6 << .3 << 6*second << .6;
        QTest::newRow("unknown-duration") << qlonglong(0) << false << qlonglong(0) << false << qlonglong(0) << .6 << .3 << qlonglong(0) << .6;
        QTest::newRow("unknown-replacement") << qlonglong(0) << false << 30*second << false << qlonglong(0) << .6 << .3 << qlonglong(0) << .6;
        QTest::newRow("unknown-time-clears-old-position") << qlonglong(0) << false << qlonglong(0) << false << 9*second << -1. << .6 << 9*second << -1.;
        QTest::newRow("initial-live-percentage") << 30*second << true << qlonglong(0) << false << qlonglong(0) << .3 << -1. << 9*second << .3;
        QTest::newRow("live-time-keeps-absolute-clock") << 30*second << true << 30*second << true << 36000*second << -1. << .6 << 36000*second << -1.;
        QTest::newRow("replacement-live-status") << 30*second << true << 30*second << false << 36000*second << -1. << .6 << 36000*second << -1.;
        QTest::newRow("replacement-finite-status") << 10*second << false << 30*second << true << 6*second << -1. << .8 << 6*second << .6;
    }
    void pendingPoint()
    {
        QFETCH(qlonglong,inputLength); QFETCH(bool,inputLive);
        QFETCH(qlonglong,outputLength); QFETCH(bool,outputLive);
        QFETCH(qlonglong,time); QFETCH(double,position); QFETCH(double,previousPosition);
        QFETCH(qlonglong,expectedTime); QFETCH(double,expectedPosition);
        vlc_player_timer_point point{};
        QCOMPARE(vlc_test_pending_seek_point(inputLength,inputLive,outputLength,outputLive,
            time,position,previousPosition,&point),2u);
        QCOMPARE(point.ts,vlc_tick_t(expectedTime));
        QCOMPARE(point.position,expectedPosition);
        QCOMPARE(point.length,vlc_tick_t(inputLength));
        QCOMPARE(point.live,inputLive);
        QCOMPARE(point.system_date,VLC_TICK_MAX);
        QCOMPARE(point.rate,1.);
    }
};
QTEST_GUILESS_MAIN(TestPlayerSeekTimerPoint)
#include "test_player_seek_timer_point.moc"
