// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "maininterface/recording_preferences_model.hpp"
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include "util/cast_discovery_policy.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <limits>
#ifdef _WIN32
# include <windows.h>
#endif
extern "C" vlc_object_t *vlc_test_library_object(libvlc_instance_t *);

class TestRecordingPreferences final : public QObject
{
    Q_OBJECT
    QTemporaryDir temp;
    libvlc_instance_t *instance = nullptr;
    qt_intf_t *intf = nullptr;
    std::unique_ptr<QSettings> settings;
    std::unique_ptr<RecordingPreferencesModel> model;
    QString config, directory, interfaceFile;
    static QByteArray bytes(const QString &name)
    { QFile file(name); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{}; }
    static QString native(const char *key)
    { char *raw = config_GetPsz(key); const auto value = QString::fromUtf8(raw ? raw : ""); free(raw); return value; }
    void open(bool available = true, QList<QByteArray> extra = {})
    {
        QList<QByteArray> args = {QByteArray("--config=") + config.toUtf8(), "--no-ignore-config", "--quiet",
            "--no-media-library", "--no-metadata-network-access", "--no-auto-preparse", "--extraintf=", "--control="};
        args.append(extra);
        QList<const char *> argv; for (const auto &arg : args) argv.append(arg.constData());
        instance = libvlc_new(argv.size(), argv.data()); QVERIFY(instance);
        intf = vlc_object_create<qt_intf_t>(vlc_test_library_object(instance)); QVERIFY(intf);
        settings = std::make_unique<QSettings>(interfaceFile, QSettings::IniFormat);
        intf->mainSettings = settings.get();
        model = std::make_unique<RecordingPreferencesModel>(intf, settings.get(),
            RecordingPreferencesModel::Consumers{available,available,available,available,available});
    }
    void close()
    {
        model.reset(); if (intf) { vlc_object_delete(intf); intf = nullptr; }
        settings.reset(); if (instance) { libvlc_release(instance); instance = nullptr; }
    }
private slots:
    void init()
    {
        QVERIFY(temp.isValid());
        directory = QDir(temp.path()).filePath(QString::fromLatin1(QTest::currentTestFunction()) + QString::fromUtf8("-Ää"));
        QVERIFY(QDir().mkpath(directory)); config = QDir(directory).filePath("vlcrc");
        interfaceFile = QDir(directory).filePath("interface.ini");
        const auto userdata = QDir(directory).filePath("userdata"); QVERIFY(QDir().mkpath(userdata));
        qputenv("VLC_USERDATA_PATH", userdata.toUtf8());
#ifdef _WIN32
        SetEnvironmentVariableW(L"VLC_USERDATA_PATH", reinterpret_cast<LPCWSTR>(userdata.utf16()));
#endif
        open();
    }
    void cleanup() { close(); }
    void openingAndReloadNeverWriteDefaults()
    {
        QCOMPARE(model->values().size(),11);
        QVERIFY(model->values().value("discoverDevices").toBool());
        QVERIFY(model->capabilities().value("encodingPreset").toBool());
        QVERIFY(model->capabilities().value("castQuality").toBool());
        QCOMPARE(model->choices().value("castQuality").toList().first().toMap().value("name").toString(),QStringLiteral("Automatic"));
        QCOMPARE(model->values().value("castQuality"),model->choices().value("castQuality").toList().first().toMap().value("value"));
        QVERIFY(!QFile::exists(config)); QVERIFY(!QFile::exists(interfaceFile));
        model->reload(); QVERIFY(!QFile::exists(config)); QVERIFY(!QFile::exists(interfaceFile));
    }
    void disabledConsumersRejectWithoutSaving()
    {
        close(); open(false);
        for (const auto &value : model->capabilities()) QVERIFY(!value.toBool());
        QVERIFY(!model->setValue("includeAudio",false)); QVERIFY(!model->setValue("discoverDevices",false));
        QVERIFY(!model->chooseRecordingDirectory()); QVERIFY(!model->openAdvanced());
        QVERIFY(!QFile::exists(config)); QVERIFY(!QFile::exists(interfaceFile));
    }
    void strictTypesAndChoicesPreserveMemoryAndDisk()
    {
        QVERIFY(model->setValue("includeAudio",false)); const auto before = model->values(); const auto disk = bytes(config);
        const QList<QPair<QString,QVariant>> edits = {{"includeAudio",1},{"nativeRecording",QStringLiteral("true")},
            {"discoverDevices",0},{"castQuality",true},{"castQuality",QStringLiteral("1")},{"castQuality",1.5},
            {"castQuality",-1},{"castQuality",4},{"castQuality",std::numeric_limits<double>::quiet_NaN()},
            {"encodingPreset",QStringLiteral("owned-invalid")},{"tuning",QStringLiteral("owned-invalid")},
            {"recordingDirectory",true},{"recordingDirectory",QStringLiteral("relative/path")},
            {"recordingDirectory",QStringLiteral("https://owned.invalid/folder")},{"missing",false}};
        for (const auto &edit : edits) {
            QVERIFY(!model->setValue(edit.first,edit.second)); QVERIFY(!model->lastError().isEmpty());
            QCOMPARE(model->values(),before); QCOMPARE(bytes(config),disk);
        }
        QVERIFY(!QFile::exists(interfaceFile));
    }
    void unicodeRecordingFolderSurvivesRestartAndEmptyRestoresDefault()
    {
        const auto folder = QDir(directory).filePath(QString::fromUtf8("Videos Ää with spaces")); QVERIFY(QDir().mkpath(folder));
        QVERIFY(model->setValue("recordingDirectory",folder)); QCOMPARE(native("input-record-path"),folder);
        close(); open(); QCOMPARE(model->values().value("recordingDirectory").toString(),folder);
        char *inherited=var_InheritString(intf,"input-record-path"); QCOMPARE(QString::fromUtf8(inherited),folder); free(inherited);
        const auto file = QDir(directory).filePath("ordinary-file"); QFile sample(file); QVERIFY(sample.open(QIODevice::WriteOnly)); sample.close();
        QVERIFY(!model->setValue("recordingDirectory",file));
        QVERIFY(!model->setValue("recordingDirectory",QString(folder + QChar('\n'))));
        QVERIFY(model->setValue("recordingDirectory",QString{})); QCOMPARE(native("input-record-path"),QString{});
    }
    void nativeTrackAndEncoderPreferencesPersist()
    {
        for (const auto *key : {"nativeRecording","playLocally","includeAudio","includeVideo","includeSubtitles"})
            QVERIFY(model->setValue(QLatin1String(key),false));
        QVERIFY(model->setValue("encodingPreset",QStringLiteral("ultrafast")));
        QVERIFY(model->setValue("tuning",QStringLiteral("zerolatency")));
        const auto before = model->values(); close(); open(); QCOMPARE(model->values(),before);
        QCOMPARE(native("sout-x264-preset"),QStringLiteral("ultrafast")); QCOMPARE(native("sout-x264-tune"),QStringLiteral("zerolatency"));
        for (const auto &entry : model->choices().value("castQuality").toList())
            QVERIFY(model->setValue("castQuality",entry.toMap().value("value")));
        const auto quality = model->values().value("castQuality"); close(); open(); QCOMPARE(model->values().value("castQuality"),quality);
    }
    void customSavedValuesRemainExplicitAndUnchanged()
    {
        config_PutPsz("sout-x264-preset","owned-custom"); config_PutInt("sout-chromecast-conversion-quality",99);
        QCOMPARE(config_SaveConfigFile(intf),0); model->reload(); const auto disk=bytes(config);
        QCOMPARE(model->values().value("encodingPreset").toString(),QStringLiteral("owned-custom"));
        QCOMPARE(model->values().value("castQuality").toInt(),99); model->reload(); QCOMPARE(bytes(config),disk);
        QVERIFY(model->setValue("includeVideo",false)); QCOMPARE(native("sout-x264-preset"),QStringLiteral("owned-custom"));
        QCOMPARE(config_GetInt("sout-chromecast-conversion-quality"),int64_t(99));
    }
    void failedNativeSaveRestoresValuesThenRetries()
    {
#ifdef _WIN32
        QVERIFY(model->setValue("includeAudio",false)); const auto before=model->values(); const auto disk=bytes(config);
        HANDLE lock=CreateFileW(reinterpret_cast<LPCWSTR>(config.utf16()),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        QVERIFY(lock!=INVALID_HANDLE_VALUE); const bool saved=model->setValue("includeAudio",true); CloseHandle(lock);
        QVERIFY(!saved); QCOMPARE(model->values(),before); QCOMPARE(bytes(config),disk); QCOMPARE(config_GetInt("sout-audio"),int64_t(0));
        QVERIFY(model->setValue("includeAudio",true)); QVERIFY(model->lastError().isEmpty());
#else
        QSKIP("Windows sharing rejection is tested on the target platform.");
#endif
    }
    void discoveryPolicyPersistsAndOnlyAcceptedOffRequestsStop()
    {
        QSignalSpy disabled(model.get(),&RecordingPreferencesModel::discoveryDisabled);
        QVERIFY(vlc::qt::castDiscoveryEnabled(settings.get()));
        QVERIFY(model->setValue("discoverDevices",false)); QCOMPARE(disabled.count(),1);
        QVERIFY(!vlc::qt::castDiscoveryEnabled(settings.get())); QVERIFY(!QFile::exists(config));
        close(); open(); QVERIFY(!model->values().value("discoverDevices").toBool());
        QVERIFY(model->setValue("discoverDevices",true)); QVERIFY(vlc::qt::castDiscoveryEnabled(settings.get()));
    }
    void rejectedInterfaceSavePreservesDiscoveryAndDoesNotStop()
    {
#ifdef _WIN32
        QVERIFY(model->setValue("discoverDevices",true)); const auto disk=bytes(interfaceFile);
        QSignalSpy disabled(model.get(),&RecordingPreferencesModel::discoveryDisabled);
        HANDLE lock=CreateFileW(reinterpret_cast<LPCWSTR>(interfaceFile.utf16()),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        QVERIFY(lock!=INVALID_HANDLE_VALUE); const bool saved=model->setValue("discoverDevices",false); CloseHandle(lock);
        QVERIFY(!saved); QVERIFY(model->values().value("discoverDevices").toBool()); QVERIFY(vlc::qt::castDiscoveryEnabled(settings.get()));
        QCOMPARE(disabled.count(),0); QCOMPARE(bytes(interfaceFile),disk);
        QVERIFY(model->setValue("discoverDevices",false)); QCOMPARE(disabled.count(),1);
#else
        QSKIP("Windows sharing rejection is tested on the target platform.");
#endif
    }
    void commandLineOverridesAndApplyTimingStayVisible()
    {
        close(); open(true,{"--no-sout-audio","--sout-x264-preset=ultrafast"});
        QCOMPARE(model->applyModes().value("includeAudio").toString(),QStringLiteral("commandLineOverride"));
        QCOMPARE(model->applyModes().value("recordingDirectory").toString(),QStringLiteral("nextInput"));
        QCOMPARE(model->applyModes().value("discoverDevices").toString(),QStringLiteral("live"));
        QVERIFY(model->setValue("encodingPreset",QStringLiteral("fast")));
        QCOMPARE(native("sout-x264-preset"),QStringLiteral("fast"));
        char *overrideValue=var_InheritString(intf,"sout-x264-preset"); QCOMPARE(QString::fromUtf8(overrideValue),QStringLiteral("ultrafast")); free(overrideValue);
        close(); open(); QCOMPARE(model->values().value("encodingPreset").toString(),QStringLiteral("fast"));
    }
    void headlessPickerAndAdvancedActionsDoNotWrite()
    {
        QVERIFY(!model->chooseRecordingDirectory()); QVERIFY(!model->busy());
        QSignalSpy advanced(model.get(),&RecordingPreferencesModel::advancedOptionsRequested);
        QVERIFY(model->openAdvanced()); QCOMPARE(advanced.count(),1);
        QVERIFY(!QFile::exists(config)); QVERIFY(!QFile::exists(interfaceFile));
    }
};
int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv); TestRecordingPreferences test; return QTest::qExec(&test,argc,argv);
}
#include <test_recording_preferences.moc>
