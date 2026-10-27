// SPDX-License-Identifier: GPL-2.0-or-later
#include "util/detached_window_placement.hpp"
#include <QTest>

class TestDetachedWindowPlacement:public QObject {
    Q_OBJECT
private slots:
    void visibleFrame_data() {
        QTest::addColumn<QRect>("owner");
        QTest::addColumn<QSize>("requested");
        QTest::addColumn<QMargins>("frame");
        QTest::addColumn<QVector<QRect>>("areas");
        QTest::addColumn<int>("ownerScreen");
        QTest::addColumn<int>("expectedScreen");
        const QMargins frame(8,30,8,8);
        const QVector<QRect> one{QRect(0,0,1920,1040)};
        QTest::newRow("right-room")<<QRect(100,100,800,600)<<QSize(400,600)<<frame<<one<<0<<0;
        QTest::newRow("left-room")<<QRect(1100,100,700,600)<<QSize(400,600)<<frame<<one<<0<<0;
        QTest::newRow("maximized-owner")<<one[0]<<QSize(400,1040)<<frame<<one<<0<<0;
        QTest::newRow("taskbar-top-left")<<QRect(0,0,1920,1080)<<QSize(400,1080)<<frame<<QVector<QRect>{QRect(40,60,1880,1020)}<<0<<0;
        QTest::newRow("negative-origin-only")<<QRect(-1920,0,1920,1040)<<QSize(400,1040)<<frame<<QVector<QRect>{QRect(-1920,0,1920,1040)}<<0<<0;
        QTest::newRow("adjacent-other-display")<<QRect(-1920,40,1920,1040)<<QSize(400,1040)<<frame<<QVector<QRect>{QRect(-1920,40,1920,1040),QRect(0,0,2560,1392)}<<0<<1;
        QTest::newRow("display-above")<<QRect(0,-1080,1920,1080)<<QSize(400,1080)<<frame<<QVector<QRect>{QRect(0,-1080,1920,1040)}<<0<<0;
        QTest::newRow("tiny-work-area")<<QRect(0,0,320,240)<<QSize(400,1000)<<frame<<QVector<QRect>{QRect(0,0,320,240)}<<0<<0;
        QTest::newRow("invalid-owner-screen")<<QRect(-5000,5000,900,600)<<QSize(400,600)<<frame<<one<<-1<<0;
        QTest::newRow("invalid-area-skipped")<<QRect(0,0,1920,1040)<<QSize(400,1040)<<frame<<QVector<QRect>{QRect(),one[0]}<<0<<1;
        QTest::newRow("scaled-logical-size")<<QRect(0,0,1700,900)<<QSize(600,1200)<<QMargins(12,45,12,12)<<QVector<QRect>{QRect(0,0,1700,900)}<<0<<0;
    }
    void visibleFrame() {
        QFETCH(QRect,owner);QFETCH(QSize,requested);QFETCH(QMargins,frame);
        QFETCH(QVector<QRect>,areas);QFETCH(int,ownerScreen);QFETCH(int,expectedScreen);
        const auto result=vlc::qt::detachedWindowPlacement(owner,requested,frame,areas,ownerScreen);
        QCOMPARE(result.screen,expectedScreen);
        const auto outer=result.client.marginsAdded(frame);
        QVERIFY2(areas[result.screen].contains(outer),qPrintable(QStringLiteral("Frame %1,%2 %3x%4 outside work area")
            .arg(outer.x()).arg(outer.y()).arg(outer.width()).arg(outer.height())));
        QVERIFY(result.client.width()>0);QVERIFY(result.client.height()>0);
        QVERIFY(result.client.width()<=requested.width());QVERIFY(result.client.height()<=requested.height());
    }
    void preserveAdjacentPlacement() {
        const QVector<QRect> areas{QRect(0,0,1920,1040)};
        const auto right=vlc::qt::detachedWindowPlacement(QRect(100,100,800,600),QSize(400,600),QMargins(8,30,8,8),areas,0);
        QCOMPARE(right.client,QRect(918,130,400,600));
        const auto left=vlc::qt::detachedWindowPlacement(QRect(1100,100,700,600),QSize(400,600),QMargins(8,30,8,8),areas,0);
        QCOMPARE(left.client,QRect(682,130,400,600));
    }
    void noAvailableDisplay() {
        const auto result=vlc::qt::detachedWindowPlacement(QRect(0,0,800,600),QSize(400,600),QMargins(),{},-1);
        QCOMPARE(result.screen,-1);QVERIFY(!result.client.isValid());
    }
};
QTEST_GUILESS_MAIN(TestDetachedWindowPlacement)
#include "test_detached_window_placement.moc"
