// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "maininterface/performance_preferences_model.hpp"
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <vlc_process_priority.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <limits>
#ifdef _WIN32
# include <windows.h>
#endif
extern "C" vlc_object_t *vlc_test_library_object(libvlc_instance_t *);

// Changes scheduling only in this quiet owned test process, with restoration
// in cleanup before any subsequent case. Consumer flags enable the actual
// model; codec/frame/clock consumers have separate native acceptance gates.
class TestPerformancePreferences final : public QObject
{
    Q_OBJECT
    QTemporaryDir temp;
    libvlc_instance_t *instance = nullptr;
    qt_intf_t *intf = nullptr;
    std::unique_ptr<PerformancePreferencesModel> model;
    QString config, directory;
    vlc_process_priority_state initial{};
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
        model = std::make_unique<PerformancePreferencesModel>(intf,
            PerformancePreferencesModel::Consumers{available,available,available,available,available,available});
    }
    void close()
    {
        model.reset(); if (intf) { vlc_object_delete(intf); intf = nullptr; }
        if (instance) { libvlc_release(instance); instance = nullptr; }
    }
    void store(const char *key, const char *value)
    { config_PutPsz(key,value); QCOMPARE(config_SaveConfigFile(intf),0); model->reload(); }
private slots:
    void init()
    {
        QCOMPARE(vlc_process_priority_get(&initial),VLC_SUCCESS);
        QVERIFY(temp.isValid());
        directory = QDir(temp.path()).filePath(QString::fromLatin1(QTest::currentTestFunction()) + QString::fromUtf8("-Ää"));
        QVERIFY(QDir().mkpath(directory)); config = QDir(directory).filePath("vlcrc");
        const auto userdata = QDir(directory).filePath("userdata"); QVERIFY(QDir().mkpath(userdata));
        qputenv("VLC_USERDATA_PATH", userdata.toUtf8());
#ifdef _WIN32
        SetEnvironmentVariableW(L"VLC_USERDATA_PATH", reinterpret_cast<LPCWSTR>(userdata.utf16()));
#endif
        open();
    }
    void cleanup()
    {
        vlc_process_priority_state restored;
        const auto result = vlc_process_priority_apply(initial.level,initial.realtime,&restored);
        close(); QCOMPARE(result,VLC_SUCCESS); QCOMPARE(restored.level,initial.level); QCOMPARE(restored.realtime,initial.realtime);
    }
    void openingAndReloadNeverWriteDefaults()
    {
        QCOMPARE(model->values().size(),14); QCOMPARE(model->values().value("postprocessing").toInt(),-1);
        QCOMPARE(model->values().value("encoder").toString(),QStringLiteral("any"));
        QCOMPARE(model->values().value("decoder").toString(),QStringLiteral("any"));
        QVERIFY(model->capabilities().value("hardwareDecoder").toBool());
        QVERIFY(model->capabilities().value("postprocessing").toBool());
        QVERIFY(!QFile::exists(config)); model->reload(); QVERIFY(!QFile::exists(config));
        QVERIFY(model->lastError().isEmpty());
    }
    void disabledConsumersDoNotSaveOrRescan()
    {
        close(); open(false);
        for (const auto &key : model->capabilities().keys())
            if (key != QStringLiteral("advanced")) QVERIFY(!model->capabilities().value(key).toBool());
        QVERIFY(!model->setValue("decodingThreads",2)); QVERIFY(!model->setValue("processPriority",-1));
        QVERIFY(!model->setValue("encoder",QStringLiteral("any"))); QVERIFY(!model->rescanPlugins());
        QVERIFY(!QFile::exists(config)); QVERIFY(!model->busy());
    }
    void strictValidationPreservesMemoryAndDisk()
    {
        QVERIFY(model->setValue("decodingThreads",2)); const auto before=model->values(); const auto disk=bytes(config);
        const QList<QPair<QString,QVariant>> edits = {{"decodingThreads",true},{"decodingThreads",QStringLiteral("2")},
            {"decodingThreads",-1},{"decodingThreads",33},{"decodingThreads",0.5},
            {"decodingThreads",std::numeric_limits<double>::quiet_NaN()},
            {"clockJitter",60001},{"clockAverage",1001},{"dropLateFrames",1},{"processPriority",3},
            {"realTimePriority",1},{"postprocessing",0},{"postprocessing",7},{"clockSync",2},{"loopFilter",5},
            {"encoder",QStringLiteral("missing-owned-module")},{"decoder",true},{"missing",1}};
        for (const auto &edit : edits) {
            QVERIFY2(!model->setValue(edit.first,edit.second),qPrintable(edit.first));
            QVERIFY(!model->lastError().isEmpty()); QCOMPARE(model->values(),before); QCOMPARE(bytes(config),disk);
        }
    }
    void integerAndBooleanFieldsSurviveRestart()
    {
        QVERIFY(model->setValue("decodingThreads",32)); QVERIFY(model->setValue("clockJitter",60000));
        QVERIFY(model->setValue("clockAverage",0)); QVERIFY(model->setValue("dropLateFrames",false));
        for (const auto &entry : model->choices().value("loopFilter").toList())
            QVERIFY(model->setValue("loopFilter",entry.toMap().value("value")));
        for (const auto &entry : model->choices().value("clockSync").toList())
            QVERIFY(model->setValue("clockSync",entry.toMap().value("value")));
        close(); open(); QCOMPARE(model->values().value("decodingThreads").toInt(),32);
        QCOMPARE(model->values().value("clockJitter").toInt(),60000); QCOMPARE(model->values().value("clockAverage").toInt(),0);
        QVERIFY(!model->values().value("dropLateFrames").toBool());
    }
    void hardwareOffPreservesDeviceAndAutomaticRestoresFlag()
    {
        store("dec-dev","owned-missing-device"); const auto disk=bytes(config);
        QCOMPARE(model->values().value("hardwareDecoder").toString(),QStringLiteral("owned-missing-device"));
        model->reload(); QCOMPARE(bytes(config),disk);
        QVERIFY(model->setValue("hardwareDecoder",QStringLiteral("off"))); QCOMPARE(native("dec-dev"),QStringLiteral("owned-missing-device"));
        close(); open(); QCOMPARE(model->values().value("hardwareDecoder").toString(),QStringLiteral("off"));
        QVERIFY(model->setValue("hardwareDecoder",QStringLiteral("any"))); QVERIFY(config_GetInt("hw-dec"));
        QCOMPARE(native("dec-dev"),QStringLiteral("any"));
    }
    void moduleInventoryAndCustomPreferencesRemainExplicit()
    {
        for (const auto &key : {QStringLiteral("encoder"),QStringLiteral("decoder")}) {
            const auto choices=model->choices().value(key).toList(); QVERIFY(choices.size()>1);
            QCOMPARE(choices.first().toMap().value("value").toString(),QStringLiteral("any"));
            QVERIFY(model->setValue(key,choices.last().toMap().value("value")));
        }
        store("codec","owned-custom-decoder"); store("sout-transcode-preferred-encoder","owned-custom-encoder");
        const auto disk=bytes(config); close(); open();
        QCOMPARE(model->values().value("decoder").toString(),QStringLiteral("owned-custom-decoder"));
        QCOMPARE(model->values().value("encoder").toString(),QStringLiteral("owned-custom-encoder"));
        QCOMPARE(bytes(config),disk);
        QVERIFY(model->setValue("encoder",QStringLiteral("any"))); QVERIFY(model->setValue("decoder",QStringLiteral("any")));
    }
    void postprocessingPreservesOtherFiltersAndInlineOptions()
    {
        store("video-filter","adjust{brightness=1.2}:pp{q=2,name='deblock, dering'}:sharpen{sigma=0.3}");
        QCOMPARE(model->values().value("postprocessing").toInt(),2);
        QVERIFY(model->setValue("postprocessing",5));
        QCOMPARE(native("video-filter"),QStringLiteral("adjust{brightness=1.2}:postproc{name='deblock, dering',q=5}:sharpen{sigma=0.3}"));
        close(); open(); QCOMPARE(model->values().value("postprocessing").toInt(),5);
        QVERIFY(model->setValue("postprocessing",-1));
        QCOMPARE(native("video-filter"),QStringLiteral("adjust{brightness=1.2}:sharpen{sigma=0.3}"));
        QVERIFY(model->setValue("postprocessing",3));
        QCOMPARE(native("video-filter"),QStringLiteral("adjust{brightness=1.2}:sharpen{sigma=0.3}:postproc{q=3}"));
    }
    void duplicatePostprocessingRequiresOffAndInvalidChainIsPreserved()
    {
        store("video-filter","postproc{q=1}:adjust:postprocess{q=4}");
        QCOMPARE(model->values().value("postprocessing").toString(),QStringLiteral("custom")); const auto disk=bytes(config);
        QVERIFY(!model->setValue("postprocessing",3)); QCOMPARE(bytes(config),disk);
        QVERIFY(model->setValue("postprocessing",-1)); QCOMPARE(native("video-filter"),QStringLiteral("adjust"));
        store("video-filter","postproc{q=invalid}:adjust"); const auto invalid=bytes(config);
        QVERIFY(!model->capabilities().value("postprocessing").toBool());
        QVERIFY(!model->setValue("postprocessing",-1)); QCOMPARE(bytes(config),invalid);
    }
    void rejectedSaveRestoresCompoundFiltersThenRetries()
    {
#ifdef _WIN32
        store("video-filter","adjust:pp{q=2,name='deblock'}");
        const auto before=model->values(); const auto disk=bytes(config); const auto chain=native("video-filter");
        HANDLE lock=CreateFileW(reinterpret_cast<LPCWSTR>(config.utf16()),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        QVERIFY(lock!=INVALID_HANDLE_VALUE); const bool accepted=model->setValue("postprocessing",6); CloseHandle(lock);
        QVERIFY(!accepted); QCOMPARE(model->values(),before); QCOMPARE(bytes(config),disk); QCOMPARE(native("video-filter"),chain);
        QVERIFY(model->setValue("postprocessing",6)); QVERIFY(model->lastError().isEmpty());
#else
        QSKIP("Windows native sharing rejection.");
#endif
    }
    void priorityAppliesExactClassAndPersists()
    {
        QVERIFY(model->setValue("processPriority",-1)); vlc_process_priority_state state;
        QCOMPARE(vlc_process_priority_get(&state),VLC_SUCCESS); QCOMPARE(state.level,-1); QVERIFY(!state.realtime);
        QCOMPARE(config_GetInt("win32-process-priority"),-1);
        close(); open(); QCOMPARE(vlc_process_priority_get(&state),VLC_SUCCESS); QCOMPARE(state.level,-1);
        QCOMPARE(model->values().value("processPriority").toInt(),-1);
        QVERIFY(model->setValue("processPriority",0));
    }
    void failedPrioritySaveRestoresWindowsAndConfiguration()
    {
#ifdef _WIN32
        QVERIFY(model->setValue("processPriority",0)); const auto before=model->values(); const auto disk=bytes(config);
        HANDLE lock=CreateFileW(reinterpret_cast<LPCWSTR>(config.utf16()),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        QVERIFY(lock!=INVALID_HANDLE_VALUE); const bool accepted=model->setValue("processPriority",-1); CloseHandle(lock);
        vlc_process_priority_state state; QCOMPARE(vlc_process_priority_get(&state),VLC_SUCCESS);
        QVERIFY(!accepted); QCOMPARE(state.level,0); QCOMPARE(model->values(),before); QCOMPARE(bytes(config),disk);
        QCOMPARE(config_GetInt("win32-process-priority"),0); QVERIFY(model->setValue("processPriority",-1));
#else
        QSKIP("Windows native sharing rejection.");
#endif
    }
    void realtimeRequiresExactGrantOrLeavesSavedPolicyUnchanged()
    {
        QVERIFY(model->setValue("processPriority",0)); const auto disk=bytes(config);
        const bool accepted=model->setValue("realTimePriority",true);
        vlc_process_priority_state observed,restored;
        const auto read=vlc_process_priority_get(&observed);
        const auto restore=vlc_process_priority_apply(0,false,&restored); // Before assertions or output.
        QCOMPARE(read,VLC_SUCCESS); QCOMPARE(restore,VLC_SUCCESS);
        if (accepted) {
            QVERIFY(observed.realtime); QVERIFY(config_GetInt("win32-realtime-priority"));
            QVERIFY(model->setValue("realTimePriority",false));
        } else {
            QVERIFY(!observed.realtime); QCOMPARE(observed.level,0); QCOMPARE(bytes(config),disk);
            QVERIFY(!config_GetInt("win32-realtime-priority")); QVERIFY(!model->lastError().isEmpty());
        }
    }
    void savedUnappliedRealtimeRemainsExplicitAndClearable()
    {
        QVERIFY(model->setValue("processPriority",0));
        config_PutInt("win32-realtime-priority",1); QCOMPARE(config_SaveConfigFile(intf),0);
        model->reload(); vlc_process_priority_state state;
        QCOMPARE(vlc_process_priority_get(&state),VLC_SUCCESS); QVERIFY(!state.realtime);
        QVERIFY(model->values().value("prioritySavedRealTime").toBool());
        QCOMPARE(model->values().value("realTimePriority").toString(),QStringLiteral("requested"));
        QVERIFY(model->setValue("realTimePriority",false));
        QCOMPARE(config_GetInt("win32-realtime-priority"),0);
        QCOMPARE(model->values().value("realTimePriority"),QVariant(false));
    }
    void commandLineOverridesAndApplyTimingAreVisible()
    {
        close(); open(true,{"--avcodec-threads=1","--video-filter=adjust","--no-hw-dec","--codec=avcodec"});
        for (const auto &key : {"decodingThreads","postprocessing","hardwareDecoder","decoder"})
            QCOMPARE(model->applyModes().value(key).toString(),QStringLiteral("commandLineOverride"));
        QCOMPARE(model->applyModes().value("encoder").toString(),QStringLiteral("nextJob"));
        QCOMPARE(model->applyModes().value("dropLateFrames").toString(),QStringLiteral("restart"));
        QCOMPARE(model->applyModes().value("clockJitter").toString(),QStringLiteral("nextInput"));
        QCOMPARE(model->applyModes().value("processPriority").toString(),QStringLiteral("live"));
        QVERIFY(model->setValue("decodingThreads",4)); QCOMPARE(config_GetInt("avcodec-threads"),4);
        QCOMPARE(var_InheritInteger(intf,"avcodec-threads"),1);
        close(); open(); QCOMPARE(model->values().value("decodingThreads").toInt(),4);
    }
    void actualRescanExposesBusyAndCompletionWithoutSavingPreferences()
    {
        QVERIFY(model->capabilities().value("rescan").toBool()); QSignalSpy busy(model.get(),&PerformancePreferencesModel::busyChanged);
        QVERIFY(model->rescanPlugins()); QVERIFY(model->busy());
        QVERIFY(!model->setValue("decodingThreads",2)); QVERIFY(!model->rescanPlugins());
        QTRY_VERIFY_WITH_TIMEOUT(!model->busy(),65000);
        QVERIFY2(model->lastError().isEmpty(),qPrintable(model->lastError()));
        QVERIFY(model->status().contains(QStringLiteral("Restart"))); QVERIFY(busy.count()>=2); QVERIFY(!QFile::exists(config));
    }
};
int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv); TestPerformancePreferences test; return QTest::qExec(&test,argc,argv);
}
#include <test_performance_preferences.moc>
