// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "util/navigation_history.hpp"
#include "network/networktreeitem.hpp"
#include <QCoreApplication>
#include <QList>
#include <QSignalSpy>
#include <QTest>

class TestNavigationHistory : public QObject
{
    Q_OBJECT
private slots:
    void emptyActions()
    {
        NavigationHistory history;
        QSignalSpy navigate(&history, &NavigationHistory::navigate);
        history.previous();
        history.next();
        QVERIFY(history.isPreviousEmpty());
        QVERIFY(history.isNextEmpty());
        QCOMPARE(navigate.count(), 0);
    }

    void retainsViewState()
    {
        NavigationHistory history;
        history.push({"home"});
        history.push({"video"}, {{"initialIndex", 7}});
        history.viewProp()->insert("scroll", 120);
        history.push({"music"}, {{"initialIndex", 2}});
        QSignalSpy navigate(&history, &NavigationHistory::navigate);
        history.previous(Qt::BacktabFocusReason);
        QCOMPARE(history.viewPath(), QStringList{"video"});
        QCOMPARE(history.viewProp()->value("initialIndex").toInt(), 7);
        QCOMPARE(history.viewProp()->value("scroll").toInt(), 120);
        QVERIFY(!history.isNextEmpty());
        QCOMPARE(navigate.at(0).at(0).value<Qt::FocusReason>(), Qt::BacktabFocusReason);
        // Page-loader normalization must retain the available forward entry.
        history.update({"video", "all", "base"});
        history.viewProp()->insert("initialIndex", 12);
        history.next(Qt::TabFocusReason);
        QCOMPARE(history.viewPath(), QStringList{"music"});
        QCOMPARE(history.viewProp()->value("initialIndex").toInt(), 2);
        QVERIFY(history.isNextEmpty());
        history.previous();
        QCOMPARE(history.viewPath(), (QStringList{"video", "all", "base"}));
        QCOMPARE(history.viewProp()->value("initialIndex").toInt(), 12);
    }

    void branchClearsForward()
    {
        NavigationHistory history;
        history.push({"home"});
        history.push({"video"});
        history.push({"music"});
        history.previous();
        history.previous();
        QVERIFY(!history.isNextEmpty());
        history.push({"network"});
        QVERIFY(history.isNextEmpty());
        history.next();
        QCOMPARE(history.viewPath(), QStringList{"network"});
        history.previous();
        QCOMPARE(history.viewPath(), QStringList{"home"});
    }

    void staleForwardEntry()
    {
        NavigationHistory history;
        history.push({"home"});
        history.push({"network"});
        auto *properties = history.viewProp();
        history.previous();
        properties->insert("tree", QVariant::fromValue(NetworkTreeItem{}));
        QSignalSpy navigate(&history, &NavigationHistory::navigate);
        history.next();
        QCOMPARE(history.viewPath(), QStringList{"home"});
        QVERIFY(history.isNextEmpty());
        QCOMPARE(navigate.count(), 0);
    }

    void expandedLeafPropertiesSurviveNormalization()
    {
        NavigationHistory history;
        history.push({"video", "all", "group"}, {{"parentId", 41}, {"title", "Selected group"}, {"initialIndex", 7}});
        auto *properties = history.viewProp();
        for (const auto &path : QList<QStringList>{{"video"}, {"video", "all"}, {"video", "all", "group"}})
        {
            history.update(path);
            QCOMPARE(history.viewProp(), properties);
            QCOMPARE(properties->value("parentId").toInt(), 41);
            QCOMPARE(properties->value("title").toString(), QString("Selected group"));
            QCOMPARE(properties->value("initialIndex").toInt(), 7);
        }
    }

    void playlistZeroIndexAndHomeFramesRemainIsolated()
    {
        NavigationHistory history;
        history.push({"home"}); history.update({"home", "base"});
        history.push({"playlists"}, {{"initialIndex", 0}});
        history.update({"playlists", "all"});
        auto *playlists = history.viewProp();
        history.push({"playlists", "list"}, {{"parentId", 29}, {"title", "Empty playlist"}, {"initialIndex", -1}});
        history.previous(Qt::BacktabFocusReason);
        QCOMPARE(history.viewPath(), (QStringList{"playlists", "all"}));
        QCOMPARE(history.viewProp(), playlists);
        QCOMPARE(history.viewProp()->value("initialIndex").toInt(), 0);
        QVERIFY(!history.isNextEmpty());
        history.push({"home"}, Qt::TabFocusReason);
        auto *home = history.viewProp();
        QVERIFY(home != playlists); QVERIFY(!home->contains("initialIndex"));
        history.update({"home", "base"});
        QVERIFY(history.isNextEmpty());
        history.previous();
        QCOMPARE(history.viewProp(), playlists);
        QCOMPARE(history.viewProp()->value("initialIndex").toInt(), 0);
        history.next();
        QCOMPARE(history.viewProp(), home);
        QCOMPARE(history.viewPath(), (QStringList{"home", "base"}));
        QVERIFY(!home->contains("initialIndex"));
    }

    void availabilityEdges()
    {
        NavigationHistory history;
        QSignalSpy previous(&history, &NavigationHistory::previousEmptyChanged);
        QSignalSpy next(&history, &NavigationHistory::nextEmptyChanged);
        history.push({"home"});
        history.push({"video"});
        history.previous();
        history.next();
        history.next();
        QCOMPARE(previous.count(), 3);
        QCOMPARE(next.count(), 2);
        QCOMPARE(next.at(0).at(0).toBool(), false);
        QCOMPARE(next.at(1).at(0).toBool(), true);
    }
};

QTEST_GUILESS_MAIN(TestNavigationHistory)
#include "test_navigation_history.moc"
