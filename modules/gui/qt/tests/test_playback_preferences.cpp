// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "test_playback_preferences.hpp"
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include "util/playback_seek_policy.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <vlc_player.h>
#include <vlc_playlist.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <climits>
#include <limits>
#ifdef _WIN32
# include <windows.h>
#endif
extern "C" vlc_object_t *vlc_test_playback_object(libvlc_instance_t *);
extern "C" bool vlc_test_playback_start_paused(vlc_player_t *);
extern "C" int vlc_test_playback_stopped_action(vlc_playlist_t *);
namespace {
using vlc_player_locker = vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock>;
using vlc_playlist_locker = vlc_locker<vlc_playlist_t, vlc_playlist_Lock, vlc_playlist_Unlock>;
QByteArray bytes(const QString &path) { QFile f(path); return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray{}; }
QString nativeText(const char *key) { char *raw = config_GetPsz(key); const auto result = QString::fromUtf8(raw ? raw : ""); free(raw); return result; }
}
bool TestPlaybackPreferences::openNative()
{
    const QByteArray option = QByteArray("--config=") + m_config.toUtf8();
    const char *args[] = {"--no-ignore-config", "--no-media-library", "--extraintf=", "--control=", "--quiet", option.constData()};
    m_instance = libvlc_new(ARRAY_SIZE(args), args);
    if (!m_instance) return false;
    const auto object = vlc_test_playback_object(m_instance);
    m_intf = vlc_object_create<qt_intf_t>(object);
    if (!m_intf) return false;
    // A genuine isolated native playlist, not the GUI's main-playlist adapter.
    // Its creation itself inherits the saved native player-rate variable.
    m_intf->p_playlist = vlc_playlist_New(object, VLC_PLAYLIST_PREPARSING_DISABLED, 1, 0);
    if (!m_intf->p_playlist) return false;
    { vlc_playlist_locker lock{m_intf->p_playlist}; m_intf->p_player = vlc_playlist_GetPlayer(m_intf->p_playlist); }
    m_model = std::make_unique<PlaybackPreferencesModel>(m_intf);
    return true;
}
void TestPlaybackPreferences::closeNative()
{
    m_model.reset();
    if (m_intf) {
        if (m_intf->p_playlist) vlc_playlist_Delete(m_intf->p_playlist);
        m_intf->p_playlist = nullptr; m_intf->p_player = nullptr;
        vlc_object_delete(m_intf); m_intf = nullptr;
    }
    if (m_instance) { libvlc_release(m_instance); m_instance = nullptr; }
}
void TestPlaybackPreferences::initTestCase()
{
    QVERIFY(m_temp.isValid());
    m_root = qEnvironmentVariable("VLC_PLAYBACK_OUTPUT");
    if (m_root.isEmpty()) m_root = m_temp.path();
    else {
        QVERIFY2(QFileInfo(m_root).isAbsolute() && !QFileInfo::exists(m_root), "Use a new owned absolute output directory");
        QVERIFY(QDir().mkpath(m_root));
    }
    const auto userdata = QDir(m_root).filePath(QStringLiteral("userdata"));
    QVERIFY(QDir().mkpath(userdata)); qputenv("VLC_USERDATA_PATH", userdata.toUtf8());
}
void TestPlaybackPreferences::init()
{
    m_case = QDir(m_root).filePath(QString::fromLatin1(QTest::currentTestFunction()));
    if (const auto tag = QTest::currentDataTag(); tag && *tag) m_case += '/' + QString::fromLatin1(tag);
    QVERIFY(QDir().mkpath(m_case)); m_config = QDir(m_case).filePath(QStringLiteral("vlcrc"));
    QVERIFY(openNative());
}
void TestPlaybackPreferences::cleanup() { closeNative(); }
void TestPlaybackPreferences::exactApiReadOnlyAndCapabilities()
{
    QCOMPARE(m_model->values().size(), 19);
    QCOMPARE(m_model->values().keys(), m_model->capabilities().keys());
    QCOMPARE(m_model->applyModes().size(), 19);
    QVERIFY(!m_model->capabilities().value(QStringLiteral("showThumbnails")).toBool());
    QVERIFY(!m_model->capabilities().value(QStringLiteral("preciseSeeking")).toBool());
    const auto initial = m_model->values(); const auto disk = bytes(m_config);
    QSignalSpy changes(m_model.get(), &PlaybackPreferencesModel::valuesChanged);
    m_model->reload(); QCOMPARE(m_model->values(), initial); QCOMPARE(bytes(m_config), disk);
    QCOMPARE(changes.size(), 0); QVERIFY(!m_model->restartRequired()); QVERIFY(m_model->lastError().isEmpty());
}
void TestPlaybackPreferences::strictInvalidWritesPreserveState()
{
    const QList<QPair<QString,QVariant>> invalid = {
        {QStringLiteral("shuffle"), 1}, {QStringLiteral("startPlayback"), QStringLiteral("true")},
        {QStringLiteral("pauseOtherAudio"), 0.}, {QStringLiteral("repeatMode"), true},
        {QStringLiteral("repeatMode"), 3}, {QStringLiteral("finishAction"), -1},
        {QStringLiteral("defaultPlaybackSpeed"), true}, {QStringLiteral("defaultPlaybackSpeed"), QStringLiteral("1.5")},
        {QStringLiteral("defaultPlaybackSpeed"), .3}, {QStringLiteral("defaultPlaybackSpeed"), 0.},
        {QStringLiteral("defaultPlaybackSpeed"), std::numeric_limits<double>::infinity()},
        {QStringLiteral("defaultPlaybackSpeed"), std::numeric_limits<double>::quiet_NaN()},
        {QStringLiteral("defaultAudioTrack"), -2}, {QStringLiteral("defaultSubtitleTrack"), .5},
        {QStringLiteral("defaultSubtitleTrack"), qulonglong(std::numeric_limits<qulonglong>::max())},
        {QStringLiteral("preferredVideoQuality"), 2160}, {QStringLiteral("continuePlayback"), 3},
        {QStringLiteral("preferredMenuLanguage"), QStringLiteral("zz")},
        {QStringLiteral("preferredMenuLanguage"), QStringLiteral("English")},
        {QStringLiteral("timeShiftFolder"), QStringLiteral("https://example.invalid/")},
        {QStringLiteral("timeShiftSize"), 1}, {QStringLiteral("timeShiftSize"), qlonglong(INT_MAX)+1},
        {QStringLiteral("defaultOpticalDrive"), QStringLiteral("relative")},
        {QStringLiteral("showThumbnails"), false}, {QStringLiteral("preciseSeeking"), true},
        {QStringLiteral("unknown"), true}};
    const auto before = m_model->values(); const auto disk = bytes(m_config);
    QSignalSpy changes(m_model.get(), &PlaybackPreferencesModel::valuesChanged);
    for (const auto &entry : invalid) {
        QVERIFY2(!m_model->setValue(entry.first,entry.second), qPrintable(entry.first));
        QVERIFY(!m_model->lastError().isEmpty()); QCOMPARE(m_model->values(),before); QCOMPARE(bytes(m_config),disk);
    }
    QCOMPARE(changes.size(),0); QVERIFY(!m_model->restartRequired());
    QVERIFY(m_model->setValue(QStringLiteral("shuffle"), before.value(QStringLiteral("shuffle"))));
    QVERIFY(m_model->lastError().isEmpty());
}
void TestPlaybackPreferences::nativeEnumChoices_data()
{
    QTest::addColumn<QString>("key"); QTest::addColumn<QByteArray>("native");
    QTest::newRow("continue") << QStringLiteral("continuePlayback") << QByteArray("restore-playback-pos");
    QTest::newRow("quality") << QStringLiteral("preferredVideoQuality") << QByteArray("preferred-resolution");
    QTest::newRow("avi") << QStringLiteral("aviRepair") << QByteArray("avi-index");
}
void TestPlaybackPreferences::nativeEnumChoices()
{
    QFETCH(QString,key); QFETCH(QByteArray,native);
    QCOMPARE(config_GetType(native.constData()),VLC_VAR_INTEGER);
    QVERIFY(m_model->setValue(QStringLiteral("preferredMenuLanguage"),QStringLiteral("fi")));
    const auto choices = m_model->choices().value(key).toList(); QVERIFY(!choices.isEmpty());
    for (const auto &choice : choices) {
        const auto number = choice.toMap().value(QStringLiteral("value")).toLongLong();
        QVERIFY2(m_model->setValue(key,number),qPrintable(m_model->lastError()));
        QCOMPARE(config_GetInt(native.constData()),number);
        const QByteArray expected = native + '=' + QByteArray::number(number); QVERIFY(bytes(m_config).contains(expected));
    }
    QVERIFY(!m_model->restartRequired());
}
void TestPlaybackPreferences::liveOrderUsesCorrectIndependentNativeBits()
{
    for (int mode : {0,1,2,0}) {
        QVERIFY(m_model->setValue(QStringLiteral("repeatMode"),mode));
        QCOMPARE(config_GetInt("repeat"),qlonglong(mode==1)); QCOMPARE(config_GetInt("loop"),qlonglong(mode==2));
        { vlc_playlist_locker lock{m_intf->p_playlist}; QCOMPARE(int(vlc_playlist_GetPlaybackRepeat(m_intf->p_playlist)),mode); }
    }
    for (bool shuffle : {true,false}) {
        QVERIFY(m_model->setValue(QStringLiteral("shuffle"),shuffle));
        vlc_playlist_locker lock{m_intf->p_playlist};
        QCOMPARE(vlc_playlist_GetPlaybackOrder(m_intf->p_playlist),shuffle ? VLC_PLAYLIST_PLAYBACK_ORDER_RANDOM : VLC_PLAYLIST_PLAYBACK_ORDER_NORMAL);
    }
    QVERIFY(!m_model->restartRequired());
}
void TestPlaybackPreferences::liveStartPausedAndFinishPrecedence()
{
    QVERIFY(m_model->setValue(QStringLiteral("startPlayback"),false));
    QVERIFY(vlc_test_playback_start_paused(m_intf->p_player)); QCOMPARE(config_GetInt("start-paused"),qlonglong(1));
    QVERIFY(m_model->setValue(QStringLiteral("startPlayback"),true)); QVERIFY(!vlc_test_playback_start_paused(m_intf->p_player));
    QVERIFY(m_model->setValue(QStringLiteral("stopAfterEachItem"),true));
    QCOMPARE(vlc_test_playback_stopped_action(m_intf->p_playlist),int(VLC_PLAYLIST_MEDIA_STOPPED_STOP));
    QVERIFY(m_model->setValue(QStringLiteral("finishAction"),1));
    QCOMPARE(vlc_test_playback_stopped_action(m_intf->p_playlist),int(VLC_PLAYLIST_MEDIA_STOPPED_EXIT));
    QVERIFY(!m_model->capabilities().value(QStringLiteral("stopAfterEachItem")).toBool());
    QVERIFY(!m_model->setValue(QStringLiteral("stopAfterEachItem"),false));
    QVERIFY(m_model->setValue(QStringLiteral("finishAction"),0));
    QCOMPARE(vlc_test_playback_stopped_action(m_intf->p_playlist),int(VLC_PLAYLIST_MEDIA_STOPPED_STOP));
    QVERIFY(m_model->setValue(QStringLiteral("stopAfterEachItem"),false));
    QCOMPARE(vlc_test_playback_stopped_action(m_intf->p_playlist),int(VLC_PLAYLIST_MEDIA_STOPPED_CONTINUE));
}
void TestPlaybackPreferences::quarterSpeedPersistenceAndRestart_data()
{
    QTest::addColumn<double>("speed");
    QTest::newRow("quarter") << .25; QTest::newRow("three-quarters") << .75;
    QTest::newRow("one-and-half") << 1.5; QTest::newRow("double") << 2.;
}
void TestPlaybackPreferences::quarterSpeedPersistenceAndRestart()
{
    QFETCH(double,speed);
    const auto before = config_GetFloat("rate");
    QVERIFY(m_model->setValue(QStringLiteral("defaultPlaybackSpeed"),speed));
    QCOMPARE(config_GetFloat("rate"),float(speed)); QVERIFY(m_model->restartRequired());
    { vlc_player_locker lock{m_intf->p_player}; QCOMPARE(vlc_player_GetRate(m_intf->p_player),before); }
    QVERIFY(m_model->setValue(QStringLiteral("defaultPlaybackSpeed"),before)); QVERIFY(!m_model->restartRequired());
    QVERIFY(m_model->setValue(QStringLiteral("defaultPlaybackSpeed"),speed));
    closeNative(); QVERIFY(openNative());
    QCOMPARE(config_GetFloat("rate"),float(speed)); QVERIFY(!m_model->restartRequired());
    { vlc_player_locker lock{m_intf->p_player}; QCOMPARE(vlc_player_GetRate(m_intf->p_player),float(speed)); }
}
void TestPlaybackPreferences::corkStartupAndExistingUnknownDefaultsPreserved()
{
    const bool before = config_GetInt("playlist-cork") != 0;
    QVERIFY(m_model->setValue(QStringLiteral("pauseOtherAudio"),!before)); QVERIFY(m_model->restartRequired());
    QCOMPARE(config_GetInt("playlist-cork"),qlonglong(!before));
    QVERIFY(m_model->setValue(QStringLiteral("pauseOtherAudio"),before)); QVERIFY(!m_model->restartRequired());
    config_PutFloat("rate",3.f); config_PutInt("preferred-resolution",4320);
    config_PutInt("audio-track",-7); config_PutInt("input-timeshift-granularity",100);
    const auto disk=bytes(m_config); m_model->reload();
    QCOMPARE(m_model->values().value(QStringLiteral("defaultPlaybackSpeed")).toFloat(),3.f);
    QCOMPARE(m_model->values().value(QStringLiteral("preferredVideoQuality")).toInt(),4320);
    QCOMPARE(m_model->values().value(QStringLiteral("defaultAudioTrack")).toInt(),-7);
    QCOMPARE(m_model->values().value(QStringLiteral("timeShiftSize")).toInt(),100);
    QCOMPARE(bytes(m_config),disk); // No load-time clamping/coercion of legacy states.
}
void TestPlaybackPreferences::trackIndexesClearOnlyTheirExplicitIdOverride()
{
    config_PutPsz("audio-track-id","audio/es/owned"); config_PutPsz("sub-track-id","subtitle/es/owned");
    config_PutPsz("audio-language","fi,en"); config_PutPsz("sub-language","sv,en"); m_model->reload();
    QVERIFY(m_model->setValue(QStringLiteral("defaultAudioTrack"),0));
    QCOMPARE(config_GetInt("audio-track"),qlonglong(0)); QVERIFY(nativeText("audio-track-id").isEmpty());
    QCOMPARE(nativeText("sub-track-id"),QStringLiteral("subtitle/es/owned"));
    QVERIFY(m_model->setValue(QStringLiteral("defaultSubtitleTrack"),1));
    QCOMPARE(config_GetInt("sub-track"),qlonglong(1)); QVERIFY(nativeText("sub-track-id").isEmpty());
    QCOMPARE(nativeText("audio-language"),QStringLiteral("fi,en")); QCOMPARE(nativeText("sub-language"),QStringLiteral("sv,en"));
    QVERIFY(m_model->setValue(QStringLiteral("defaultAudioTrack"),-1));
    closeNative(); QVERIFY(openNative()); QCOMPARE(config_GetInt("audio-track"),qlonglong(-1)); QCOMPARE(config_GetInt("sub-track"),qlonglong(1));
}
void TestPlaybackPreferences::nativeMenuCodesAndLegacyListPreservation()
{
    config_PutPsz("menu-language","fi,en,any"); m_model->reload(); const auto disk=bytes(m_config);
    QCOMPARE(m_model->values().value(QStringLiteral("preferredMenuLanguage")).toString(),QStringLiteral("fi,en,any"));
    m_model->reload(); QCOMPARE(bytes(m_config),disk);
    const auto choices=m_model->choices().value(QStringLiteral("preferredMenuLanguage")).toList(); QVERIFY(choices.size()>100);
    QVERIFY(m_model->setValue(QStringLiteral("preferredMenuLanguage"),QStringLiteral("fi")));
    QCOMPARE(nativeText("menu-language"),QStringLiteral("fi"));
    closeNative(); QVERIFY(openNative()); QCOMPARE(nativeText("menu-language"),QStringLiteral("fi"));
}
void TestPlaybackPreferences::nativeInputDefaultsAndMixedOpticalPaths()
{
    config_PutPsz("dvd","owned-dvd"); config_PutPsz("vcd","owned-vcd"); m_model->reload();
    QVERIFY(!m_model->values().value(QStringLiteral("defaultOpticalDrive")).isValid());
    const auto disk=bytes(m_config);m_model->reload();QCOMPARE(bytes(m_config),disk);
    QVERIFY(m_model->setValue(QStringLiteral("defaultOpticalDrive"),QString{}));
    QVERIFY(nativeText("dvd").isEmpty()); QVERIFY(nativeText("vcd").isEmpty());
    if(config_GetType("cd-audio")==VLC_VAR_STRING)QVERIFY(nativeText("cd-audio").isEmpty());
    QCOMPARE(config_GetType("mkv-preload-local-dir"),VLC_VAR_BOOL);
    const bool before=config_GetInt("mkv-preload-local-dir")!=0;
    QVERIFY(m_model->setValue(QStringLiteral("mkvPreload"),!before)); QCOMPARE(config_GetInt("mkv-preload-local-dir"),qlonglong(!before));
}
void TestPlaybackPreferences::timeShiftNativeBytesAndDirectory()
{
    const auto folder=QDir(m_case).filePath(QString::fromUtf8("timeshift-\xC3\xA4")); QVERIFY(QDir().mkpath(folder));
    QVERIFY(m_model->setValue(QStringLiteral("timeShiftFolder"),folder)); QCOMPARE(nativeText("input-timeshift-path"),folder);
    for(qlonglong size:{qlonglong(-1),qlonglong(1024*1024),qlonglong(50*1024*1024),qlonglong(INT_MAX)}) {
        QVERIFY(m_model->setValue(QStringLiteral("timeShiftSize"),size)); QCOMPARE(config_GetInt("input-timeshift-granularity"),size);
    }
    closeNative(); QVERIFY(openNative()); QCOMPARE(nativeText("input-timeshift-path"),folder);
    QCOMPARE(config_GetInt("input-timeshift-granularity"),qlonglong(INT_MAX));
    QVERIFY(!m_model->setValue(QStringLiteral("timeShiftFolder"),QString(folder+QStringLiteral("/does-not-exist"))));
    QVERIFY(m_model->setValue(QStringLiteral("timeShiftFolder"),QString{})); QVERIFY(nativeText("input-timeshift-path").isEmpty());
}
void TestPlaybackPreferences::preciseSeekingRequiresActualConsumerGate()
{
    QVERIFY(!m_model->setValue(QStringLiteral("preciseSeeking"),false));
    PlaybackPreferencesModel::Consumers consumers;consumers.preciseSeeking=true;
    m_model=std::make_unique<PlaybackPreferencesModel>(m_intf,consumers);
    QVERIFY(m_model->setValue(QStringLiteral("preciseSeeking"),false));
    QCOMPARE(config_GetInt("input-fast-seek"),qlonglong(1)); QCOMPARE(qtPlaybackSeekSpeed(VLC_OBJECT(m_intf)),VLC_PLAYER_SEEK_FAST);
    QVERIFY(m_model->setValue(QStringLiteral("preciseSeeking"),true));
    QCOMPARE(config_GetInt("input-fast-seek"),qlonglong(0)); QCOMPARE(qtPlaybackSeekSpeed(VLC_OBJECT(m_intf)),VLC_PLAYER_SEEK_PRECISE);
    // This executes the real native flag/helper; actual patched PlayerController
    // callsites must also be compiled and exercised by Root before enabling it.
}
void TestPlaybackPreferences::saveFailurePreservesDiskConfigAndLiveState_data()
{
    QTest::addColumn<QString>("key");QTest::addColumn<QVariant>("requested");
    QTest::newRow("int-tracks")<<QStringLiteral("defaultAudioTrack")<<QVariant(3);
    QTest::newRow("bool-live")<<QStringLiteral("shuffle")<<QVariant(true);
    QTest::newRow("float-restart")<<QStringLiteral("defaultPlaybackSpeed")<<QVariant(1.5);
}
void TestPlaybackPreferences::saveFailurePreservesDiskConfigAndLiveState()
{
#ifndef _WIN32
    QSKIP("Real Win32 read/write/no-delete lock is the native save-failure stimulus");
#else
    QFETCH(QString,key);QFETCH(QVariant,requested);
    QVERIFY(m_model->setValue(QStringLiteral("preferredMenuLanguage"),QStringLiteral("fi")));
    const auto disk=bytes(m_config);
    const auto before=m_model->values();
    QSignalSpy changed(m_model.get(),&PlaybackPreferencesModel::valuesChanged);
    const auto wide=QDir::toNativeSeparators(m_config).toStdWString();
    HANDLE lock=CreateFileW(wide.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    QVERIFY(lock!=INVALID_HANDLE_VALUE);
    HANDLE writable=CreateFileW(wide.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    const bool preflightWritable=writable!=INVALID_HANDLE_VALUE;
    if(writable!=INVALID_HANDLE_VALUE)CloseHandle(writable);
    const bool accepted=m_model->setValue(key,requested); const auto error=m_model->lastError();
    const auto failedValues=m_model->values();
    const auto failedDisk=bytes(m_config);
    bool liveRandom;{vlc_playlist_locker playlistLock{m_intf->p_playlist};liveRandom=vlc_playlist_GetPlaybackOrder(m_intf->p_playlist)==VLC_PLAYLIST_PLAYBACK_ORDER_RANDOM;}
    CloseHandle(lock);
    // Always attempt the desired recovery before asserting the failed phase.
    m_model->reload(); const bool retry=m_model->setValue(key,requested);
    QVERIFY(preflightWritable);QVERIFY(!accepted);QVERIFY(!error.isEmpty());QCOMPARE(failedValues,before);QCOMPARE(failedDisk,disk);QVERIFY(!liveRandom);
    QCOMPARE(changed.size(),1);QVERIFY2(retry,qPrintable(m_model->lastError()));
    QVERIFY(bytes(m_config)!=disk);QCOMPARE(m_model->values().value(key),requested);
#endif
}
void TestPlaybackPreferences::missingServiceAndActualPickerBoundary()
{
    QVERIFY(!m_model->chooseTimeShiftFolder());QVERIFY(!m_model->pickerActive());
    PlaybackPreferencesModel missing(nullptr);
    for(const auto &value:missing.capabilities())QVERIFY(!value.toBool());
    QVERIFY(!missing.setValue(QStringLiteral("shuffle"),true));QVERIFY(!missing.lastError().isEmpty());
}
QTEST_GUILESS_MAIN(TestPlaybackPreferences)
#include "test_playback_preferences.moc"
