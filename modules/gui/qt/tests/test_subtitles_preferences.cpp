// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include "maininterface/subtitles_preferences_model.hpp"
#include "maininterface/subtitle_preset_model.hpp"
#include "player/subtitle_appearance.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <vlc_modules.h>
#include <vlc_player.h>
using vlc_player_locker = vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock>;
#include <vlc_playlist.h>
#include <vlc_input.h>
#include <vlc_vout_osd.h>
#include <vlc_subpicture.h>
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
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#ifdef _WIN32
# include <windows.h>
#endif
extern "C" vlc_object_t *vlc_test_subtitles_object(libvlc_instance_t *);
extern "C" void vlc_test_subtitles_vmem(vlc_object_t *,libvlc_video_lock_cb,libvlc_video_unlock_cb,libvlc_video_display_cb,void *);
namespace {
QByteArray bytes(const QString &path){QFile f(path);return f.open(QIODevice::ReadOnly)?f.readAll():QByteArray{};}
bool write(const QString &path,const QByteArray &data){QFile f(path);return f.open(QIODevice::WriteOnly)&&f.write(data)==data.size();}
QString text(const char *name){auto p=vlc::wrap_cptr(config_GetPsz(name));return QString::fromUtf8(p.get()?p.get():"");}
#ifdef _WIN32
struct OwnedNoDeleteLock {
    HANDLE handle=INVALID_HANDLE_VALUE;
    explicit OwnedNoDeleteLock(const QString &path){handle=CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);}
    void unlock(){if(handle!=INVALID_HANDLE_VALUE){CloseHandle(handle);handle=INVALID_HANDLE_VALUE;}}
    ~OwnedNoDeleteLock(){unlock();}
};
#endif
}
class TestSubtitlesPreferences final:public QObject
{
    Q_OBJECT
    QTemporaryDir m_temp;
    QString m_root,m_case,m_config,m_fixtures;
    libvlc_instance_t *m_instance=nullptr;
    qt_intf_t *m_intf=nullptr;
    std::unique_ptr<SubtitlesPreferencesModel> m_model;
    std::unique_ptr<QSettings> m_settings;
    std::unique_ptr<SubtitleAppearance> m_style;
    std::unique_ptr<SubtitlePresetModel> m_presets;
    vlc_player_listener_id *m_listener=nullptr;
    QMutex m_outputMutex,m_pixelsMutex;
    vout_thread_t *m_output=nullptr;
    alignas(32) std::array<unsigned char,320*180*4> m_buffer{};
    QImage m_image;
    std::atomic<unsigned> m_frames{0};
    int m_injectedAlignment=-1,m_injectedDuration=-1;
    bool m_injectedShortcutOff=false;
    bool m_artifactFailure=false;
    static void *lockVideo(void *opaque,void **planes)
    {auto *self=static_cast<TestSubtitlesPreferences *>(opaque);self->m_pixelsMutex.lock();*planes=self->m_buffer.data();return nullptr;}
    static void unlockVideo(void *opaque,void *,void *const *)
    {auto *self=static_cast<TestSubtitlesPreferences *>(opaque);self->m_image=QImage(self->m_buffer.data(),320,180,1280,QImage::Format_RGB32).copy();self->m_pixelsMutex.unlock();self->m_frames.fetch_add(1);}
    static void displayVideo(void *,void *){}
    static void outputChanged(vlc_player_t *,vlc_player_vout_action action,vout_thread_t *output,vlc_vout_order order,vlc_es_id_t *,void *opaque)
    {
        if(action!=VLC_PLAYER_VOUT_STARTED||order!=VLC_VOUT_ORDER_PRIMARY)return;
        auto *self=static_cast<TestSubtitlesPreferences *>(opaque);QMutexLocker guard(&self->m_outputMutex);
        if(self->m_output)vout_Release(self->m_output);self->m_output=vout_Hold(output);
    }
    vlc_player_t *player()const{return m_intf->p_player;}
    vlc_player_state state()const{vlc_player_locker guard(player());return vlc_player_GetState(player());}
    QString selectedLanguage()const
    {vlc_player_locker guard(player());const auto *track=vlc_player_GetSelectedTrack(player(),SPU_ES);return track?QString::fromUtf8(track->fmt.psz_language?track->fmt.psz_language:""):QString{};}
    size_t trackCount()const{vlc_player_locker guard(player());return vlc_player_GetTrackCount(player(),SPU_ES);}
    vlc_tick_t delay()const{vlc_player_locker guard(player());return vlc_player_GetSubtitleDelay(player());}
    float rate()const{vlc_player_locker guard(player());return vlc_player_GetRate(player());}
    bool subtitlesEnabled()const{vlc_player_locker guard(player());return vlc_player_IsSubtitleEnabled(player());}
    QImage frame(const QString &name={})
    {QMutexLocker guard(&m_pixelsMutex);auto result=m_image;if(!name.isEmpty()&&!result.save(QDir(m_case).filePath(name+QStringLiteral(".png"))))m_artifactFailure=true;return result;}
    static QRect pixels(const QImage &image,int kind,int *count=nullptr)
    {
        QRect bounds;int total=0;
        for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x) {
            const auto c=image.pixelColor(x,y);
            const bool match=kind==0?c.red()>200&&c.green()>200&&c.blue()>200
                :kind==1?c.red()>180&&c.green()<80&&c.blue()<80
                :kind==2?c.blue()>180&&c.red()<80&&c.green()<80
                :kind==3?c.red()<20&&c.green()<20&&c.blue()<20
                :std::abs(c.red()-64)+std::abs(c.green()-64)+std::abs(c.blue()-64)>75;
            if(match){bounds|=QRect(x,y,1,1);++total;}
        }
        if(count)*count=total;return bounds;
    }
    bool openNative()
    {
        const QByteArray configArg=QByteArray("--config=")+m_config.toUtf8();
        const char *args[]={"--no-ignore-config","--no-media-library","--extraintf=","--control=","--no-metadata-network-access","--no-audio","--no-hw-dec","--vout=vmem","--quiet",configArg.constData()};
        m_instance=libvlc_new(ARRAY_SIZE(args),args);if(!m_instance)return false;
        auto *root=vlc_test_subtitles_object(m_instance);vlc_test_subtitles_vmem(root,lockVideo,unlockVideo,displayVideo,this);
        // Keep the genuine VMEM renderer; select the shipped non-GUI window owner without native-window fallback.
        if(var_SetString(root,"window","dummy,none")!=VLC_SUCCESS)return false;
        // Deliberate owned-variable seam makes the SAME rendered oracle runnable
        // against original core (which lacks these config options) and fixed core.
        if(m_injectedAlignment!=-1){var_Create(root,"sub-primary-alignment",VLC_VAR_INTEGER);var_SetInteger(root,"sub-primary-alignment",m_injectedAlignment);}
        if(m_injectedDuration!=-1){var_Create(root,"osd-feedback-duration",VLC_VAR_INTEGER);var_SetInteger(root,"osd-feedback-duration",m_injectedDuration);}
        if(m_injectedShortcutOff){var_Create(root,"osd-shortcut-feedback",VLC_VAR_BOOL);var_SetBool(root,"osd-shortcut-feedback",false);}
        m_intf=vlc_object_create<qt_intf_t>(root);if(!m_intf)return false;
        m_intf->p_playlist=vlc_playlist_New(root,VLC_PLAYLIST_PREPARSING_DISABLED,1,0);if(!m_intf->p_playlist)return false;
        m_intf->p_player=vlc_playlist_GetPlayer(m_intf->p_playlist);
        static const vlc_player_cbs callbacks=[]{vlc_player_cbs c{};c.on_vout_changed=outputChanged;return c;}();
        {vlc_player_locker guard(player());m_listener=vlc_player_AddListener(player(),&callbacks,this);}if(!m_listener)return false;
        SubtitlesPreferencesModel::Consumers consumers;
        if(qEnvironmentVariableIntValue("VLC_SUBTITLES_CONSUMERS_VERIFIED")==1){consumers.primaryAlignment=true;consumers.feedbackDuration=true;consumers.shortcutFeedback=true;}
        m_model=std::make_unique<SubtitlesPreferencesModel>(m_intf,nullptr,consumers);return true;
    }
    void closeNative()
    {
        m_model.reset();m_presets.reset();m_style.reset();m_settings.reset();
        bool stopped=true;
        if(m_intf&&m_intf->p_player) {
            {vlc_player_locker guard(player());vlc_player_Stop(player());}
            stopped=QTest::qWaitFor([this]{return state()==VLC_PLAYER_STATE_STOPPED;},10000);
            {vlc_player_locker guard(player());if(m_listener)vlc_player_RemoveListener(player(),m_listener);m_listener=nullptr;}
        }
        {QMutexLocker guard(&m_outputMutex);if(m_output){vout_Release(m_output);m_output=nullptr;}}
        if(m_intf){if(m_intf->p_playlist)vlc_playlist_Delete(m_intf->p_playlist);m_intf->p_playlist=nullptr;m_intf->p_player=nullptr;vlc_object_delete(m_intf);m_intf=nullptr;}
        if(m_instance){libvlc_release(m_instance);m_instance=nullptr;}
        {QMutexLocker guard(&m_pixelsMutex);m_image={};}m_frames=0;
        QVERIFY2(stopped,"Native subtitle owner did not stop within ten seconds; all owners were nevertheless joined/released");
    }
    bool start(const QString &path)
    {
        auto *item=input_item_New(QUrl::fromLocalFile(path).toEncoded().constData(),nullptr);if(!item)return false;
        int result;{vlc_player_locker guard(player());vlc_player_SetStartPaused(player(),false);result=vlc_player_SetCurrentMedia(player(),item);if(result==VLC_SUCCESS)result=vlc_player_Start(player());}
        input_item_Release(item);return result==VLC_SUCCESS;
    }
    bool makePlainSubtitle(const QString &base=QStringLiteral("owned"))
    {return write(QDir(m_case).filePath(base+QStringLiteral(".srt")),bytes(QDir(m_fixtures).filePath(QStringLiteral("plain.srt"))));}
    QString media()const{return QDir(m_case).filePath(QStringLiteral("owned.mp4"));}
private slots:
    void initTestCase()
    {
        QVERIFY(m_temp.isValid());m_root=qEnvironmentVariable("VLC_SUBTITLES_PREFERENCES_OUTPUT");
        if(m_root.isEmpty())m_root=m_temp.path();else{QVERIFY(QFileInfo(m_root).isAbsolute()&&!QFileInfo::exists(m_root));QVERIFY(QDir().mkpath(m_root));}
        m_fixtures=qEnvironmentVariable("VLC_SUBTITLES_PREFERENCES_FIXTURES");QJsonObject hashes;
        for(const auto *name:{"plain.mp4","plain.srt","languages.mkv"}) {
            const auto path=QDir(m_fixtures).filePath(QString::fromLatin1(name));QVERIFY2(QFileInfo(path).isFile(),"Generate owned fixtures with tests/generate_subtitles_fixtures.sh");
            hashes[QString::fromLatin1(name)]=QString::fromLatin1(QCryptographicHash::hash(bytes(path),QCryptographicHash::Sha256).toHex());
        }
        QVERIFY(write(QDir(m_root).filePath(QStringLiteral("generated-fixtures.json")),QJsonDocument(hashes).toJson()));
    }
    void init()
    {
        m_case=QDir(m_root).filePath(QString::fromLatin1(QTest::currentTestFunction()));
        if(const auto *tag=QTest::currentDataTag();tag&&*tag)m_case+=QLatin1Char('/')+QString::fromLatin1(tag);
        QVERIFY(QDir().mkpath(m_case));m_config=QDir(m_case).filePath(QStringLiteral("vlcrc"));
        const auto userdata=QDir(m_case).filePath(QStringLiteral("userdata"));QVERIFY(QDir().mkpath(userdata));qputenv("VLC_USERDATA_PATH",userdata.toUtf8());
        QVERIFY(write(m_config,QByteArray("[core]\nvideo-title-show=0\nosd=1\nsub-autodetect-file=1\n")));
        QVERIFY(QFile::copy(QDir(m_fixtures).filePath(QStringLiteral("plain.mp4")),media()));
        m_injectedAlignment=-1;m_injectedDuration=-1;m_injectedShortcutOff=false;m_artifactFailure=false;QVERIFY(openNative());
    }
    void cleanup(){closeNative();QVERIFY2(!m_artifactFailure,"Native rendered frame artifact could not be saved");}
    void exactApiReadOnlyAndHonestGates()
    {
        QCOMPARE(m_model->values().size(),31);QCOMPARE(m_model->capabilities().size(),31);QCOMPARE(m_model->applyModes().size(),31);
        for(const auto *key:{"ignoreAssStyling","visualizationSize"}){QVERIFY(!m_model->capabilities().value(QString::fromLatin1(key)).toBool());QVERIFY(!m_model->setValue(QString::fromLatin1(key),true));}
        SubtitlesPreferencesModel unverified(m_intf);QVERIFY(!unverified.capabilities().value(QStringLiteral("verticalPosition")).toBool());QVERIFY(!unverified.capabilities().value(QStringLiteral("feedbackDuration")).toBool());
        QVERIFY(!unverified.capabilities().value(QStringLiteral("showFeedback")).toBool());
        const auto before=m_model->values();const auto disk=bytes(m_config);QSignalSpy changed(m_model.get(),&SubtitlesPreferencesModel::valuesChanged);
        m_model->reload();QCOMPARE(m_model->values(),before);QCOMPARE(bytes(m_config),disk);QCOMPARE(changed.size(),0);QVERIFY(!m_model->restartRequired());
        SubtitlesPreferencesModel absent(nullptr);for(const auto &cap:absent.capabilities())QVERIFY(!cap.toBool());QVERIFY(!absent.setValue(QStringLiteral("autoLoad"),false));
    }
    void invalidTypesRangesAndUtf8NeverWrite()
    {
        const QList<QPair<QString,QVariant>> invalid={
            {QStringLiteral("enableSubtitles"),1},{QStringLiteral("autoLoad"),QStringLiteral("true")},
            {QStringLiteral("preferredLanguage"),QStringLiteral("zzzz,fin")},{QStringLiteral("preferredLanguage"),QStringLiteral("eng,,fin")},
            {QStringLiteral("textEncoding"),QStringLiteral("invalid-encoding")},{QStringLiteral("fontFamily"),QString(1,QChar(0xD800))},
            {QStringLiteral("fontFamily"),QStringLiteral("a\nb")},{QStringLiteral("sizeMode"),.5},
            {QStringLiteral("fontSize"),0.},{QStringLiteral("fontSize"),std::numeric_limits<double>::quiet_NaN()},
            {QStringLiteral("textColor"),QStringLiteral("#1234")},{QStringLiteral("outlineThickness"),51.},
            {QStringLiteral("textOpacity"),101.},{QStringLiteral("backgroundOpacity"),-1.},
            {QStringLiteral("shadow"),1},{QStringLiteral("shadowDistance"),1.1},{QStringLiteral("shadowAngle"),361.},
            {QStringLiteral("background"),QStringLiteral("transparent")},{QStringLiteral("subtitleDelay"),150},
            {QStringLiteral("subtitleDelay"),5001},{QStringLiteral("filenameDuration"),999},
            {QStringLiteral("searchFolders"),QStringLiteral("https://example.invalid/")},{QStringLiteral("searchFolders"),QStringLiteral("relative")},
            {QStringLiteral("searchFolders"),QStringLiteral("./subs,,./other")},{QStringLiteral("unknown"),false}};
        const auto before=m_model->values();const auto disk=bytes(m_config);QSignalSpy changed(m_model.get(),&SubtitlesPreferencesModel::valuesChanged);
        for(const auto &entry:invalid){QVERIFY(!m_model->setValue(entry.first,entry.second));QVERIFY(!m_model->lastError().isEmpty());QCOMPARE(m_model->values(),before);QCOMPARE(bytes(m_config),disk);}
        QCOMPARE(changed.size(),0);
    }
    void actualTypedPersistenceAndInactiveUnitsSurviveFreshInstance()
    {
        QVERIFY(m_model->setValue(QStringLiteral("fontFamily"),QStringLiteral("Arial")));QVERIFY(m_model->setValue(QStringLiteral("fontSize"),8.));
        QVERIFY(m_model->setValue(QStringLiteral("outlineThickness"),6.));QVERIFY(m_model->setValue(QStringLiteral("sizeMode"),1));
        QVERIFY(m_model->setValue(QStringLiteral("fontSize"),28.));QVERIFY(m_model->setValue(QStringLiteral("outlineThickness"),3.));
        QVERIFY(m_model->setValue(QStringLiteral("textColor"),QStringLiteral("#a1B2c3")));QVERIFY(m_model->setValue(QStringLiteral("textOpacity"),50.));
        QVERIFY(m_model->setValue(QStringLiteral("shadowDistance"),.125));QVERIFY(m_model->setValue(QStringLiteral("shadowAngle"),45.));
        QVERIFY(m_model->setValue(QStringLiteral("monospaceFont"),QStringLiteral("Courier New")));QVERIFY(m_model->restartRequired());
        closeNative();QVERIFY(openNative());QVERIFY(!m_model->restartRequired());
        QCOMPARE(config_GetFloat("sub-text-style-relative-size"),8.f);QCOMPARE(config_GetFloat("sub-text-style-pixel-size"),28.f);
        QCOMPARE(config_GetFloat("sub-text-style-relative-outline"),6.f);QCOMPARE(config_GetFloat("sub-text-style-pixel-outline"),3.f);
        QCOMPARE(text("sub-text-style-font"),QStringLiteral("Arial"));QCOMPARE(text("freetype-monofont"),QStringLiteral("Courier New"));
        QCOMPARE(m_model->values().value(QStringLiteral("textColor")).toString(),QStringLiteral("#A1B2C3"));
        QCOMPARE(config_GetInt("freetype-opacity"),int64_t(128));QCOMPARE(config_GetFloat("freetype-shadow-distance"),.125f);
        QVERIFY(m_model->setValue(QStringLiteral("sizeMode"),0));QCOMPARE(m_model->values().value(QStringLiteral("fontSize")).toFloat(),8.f);
    }
    void firstStyleCommitPreservesLegacyDefaultsAndDurationChain()
    {
        config_PutPsz("freetype-font","Courier New");config_PutInt("freetype-color",0x12AB34);config_PutInt("freetype-outline-color",0x112233);
        config_PutInt("freetype-background-opacity",77);config_PutInt("freetype-outline-thickness",6);
        config_PutPsz("sub-filter","subsdelay{mode=0,factor=1.5,overlap=2}:marq{marquee='owned:a'}");m_model->reload();
        const auto chain=text("sub-filter");QVERIFY(m_model->setValue(QStringLiteral("fontSize"),10.));
        QCOMPARE(config_GetInt("sub-text-style-color"),int64_t(0x12AB34));QCOMPARE(config_GetInt("sub-text-style-outline-color"),int64_t(0x112233));
        QCOMPARE(config_GetInt("sub-text-style-background-opacity"),int64_t(77));QCOMPARE(text("sub-text-style-font"),QStringLiteral("Courier New"));
        QCOMPARE(config_GetFloat("sub-text-style-relative-outline"),6.f);QCOMPARE(text("sub-filter"),chain);QVERIFY(config_GetInt("sub-text-style-embedded"));
    }
    void qmlDoubleAndNativeFloatMinimumBothWork()
    {
        QVERIFY(m_model->setValue(QStringLiteral("fontSize"),QVariant(.1)));QCOMPARE(config_GetFloat("sub-text-style-relative-size"),.1f);
        QVERIFY(m_model->setValue(QStringLiteral("fontSize"),QVariant::fromValue(.1f)));QCOMPARE(config_GetFloat("sub-text-style-relative-size"),.1f);
        const auto disk=bytes(m_config);const auto values=m_model->values();
        QVERIFY(!m_model->setValue(QStringLiteral("fontSize"),QVariant(std::nextafter(.1,0.))));
        QVERIFY(!m_model->setValue(QStringLiteral("fontSize"),QVariant::fromValue(std::nextafter(.1f,0.f))));
        QCOMPARE(bytes(m_config),disk);QCOMPARE(m_model->values(),values);
    }
    void rawUnknownDefaultsAndNativeDirectionOrderPreserved()
    {
        config_PutInt("sub-delay",99);config_PutInt("sub-text-scale",150);m_model->reload();
        QCOMPARE(m_model->values().value(QStringLiteral("subtitleDelay")).toLongLong(),qlonglong(9900));
        QVERIFY(m_model->setValue(QStringLiteral("autoLoad"),false));QCOMPARE(config_GetInt("sub-delay"),int64_t(99));QCOMPARE(config_GetInt("sub-text-scale"),int64_t(150));
        if(config_GetType("freetype-text-direction")==VLC_VAR_INTEGER){
            const auto choices=m_model->choices().value(QStringLiteral("textDirection")).toList();QCOMPARE(choices.size(),3);
            for(int i=0;i<3;++i)QCOMPARE(choices[i].toMap().value(QStringLiteral("value")).toInt(),i);
        }
    }
    void backgroundAndShadowUseRealNativeOpacity()
    {
        QVERIFY(m_model->setValue(QStringLiteral("background"),QStringLiteral("box")));QCOMPARE(config_GetInt("sub-text-style-background-opacity"),int64_t(128));
        QVERIFY(m_model->setValue(QStringLiteral("backgroundOpacity"),100.));QCOMPARE(config_GetInt("sub-text-style-background-opacity"),int64_t(255));
        QVERIFY(m_model->setValue(QStringLiteral("background"),QStringLiteral("none")));QCOMPARE(config_GetInt("sub-text-style-background-opacity"),int64_t(0));
        QVERIFY(m_model->setValue(QStringLiteral("shadow"),false));QCOMPARE(config_GetInt("freetype-shadow-opacity"),int64_t(0));
        QVERIFY(m_model->setValue(QStringLiteral("shadow"),true));QCOMPARE(config_GetInt("freetype-shadow-opacity"),int64_t(128));
    }
    void directNativeDefaultsAndRestartFingerprintRoundTrip()
    {
        const bool bold=m_model->values().value(QStringLiteral("bold")).toBool();
        QVERIFY(m_model->setValue(QStringLiteral("bold"),!bold));QVERIFY(m_model->restartRequired());
        QVERIFY(m_model->setValue(QStringLiteral("bold"),bold));QVERIFY(!m_model->restartRequired());
        QVERIFY(m_model->setValue(QStringLiteral("shadowColor"),QStringLiteral("#123456")));
        QVERIFY(m_model->setValue(QStringLiteral("backgroundColor"),QStringLiteral("#102030")));
        if(m_model->capabilities().value(QStringLiteral("textDirection")).toBool())QVERIFY(m_model->setValue(QStringLiteral("textDirection"),2));
        const auto nativeFolders=text("sub-autodetect-path");QVERIFY(m_model->setValue(QStringLiteral("searchFolders"),nativeFolders));
        QVERIFY(m_model->setValue(QStringLiteral("textEncoding"),QString{}));closeNative();QVERIFY(openNative());
        QCOMPARE(config_GetInt("freetype-shadow-color"),int64_t(0x123456));QCOMPARE(config_GetInt("freetype-background-color"),int64_t(0x102030));
        QCOMPARE(text("sub-autodetect-path"),nativeFolders);QCOMPARE(text("subsdec-encoding"),QString{});QVERIFY(!m_model->restartRequired());
        config_PutInt("osd",0);m_model->reload();const auto before=bytes(m_config);
        QVERIFY(!m_model->capabilities().value(QStringLiteral("showFeedback")).toBool());QVERIFY(!m_model->capabilities().value(QStringLiteral("showFilename")).toBool());
        QVERIFY(!m_model->setValue(QStringLiteral("showFeedback"),true));QCOMPARE(config_GetInt("osd"),int64_t(0));QCOMPARE(bytes(m_config),before);
    }
    void savedSearchFoldersUseNativeSeparatorsAndPreserveGrammar()
    {
        const auto subdir=QDir(m_case).filePath(QStringLiteral("tekstitys-\u00e4"));QVERIFY(QDir().mkpath(subdir));
        const QString requested=subdir+QStringLiteral(", ./subs, .\\subtitles");
#ifdef _WIN32
        const QString expected=QDir::toNativeSeparators(subdir)+QStringLiteral(", .\\subs, .\\subtitles");
#else
        const QString expected=requested;
#endif
        QVERIFY(m_model->setValue(QStringLiteral("searchFolders"),requested));
        QCOMPARE(text("sub-autodetect-path"),expected);
        QCOMPARE(m_model->values().value(QStringLiteral("searchFolders")).toString(),expected);
        const QByteArray expectedLine=QByteArray("sub-autodetect-path=")+expected.toUtf8();
        auto saved=bytes(m_config);saved.replace("\r\n","\n");QVERIFY(saved.split('\n').contains(expectedLine));
        QVERIFY(!m_model->restartRequired());closeNative();QVERIFY(openNative());
        QCOMPARE(text("sub-autodetect-path"),expected);QVERIFY(!m_model->restartRequired());
        // Existing unsupported/legacy values are preserved until this field is edited.
        const auto legacy=QStringLiteral("legacy/no-rewrite");const auto legacyUtf8=legacy.toUtf8();
        config_PutPsz("sub-autodetect-path",legacyUtf8.constData());m_model->reload();
        QCOMPARE(m_model->values().value(QStringLiteral("searchFolders")).toString(),legacy);
        const auto disk=bytes(m_config);m_model->reload();QCOMPARE(bytes(m_config),disk);
        QCOMPARE(text("sub-autodetect-path"),legacy);
        QVERIFY(m_model->setValue(QStringLiteral("autoLoad"),false));
        QCOMPARE(text("sub-autodetect-path"),legacy);
        QCOMPARE(m_model->values().value(QStringLiteral("searchFolders")).toString(),legacy);
    }
    void realAutomaticLoadingMatchingAndUnicodeFolders()
    {
        const auto subdir=QDir(m_case).filePath(QStringLiteral("tekstitys-ä"));QVERIFY(QDir().mkpath(subdir));
        QVERIFY(write(QDir(subdir).filePath(QStringLiteral("owned-extra.srt")),bytes(QDir(m_fixtures).filePath(QStringLiteral("plain.srt")))));
        QVERIFY(m_model->setValue(QStringLiteral("searchFolders"),subdir));QVERIFY(m_model->setValue(QStringLiteral("filenameMatching"),3));
        QVERIFY(start(media()));QTRY_VERIFY_WITH_TIMEOUT(trackCount()>0,10000);QTRY_VERIFY_WITH_TIMEOUT(!pixels(frame(),0).isEmpty(),10000);
        closeNative();QVERIFY(openNative());QVERIFY(m_model->setValue(QStringLiteral("filenameMatching"),4));
        QVERIFY(start(media()));QTRY_COMPARE_WITH_TIMEOUT(state(),VLC_PLAYER_STATE_PLAYING,10000);QTest::qWait(700);QCOMPARE(trackCount(),size_t(0));
        closeNative();QVERIFY(openNative());QVERIFY(m_model->setValue(QStringLiteral("filenameMatching"),3));QVERIFY(m_model->setValue(QStringLiteral("autoLoad"),false));
        QVERIFY(start(media()));QTRY_COMPARE_WITH_TIMEOUT(state(),VLC_PLAYER_STATE_PLAYING,10000);QTest::qWait(700);QCOMPARE(trackCount(),size_t(0));
    }
    void actualLanguagePolicyAffectsNextInputNotCurrentTrack()
    {
        const auto file=QDir(m_fixtures).filePath(QStringLiteral("languages.mkv"));QVERIFY(m_model->setValue(QStringLiteral("preferredLanguage"),QStringLiteral("eng")));
        QVERIFY(start(file));QTRY_COMPARE_WITH_TIMEOUT(selectedLanguage(),QStringLiteral("eng"),10000);
        QVERIFY(m_model->setValue(QStringLiteral("preferredLanguage"),QStringLiteral("fin")));QCOMPARE(selectedLanguage(),QStringLiteral("eng"));QVERIFY(!m_model->restartRequired());
        {vlc_player_locker guard(player());auto lang=vlc::wrap_cptr(vlc_player_GetCategoryLanguage(player(),SPU_ES));QCOMPARE(QByteArray(lang.get()),QByteArray("fin"));vlc_player_Stop(player());}
        QTRY_COMPARE_WITH_TIMEOUT(state(),VLC_PLAYER_STATE_STOPPED,10000);
        {vlc_player_locker guard(player());QCOMPARE(vlc_player_SetCurrentMedia(player(),nullptr),VLC_SUCCESS);}
        QVERIFY(start(file));QTRY_COMPARE_WITH_TIMEOUT(selectedLanguage(),QStringLiteral("fin"),10000);
    }
    void nativeDelayTenthsAndRestartDefaultIsolation()
    {
        QVERIFY(makePlainSubtitle());QVERIFY(start(media()));QTRY_VERIFY_WITH_TIMEOUT(trackCount()>0,10000);QCOMPARE(delay(),vlc_tick_t(0));
        QVERIFY(m_model->setValue(QStringLiteral("subtitleDelay"),1200));QCOMPARE(config_GetInt("sub-delay"),int64_t(12));QCOMPARE(delay(),vlc_tick_t(0));QVERIFY(!m_model->restartRequired());
        auto *dummy=vlc_player_vout_Hold(player());QVERIFY(dummy);const bool enabled=var_GetBool(dummy,"sub-text-style-enabled");
        QVERIFY(m_model->setValue(QStringLiteral("textColor"),QStringLiteral("#FF0000")));QVERIFY(m_model->restartRequired());QCOMPARE(var_GetBool(dummy,"sub-text-style-enabled"),enabled);vout_Release(dummy);
        closeNative();QVERIFY(openNative());QVERIFY(start(media()));QTRY_VERIFY_WITH_TIMEOUT(trackCount()>0,10000);QCOMPARE(delay(),VLC_TICK_FROM_MS(1200));
        QTRY_VERIFY_WITH_TIMEOUT(!pixels(frame(),1).isEmpty(),10000);frame(QStringLiteral("delayed-red-after-restart"));
    }
    void subtitleEnableDefaultPreservesCurrentPlaybackThenAppliesOnRestart()
    {
        QVERIFY(makePlainSubtitle());QVERIFY(start(media()));QTRY_VERIFY_WITH_TIMEOUT(!pixels(frame(),0).isEmpty(),10000);
        QVERIFY(m_model->setValue(QStringLiteral("enableSubtitles"),false));QVERIFY(m_model->restartRequired());QVERIFY(subtitlesEnabled());
        QVERIFY(!pixels(frame(),0).isEmpty());closeNative();QVERIFY(openNative());QVERIFY(!subtitlesEnabled());
        QVERIFY(start(media()));QTRY_VERIFY_WITH_TIMEOUT(m_frames.load()>8,10000);QVERIFY(pixels(frame(QStringLiteral("subtitles-disabled")),0).isEmpty());
        QVERIFY(m_model->setValue(QStringLiteral("enableSubtitles"),true));closeNative();QVERIFY(openNative());QVERIFY(start(media()));QTRY_VERIFY_WITH_TIMEOUT(!pixels(frame(),0).isEmpty(),10000);
    }
    void savedAppearanceChangesActualRenderedPixels_data()
    {
        QTest::addColumn<QString>("key");QTest::addColumn<QVariant>("value");
        QTest::newRow("text-red")<<QStringLiteral("textColor")<<QVariant(QStringLiteral("#FF0000"));
        QTest::newRow("larger-font")<<QStringLiteral("fontSize")<<QVariant(28.);
        QTest::newRow("blue-outline")<<QStringLiteral("outlineColor")<<QVariant(QStringLiteral("#0000FF"));
        QTest::newRow("opaque-background")<<QStringLiteral("backgroundOpacity")<<QVariant(100.);
        QTest::newRow("transparent-text")<<QStringLiteral("textOpacity")<<QVariant(0.);
    }
    void savedAppearanceChangesActualRenderedPixels()
    {
        QFETCH(QString,key);QFETCH(QVariant,value);QVERIFY(makePlainSubtitle());
        QVERIFY(m_model->setValue(QStringLiteral("sizeMode"),1));QVERIFY(m_model->setValue(QStringLiteral("fontSize"),18.));
        QVERIFY(m_model->setValue(QStringLiteral("outlineThickness"),key==QStringLiteral("outlineColor")?3.:0.));QVERIFY(m_model->setValue(QStringLiteral("shadow"),false));
        closeNative();QVERIFY(openNative());QVERIFY(start(media()));QTRY_VERIFY_WITH_TIMEOUT(!pixels(frame(),0).isEmpty(),10000);const auto before=frame(QStringLiteral("baseline"));
        QVERIFY(m_model->setValue(key,value));QVERIFY(m_model->restartRequired());closeNative();QVERIFY(openNative());QVERIFY(start(media()));
        QTRY_VERIFY_WITH_TIMEOUT(m_frames.load()>8,10000);QTest::qWait(300);const auto after=frame(QStringLiteral("saved-restarted"));QVERIFY(!after.isNull());
        if(key==QStringLiteral("textColor"))QVERIFY(!pixels(after,1).isEmpty());
        else if(key==QStringLiteral("fontSize"))QVERIFY(pixels(after,0).height()>pixels(before,0).height()*1.3);
        else if(key==QStringLiteral("outlineColor"))QVERIFY(!pixels(after,2).isEmpty());
        else if(key==QStringLiteral("backgroundOpacity")){int a=0,b=0;pixels(before,3,&a);pixels(after,3,&b);QVERIFY(b>a+100);}
        else QVERIFY(pixels(after,0).isEmpty());
    }
    void defaultPresetRefreshPreservesSavedAndLiveState()
    {
        QVERIFY(makePlainSubtitle());QVERIFY(start(media()));QTRY_VERIFY_WITH_TIMEOUT(!pixels(frame(),0).isEmpty(),10000);
        m_settings=std::make_unique<QSettings>(QDir(m_case).filePath(QStringLiteral("presets.ini")),QSettings::IniFormat);
        m_style=std::make_unique<SubtitleAppearance>(player());
        {QMutexLocker guard(&m_outputMutex);QVERIFY(m_output);m_style->resetOutput(m_output);}
        m_presets=std::make_unique<SubtitlePresetModel>(player(),m_style.get(),m_settings.get(),VLC_OBJECT(m_intf));
        QTRY_VERIFY(m_presets->canSave());m_style->setTextColor(QStringLiteral("#FF0000"));QVERIFY(m_presets->save(QStringLiteral("owned preset")));
        m_style->setTextColor(QStringLiteral("#0000FF"));QTRY_VERIFY(m_presets->edited());
        const auto settingsFile=m_settings->fileName();const auto stored=bytes(settingsFile);const auto entries=m_presets->entries();const auto selected=m_presets->selectedId();
        m_model=std::make_unique<SubtitlesPreferencesModel>(m_intf,m_presets.get(),SubtitlesPreferencesModel::Consumers{});
        QVERIFY(m_model->setValue(QStringLiteral("fontSize"),9.));QCOMPARE(m_style->textColor(),QStringLiteral("#0000FF"));
        QVERIFY(m_presets->edited());QCOMPARE(m_presets->entries(),entries);QCOMPARE(m_presets->selectedId(),selected);QCOMPARE(bytes(settingsFile),stored);
        m_presets.reset();QVERIFY(m_model->setValue(QStringLiteral("fontSize"),10.)); // guarded optional lifetime
    }
    void corePrimaryPlacementUsesActualSubtitlePixels()
    {
        closeNative();m_injectedAlignment=4;QVERIFY(openNative());QVERIFY(makePlainSubtitle());QVERIFY(start(media()));
        QTRY_VERIFY_WITH_TIMEOUT(!pixels(frame(),0).isEmpty(),10000);const auto bounds=pixels(frame(QStringLiteral("primary-top")),0);
        QVERIFY2(bounds.center().y()<60,"Relative primary subtitle remains at bottom: original core must fail this SAME desired oracle");
    }
    void verifiedConsumerControlsPersistAndDoNotAcceptOrphanTypes()
    {
        QVERIFY2(qEnvironmentVariableIntValue("VLC_SUBTITLES_CONSUMERS_VERIFIED")==1,"Root must replay original/fixed renderer pair before opting in to consumers");
        QVERIFY(m_model->capabilities().value(QStringLiteral("verticalPosition")).toBool());QVERIFY(m_model->capabilities().value(QStringLiteral("feedbackDuration")).toBool());
        QVERIFY(m_model->capabilities().value(QStringLiteral("showFeedback")).toBool());
        const auto before=bytes(m_config);QVERIFY(!m_model->setValue(QStringLiteral("verticalPosition"),2));QVERIFY(!m_model->setValue(QStringLiteral("feedbackDuration"),499));QVERIFY(!m_model->setValue(QStringLiteral("feedbackDuration"),true));QCOMPARE(bytes(m_config),before);
        QVERIFY(m_model->setValue(QStringLiteral("feedbackDuration"),3000));QVERIFY(!m_model->restartRequired());
        QVERIFY(m_model->setValue(QStringLiteral("verticalPosition"),4));QVERIFY(m_model->restartRequired());closeNative();QVERIFY(openNative());
        QCOMPARE(config_GetInt("osd-feedback-duration"),int64_t(3000));QCOMPARE(config_GetInt("sub-primary-alignment"),int64_t(4));QVERIFY(!m_model->restartRequired());
        QVERIFY(m_model->setValue(QStringLiteral("showFeedback"),false));QCOMPARE(config_GetInt("osd"),int64_t(1));QCOMPARE(config_GetInt("osd-shortcut-feedback"),int64_t(0));QVERIFY(!m_model->restartRequired());
    }
    void coreShortcutSwitchLeavesExplicitMessagesIndependent_data()
    {QTest::addColumn<int>("kind");QTest::newRow("text")<<0;QTest::newRow("icon")<<1;QTest::newRow("slider")<<2;}
    void coreShortcutSwitchLeavesExplicitMessagesIndependent()
    {
        QFETCH(int,kind);closeNative();m_injectedShortcutOff=true;QVERIFY(openNative());QVERIFY(start(media()));QTRY_VERIFY_WITH_TIMEOUT(m_frames.load()>5,10000);
        vout_thread_t *output;{QMutexLocker guard(&m_outputMutex);output=m_output?vout_Hold(m_output):nullptr;}QVERIFY(output);
        const auto channel=vout_RegisterSubpictureChannel(output);QVERIFY(channel>=0);
        // Explicit title-like messages must remain visible when shortcuts are off.
        vout_OSDText(output,int(channel),SUBPICTURE_ALIGN_BOTTOM,VLC_TICK_FROM_MS(500),"Explicit filename-like caller");
        QTRY_VERIFY_WITH_TIMEOUT(!pixels(frame(),0).isEmpty(),400);QTest::qWait(700);
        if(kind==0)vout_OSDMessage(output,int(channel),"Owned shortcut");else if(kind==1)vout_OSDIcon(output,int(channel),OSD_PAUSE_ICON);else vout_OSDSlider(output,int(channel),70,OSD_VERT_SLIDER);
        QTest::qWait(300);const auto suppressed=frame(QStringLiteral("shortcut-off"));vout_UnregisterSubpictureChannel(output,size_t(channel));vout_Release(output);
        QVERIFY2(pixels(suppressed,4).isEmpty(),"Original core ignores the shortcut-only switch; same desired oracle must fail there");
    }
    void explicitOsdDurationAndBottomPlacementRemainIndependent()
    {
        closeNative();m_injectedAlignment=4;m_injectedDuration=500;QVERIFY(openNative());QVERIFY(start(media()));QTRY_VERIFY_WITH_TIMEOUT(m_frames.load()>5,10000);
        vout_thread_t *output;{QMutexLocker guard(&m_outputMutex);output=m_output?vout_Hold(m_output):nullptr;}QVERIFY(output);
        const auto channel=vout_RegisterSubpictureChannel(output);QVERIFY(channel>=0);
        vout_OSDText(output,int(channel),SUBPICTURE_ALIGN_BOTTOM|SUBPICTURE_ALIGN_RIGHT,VLC_TICK_FROM_MS(2200),"Explicit caller");
        QTRY_VERIFY_WITH_TIMEOUT(!pixels(frame(),0).isEmpty(),500);QTest::qWait(1300);const auto bounds=pixels(frame(QStringLiteral("explicit-osd-after-1300ms")),0);
        QTest::qWait(1200);const auto expired=frame(QStringLiteral("explicit-osd-expired"));vout_UnregisterSubpictureChannel(output,size_t(channel));vout_Release(output);
        QVERIFY(!bounds.isEmpty());QVERIFY(bounds.center().y()>110);QVERIFY(pixels(expired,4).isEmpty());
    }
    void realPlayerShortcutDoesNotFlushExistingFilenameWhenDisabled()
    {
        closeNative();m_injectedShortcutOff=true;QVERIFY(openNative());QVERIFY(start(media()));QTRY_VERIFY_WITH_TIMEOUT(m_frames.load()>5,10000);
        vout_thread_t *output;{QMutexLocker guard(&m_outputMutex);output=m_output?vout_Hold(m_output):nullptr;}QVERIFY(output);
        vout_OSDText(output,VOUT_SPU_CHANNEL_OSD,SUBPICTURE_ALIGN_BOTTOM,VLC_TICK_FROM_MS(3000),"Owned filename caption");
        QTRY_VERIFY_WITH_TIMEOUT(!pixels(frame(),0).isEmpty(),500);
        {vlc_player_locker guard(player());QVERIFY(vlc_player_CanChangeRate(player()));vlc_player_ChangeRate(player(),2.f);}
        QTRY_COMPARE_WITH_TIMEOUT(rate(),2.f,2000);
        QTest::qWait(300);const auto retained=pixels(frame(QStringLiteral("filename-after-real-rate-shortcut")),0);vout_Release(output);
        QVERIFY2(!retained.isEmpty()&&retained.center().y()>110,"Disabled real shortcut flushed/replaced the explicit filename caption");
    }
    void actualFilenameMessageUsesSavedShowAndDurationDefaults()
    {
        QVERIFY(m_model->setValue(QStringLiteral("showFilename"),true));QVERIFY(m_model->setValue(QStringLiteral("filenameDuration"),1500));
        closeNative();QVERIFY(openNative());QVERIFY(start(media()));QTRY_VERIFY_WITH_TIMEOUT(!pixels(frame(),0).isEmpty(),10000);frame(QStringLiteral("filename-visible"));
        QTest::qWait(1900);QVERIFY(pixels(frame(QStringLiteral("filename-expired")),4).isEmpty());
    }
    void coreFeedbackDurationUsesActualTextIconsAndSliders_data()
    {QTest::addColumn<int>("kind");QTest::newRow("text")<<0;QTest::newRow("icon")<<1;QTest::newRow("slider")<<2;}
    void coreFeedbackDurationUsesActualTextIconsAndSliders()
    {
        QFETCH(int,kind);closeNative();m_injectedDuration=3000;QVERIFY(openNative());QVERIFY(start(media()));QTRY_VERIFY_WITH_TIMEOUT(m_frames.load()>5,10000);
        vout_thread_t *output;{QMutexLocker guard(&m_outputMutex);output=m_output?vout_Hold(m_output):nullptr;}QVERIFY(output);
        const auto channel=vout_RegisterSubpictureChannel(output);QVERIFY(channel>=0);
        if(kind==0)vout_OSDMessage(output,int(channel),"Owned feedback");else if(kind==1)vout_OSDIcon(output,int(channel),OSD_PAUSE_ICON);else vout_OSDSlider(output,int(channel),70,OSD_VERT_SLIDER);
        QTRY_VERIFY_WITH_TIMEOUT(!pixels(frame(),4).isEmpty(),500);const auto started=frame(QStringLiteral("feedback-start"));QVERIFY(!started.isNull());
        QTest::qWait(1600);const auto held=frame(QStringLiteral("feedback-at-1600ms"));
        const bool stillVisible=!pixels(held,4).isEmpty();QTest::qWait(1900);const auto expired=frame(QStringLiteral("feedback-at-3500ms"));
        vout_UnregisterSubpictureChannel(output,size_t(channel));vout_Release(output);
        QVERIFY2(stillVisible,"Shortcut overlay expired at legacy duration: original core must fail the SAME desired oracle");QVERIFY(pixels(expired,4).isEmpty());
    }
    void checkedSaveFailureRollbackAndRealRetry_data()
    {
        QTest::addColumn<QString>("key");QTest::addColumn<QVariant>("value");
        QTest::newRow("bool")<<QStringLiteral("autoLoad")<<QVariant(false);
        QTest::newRow("int")<<QStringLiteral("subtitleDelay")<<QVariant(1200);
        QTest::newRow("float")<<QStringLiteral("shadowDistance")<<QVariant(.125);
        QTest::newRow("style-pack")<<QStringLiteral("fontSize")<<QVariant(10.);
        QTest::newRow("language-cache")<<QStringLiteral("preferredLanguage")<<QVariant(QStringLiteral("fin"));
    }
    void checkedSaveFailureRollbackAndRealRetry()
    {
#ifndef _WIN32
        QSKIP("Real Windows no-delete replacement failure fixture");
#else
        QFETCH(QString,key);QFETCH(QVariant,value);QCOMPARE(config_SaveConfigFile(m_intf),0);
        const auto native=[] {return QVariantList{bool(config_GetInt("sub-autodetect-file")),qlonglong(config_GetInt("sub-delay")),config_GetFloat("freetype-shadow-distance"),bool(config_GetInt("sub-text-style-enabled")),config_GetFloat("sub-text-style-relative-size"),text("sub-language")};};
        const auto before=m_model->values();const auto disk=bytes(m_config);const auto initial=native();QByteArray languageBefore;
        {vlc_player_locker guard(player());auto language=vlc::wrap_cptr(vlc_player_GetCategoryLanguage(player(),SPU_ES));languageBefore=QByteArray(language.get());}
        QSignalSpy changed(m_model.get(),&SubtitlesPreferencesModel::valuesChanged);OwnedNoDeleteLock lock(m_config);QVERIFY(lock.handle!=INVALID_HANDLE_VALUE);
        const bool accepted=m_model->setValue(key,value);const auto failedNative=native();const auto failedDisk=bytes(m_config);const auto failedValues=m_model->values();const auto error=m_model->lastError();const auto failedSignals=changed.size();QByteArray languageFailed;
        {vlc_player_locker guard(player());auto language=vlc::wrap_cptr(vlc_player_GetCategoryLanguage(player(),SPU_ES));languageFailed=QByteArray(language.get());}
        lock.unlock();m_model->reload();const bool retried=m_model->setValue(key,value);
        QVERIFY(!accepted);QVERIFY(!error.isEmpty());QCOMPARE(failedNative,initial);QCOMPARE(failedDisk,disk);QCOMPARE(failedValues,before);QCOMPARE(failedSignals,0);QCOMPARE(languageFailed,languageBefore);
        QVERIFY(retried);QVERIFY(m_model->lastError().isEmpty());QVERIFY(bytes(m_config)!=disk);QVERIFY(changed.size()>0);
#endif
    }
};
QTEST_GUILESS_MAIN(TestSubtitlesPreferences)
#include "test_subtitles_preferences.moc"
