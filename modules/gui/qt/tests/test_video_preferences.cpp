// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include "maininterface/video_preferences_model.hpp"
#include <vlc/vlc.h>
#include <vlc_player.h>
using vlc_player_locker = vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock>;
#include <vlc_playlist.h>
#include <vlc_input.h>
#include <vlc_cxx_helpers.hpp>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMutex>
#include <QMutexLocker>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <memory>
#include <limits>
#include <cmath>
#include <climits>
#ifdef _WIN32
# include <windows.h>
#endif
extern "C" vlc_object_t *vlc_test_video_preferences_object(libvlc_instance_t *);
namespace {
QByteArray bytes(const QString &path){QFile f(path);return f.open(QIODevice::ReadOnly)?f.readAll():QByteArray{};}
QString text(const char *key){char *raw=config_GetPsz(key);const auto result=QString::fromUtf8(raw?raw:"");free(raw);return result;}
#ifdef _WIN32
struct OwnedNoDeleteLock {
    HANDLE handle=INVALID_HANDLE_VALUE;
    explicit OwnedNoDeleteLock(const QString &path){handle=CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);}
    void unlock(){if(handle!=INVALID_HANDLE_VALUE){CloseHandle(handle);handle=INVALID_HANDLE_VALUE;}}
    ~OwnedNoDeleteLock(){unlock();}
};
#endif
}
class TestVideoPreferences final:public QObject
{
    Q_OBJECT
    QTemporaryDir m_temp;
    QString m_root,m_case,m_config,m_fixtureDir;
    libvlc_instance_t *m_instance=nullptr;
    qt_intf_t *m_intf=nullptr;
    std::unique_ptr<VideoPreferencesModel> m_model;
    vlc_player_listener_id *m_listener=nullptr;
    QMutex m_outputLock;
    vout_thread_t *m_output=nullptr;
    static void outputChanged(vlc_player_t *,enum vlc_player_vout_action action,vout_thread_t *output,
                              enum vlc_vout_order order,vlc_es_id_t *,void *opaque)
    {
        if(action!=VLC_PLAYER_VOUT_STARTED||order!=VLC_VOUT_ORDER_PRIMARY)return;
        auto *self=static_cast<TestVideoPreferences *>(opaque);QMutexLocker lock(&self->m_outputLock);
        if(self->m_output)vout_Release(self->m_output);self->m_output=vout_Hold(output);
    }
    vlc_player_t *player()const{return m_intf->p_player;}
    vlc_player_state state()const{vlc_player_locker lock(player());return vlc_player_GetState(player());}
    bool outputReady(){QMutexLocker lock(&m_outputLock);return m_output!=nullptr;}
    bool openNative()
    {
        const QByteArray option=QByteArray("--config=")+m_config.toUtf8();
        const char *args[]={"--no-ignore-config","--no-media-library","--extraintf=","--control=","--no-metadata-network-access",
            "--no-sub-autodetect-file","--no-audio","--no-hw-dec","--vout=dummy","--no-video-title-show","--no-osd","--quiet",option.constData()};
        m_instance=libvlc_new(ARRAY_SIZE(args),args);if(!m_instance)return false;
        const auto root=vlc_test_video_preferences_object(m_instance);m_intf=vlc_object_create<qt_intf_t>(root);if(!m_intf)return false;
        m_intf->p_playlist=vlc_playlist_New(root,VLC_PLAYLIST_PREPARSING_DISABLED,1,0);if(!m_intf->p_playlist)return false;
        m_intf->p_player=vlc_playlist_GetPlayer(m_intf->p_playlist);
        static const vlc_player_cbs callbacks=[]{vlc_player_cbs c{};c.on_vout_changed=outputChanged;return c;}();
        {vlc_player_locker lock(player());m_listener=vlc_player_AddListener(player(),&callbacks,this);}
        if(!m_listener)return false;
        m_model=std::make_unique<VideoPreferencesModel>(m_intf);return true;
    }
    void closeNative()
    {
        m_model.reset();
        if(m_intf&&m_intf->p_player){
            {vlc_player_locker lock(player());vlc_player_Stop(player());}
            QTRY_COMPARE_WITH_TIMEOUT(state(),VLC_PLAYER_STATE_STOPPED,10000);
            {vlc_player_locker lock(player());if(m_listener)vlc_player_RemoveListener(player(),m_listener);m_listener=nullptr;}
        }
        {QMutexLocker lock(&m_outputLock);if(m_output){vout_Release(m_output);m_output=nullptr;}}
        if(m_intf){if(m_intf->p_playlist)vlc_playlist_Delete(m_intf->p_playlist);m_intf->p_playlist=nullptr;m_intf->p_player=nullptr;vlc_object_delete(m_intf);m_intf=nullptr;}
        if(m_instance){libvlc_release(m_instance);m_instance=nullptr;}
    }
    bool start(const QString &name)
    {
        const auto uri=QUrl::fromLocalFile(QDir(m_fixtureDir).filePath(name)).toEncoded();
        auto *item=input_item_New(uri.constData(),nullptr);if(!item)return false;
        int result;{vlc_player_locker lock(player());vlc_player_SetStartPaused(player(),false);result=vlc_player_SetCurrentMedia(player(),item);if(result==VLC_SUCCESS)result=vlc_player_Start(player());}
        input_item_Release(item);return result==VLC_SUCCESS;
    }
    QImage capture(const QString &directory)
    {
        // Genuine native snapshot callback: real filter/encoder/file path.
        // It waits for the next decoded frame, never substitutes a QML image.
        if(!m_model->setValue(QStringLiteral("snapshotDirectory"),directory)
           ||!m_model->setValue(QStringLiteral("snapshotPrefix"),QStringLiteral("owned-proof-"))
           ||!m_model->setValue(QStringLiteral("snapshotFormat"),QStringLiteral("png"))
           ||!m_model->setValue(QStringLiteral("snapshotSequential"),true)
           ||!m_model->setValue(QStringLiteral("snapshotPreview"),false))return {};
        const auto before=QDir(directory).entryList({QStringLiteral("*.png")},QDir::Files);
        vout_thread_t *output;{QMutexLocker lock(&m_outputLock);output=m_output?vout_Hold(m_output):nullptr;}
        if(!output)return {};var_TriggerCallback(output,"video-snapshot");vout_Release(output);
        const auto after=QDir(directory).entryList({QStringLiteral("*.png")},QDir::Files);
        for(const auto &name:after)if(!before.contains(name))return QImage(QDir(directory).filePath(name));
        return {};
    }
private slots:
    void initTestCase()
    {
        QVERIFY(m_temp.isValid());m_root=qEnvironmentVariable("VLC_VIDEO_PREFERENCES_OUTPUT");
        if(m_root.isEmpty())m_root=m_temp.path();else{QVERIFY(QFileInfo(m_root).isAbsolute()&&!QFileInfo::exists(m_root));QVERIFY(QDir().mkpath(m_root));}
        m_fixtureDir=qEnvironmentVariable("VLC_VIDEO_PREFERENCES_FIXTURES");
        QJsonObject hashes;
        for(const auto *name:{"quadrants.mp4","subtitle-style.mp4"}){
            const auto file=QDir(m_fixtureDir).filePath(QString::fromLatin1(name));QVERIFY2(QFileInfo(file).isFile(),"Use exact generated slice fixtures, never human media");
            hashes[QString::fromLatin1(name)]=QString::fromLatin1(QCryptographicHash::hash(bytes(file),QCryptographicHash::Sha256).toHex());
        }
        QFile receipt(QDir(m_root).filePath(QStringLiteral("generated-fixtures.json")));QVERIFY(receipt.open(QIODevice::WriteOnly));receipt.write(QJsonDocument(hashes).toJson());
    }
    void init()
    {
        m_case=QDir(m_root).filePath(QString::fromLatin1(QTest::currentTestFunction()));
        if(const auto *tag=QTest::currentDataTag();tag&&*tag)m_case+=QLatin1Char('/')+QString::fromLatin1(tag);
        QVERIFY(QDir().mkpath(m_case));m_config=QDir(m_case).filePath(QStringLiteral("vlcrc"));
        const auto userdata=QDir(m_case).filePath(QStringLiteral("userdata"));QVERIFY(QDir().mkpath(userdata));qputenv("VLC_USERDATA_PATH",userdata.toUtf8());
        QVERIFY(openNative());
    }
    void cleanup(){closeNative();}
    void exactApiAndReadOnlyReload()
    {
        QCOMPARE(m_model->values().size(),26);QCOMPARE(m_model->capabilities().size(),27);QCOMPARE(m_model->applyModes().size(),27);
        QVERIFY(!m_model->capabilities().value(QStringLiteral("wallpaper")).toBool());QVERIFY(!m_model->restartRequired());
        const auto before=m_model->values();const auto disk=bytes(m_config);QSignalSpy changed(m_model.get(),&VideoPreferencesModel::valuesChanged);
        m_model->reload();QCOMPARE(m_model->values(),before);QCOMPARE(bytes(m_config),disk);QCOMPARE(changed.size(),0);
        VideoPreferencesModel absent(nullptr);for(const auto &value:absent.capabilities())QVERIFY(!value.toBool());
        QVERIFY(!absent.setValue(QStringLiteral("enableVideo"),false));QVERIFY(!absent.lastError().isEmpty());
    }
    void invalidTypesRangesAndPathsPreserveNativeState()
    {
        const QList<QPair<QString,QVariant>> invalid={
            {QStringLiteral("enableVideo"),1},{QStringLiteral("disableScreensaver"),2},{QStringLiteral("deinterlace"),true},
            {QStringLiteral("hue"),QStringLiteral("10")},{QStringLiteral("hue"),181.},{QStringLiteral("gamma"),0.},
            {QStringLiteral("brightness"),std::numeric_limits<double>::quiet_NaN()},{QStringLiteral("contrast"),std::numeric_limits<double>::infinity()},
            {QStringLiteral("saturation"),3.1},{QStringLiteral("position"),3},{QStringLiteral("zoom"),1.0},{QStringLiteral("zoom"),QStringLiteral("7")},
            {QStringLiteral("deinterlaceMode"),QStringLiteral("not-a-native-mode")},{QStringLiteral("aspectRatio"),QStringLiteral("0:9")},
            {QStringLiteral("crop"),QStringLiteral("1:0")},{QStringLiteral("customAspectRatios"),QStringLiteral("16:9,,4:3")},
            {QStringLiteral("customCropRatios"),QStringLiteral(" 4:3")},{QStringLiteral("monitorPixelAspectRatio"),QStringLiteral("nan")},
            {QStringLiteral("snapshotDirectory"),QStringLiteral("https://example.invalid/")},{QStringLiteral("snapshotPrefix"),QStringLiteral("../outside-")},
            {QStringLiteral("snapshotFormat"),QStringLiteral("gif")},{QStringLiteral("snapshotSize"),QStringLiteral("99999x99999")},
            {QStringLiteral("snapshotSize"),2},{QStringLiteral("timeShiftBufferSize"),1},{QStringLiteral("timeShiftBufferSize"),qlonglong(INT_MAX)+1},
            {QStringLiteral("unknown"),false}};
        const auto before=m_model->values(),caps=m_model->capabilities();const auto disk=bytes(m_config);QSignalSpy changed(m_model.get(),&VideoPreferencesModel::valuesChanged);
        for(const auto &item:invalid){QVERIFY(!m_model->setValue(item.first,item.second));QVERIFY(!m_model->lastError().isEmpty());QCOMPARE(m_model->values(),before);QCOMPARE(bytes(m_config),disk);}
        QCOMPARE(m_model->capabilities(),caps);QCOMPARE(changed.size(),0);
    }
    void boolIntegerAndUtf8PersistenceSurvivesFreshInstance()
    {
        QVERIFY(m_model->setValue(QStringLiteral("enableVideo"),false));QVERIFY(m_model->setValue(QStringLiteral("snapshotPrefix"),QStringLiteral("näyte-")));
        QVERIFY(m_model->setValue(QStringLiteral("timeShiftBufferSize"),qlonglong(16*1024*1024)));QVERIFY(!m_model->restartRequired());
        closeNative();QVERIFY(openNative());QVERIFY(!m_model->values().value(QStringLiteral("enableVideo")).toBool());
        QCOMPARE(m_model->values().value(QStringLiteral("snapshotPrefix")).toString(),QStringLiteral("näyte-"));
        QCOMPARE(m_model->values().value(QStringLiteral("timeShiftBufferSize")).toLongLong(),qlonglong(16*1024*1024));
        QCOMPARE(config_GetInt("video"),int64_t(0));QCOMPARE(config_GetInt("input-timeshift-granularity"),int64_t(16*1024*1024));
        QVERIFY(m_model->setValue(QStringLiteral("gamma"),QVariant::fromValue(.01f)));QVERIFY(m_model->restartRequired());
        closeNative();QVERIFY(openNative());QCOMPARE(config_GetFloat("gamma"),.01f);QCOMPARE(m_model->values().value(QStringLiteral("gamma")).toFloat(),.01f);
        QVERIFY(!m_model->restartRequired());
    }
    void rawFullscreenOnlyScreensaverPolicyIsNotRewritten()
    {
        config_PutInt("disable-screensaver",2);m_model->reload();QCOMPARE(m_model->nativeScreensaverMode(),2);
        QVERIFY(m_model->values().value(QStringLiteral("disableScreensaver")).toBool());
        QVERIFY(m_model->setValue(QStringLiteral("snapshotPrefix"),QStringLiteral("other-")));QCOMPARE(config_GetInt("disable-screensaver"),int64_t(2));
        QVERIFY(m_model->setValue(QStringLiteral("disableScreensaver"),false));QCOMPARE(config_GetInt("disable-screensaver"),int64_t(0));
        QVERIFY(m_model->setValue(QStringLiteral("disableScreensaver"),true));QCOMPARE(config_GetInt("disable-screensaver"),int64_t(1));
        const int64_t unknown=int64_t(INT_MAX)+2;config_PutInt("disable-screensaver",unknown);m_model->reload();
        QCOMPARE(m_model->nativeScreensaverMode(),-1);QVERIFY(!m_model->capabilities().value(QStringLiteral("disableScreensaver")).toBool());
        QVERIFY(!m_model->setValue(QStringLiteral("disableScreensaver"),true));
        QVERIFY(m_model->setValue(QStringLiteral("snapshotPrefix"),QStringLiteral("preserve-unknown-")));QCOMPARE(config_GetInt("disable-screensaver"),unknown);
    }
    void wallpaperAndGuilessPickerAreHonestRefusals()
    {
        const auto disk=bytes(m_config);const auto before=m_model->values();
        QVERIFY(!m_model->setValue(QStringLiteral("wallpaper"),true));QVERIFY(!m_model->chooseSnapshotDirectory());
        QVERIFY(!m_model->pickerActive());QCOMPARE(bytes(m_config),disk);QCOMPARE(m_model->values(),before);
    }
    void effectiveInlinePictureDefaultsAndOtherFiltersArePreserved()
    {
        config_PutPsz("video-filter","marq{marquee='owned:a:b'}:adjust{hue=30,brightness=1.25}:rotate{angle=0}");m_model->reload();
        QCOMPARE(m_model->values().value(QStringLiteral("hue")).toFloat(),30.f);QCOMPARE(m_model->values().value(QStringLiteral("brightness")).toFloat(),1.25f);
        QVERIFY(m_model->setValue(QStringLiteral("contrast"),1.5));
        QCOMPARE(config_GetFloat("hue"),30.f);QCOMPARE(config_GetFloat("brightness"),1.25f);QCOMPARE(config_GetFloat("contrast"),1.5f);
        QCOMPARE(text("video-filter"),QStringLiteral("marq{marquee='owned:a:b'}:adjust:rotate{angle=0}"));
        QVERIFY(m_model->resetPicture());QCOMPARE(text("video-filter"),QStringLiteral("marq{marquee='owned:a:b'}:rotate{angle=0}"));
        QCOMPARE(config_GetFloat("hue"),0.f);QCOMPARE(config_GetFloat("brightness"),1.f);QCOMPARE(config_GetFloat("contrast"),1.f);
    }
    void complexAdjustChainsRefuseWithoutLosingOptions()
    {
        for(const auto *chain:{"adjust{unknown=1}","adjust:adjust{hue=30}","adjust{hue=nan}"}){
            config_PutPsz("video-filter",chain);m_model->reload();const auto before=text("video-filter");const auto disk=bytes(m_config);
            QVERIFY(!m_model->capabilities().value(QStringLiteral("hue")).toBool());QVERIFY(!m_model->setValue(QStringLiteral("hue"),20.));QVERIFY(!m_model->resetPicture());
            QCOMPARE(text("video-filter"),before);QCOMPARE(bytes(m_config),disk);
        }
    }
    void pairedSnapshotSizeAndUnknownNativeDefaultsArePreserved()
    {
        config_PutInt("snapshot-width",777);config_PutInt("snapshot-height",555);m_model->reload();
        QCOMPARE(m_model->values().value(QStringLiteral("snapshotSize")).toString(),QStringLiteral("777x555"));
        QVERIFY(m_model->setValue(QStringLiteral("snapshotPrefix"),QStringLiteral("unchanged-size-")));QCOMPARE(config_GetInt("snapshot-width"),int64_t(777));
        QVERIFY(m_model->setValue(QStringLiteral("snapshotSize"),QStringLiteral("640x360")));QCOMPARE(config_GetInt("snapshot-width"),int64_t(640));QCOMPARE(config_GetInt("snapshot-height"),int64_t(360));
        QVERIFY(m_model->setValue(QStringLiteral("snapshotSize"),QStringLiteral("source")));QCOMPARE(config_GetInt("snapshot-width"),int64_t(-1));QCOMPARE(config_GetInt("snapshot-height"),int64_t(-1));
    }
    void restartFingerprintRevertsAndCurrentDummyRemainsUnchanged()
    {
        auto *dummy=vlc_player_vout_Hold(player());QVERIFY(dummy);const auto old=var_GetFloat(dummy,"zoom");
        QVERIFY(m_model->setValue(QStringLiteral("zoom"),QStringLiteral("2")));QVERIFY(m_model->restartRequired());QCOMPARE(var_GetFloat(dummy,"zoom"),old);
        QVERIFY(!m_model->values().value(QStringLiteral("scaleToWindow")).toBool());
        QVERIFY(m_model->setValue(QStringLiteral("zoom"),QStringLiteral("fit")));QVERIFY(!m_model->restartRequired());vout_Release(dummy);
    }
    void actualFramingAndNativeCustomChoicesAfterRestart()
    {
        QVERIFY(m_model->setValue(QStringLiteral("zoom"),QStringLiteral("2")));QVERIFY(m_model->setValue(QStringLiteral("aspectRatio"),QStringLiteral("16:9")));
        QVERIFY(m_model->setValue(QStringLiteral("crop"),QStringLiteral("4:3")));QVERIFY(m_model->setValue(QStringLiteral("customAspectRatios"),QStringLiteral("21:9,32:9")));
        QVERIFY(m_model->setValue(QStringLiteral("customCropRatios"),QStringLiteral("7:5")));QVERIFY(m_model->setValue(QStringLiteral("position"),9));
        closeNative();QVERIFY(openNative());QVERIFY(!m_model->restartRequired());QVERIFY(start(QStringLiteral("quadrants.mp4")));
        QTRY_VERIFY_WITH_TIMEOUT(outputReady(),10000);QTRY_COMPARE_WITH_TIMEOUT(state(),VLC_PLAYER_STATE_PLAYING,10000);
        QMutexLocker lock(&m_outputLock);QCOMPARE(var_GetFloat(m_output,"zoom"),2.f);QVERIFY(!var_GetBool(m_output,"autoscale"));
        const auto aspect=vlc::wrap_cptr(var_GetString(m_output,"aspect-ratio"));const auto crop=vlc::wrap_cptr(var_GetString(m_output,"crop"));
        QCOMPARE(QByteArray(aspect.get()),QByteArray("16:9"));QCOMPARE(QByteArray(crop.get()),QByteArray("4:3"));QCOMPARE(var_GetInteger(m_output,"align"),int64_t(9));
        vlc_value_t *values=nullptr;char **labels=nullptr;size_t count=0;QCOMPARE(var_Change(m_output,"aspect-ratio",VLC_VAR_GETCHOICES,&count,&values,&labels),VLC_SUCCESS);
        bool found=false;for(size_t i=0;i<count;++i){found|=QByteArray(values[i].psz_string)=="21:9";free(values[i].psz_string);free(labels[i]);}free(values);free(labels);QVERIFY(found);
    }
    void actualPictureFilterPixelsAfterSavedRestart_data()
    {
        QTest::addColumn<QString>("key");QTest::addColumn<double>("value");QTest::addColumn<QString>("source");
        QTest::newRow("hue")<<QStringLiteral("hue")<<90.<<QStringLiteral("quadrants.mp4");
        QTest::newRow("brightness")<<QStringLiteral("brightness")<<0.<<QStringLiteral("subtitle-style.mp4");
        QTest::newRow("contrast")<<QStringLiteral("contrast")<<0.<<QStringLiteral("subtitle-style.mp4");
        QTest::newRow("saturation")<<QStringLiteral("saturation")<<0.<<QStringLiteral("quadrants.mp4");
        QTest::newRow("gamma")<<QStringLiteral("gamma")<<2.<<QStringLiteral("subtitle-style.mp4");
    }
    void actualPictureFilterPixelsAfterSavedRestart()
    {
        QFETCH(QString,key);QFETCH(double,value);QFETCH(QString,source);
        const auto baselineDir=QDir(m_case).filePath(QStringLiteral("baseline"));QVERIFY(QDir().mkpath(baselineDir));
        QVERIFY(start(source));QTRY_VERIFY_WITH_TIMEOUT(outputReady(),10000);const auto before=capture(baselineDir);QVERIFY(!before.isNull());
        QVERIFY(m_model->setValue(key,value));QVERIFY(m_model->restartRequired());closeNative();QVERIFY(openNative());
        QVERIFY(start(source));QTRY_VERIFY_WITH_TIMEOUT(outputReady(),10000);
        const auto afterDir=QDir(m_case).filePath(QStringLiteral("changed"));QVERIFY(QDir().mkpath(afterDir));const auto after=capture(afterDir);QVERIFY(!after.isNull());
        const QPoint sample(before.width()/4,before.height()/4);const auto old=before.pixelColor(sample),changed=after.pixelColor(sample);
        const int distance=std::abs(old.red()-changed.red())+std::abs(old.green()-changed.green())+std::abs(old.blue()-changed.blue());QVERIFY(distance>30);
        if(key==QStringLiteral("brightness"))QVERIFY(qMax(changed.red(),qMax(changed.green(),changed.blue()))<12);
        if(key==QStringLiteral("saturation"))QVERIFY(qMax(changed.red(),qMax(changed.green(),changed.blue()))-qMin(changed.red(),qMin(changed.green(),changed.blue()))<10);
        if(key==QStringLiteral("gamma"))QVERIFY(changed.red()>old.red()+20);
    }
    void genuineSnapshotsRespectSizePrefixFormatAndSequentialNames()
    {
        QVERIFY(m_model->setValue(QStringLiteral("snapshotSize"),QStringLiteral("640x360")));QVERIFY(start(QStringLiteral("quadrants.mp4")));QTRY_VERIFY_WITH_TIMEOUT(outputReady(),10000);
        const auto directory=QDir(m_case).filePath(QStringLiteral("snapshots"));QVERIFY(QDir().mkpath(directory));
        const auto first=capture(directory),second=capture(directory);QVERIFY(!first.isNull()&&!second.isNull());QCOMPARE(first.size(),QSize(640,360));
        const auto names=QDir(directory).entryList({QStringLiteral("owned-proof-*.png")},QDir::Files);QCOMPARE(names.size(),2);QVERIFY(names[0]!=names[1]);
        for(const auto &name:names){const auto raw=bytes(QDir(directory).filePath(name));QVERIFY(raw.startsWith(QByteArray("\x89PNG\r\n\x1a\n",8)));}
    }
    void resetPictureRestoresActualNativePixelsAfterRestart()
    {
        QVERIFY(m_model->setValue(QStringLiteral("brightness"),0.));QVERIFY(m_model->setValue(QStringLiteral("gamma"),2.));
        closeNative();QVERIFY(openNative());QVERIFY(start(QStringLiteral("subtitle-style.mp4")));QTRY_VERIFY_WITH_TIMEOUT(outputReady(),10000);
        const auto blackDir=QDir(m_case).filePath(QStringLiteral("black"));QVERIFY(QDir().mkpath(blackDir));const auto black=capture(blackDir);QVERIFY(!black.isNull());
        const auto dark=black.pixelColor(black.width()/4,black.height()/4);QVERIFY(qMax(dark.red(),qMax(dark.green(),dark.blue()))<12);
        QVERIFY(m_model->resetPicture());QVERIFY(m_model->restartRequired());closeNative();QVERIFY(openNative());
        // Brightness changes luma, preserving saturated chroma. Use the
        // generated neutral source for the exact black/restored-gray proof.
        QVERIFY(start(QStringLiteral("subtitle-style.mp4")));QTRY_VERIFY_WITH_TIMEOUT(outputReady(),10000);
        const auto neutralDir=QDir(m_case).filePath(QStringLiteral("restored-neutral"));QVERIFY(QDir().mkpath(neutralDir));
        const auto neutral=capture(neutralDir);QVERIFY(!neutral.isNull());
        const auto gray=neutral.pixelColor(neutral.width()/4,neutral.height()/4);
        QVERIFY(gray.red()>55&&gray.red()<75&&gray.green()>55&&gray.green()<75&&gray.blue()>55&&gray.blue()<75);
        QCOMPARE(config_GetFloat("brightness"),1.f);QCOMPARE(config_GetFloat("gamma"),1.f);
        closeNative();QVERIFY(openNative());
        QVERIFY(start(QStringLiteral("quadrants.mp4")));QTRY_VERIFY_WITH_TIMEOUT(outputReady(),10000);
        const auto restoredDir=QDir(m_case).filePath(QStringLiteral("restored"));QVERIFY(QDir().mkpath(restoredDir));const auto restored=capture(restoredDir);QVERIFY(!restored.isNull());
        const auto red=restored.pixelColor(restored.width()/4,restored.height()/4);QVERIFY(red.red()>180&&red.green()<80&&red.blue()<80);
        QVERIFY(!text("video-filter").contains(QStringLiteral("adjust")));QVERIFY(!m_model->restartRequired());
    }
    void checkedSaveFailureRollbackAndOrdinaryRetry_data()
    {
        QTest::addColumn<QString>("key");QTest::addColumn<QVariant>("value");
        QTest::newRow("boolean")<<QStringLiteral("enableVideo")<<QVariant(false);
        QTest::newRow("integer")<<QStringLiteral("timeShiftBufferSize")<<QVariant(qlonglong(16*1024*1024));
        QTest::newRow("float-chain")<<QStringLiteral("gamma")<<QVariant(2.);
        QTest::newRow("paired-size")<<QStringLiteral("snapshotSize")<<QVariant(QStringLiteral("640x360"));
    }
    void checkedSaveFailureRollbackAndOrdinaryRetry()
    {
#ifndef _WIN32
        QSKIP("Windows deny-delete fixture; Root's native acceptance is Windows");
#else
        QFETCH(QString,key);QFETCH(QVariant,value);QCOMPARE(config_SaveConfigFile(m_intf),0);
        const auto disk=bytes(m_config);const auto before=m_model->values();const auto filter=text("video-filter");QSignalSpy changed(m_model.get(),&VideoPreferencesModel::valuesChanged);
        const auto nativeState=[] {return QVariantList{qlonglong(config_GetInt("video")),qlonglong(config_GetInt("input-timeshift-granularity")),config_GetFloat("gamma"),text("video-filter"),qlonglong(config_GetInt("snapshot-width")),qlonglong(config_GetInt("snapshot-height"))};};
        const auto nativeBefore=nativeState();
        OwnedNoDeleteLock lock(m_config);QVERIFY(lock.handle!=INVALID_HANDLE_VALUE);
        const bool rejected=m_model->setValue(key,value);const auto failedDisk=bytes(m_config);const auto failed=m_model->values();const auto error=m_model->lastError();
        const auto nativeFailed=nativeState();
        lock.unlock();m_model->reload();const bool retried=m_model->setValue(key,value);
        QVERIFY(!rejected);QVERIFY(!error.isEmpty());QCOMPARE(failedDisk,disk);QCOMPARE(failed,before);
        QCOMPARE(nativeFailed,nativeBefore);
        QVERIFY(retried);QVERIFY(m_model->lastError().isEmpty());QVERIFY(bytes(m_config)!=disk);QVERIFY(changed.size()>0);
        if(key!=QStringLiteral("gamma"))QCOMPARE(text("video-filter"),filter);
#endif
    }
};
QTEST_GUILESS_MAIN(TestVideoPreferences)
#include "test_video_preferences.moc"
