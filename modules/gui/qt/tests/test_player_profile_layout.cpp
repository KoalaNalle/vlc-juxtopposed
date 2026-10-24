// SPDX-License-Identifier: GPL-2.0-or-later
#include "maininterface/player_profile_layout.hpp"
#include "player/player_controlbar_model.hpp"
#include "player/control_list_model.hpp"
#include <QAbstractItemModelTester>
#include <QJsonDocument>
#include <QApplication>
#include <QtPlugin>
#ifdef _WIN32
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif
#include <QSet>
#include <QSignalSpy>
#include <QTest>

class TestPlayerProfileLayout final : public QObject
{
    Q_OBJECT
    using C = ControlListModel;
private slots:
    void nativeModelsAndDefaultLayoutAreStable() {
        PlayerProfileLayout layout; QString error;
        QVERIFY(PlayerProfileLayout::validate(layout.snapshot(),error)); QCOMPARE(layout.snapshot(),PlayerProfileLayout::defaults());
        auto *video = layout.video(); auto *audio = layout.audio();
        QVERIFY(video); QVERIFY(audio); QVERIFY(video != audio); QVERIFY(layout.draftVideo() != video);
        QCOMPARE(video->left()->getControls(),QVector<int>({C::PREVIOUS_BUTTON,C::PLAY_BUTTON,C::NEXT_BUTTON}));
        QCOMPARE(video->right()->rowCount(),5); QVERIFY(!layout.videoTimelineAbove()); QVERIFY(layout.audioTimelineAbove());
        QVERIFY(layout.apply(PlayerProfileLayout::defaults(),error)); QCOMPARE(layout.video(),video); QCOMPARE(layout.audio(),audio);
        QVERIFY(!video->dirty()); QVERIFY(!audio->dirty());
    }
    void paletteMatchesAll31AuthoredEntriesAndOrder() {
        const auto palette = PlayerProfileLayout::palette(); QCOMPARE(palette.size(),31); QSet<int> ids;
        const QStringList expected = {QStringLiteral("Spacer"),QStringLiteral("Divider"),QStringLiteral("Playback Speed"),QStringLiteral("Forward 10s"),
            QStringLiteral("Backward 10s"),QStringLiteral("Forward 15s"),QStringLiteral("Backward 15s"),QStringLiteral("Stop"),QStringLiteral("Repeat"),
            QStringLiteral("Shuffle"),QStringLiteral("Record"),QStringLiteral("Snapshot"),QStringLiteral("Picture in Picture"),QStringLiteral("HDR"),
            QStringLiteral("Zoom"),QStringLiteral("Right Sidebar"),QStringLiteral("Quit VLC"),QStringLiteral("VLC Settings"),QStringLiteral("Go Home"),
            QStringLiteral("Thumbnail"),QStringLiteral("File Name"),QStringLiteral("Artist Name"),QStringLiteral("Folder Name"),QStringLiteral("Playlist Name"),
            QStringLiteral("Properties"),QStringLiteral("Replay"),QStringLiteral("Chapters"),QStringLiteral("Favorite"),QStringLiteral("Bookmark"),
            QStringLiteral("Bookmarks"),QStringLiteral("Playlists")};
        for (int i=0;i<palette.size();++i) {const auto tile=palette[i].toMap();const int id=tile.value(QStringLiteral("id")).toInt();
            QCOMPARE(tile.value(QStringLiteral("name")).toString(),expected[i]); QVERIFY(PlayerProfileLayout::supportedTile(id)); QVERIFY(!ids.contains(id)); ids.insert(id);}
        QCOMPARE(int(C::PLAY_BUTTON),0); QCOMPARE(int(C::WIDGET_SPACER),0x40); QCOMPARE(int(C::WIDGET_MAX),0x42);
        QCOMPARE(int(C::JUXTO_FORWARD_15),0x50); QVERIFY(!PlayerProfileLayout::supportedTile(C::RESERVED_0X2B));
        PlayerProfileLayout layout;
        QVERIFY(C::JUXTO_VLC_SETTINGS != C::EXTENDED_BUTTON);
        QCOMPARE(layout.tileInfo(C::JUXTO_VLC_SETTINGS).value(QStringLiteral("name")).toString(),QStringLiteral("VLC Settings"));
        QCOMPARE(layout.tileInfo(C::EXTENDED_BUTTON).value(QStringLiteral("name")).toString(),QStringLiteral("Playback settings"));
    }
    void openingEditingAndCancellingNeverMutateActive() {
        PlayerProfileLayout layout; const auto before=layout.snapshot(); QSignalSpy changed(&layout,&PlayerProfileLayout::changed);
        layout.beginEdit(); QVERIFY(layout.editing());
        QVERIFY(layout.insertTile(0,0,0,C::JUXTO_FORWARD_15)); QVERIFY(layout.removeTile(0,0,1)); QVERIFY(layout.setTimelineAbove(0,true));
        QVERIFY(layout.draftSnapshot()!=before); QCOMPARE(layout.snapshot(),before); QCOMPARE(changed.size(),0);
        layout.cancelEdit(); QVERIFY(!layout.editing()); QCOMPARE(layout.snapshot(),before); QCOMPARE(changed.size(),0);
        layout.beginEdit(); QCOMPARE(layout.draftSnapshot(),before);
    }
    void nativeColumnEditsPreserveOrderAndOtherPlayer() {
        PlayerProfileLayout layout; const auto audio=layout.audio()->serializeModels(); layout.beginEdit();
        QAbstractItemModelTester left(layout.draftVideo()->left(),QAbstractItemModelTester::FailureReportingMode::QtTest);
        QAbstractItemModelTester right(layout.draftVideo()->right(),QAbstractItemModelTester::FailureReportingMode::QtTest);
        QVERIFY(layout.moveTile(0,0,0,0,3));
        QCOMPARE(layout.draftVideo()->left()->getControls(),QVector<int>({C::PLAY_BUTTON,C::NEXT_BUTTON,C::PREVIOUS_BUTTON}));
        QVERIFY(layout.moveTile(0,0,1,2,0));
        QCOMPARE(layout.draftVideo()->right()->getControls().first(),int(C::NEXT_BUTTON));
        QVERIFY(layout.removeTile(0,2,0)); QVERIFY(layout.insertTile(0,0,1,C::JUXTO_FOLDER_NAME));
        QCOMPARE(layout.draftVideo()->left()->getControls(),QVector<int>({C::PLAY_BUTTON,C::JUXTO_FOLDER_NAME,C::PREVIOUS_BUTTON}));
        QVERIFY(layout.draftAudio()->serializeModels()==audio); QVERIFY(layout.audio()->serializeModels()==audio);
        QString error; const auto saved=layout.draftSnapshot(); QVERIFY(layout.apply(saved,error)); QCOMPARE(layout.snapshot(),saved);
        QCOMPARE(layout.video()->left()->getControls(),layout.draftVideo()->left()->getControls());
    }
    void invalidIndicesAndTileIdsNeverMutateDraft() {
        PlayerProfileLayout layout; const auto active=layout.snapshot();
        QVERIFY(!layout.insertTile(0,0,0,C::STOP_BUTTON)); layout.beginEdit(); const auto before=layout.draftSnapshot();
        QVERIFY(!layout.insertTile(-1,0,0,C::STOP_BUTTON)); QVERIFY(!layout.insertTile(0,3,0,C::STOP_BUTTON));
        QVERIFY(!layout.insertTile(0,0,-1,C::STOP_BUTTON)); QVERIFY(!layout.insertTile(0,0,999,C::STOP_BUTTON));
        QVERIFY(!layout.insertTile(0,0,0,255)); QVERIFY(!layout.moveTile(0,0,999,1,0)); QVERIFY(!layout.moveTile(0,0,0,1,1));
        QVERIFY(!layout.removeTile(0,-1,0)); QVERIFY(!layout.removeTile(0,0,-1)); QVERIFY(!layout.removeTile(0,0,999));
        QVERIFY(!layout.setTimelineAbove(3,true)); QCOMPARE(layout.draftSnapshot(),before); QCOMPARE(layout.snapshot(),active);
        QVERIFY(!layout.lastError().isEmpty()); QVERIFY(layout.insertTile(0,0,0,C::STOP_BUTTON)); QVERIFY(layout.lastError().isEmpty());
    }
    void allPaletteTilesUseNativeModelsAndRoundTripJson() {
        PlayerProfileLayout layout; layout.beginEdit();
        for (const auto &entry:PlayerProfileLayout::palette()) QVERIFY(layout.insertTile(0,1,layout.draftVideo()->center()->rowCount(),entry.toMap().value(QStringLiteral("id")).toInt()));
        const auto draft=layout.draftSnapshot(); const auto encoded=QJsonDocument::fromVariant(draft).toJson();
        const auto decoded=QJsonDocument::fromJson(encoded).toVariant().toMap(); QString error;
        QVERIFY2(layout.apply(decoded,error),qPrintable(error)); QCOMPARE(layout.snapshot(),draft);
        QCOMPARE(layout.video()->center()->rowCount(),31);
        for(int i=0;i<31;++i) QCOMPARE(layout.video()->center()->data(layout.video()->center()->index(i),C::ID_ROLE).toInt(),
            PlayerProfileLayout::palette()[i].toMap().value(QStringLiteral("id")).toInt());
    }
    void boundsAndMalformedSnapshotsPreserveActive() {
        PlayerProfileLayout layout; QString error; const auto before=layout.snapshot(); auto invalid=before;
        invalid[QStringLiteral("version")]=true; QVERIFY(!layout.apply(invalid,error)); invalid=before;
        invalid[QStringLiteral("global")]=QVariantMap{}; QVERIFY(!layout.apply(invalid,error)); invalid=before;
        auto video=invalid.value(QStringLiteral("video")).toMap(); video[QStringLiteral("timelineAbove")]=1;
        invalid[QStringLiteral("video")]=video; QVERIFY(!layout.apply(invalid,error)); invalid=before;
        video=invalid.value(QStringLiteral("video")).toMap(); video[QStringLiteral("columns")]=QVariantList{QVariantList{0},QVariantList{1.25},QVariantList{}};
        invalid[QStringLiteral("video")]=video; QVERIFY(!layout.apply(invalid,error)); QCOMPARE(layout.snapshot(),before);
        layout.beginEdit(); int count=0; for(const auto &column:layout.draftVideo()->serializeModels())count+=column.size();
        for(;count<64;++count) {QVERIFY(layout.insertTile(0,1,0,C::WIDGET_SPACER));}
        const auto full=layout.draftSnapshot();
        QVERIFY(!layout.insertTile(0,1,0,C::WIDGET_SPACER)); QCOMPARE(layout.draftSnapshot(),full); QVERIFY(PlayerProfileLayout::validate(full,error));
    }
    void resetIsDraftOnlyAndReopenStartsFromAcceptedLayout() {
        PlayerProfileLayout layout; layout.beginEdit(); QVERIFY(layout.insertTile(0,1,0,C::JUXTO_PLAYLISTS));
        QVERIFY(layout.setTimelineAbove(0,true)); QString error; const auto accepted=layout.draftSnapshot();
        QVERIFY(layout.apply(accepted,error)); layout.cancelEdit(); layout.beginEdit(); QCOMPARE(layout.draftSnapshot(),accepted);
        layout.resetDraft(); QCOMPARE(layout.draftSnapshot(),PlayerProfileLayout::defaults()); QCOMPARE(layout.snapshot(),accepted);
        layout.cancelEdit(); layout.beginEdit(); QCOMPARE(layout.draftSnapshot(),accepted);
    }
    void qmlSnapshotsAndDescriptorsFollowNativeModels() {
        PlayerProfileLayout layout;
        QVERIFY(layout.defaultVideo());
        QCOMPARE(layout.property("current").toMap(),layout.snapshot());
        layout.beginEdit(); QVERIFY(layout.insertTile(0,1,0,C::JUXTO_FOLDER_NAME));
        QCOMPARE(layout.property("draft").toMap(),layout.draftSnapshot());
        QVERIFY(layout.defaultVideo());
        QString error; QVERIFY(layout.apply(layout.draftSnapshot(),error)); QVERIFY(!layout.defaultVideo());
        for(const auto &entry:layout.palette()) {
            const auto id=entry.toMap().value(QStringLiteral("id")).toInt();
            const auto info=layout.tileInfo(id);
            QCOMPARE(info.value(QStringLiteral("id")).toInt(),id);
            QVERIFY(!info.value(QStringLiteral("name")).toString().isEmpty());
            QVERIFY(!info.value(QStringLiteral("kind")).toString().isEmpty());
        }
        QVERIFY(layout.tileInfo(255).isEmpty());
        QCOMPARE(layout.tileInfo(C::JUXTO_FOLDER_NAME).value(QStringLiteral("key")).toString(),QStringLiteral("folder"));
    }
};
QTEST_MAIN(TestPlayerProfileLayout)
#include "test_player_profile_layout.moc"
