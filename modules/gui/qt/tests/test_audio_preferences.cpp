// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include "maininterface/audio_preferences_model.hpp"
#include "player/audio_equalizer.hpp"
#include "util/audio_defaults_policy.hpp"
#include "util/audio_windows_devices.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <vlc_modules.h>
#include <vlc_player.h>
#include <vlc_playlist.h>
#include <vlc_input.h>
#include <vlc_aout.h>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
extern "C" const char vlc_module_name[] = "audio_preferences";
extern "C" vlc_object_t *vlc_test_general_instance_object(libvlc_instance_t *);
extern "C" int vlc_test_audio_preferences_amem(vlc_object_t *,
    void (*)(void *,const void *,unsigned,int64_t),void *);
using AudioPcmPlayerLocker = vlc_locker<vlc_player_t,vlc_player_Lock,vlc_player_Unlock>;

class TestAudioPreferences final : public QObject
{
    Q_OBJECT
    QTemporaryDir m_temporary;
    QString m_output,m_config;
    libvlc_instance_t *m_instance=nullptr;
    qt_intf_t *m_intf=nullptr;
    std::unique_ptr<AudioPreferencesModel> m_model;
    std::unique_ptr<AudioEqualizer> m_liveEqualizer;
    std::atomic<float> m_rms{0}; std::atomic<unsigned> m_buffers{0};
    static QByteArray bytes(const QString &path) { QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray{}; }
    static bool write(const QString &path,const QByteArray &data) { QFile file(path);return file.open(QIODevice::WriteOnly)&&file.write(data)==data.size(); }
    static QString text(const char *key) { char *raw=config_GetPsz(key);const auto result=QString::fromUtf8(raw?raw:"");free(raw);return result; }
    static void samples(void *opaque,const void *data,unsigned frames,int64_t) {
        auto *self=static_cast<TestAudioPreferences *>(opaque);const auto *pcm=static_cast<const float *>(data);
        double sum=0;for(unsigned i=0;i<frames;++i)sum+=double(pcm[i])*pcm[i];
        self->m_rms.store(frames?float(std::sqrt(sum/frames)):0);self->m_buffers.fetch_add(1);
    }
    bool openNative() {
        const QByteArray option=QByteArray("--config=")+m_config.toUtf8();
        const char *args[]={"--no-ignore-config","--no-media-library","--quiet","--vout=vdummy",option.constData()};
        m_instance=libvlc_new(ARRAY_SIZE(args),args);if(!m_instance)return false;
        m_intf=vlc_object_create<qt_intf_t>(vlc_test_general_instance_object(m_instance));
        if(!m_intf)return false;m_model=std::make_unique<AudioPreferencesModel>(m_intf);return true;
    }
    void closeNative() {
        m_liveEqualizer.reset();
        bool stopped=true;
        if(m_intf&&m_intf->p_playlist){
            stopOwnedPcm();stopped=QTest::qWaitFor([this]{return ownedPcmState()==VLC_PLAYER_STATE_STOPPED;},10000);
            releaseOwnedPcm();
        }
        m_model.reset();if(m_intf){vlc_object_delete(m_intf);m_intf=nullptr;}
        if(m_instance){libvlc_release(m_instance);m_instance=nullptr;}
        QVERIFY2(stopped,"Native PCM owner did not stop within ten seconds; all owners were nevertheless joined/released");
    }
    void enableStepConsumer() {AudioPreferencesModel::Consumers linked;linked.authoredVolumeStep=true;m_model=std::make_unique<AudioPreferencesModel>(m_intf,linked);}
    vlc_player_state ownedPcmState() const {
        AudioPcmPlayerLocker lock(m_intf->p_player);return vlc_player_GetState(m_intf->p_player);
    }
    void stopOwnedPcm() {
        vlc_playlist_Lock(m_intf->p_playlist);vlc_playlist_Stop(m_intf->p_playlist);vlc_playlist_Unlock(m_intf->p_playlist);
    }
    void releaseOwnedPcm() {
        // Delete joins the actual native player/input before callback data can die.
        m_liveEqualizer.reset();vlc_playlist_Delete(m_intf->p_playlist);
        m_intf->p_playlist=nullptr;m_intf->p_player=nullptr;
    }
    bool startOwnedPcm(const QString &path) {
        if(m_intf->p_playlist)return false;
        m_rms=0;m_buffers=0;
        auto *root=vlc_test_general_instance_object(m_instance);
        // Fixture-only transport, installed before the native owner creates aout.
        // Never mask or manually copy the saved audio-filter/EQ defaults.
        if(vlc_test_audio_preferences_amem(root,samples,this)!=VLC_SUCCESS)return false;
        m_intf->p_playlist=vlc_playlist_New(root,VLC_PLAYLIST_PREPARSING_DISABLED,1,0);
        if(!m_intf->p_playlist)return false;
        m_intf->p_player=vlc_playlist_GetPlayer(m_intf->p_playlist);
        const QByteArray uri=QUrl::fromLocalFile(path).toEncoded();
        auto *media=input_item_New(uri.constData(),nullptr);if(!media)return false;
        vlc_playlist_Lock(m_intf->p_playlist);
        int result=vlc_playlist_InsertOne(m_intf->p_playlist,0,media);
        if(result==VLC_SUCCESS)result=vlc_playlist_PlayAt(m_intf->p_playlist,0);
        vlc_playlist_Unlock(m_intf->p_playlist);input_item_Release(media);
        return result==VLC_SUCCESS;
    }
private slots:
    void initTestCase() {
        QVERIFY(m_temporary.isValid());m_output=qEnvironmentVariable("VLC_AUDIO_DEFAULTS_OUTPUT");
        if(m_output.isEmpty())m_output=m_temporary.path();else {QVERIFY(QFileInfo(m_output).isAbsolute());QVERIFY(!QFileInfo::exists(m_output));QVERIFY(QDir().mkpath(m_output));}
        const auto userdata=QDir(m_output).filePath(QStringLiteral("userdata"));QVERIFY(QDir().mkpath(userdata));qputenv("VLC_USERDATA_PATH",userdata.toUtf8());
    }
    void init() {
        const auto name=QString::fromLatin1(QTest::currentTestFunction());
        auto folder=QDir(m_output).filePath(name);if(const auto tag=QTest::currentDataTag();tag&&*tag)folder+='/'+QString::fromLatin1(tag);
        QVERIFY(QDir().mkpath(folder));m_config=QDir(folder).filePath(QStringLiteral("vlcrc"));QVERIFY(openNative());
    }
    void cleanup() {closeNative();}
    void exactApiDefaultsAndHonestConsumerGates() {
        QCOMPARE(m_model->values().size(),32);QCOMPARE(m_model->values().keys(),m_model->capabilities().keys());
        QVERIFY(!m_model->restartRequired());QVERIFY(!QFileInfo::exists(m_config));
        for(const auto *key:{"nightMode","spatialAudio","spatialAudioMode","visualizationSize","outputDevice","startupVolume","passthrough","volumeStep"})
            QVERIFY2(!m_model->capabilities().value(QString::fromLatin1(key)).toBool(),key);
        QVERIFY(m_model->devices().isEmpty());QCOMPARE(m_model->equalizerFrequencies().size(),10);
        QCOMPARE(m_model->options().value(QStringLiteral("equalizerPreset")).toList().size(),18);
        QCOMPARE(m_model->ranges().value(QStringLiteral("startupVolume")).toMap().value(QStringLiteral("maximum")).toDouble(),125.);
        const auto before=m_model->values();m_model->reload();QCOMPARE(m_model->values(),before);QVERIFY(!QFileInfo::exists(m_config));
        const auto speakers=m_model->options().value(QStringLiteral("speakerSetup")).toList();
        bool hasBinaural=false;for(const auto &row:speakers)hasBinaural|=row.toMap().value(QStringLiteral("value")).toInt()==2;
        QCOMPARE(hasBinaural,module_exists("spatialaudio"));
    }
    void nativeBooleanDefaults_data() {
        QTest::addColumn<QString>("key");QTest::addColumn<QByteArray>("option");
        QTest::newRow("audio")<<QStringLiteral("audioEnabled")<<QByteArray("audio");
        QTest::newRow("remember")<<QStringLiteral("rememberVolume")<<QByteArray("volume-save");
        QTest::newRow("two-pass")<<QStringLiteral("equalizerTwoPass")<<QByteArray("equalizer-2pass");
    }
    void nativeBooleanDefaults() {
        QFETCH(QString,key);QFETCH(QByteArray,option);QCOMPARE(config_GetType(option.constData()),VLC_VAR_BOOL);
        QVERIFY(m_model->capabilities().value(key).toBool());const bool old=config_GetInt(option.constData());
        QSignalSpy changed(m_model.get(),&AudioPreferencesModel::valuesChanged);
        QVERIFY2(m_model->setValue(key,!old),qPrintable(m_model->lastError()));QCOMPARE(bool(config_GetInt(option.constData())),!old);
        QCOMPARE(m_model->values().value(key).toBool(),!old);QCOMPARE(changed.size(),1);QVERIFY(m_model->restartRequired());
        QVERIFY(m_model->setValue(key,old));QVERIFY(!m_model->restartRequired());
        QVERIFY(m_model->setValue(key,!old));closeNative();QVERIFY(openNative());QCOMPARE(bool(config_GetInt(option.constData())),!old);QVERIFY(!m_model->restartRequired());
    }
    void filterTogglesPreserveUnknownOptionsAndOrder_data() {
        QTest::addColumn<QString>("key");QTest::addColumn<QString>("module");
        QTest::newRow("norm")<<QStringLiteral("normalizeLoudness")<<QStringLiteral("normvol");
        QTest::newRow("eq")<<QStringLiteral("equalizerEnabled")<<QStringLiteral("equalizer");
        QTest::newRow("wide")<<QStringLiteral("stereoWidening")<<QStringLiteral("stereo_widen");
        QTest::newRow("compressor")<<QStringLiteral("compressor")<<QStringLiteral("compressor");
        QTest::newRow("chorus")<<QStringLiteral("chorusFlanger")<<QStringLiteral("chorus_flanger");
        QTest::newRow("karaoke")<<QStringLiteral("karaoke")<<QStringLiteral("karaoke");
    }
    void filterTogglesPreserveUnknownOptionsAndOrder() {
        QFETCH(QString,key);QFETCH(QString,module);QVERIFY(module_exists(qtu(module)));
        const QString opaque=QStringLiteral("unrelated{url='https://example/a:b',nested={value=2}}:other");
        config_PutPsz("audio-filter",qtu(opaque));m_model=std::make_unique<AudioPreferencesModel>(m_intf);
        QVERIFY2(m_model->setValue(key,true),qPrintable(m_model->lastError()));QCOMPARE(text("audio-filter"),opaque+':'+module);
        QVERIFY(m_model->setValue(key,false));QCOMPARE(text("audio-filter"),opaque);QVERIFY(!m_model->restartRequired());
        // Refuse malformed opaque options rather than destroying user text.
        config_PutPsz("audio-filter","unrelated{bad='unterminated:normvol");m_model->reload();const auto disk=bytes(m_config);
        QVERIFY(!m_model->capabilities().value(key).toBool());QVERIFY(!m_model->setValue(key,true));QCOMPARE(bytes(m_config),disk);
    }
    void exactNumericEnumsUnitsAndPairedSoxrPolicy() {
        enableStepConsumer();QVERIFY(m_model->setValue(QStringLiteral("volumeStep"),2.5));QCOMPARE(config_GetFloat("volume-step"),6.4f);
        QVERIFY(std::abs(m_model->values().value(QStringLiteral("volumeStep")).toDouble()-2.5)<.000001);
        QVERIFY(m_model->setValue(QStringLiteral("maximumVolume"),200.0));QCOMPARE(config_GetInt("qt-max-volume"),int64_t(200));
        QVERIFY(m_model->setValue(QStringLiteral("speakerSetup"),4.0));QCOMPARE(config_GetInt("mix-mode"),int64_t(4));
        QVERIFY(m_model->setValue(QStringLiteral("audioDelayMs"),-125.0));QCOMPARE(config_GetInt("audio-desync"),int64_t(-125));
        QVERIFY(m_model->setValue(QStringLiteral("detectDolby"),2));QCOMPARE(config_GetInt("force-dolby-surround"),int64_t(2));
        QVERIFY(module_exists("soxr"));QVERIFY(m_model->setValue(QStringLiteral("resamplingQuality"),3));
        QCOMPARE(text("audio-resampler"),QStringLiteral("soxr"));QCOMPARE(config_GetInt("soxr-resampler-quality"),int64_t(3));
        QCOMPARE(m_model->values().value(QStringLiteral("resamplingQuality")).toInt(),3);
    }
    void realReadOnlyWindowsDevicesAndCheckedStartupDefaults() {
#ifdef _WIN32
        QVERIFY(module_exists("mmdevice"));
        AudioPreferencesModel::Consumers linked;linked.windowsDevices=true;
        m_model=std::make_unique<AudioPreferencesModel>(m_intf,linked);
        QVERIFY(m_model->capabilities().value(QStringLiteral("outputDevice")).toBool());
        const auto devices=m_model->devices();QVERIFY(!devices.isEmpty());QSet<QString> ids;
        for(const auto &entry:devices){const auto row=entry.toMap();
            QCOMPARE(row.size(),2);const auto id=row.value(QStringLiteral("value")).toString();
            QVERIFY(!ids.contains(id));ids.insert(id);QVERIFY(!row.value(QStringLiteral("name")).toString().isEmpty());}
        QVERIFY(ids.contains(QString{}));
        QVERIFY(!m_model->setValue(QStringLiteral("outputDevice"),QStringLiteral("nonexistent-owned-fixture-device")));
        QVERIFY(!QFileInfo::exists(m_config));
        QVERIFY(m_model->setValue(QStringLiteral("rememberVolume"),false));
        QVERIFY(m_model->setValue(QStringLiteral("startupVolume"),75.));
        QCOMPARE(text("aout"),QStringLiteral("mmdevice"));QCOMPARE(config_GetFloat("mmdevice-volume"),.75f);
        QVERIFY(!m_model->setValue(QStringLiteral("startupVolume"),200.));QCOMPARE(config_GetFloat("mmdevice-volume"),.75f);
        QVERIFY(m_model->setValue(QStringLiteral("startupVolume"),125.));QCOMPARE(config_GetFloat("mmdevice-volume"),1.25f);
        // Device/default-volume acceptance must not enable an unverified receiver.
        QVERIFY(!m_model->capabilities().value(QStringLiteral("passthrough")).toBool());
        const auto beforeReceiver=text("aout");const auto beforePassthrough=config_GetInt("mmdevice-passthrough");
        const auto beforeReceiverDisk=bytes(m_config);
        QVERIFY(!m_model->setValue(QStringLiteral("passthrough"),!bool(beforePassthrough)));
        QCOMPARE(config_GetInt("mmdevice-passthrough"),beforePassthrough);
        QCOMPARE(text("aout"),beforeReceiver);QCOMPARE(bytes(m_config),beforeReceiverDisk);
        // Explicitly link the receiver only for the existing storage/mapping
        // tests below. These do not establish physical receiver playback.
        linked.windowsPassthrough=true;
        m_model=std::make_unique<AudioPreferencesModel>(m_intf,linked);
        QVERIFY(m_model->capabilities().value(QStringLiteral("passthrough")).toBool());
        const auto unrelatedSpdif=config_GetInt("spdif");config_PutInt("mmdevice-passthrough",1);m_model->reload();
        const auto disk=bytes(m_config);QVERIFY(m_model->setValue(QStringLiteral("passthrough"),true));
        QCOMPARE(config_GetInt("mmdevice-passthrough"),int64_t(1));QCOMPARE(bytes(m_config),disk);
        QVERIFY(m_model->setValue(QStringLiteral("passthrough"),false));QCOMPARE(config_GetInt("mmdevice-passthrough"),int64_t(0));
        QVERIFY(m_model->setValue(QStringLiteral("passthrough"),true));QCOMPARE(config_GetInt("mmdevice-passthrough"),int64_t(2));
        QCOMPARE(config_GetInt("spdif"),unrelatedSpdif);
        config_PutInt("mmdevice-passthrough",42);m_model->reload();QVERIFY(!m_model->values().value(QStringLiteral("passthrough")).isValid());
        QVERIFY(m_model->setValue(QStringLiteral("passthrough"),false));QCOMPARE(config_GetInt("mmdevice-passthrough"),int64_t(0));
        closeNative();QVERIFY(openNative());QCOMPARE(config_GetFloat("mmdevice-volume"),1.25f);
        QCOMPARE(text("aout"),QStringLiteral("mmdevice"));QVERIFY(!config_GetInt("volume-save"));
        // A manually selected unrelated output remains owned by its native UI.
        config_PutPsz("aout","unrelated-output");m_model=std::make_unique<AudioPreferencesModel>(m_intf,linked);
        for(const auto *key:{"outputDevice","startupVolume","passthrough"})
            QVERIFY(!m_model->capabilities().value(QString::fromLatin1(key)).toBool());
        const auto saved=bytes(m_config);QVERIFY(!m_model->setValue(QStringLiteral("startupVolume"),50.));
        QCOMPARE(text("aout"),QStringLiteral("unrelated-output"));QCOMPARE(bytes(m_config),saved);
#else
        QVariantList devices;QVERIFY(!enumerateAudioWindowsDevices(devices));QVERIFY(devices.isEmpty());
#endif
    }
    void presetBandsPreampAndEffectiveRestartRestoration() {
        QVERIFY(module_exists("equalizer"));QVERIFY(m_model->setValue(QStringLiteral("equalizerEnabled"),true));
        QVERIFY(!text("equalizer-bands").isEmpty());QCOMPARE(m_model->values().value(QStringLiteral("equalizerBands")).toList().size(),10);
        QVERIFY(m_model->setValue(QStringLiteral("equalizerPreset"),QStringLiteral("rock")));
        QCOMPARE(text("equalizer-preset"),QStringLiteral("rock"));QCOMPARE(config_GetFloat("equalizer-preamp"),5.f);
        const auto old=m_model->values().value(QStringLiteral("equalizerBands")).toList();auto changed=old;changed[0]=-7.5;
        QVERIFY(m_model->setValue(QStringLiteral("equalizerBands"),changed));QCOMPARE(text("equalizer-preset"),QString{});
        QCOMPARE(m_model->values().value(QStringLiteral("equalizerBands")).toList()[0].toFloat(),-7.5f);
        QVERIFY(m_model->setValue(QStringLiteral("equalizerPreamp"),-12.));QCOMPARE(config_GetFloat("equalizer-preamp"),-12.f);
        QVERIFY(m_model->setValue(QStringLiteral("equalizerPreset"),QStringLiteral("flat")));
        QVERIFY(m_model->setValue(QStringLiteral("equalizerEnabled"),false));QVERIFY(!m_model->restartRequired());
    }
    void parametersCanBePreparedWhileEffectsAreOffAndPitchOwnsItsFilter() {
        QVERIFY(m_model->setValue(QStringLiteral("normalizationTarget"),1.25));QCOMPARE(config_GetFloat("norm-max-level"),1.25f);
        QVERIFY(!text("audio-filter").contains(QStringLiteral("normvol")));
        QVERIFY(m_model->setValue(QStringLiteral("replayGainMode"),QStringLiteral("album")));QCOMPARE(text("audio-replay-gain-mode"),QStringLiteral("album"));
        QVERIFY(m_model->setValue(QStringLiteral("replayGainPreamp"),-3.5));QCOMPARE(config_GetFloat("audio-replay-gain-preamp"),-3.5f);
        QVERIFY(m_model->setValue(QStringLiteral("pitchShift"),3.));QCOMPARE(config_GetFloat("pitch-shift"),3.f);QVERIFY(text("audio-filter").contains(QStringLiteral("scaletempo_pitch")));
        QVERIFY(m_model->setValue(QStringLiteral("pitchShift"),0.));QVERIFY(!text("audio-filter").contains(QStringLiteral("scaletempo_pitch")));
    }
    void normalizationTargetCanonicalMinimum_data() {
        QTest::addColumn<bool>("modelReadback");
        QTest::newRow("native-float")<<false;
        QTest::newRow("exact-model-double-readback")<<true;
    }
    void normalizationTargetCanonicalMinimum() {
        QFETCH(bool,modelReadback);
        const QString key=QStringLiteral("normalizationTarget");const float minimum=0.000001f;
        QVERIFY(m_model->capabilities().value(key).toBool());
        // The native float is below the decimal double, but is the exact
        // canonical minimum accepted by the native serializer/readback.
        QVERIFY(double(minimum)<0.000001);
        QVariant requested=QVariant::fromValue(minimum);
        if(modelReadback) {
            config_PutFloat("norm-max-level",minimum);m_model->reload();
            requested=m_model->values().value(key);
            QCOMPARE(requested.typeId(),int(QMetaType::Double));
            QCOMPARE(requested.toDouble(),double(minimum));
        }
        // Make the desired write real in both rows, rather than accepting
        // a no-op because the current value already happens to match.
        QVERIFY2(m_model->setValue(key,1.25),qPrintable(m_model->lastError()));
        QCOMPARE(config_GetFloat("norm-max-level"),1.25f);const auto before=bytes(m_config);
        QVERIFY2(m_model->setValue(key,requested),qPrintable(m_model->lastError()));
        QVERIFY(m_model->lastError().isEmpty());QCOMPARE(config_GetFloat("norm-max-level"),minimum);
        QCOMPARE(m_model->values().value(key).toDouble(),double(minimum));
        QVERIFY(bytes(m_config)!=before);QVERIFY(bytes(m_config).contains("norm-max-level=0.000001"));
        closeNative();QVERIFY(openNative());QCOMPARE(config_GetFloat("norm-max-level"),minimum);
        const auto readback=m_model->values().value(key);QCOMPARE(readback.typeId(),int(QMetaType::Double));
        QCOMPARE(readback.toDouble(),double(minimum));const auto saved=bytes(m_config);
        QVERIFY2(m_model->setValue(key,readback),qPrintable(m_model->lastError()));
        QCOMPARE(bytes(m_config),saved);QVERIFY(!m_model->restartRequired());
    }
    void normalizationTargetRejectsBelowBoundaryAndInvalidTypes_data() {
        QTest::addColumn<QVariant>("requested");const float minimum=0.000001f;
        QTest::newRow("native-float-below")<<QVariant::fromValue(std::nextafter(minimum,0.f));
        QTest::newRow("double-immediately-below")<<QVariant(std::nextafter(double(minimum),0.));
        QTest::newRow("zero")<<QVariant(0.);
        QTest::newRow("nan")<<QVariant(std::numeric_limits<double>::quiet_NaN());
        QTest::newRow("infinity")<<QVariant(std::numeric_limits<double>::infinity());
        QTest::newRow("boolean")<<QVariant(true);
        QTest::newRow("numeric-string")<<QVariant(QStringLiteral("0.000001"));
        QTest::newRow("integer-zero")<<QVariant(0);
    }
    void normalizationTargetRejectsBelowBoundaryAndInvalidTypes() {
        QFETCH(QVariant,requested);const QString key=QStringLiteral("normalizationTarget");
        QVERIFY(m_model->capabilities().value(key).toBool());
        QVERIFY2(m_model->setValue(key,1.25),qPrintable(m_model->lastError()));
        const auto before=m_model->values();const auto disk=bytes(m_config);
        const auto native=config_GetFloat("norm-max-level");const bool restart=m_model->restartRequired();
        QSignalSpy changed(m_model.get(),&AudioPreferencesModel::valuesChanged);
        QVERIFY(!m_model->setValue(key,requested));QVERIFY(!m_model->lastError().isEmpty());
        QCOMPARE(m_model->values(),before);QCOMPARE(config_GetFloat("norm-max-level"),native);
        QCOMPARE(bytes(m_config),disk);QCOMPARE(m_model->restartRequired(),restart);QCOMPARE(changed.size(),0);
    }
    void invalidUnsupportedAndUnknownWritesNeverCommit() {
        const auto before=m_model->values();const auto disk=bytes(m_config);
        auto bands=QVariantList{};for(int i=0;i<10;++i)bands.push_back(0.);bands[3]=true;
        const QList<QPair<QString,QVariant>> invalid={{"audioEnabled",1},{"rememberVolume","false"},{"equalizerTwoPass",1.},
            {"maximumVolume",99.5},{"maximumVolume",301},{"speakerSetup",6},{"audioDelayMs",.5},{"detectDolby",true},
            {"normalizationTarget",0.},{"replayGainPreamp",QStringLiteral("1")},{"pitchShift",13.},{"equalizerPreamp",std::numeric_limits<double>::quiet_NaN()},
            {"equalizerBands",bands},{"equalizerPreset",QStringLiteral("fake")},{"preferredLanguage",QStringLiteral("not-a-language")},
            {"visualizer",QStringLiteral("fake")},{"mediaRole",QStringLiteral("fake")},{"nightMode",true},{"spatialAudio",true},
            {"spatialAudioMode",QStringLiteral("headphones")},{"visualizationSize",QStringLiteral("match-window")},{"startupVolume",200.},
            {"outputDevice",QStringLiteral("invented-device")},{"passthrough",true},{"volumeStep",1.},{"unknown",true}};
        QSignalSpy changed(m_model.get(),&AudioPreferencesModel::valuesChanged);
        for(const auto &entry:invalid){QVERIFY2(!m_model->setValue(entry.first,entry.second),qPrintable(entry.first));QVERIFY(!m_model->lastError().isEmpty());QCOMPARE(m_model->values(),before);QCOMPARE(bytes(m_config),disk);QVERIFY(!m_model->restartRequired());}
        QCOMPARE(changed.size(),0);
    }
    void nativeFailureRestoresPairedFilterAndFloatThenRecovers() {
        QVERIFY(m_model->setValue(QStringLiteral("pitchShift"),2.));const auto before=m_model->values();const auto disk=bytes(m_config);
        QVERIFY(QFile::remove(m_config));QVERIFY(QDir().mkdir(m_config));QVERIFY(!m_model->setValue(QStringLiteral("pitchShift"),0.));
        QVERIFY(!m_model->lastError().isEmpty());QCOMPARE(m_model->values(),before);QCOMPARE(config_GetFloat("pitch-shift"),2.f);QVERIFY(text("audio-filter").contains(QStringLiteral("scaletempo_pitch")));
        QVERIFY(QDir().rmdir(m_config));QVERIFY(write(m_config,disk));QVERIFY(m_model->setValue(QStringLiteral("pitchShift"),0.));QVERIFY(m_model->lastError().isEmpty());
    }
    void legacyCustomAndNonfiniteStateIsPreservedWithoutStartupWrites() {
        config_PutPsz("audio-language","legacy-language-value");config_PutPsz("audio-resampler","custom-resampler");
        config_PutFloat("audio-replay-gain-preamp",std::numeric_limits<float>::quiet_NaN());
        m_model=std::make_unique<AudioPreferencesModel>(m_intf);QVERIFY(!m_model->restartRequired());QVERIFY(!QFileInfo::exists(m_config));
        QCOMPARE(m_model->values().value(QStringLiteral("preferredLanguage")).toString(),QStringLiteral("legacy-language-value"));
        QVERIFY(!m_model->values().value(QStringLiteral("replayGainPreamp")).isValid());QVERIFY(!m_model->capabilities().value(QStringLiteral("resamplingQuality")).toBool());
        QVERIFY(m_model->setValue(QStringLiteral("replayGainPreamp"),0.));QCOMPARE(config_GetFloat("audio-replay-gain-preamp"),0.f);
        QCOMPARE(text("audio-language"),QStringLiteral("legacy-language-value"));QCOMPARE(text("audio-resampler"),QStringLiteral("custom-resampler"));
    }
    void actualLanguageVisualizationRoleAndFrequencyNotifications() {
        QVERIFY(m_model->setValue(QStringLiteral("preferredLanguage"),QStringLiteral("en,fi,any")));QCOMPARE(text("audio-language"),QStringLiteral("en,fi,any"));
        // Real MMDevice requests eConsole and does not read the global role.
        // A stored flag alone must not claim a working Windows policy control.
        const auto role=text("role");const auto disk=bytes(m_config);
        QVERIFY(!m_model->capabilities().value(QStringLiteral("mediaRole")).toBool());
        QVERIFY(!m_model->setValue(QStringLiteral("mediaRole"),QStringLiteral("music")));
        QCOMPARE(text("role"),role);QCOMPARE(bytes(m_config),disk);
        QVERIFY(module_exists("visual"));QVERIFY(m_model->setValue(QStringLiteral("visualizer"),QStringLiteral("spectrum")));
        QCOMPARE(text("audio-visual"),QStringLiteral("visual"));QCOMPARE(text("effect-list"),QStringLiteral("spectrum"));
        QSignalSpy frequencies(m_model.get(),&AudioPreferencesModel::equalizerFrequenciesChanged);
        const bool old=config_GetInt("equalizer-vlcfreqs");config_PutInt("equalizer-vlcfreqs",!old);m_model->reload();
        QCOMPARE(frequencies.size(),1);QCOMPARE(m_model->equalizerFrequencies()[0].toDouble(),old?31.25:60.);
    }
    void genuineOutputVisualizationCallbackSurvivesFilterRestart() {
        const auto fixture=qEnvironmentVariable("VLC_AUDIO_DEFAULTS_FIXTURE");
        QVERIFY(!fixture.isEmpty()&&QFileInfo(fixture).isFile());
        QVERIFY(module_exists("visual"));QVERIFY(startOwnedPcm(fixture));
        QTRY_VERIFY_WITH_TIMEOUT(m_buffers.load()>5,10000);
        std::unique_ptr<audio_output_t,decltype(&aout_Release)> aout(
            vlc_player_aout_Hold(m_intf->p_player),aout_Release);
        QVERIFY(aout);
        QCOMPARE(var_Type(aout.get(),"visual")&VLC_VAR_CLASS,VLC_VAR_STRING);
        auto value=[&aout](const char *name) {
            char *raw=var_GetString(aout.get(),name);
            const QString result=QString::fromUtf8(raw?raw:"");free(raw);return result;
        };
        // Only the production filter callback maps a named effect to the
        // internal visualization module and requests the live filter restart.
        QCOMPARE(var_SetString(aout.get(),"visual","spectrum"),VLC_SUCCESS);
        QCOMPARE(value("audio-visual"),QStringLiteral("visual"));
        QCOMPARE(value("effect-list"),QStringLiteral("spectrum"));
        auto count=m_buffers.load();QTRY_VERIFY_WITH_TIMEOUT(m_buffers.load()>count+4,10000);
        QCOMPARE(var_SetString(aout.get(),"visual",""),VLC_SUCCESS);
        QCOMPARE(value("audio-visual"),QStringLiteral("none"));
        count=m_buffers.load();QTRY_VERIFY_WITH_TIMEOUT(m_buffers.load()>count+4,10000);
        // The restarted pipeline must install its own callback again.
        QCOMPARE(var_SetString(aout.get(),"visual","scope"),VLC_SUCCESS);
        QCOMPARE(value("audio-visual"),QStringLiteral("visual"));
        QCOMPARE(value("effect-list"),QStringLiteral("scope"));
        QCOMPARE(var_SetString(aout.get(),"visual",""),VLC_SUCCESS);
        QCOMPARE(value("audio-visual"),QStringLiteral("none"));
    }
    void genuineSavedDefaultsDriveNewOutputAndLiveEqualizerPcm() {
        const auto fixture=qEnvironmentVariable("VLC_AUDIO_DEFAULTS_FIXTURE");
        QVERIFY2(!fixture.isEmpty()&&QFileInfo(fixture).isFile(),"Set the generated owned WAV path; this real PCM case is not a skipped acceptance claim");
        QVERIFY(module_exists("equalizer"));QVERIFY(startOwnedPcm(fixture));QTRY_VERIFY_WITH_TIMEOUT(m_buffers.load()>5,10000);QTest::qWait(300);
        const float baseline=m_rms.load();QVERIFY(baseline>0.001f);
        stopOwnedPcm();QTRY_COMPARE_WITH_TIMEOUT(ownedPcmState(),VLC_PLAYER_STATE_STOPPED,10000);
        releaseOwnedPcm();
        QVERIFY(m_model->setValue(QStringLiteral("equalizerEnabled"),true));
        QVERIFY(m_model->setValue(QStringLiteral("equalizerPreamp"),-12.));
        closeNative();QVERIFY(openNative());QVERIFY(startOwnedPcm(fixture));QTRY_VERIFY_WITH_TIMEOUT(m_buffers.load()>5,10000);QTest::qWait(300);
        m_liveEqualizer=std::make_unique<AudioEqualizer>(m_intf->p_player);m_liveEqualizer->refresh();QVERIFY(m_liveEqualizer->available());
        QVERIFY(m_liveEqualizer->enabled());QCOMPARE(m_liveEqualizer->preamp(),-12.f);const float quiet=m_rms.load();
        QVERIFY2(quiet>baseline*.02f&&quiet<baseline*.2f,"Saved real EQ defaults did not attenuate actual new-output PCM");
        m_liveEqualizer->setEnabled(false);const auto count=m_buffers.load();QTRY_VERIFY_WITH_TIMEOUT(m_buffers.load()>count+4,10000);QTest::qWait(300);
        QVERIFY2(m_rms.load()>quiet*3.f,"Genuine existing live controller did not restore actual output after disabling EQ");
        // Live playback edits stay separate from the stored Audio defaults.
        QCOMPARE(config_GetFloat("equalizer-preamp"),-12.f);QVERIFY(text("audio-filter").contains(QStringLiteral("equalizer")));
    }
};
QTEST_GUILESS_MAIN(TestAudioPreferences)
#include "test_audio_preferences.moc"
