// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "playlist/playlist_controller.hpp"
#include <vlc/vlc.h>
#include <vlc_playlist.h>
#include <QDir>
#include <QFileInfo>
#include <QMetaProperty>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

extern "C" vlc_object_t *vlc_test_playback_object(libvlc_instance_t *);
using SourceController = vlc::playlist::PlaylistController;
using SourceMedia = vlc::playlist::Media;
using SourceLocker = vlc::playlist::vlc_playlist_locker;
class TestPlaylistSource : public QObject
{
    Q_OBJECT
private:
    QTemporaryDir m_temporary;
    libvlc_instance_t *m_instance = nullptr;
    vlc_playlist_t *m_playlist = nullptr;
    std::unique_ptr<SourceController> m_controller;
    static SourceMedia media(qint64 source) {
        SourceMedia result(QStringLiteral("file:///C:/owned/same.flac"), QStringLiteral("Shared owned track"));
        result.setSourcePlaylistId(source);return result;
    }
    uint64_t idAt(size_t index) {
        SourceLocker locker(m_playlist);
        return vlc_playlist_item_GetId(vlc_playlist_Get(m_playlist,index));
    }
    void select(uint index, qint64 source) {
        const auto id=idAt(index);m_controller->goTo(index,false);
        QTRY_VERIFY(m_controller->getCurrentItem().raw());
        QTRY_COMPARE(vlc_playlist_item_GetId(m_controller->getCurrentItem().raw()),id);
        QCOMPARE(m_controller->currentSourcePlaylistId(),source);
    }
private slots:
    void initTestCase() {
        QVERIFY(m_temporary.isValid());
        auto root=qEnvironmentVariable("VLC_PLAYLIST_SOURCE_OUTPUT");
        if(root.isEmpty())root=m_temporary.path();
        else {QVERIFY(QFileInfo(root).isAbsolute()&&!QFileInfo::exists(root));QVERIFY(QDir().mkpath(root));}
        const auto userdata=QDir(root).filePath(QStringLiteral("userdata"));
        QVERIFY(QDir().mkpath(userdata));qputenv("VLC_USERDATA_PATH",userdata.toUtf8());
    }
    void init() {
        const char *arguments[]={"--ignore-config","--no-media-library","--extraintf=","--control=","--aout=dummy","--quiet"};
        m_instance=libvlc_new(ARRAY_SIZE(arguments),arguments);QVERIFY(m_instance);
        m_playlist=vlc_playlist_New(vlc_test_playback_object(m_instance),VLC_PLAYLIST_PREPARSING_DISABLED,1,0);
        QVERIFY(m_playlist);m_controller=std::make_unique<SourceController>(m_playlist);
        QTRY_VERIFY(m_controller->isInitialized());
    }
    void cleanup() {
        m_controller.reset();
        if(m_playlist)vlc_playlist_Delete(m_playlist);
        m_playlist=nullptr;
        if(m_instance)libvlc_release(m_instance);
        m_instance=nullptr;
    }
    void identicalUrlsRetainDistinctQueueOrigins() {
        m_controller->append(QVector<SourceMedia>{media(101),media(202),media(0)});
        QTRY_COMPARE(m_controller->count(),3);
        QVERIFY(idAt(0)!=idAt(1)&&idAt(1)!=idAt(2));
        select(0,101);select(1,202);select(2,0);
    }
    void insertAndMoveKeepOriginByNativeItemIdentity() {
        m_controller->append(QVector<SourceMedia>{media(101),media(202)});
        QTRY_COMPARE(m_controller->count(),2);select(0,101);
        const auto current=m_controller->getCurrentItem();const auto currentId=vlc_playlist_item_GetId(current.raw());
        m_controller->insert(size_t(0),QVector<SourceMedia>{media(303)});
        QTRY_COMPARE(m_controller->count(),3);
        QCOMPARE(vlc_playlist_item_GetId(m_controller->getCurrentItem().raw()),currentId);
        QCOMPARE(m_controller->currentSourcePlaylistId(),qint64(101));
        m_controller->move(QVector<PlaylistItem>{current},3,-1);
        QTRY_COMPARE(m_controller->currentIndex(),2);
        QCOMPARE(vlc_playlist_item_GetId(m_controller->getCurrentItem().raw()),currentId);
        QCOMPARE(m_controller->currentSourcePlaylistId(),qint64(101));
        select(0,303);select(1,202);
    }
    void requestedInsertBeyondEndUsesActualNativePosition() {
        m_controller->append(QVector<SourceMedia>{media(0)});QTRY_COMPARE(m_controller->count(),1);
        m_controller->insert(size_t(100),QVector<SourceMedia>{media(404)});
        QTRY_COMPARE(m_controller->count(),2);select(1,404);select(0,0);
    }
    void deletionCannotReattachLateOriginAndKeepsQueue() {
        m_controller->append(QVector<SourceMedia>{media(101)});QTRY_COMPARE(m_controller->count(),1);select(0,101);
        const auto input=m_controller->getCurrentItem().inputItem();QSignalSpy changes(m_controller.get(),&SourceController::currentSourcePlaylistChanged);
        m_controller->forgetSourcePlaylist(101);
        QCOMPARE(m_controller->currentSourcePlaylistId(),qint64(0));QVERIFY(!changes.isEmpty());
        QCOMPARE(m_controller->getCurrentItem().inputItem(),input);QCOMPARE(m_controller->count(),1);
        m_controller->append(QVector<SourceMedia>{media(101),media(202)});
        QTRY_COMPARE(m_controller->count(),3);select(1,0);select(2,202);
    }
    void nativeRemovalAndResetNeverLeaveAnOriginOnAnotherEntry() {
        m_controller->append(QVector<SourceMedia>{media(101),media(202),media(0)});
        QTRY_COMPARE(m_controller->count(),3);select(0,101);
        m_controller->remove(QVector<PlaylistItem>{m_controller->getCurrentItem()},0);
        QTRY_COMPARE(m_controller->count(),2);select(0,202);select(1,0);
        {SourceLocker locker(m_playlist);vlc_playlist_Clear(m_playlist);}
        QTRY_COMPARE(m_controller->count(),0);QTRY_VERIFY(!m_controller->getCurrentItem().raw());
        QCOMPARE(m_controller->currentSourcePlaylistId(),qint64(0));
        m_controller->append(QVector<SourceMedia>{media(303)});QTRY_COMPARE(m_controller->count(),1);select(0,303);
    }
    void invalidSourceIsUnknownAndMetadataIsReadOnly() {
        m_controller->append(QVector<SourceMedia>{media(-5)});QTRY_COMPARE(m_controller->count(),1);select(0,0);
        const auto property=m_controller->metaObject()->property(m_controller->metaObject()->indexOfProperty("currentSourcePlaylistId"));
        QVERIFY(property.isReadable()&&!property.isWritable());
        m_controller->forgetSourcePlaylist(-1);m_controller->forgetSourcePlaylist(0);
        QCOMPARE(m_controller->currentSourcePlaylistId(),qint64(0));QCOMPARE(m_controller->count(),1);
    }
};
QTEST_GUILESS_MAIN(TestPlaylistSource)
#include "test_playlist_source.moc"
