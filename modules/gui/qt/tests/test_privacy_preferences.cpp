// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "maininterface/privacy_preferences_model.hpp"
#include "maininterface/library_preferences_model.hpp"
#include "qt.hpp"
#include "util/recent_network_history.hpp"
#include "maininterface/compositor.hpp"
#include "medialibrary/mlhomeactivitydata.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <vlc_player.h>
#include <vlc_media_library.h>
#include <QDir>
#include <QElapsedTimer>
#include <QThread>
#include <QFile>
#include <QSettings>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#ifdef _WIN32
# include <windows.h>
#endif
extern "C" vlc_object_t *vlc_test_library_object(libvlc_instance_t *);
extern "C" uint64_t vlc_test_privacy_epoch(vlc_player_t *);
extern "C" bool vlc_test_privacy_remember(vlc_player_t *);
extern "C" char *vlc_test_privacy_pattern(vlc_player_t *);
class TestPrivacyPreferences final : public QObject
{
    Q_OBJECT
    QTemporaryDir temp;
    libvlc_instance_t *instance=nullptr;
    qt_intf_t *intf=nullptr;
    std::unique_ptr<QSettings> settings;
    std::unique_ptr<PrivacyPreferencesModel> model;
    QString config,caseDir;
    int invalidations=0,clears=0;
    bool processDuringClear=false;
    QString visitError;
    static QByteArray bytes(const QString &path) { QFile f(path); return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray{}; }
    PrivacyPreferencesModel::LocalHistory callbacks()
    {
        return {[this]{++invalidations;},[this]{++clears;if(processDuringClear)QCoreApplication::processEvents();return visitError;}};
    }
    void open(bool library=true)
    {
        const QList<QByteArray> args={QByteArray("--config=")+config.toUtf8(),"--no-ignore-config","--quiet",
            library ? "--media-library" : "--no-media-library","--no-ml-network-discovery","--no-metadata-network-access","--no-auto-preparse",
            "--extraintf=","--control=","--aout=dummy","--vout=dummy"};
        QList<const char *> argv; for(const auto &a:args)argv.append(a.constData());
        instance=libvlc_new(argv.size(),argv.data()); QVERIFY(instance);
        auto *root=vlc_test_library_object(instance);
        intf=vlc_object_create<qt_intf_t>(root);QVERIFY(intf);
        intf->p_player=vlc_player_New(root,VLC_PLAYER_LOCK_NORMAL);QVERIFY(intf->p_player);
        model=std::make_unique<PrivacyPreferencesModel>(intf,settings.get(),callbacks());
    }
    void close()
    {
        model.reset();
        if(intf){vlc_player_Delete(intf->p_player);vlc_object_delete(intf);intf=nullptr;}
        if(instance){libvlc_release(instance);instance=nullptr;}
    }
    uint64_t epoch() { vlc_player_Lock(intf->p_player);auto value=vlc_test_privacy_epoch(intf->p_player);vlc_player_Unlock(intf->p_player);return value; }
private slots:
    void init()
    {
        QVERIFY(temp.isValid());caseDir=QDir(temp.path()).filePath(QString::fromLatin1(QTest::currentTestFunction())+QString::fromLatin1(QTest::currentDataTag()));
        QVERIFY(QDir().mkpath(caseDir));config=QDir(caseDir).filePath("vlcrc");
        const auto userdata=QDir(caseDir).filePath("userdata");QVERIFY(QDir().mkpath(userdata));
        qputenv("VLC_USERDATA_PATH",userdata.toUtf8());
#ifdef _WIN32
        SetEnvironmentVariableW(L"VLC_USERDATA_PATH",reinterpret_cast<LPCWSTR>(userdata.utf16()));
#endif
        settings=std::make_unique<QSettings>(QDir(caseDir).filePath("qt.ini"),QSettings::IniFormat);
        invalidations=clears=0;processDuringClear=false;visitError.clear();open();
    }
    void cleanup(){close();settings.reset();}
    void initialContractHasNoWrites()
    {
        QCOMPARE(model->values().size(),8);QVERIFY(!QFile::exists(config));
        QVERIFY(model->capabilities().value("saveRecentlyPlayed").toBool());
        QVERIFY(model->capabilities().value("historyExclusion").toBool());
        QVERIFY(!model->capabilities().value("submitPlayedTracks").toBool());
        QVERIFY(!model->capabilities().value("lastfmPassword").toBool());
        QCOMPARE(invalidations,0);QVERIFY(!model->busy());QVERIFY(!model->restartRequired());
    }
    void typedInvalidInputsPreserveStorage()
    {
        QVERIFY(model->setValue("historyExclusion",QStringLiteral("private*")));
        const auto disk=bytes(config);const auto before=model->values();const auto generation=epoch();
        const QList<QPair<QString,QVariant>> invalid={{"saveRecentlyPlayed",1},{"rememberPlaybackPosition",QStringLiteral("false")},
            {"historyExclusion",QStringLiteral("bad\n")},{"historyExclusion",QString(QChar(0xd800))},
            {"historyExclusion",QString(4097,'x')},{"internetMetadata",QByteArray("true")},
            {"submitPlayedTracks",true},{"lastfmPassword",QStringLiteral("owned-placeholder")},{"unknown",false}};
        for(const auto &edit:invalid) { QVERIFY(!model->setValue(edit.first,edit.second));QVERIFY(!model->lastError().isEmpty());QCOMPARE(model->values(),before);QCOMPARE(bytes(config),disk);QCOMPARE(epoch(),generation); }
    }
    void independentStoragePermissionAndRestart()
    {
        const auto restore=config_GetInt("restore-playback-pos");
        QVERIFY(model->setValue("rememberPlaybackPosition",false));
        QCOMPARE(config_GetInt("restore-playback-pos"),restore);
        vlc_player_Lock(intf->p_player);const bool remembers=vlc_test_privacy_remember(intf->p_player);vlc_player_Unlock(intf->p_player);
        QVERIFY(!remembers);QVERIFY(!model->restartRequired());
        QVERIFY(model->setValue("historyExclusion",QStringLiteral("Private ?音*")));
        vlc_player_Lock(intf->p_player);char *pattern=vlc_test_privacy_pattern(intf->p_player);vlc_player_Unlock(intf->p_player);
        QCOMPARE(QString::fromUtf8(pattern),QStringLiteral("Private ?音*"));free(pattern);
        QCOMPARE(invalidations,2);QVERIFY(bytes(config).contains("remember-playback-pos=0"));
        close();open();QCOMPARE(model->values().value("rememberPlaybackPosition").toBool(),false);
        QCOMPARE(model->values().value("historyExclusion").toString(),QStringLiteral("Private ?音*"));
        QCOMPARE(config_GetInt("restore-playback-pos"),restore);
    }
    void genuineSaveFailureRollbackAndRetry()
    {
#ifdef _WIN32
        QVERIFY(model->setValue("historyExclusion",QStringLiteral("before")));
        const auto disk=bytes(config);const auto before=model->values();const auto generation=epoch();const int count=invalidations;
        HANDLE locked=CreateFileW(reinterpret_cast<LPCWSTR>(config.utf16()),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        QVERIFY(locked!=INVALID_HANDLE_VALUE);
        const bool accepted=model->setValue("historyExclusion",QStringLiteral("after"));CloseHandle(locked);
        QVERIFY(!accepted);QVERIFY(!model->lastError().isEmpty());QCOMPARE(model->values(),before);QCOMPARE(bytes(config),disk);QCOMPARE(epoch(),generation);QCOMPARE(invalidations,count);
        QVERIFY(model->setValue("historyExclusion",QStringLiteral("after")));QVERIFY(model->lastError().isEmpty());QVERIFY(epoch()!=generation);QCOMPARE(invalidations,count+1);
#endif
    }
    void sharedMetadataUsesLibraryNativeOption()
    {
        LibraryPreferencesModel::Consumers consumers;consumers.metadataFetch=true;
        LibraryPreferencesModel library(intf,consumers);
        connect(model.get(),&PrivacyPreferencesModel::metadataPreferenceChanged,&library,&LibraryPreferencesModel::reload);
        connect(&library,&LibraryPreferencesModel::valuesChanged,model.get(),&PrivacyPreferencesModel::reload);
        QVERIFY(model->setValue("internetMetadata",true));QCOMPARE(library.values().value("metadataSource").toString(),QStringLiteral("network"));
        QVERIFY(model->restartRequired());QVERIFY(library.setValue("metadataSource",QStringLiteral("local")));
        QCOMPARE(model->values().value("internetMetadata").toBool(),false);QVERIFY(!model->restartRequired());
    }
    void legacySubmissionCanOnlyBeDisabled()
    {
        config_PutPsz("extraintf","http:audioscrobbler:logger");model->reload();
        QVERIFY(model->capabilities().value("submitPlayedTracks").toBool());
        QVERIFY(model->setValue("submitPlayedTracks",false));
        char *interfaces=config_GetPsz("extraintf");QCOMPARE(QByteArray(interfaces),QByteArray("http:logger"));free(interfaces);
        QVERIFY(!model->capabilities().value("submitPlayedTracks").toBool());
        QVERIFY(!model->setValue("submitPlayedTracks",true));QVERIFY(model->restartRequired());
        config_PutPsz("extraintf","http{host='127.0.0.1:9'}:audioscrobbler{lastfm-username='owned:placeholder'}:logger{file='owned:log'}");model->reload();
        QVERIFY(model->capabilities().value("submitPlayedTracks").toBool());
        QVERIFY(model->setValue("submitPlayedTracks",false));
        interfaces=config_GetPsz("extraintf");QCOMPARE(QByteArray(interfaces),QByteArray("http{host='127.0.0.1:9'}:logger{file='owned:log'}"));free(interfaces);
    }
    void libraryOffCapabilityDoesNotOpenDatabase()
    {
        close();open(false);
        QVERIFY(!vlc_ml_instance_get(intf));QVERIFY(!var_InheritBool(intf,"media-library"));
        QVERIFY(model->capabilities().value("clearHistory").toBool());
        const auto database=QDir(caseDir).filePath("userdata/ml/ml.db");
        QVERIFY(!QFile::exists(database));model->reload();QVERIFY(!QFile::exists(database));
        QVERIFY(model->setValue("saveRecentlyPlayed",false));
        QVERIFY(!QFile::exists(database));QVERIFY(!vlc_ml_instance_get(intf));
        close();open(false);
        QVERIFY(!model->values().value("saveRecentlyPlayed").toBool());
        QVERIFY(model->capabilities().value("clearHistory").toBool());
        QVERIFY(!QFile::exists(database));
    }
    void nativeClearBusyAndLocalSuccess_data()
    {
        QTest::addColumn<bool>("library");
        QTest::newRow("Library-on")<<true;
        QTest::newRow("Library-off")<<false;
    }
    void libraryOffHomeStoreFailureAndRetry()
    {
#ifdef _WIN32
        close();open(false);
        MLHomeVisitStore visits(settings.get());
        QVERIFY(visits.record(MLItemId(7,VLC_ML_PARENT_FOLDER),123));
        QVERIFY(visits.record(MLItemId(9,VLC_ML_PARENT_PLAYLIST),456));
        const auto previous=bytes(settings->fileName());
        model.reset();
        model=std::make_unique<PrivacyPreferencesModel>(intf,settings.get(),
            PrivacyPreferencesModel::LocalHistory{[]{},[this]{
                MLHomeVisitStore persisted(settings.get());
                return persisted.clear() ? QString{} : QStringLiteral("Owned Home store write failure");
            }});
        const auto path=settings->fileName();
        HANDLE locked=CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        QVERIFY(locked!=INVALID_HANDLE_VALUE);
        auto unlock=qScopeGuard([&]{CloseHandle(locked);});
        QVERIFY(model->clearHistory());QTRY_VERIFY_WITH_TIMEOUT(!model->busy(),10000);
        QVERIFY(!model->lastError().isEmpty());QVERIFY(model->status().isEmpty());
        QCOMPARE(bytes(path),previous);QCOMPARE(MLHomeVisitStore(settings.get()).visits().size(),2);
        CloseHandle(locked);unlock.dismiss();
        QVERIFY(model->clearHistory());QTRY_VERIFY_WITH_TIMEOUT(!model->busy(),10000);
        QVERIFY(model->lastError().isEmpty());QVERIFY(MLHomeVisitStore(settings.get()).visits().isEmpty());
        QVERIFY(!vlc_ml_instance_get(intf));QVERIFY(!var_InheritBool(intf,"media-library"));
        MLHomeVisitStore current(settings.get());
        QVERIFY(current.record(MLItemId(7,VLC_ML_PARENT_FOLDER),789));
        QCOMPARE(MLHomeVisitStore(settings.get()).visits().size(),1);
#endif
    }
    void nativeClearBusyAndLocalSuccess()
    {
        QFETCH(bool,library);
        if(!library){close();open(false);}
        auto *ml=vlc_ml_history_get(intf);QVERIFY(ml);
        auto *media=vlc_ml_new_stream(ml,"http://127.0.0.1:9/owned-no-fetch");QVERIFY(media);
        const auto id=media->i_id;vlc_ml_release(media);QCOMPARE(vlc_ml_media_update_progress(ml,id,.5),VLC_SUCCESS);
        settings->setValue("filedialog-path",QStringLiteral("owned-old-location"));settings->setValue("OpenDialog/netMRL",QStringList{QStringLiteral("http://127.0.0.1/old")});settings->sync();
        QSignalSpy cleared(model.get(),&PrivacyPreferencesModel::fileDialogHistoryCleared);
        QSignalSpy networkCleared(model.get(),&PrivacyPreferencesModel::networkHistoryCleared);
        QSignalSpy busy(model.get(),&PrivacyPreferencesModel::busyChanged);
        QVERIFY(model->clearHistory());QVERIFY(model->busy());QVERIFY(!model->clearHistory());
        QVERIFY(!model->setValue("saveRecentlyPlayed",false));
        QTRY_VERIFY_WITH_TIMEOUT(!model->busy(),10000);
        QVERIFY(model->lastError().isEmpty());QCOMPARE(model->status(),QStringLiteral("History cleared."));
        QCOMPARE(clears,1);QCOMPARE(cleared.size(),1);QCOMPARE(networkCleared.size(),1);
        QVERIFY(!settings->contains("OpenDialog/netMRL"));QCOMPARE(busy.size(),2);
        QVERIFY(!settings->contains("filedialog-path"));QCOMPARE(vlc_ml_count_history(ml,nullptr,VLC_ML_HISTORY_TYPE_GLOBAL),size_t(0));
        QCOMPARE(vlc_ml_instance_get(intf)!=nullptr,library);
        QCOMPARE(var_InheritBool(intf,"media-library"),library);
    }
    void partialLocalFailureReportsRetry()
    {
        visitError=QStringLiteral("Owned local store rejection");QVERIFY(model->clearHistory());
        QTRY_VERIFY_WITH_TIMEOUT(!model->busy(),10000);
        QVERIFY(model->lastError().contains("some local history"));QCOMPARE(model->status(),QString{});
        visitError.clear();QVERIFY(model->clearHistory());QTRY_VERIFY_WITH_TIMEOUT(!model->busy(),10000);
        QVERIFY(model->lastError().isEmpty());QCOMPARE(clears,2);
    }
    void legacySavedPolicyReloadsCurrentConsumers()
    {
        config_PutInt("save-recentplay",false);config_PutInt("remember-playback-pos",false);
        config_PutPsz("history-exclude-pattern","owned legacy pattern");
        model->reload();QVERIFY(!var_InheritBool(intf,"save-recentplay"));
        vlc_player_Lock(intf->p_player);const bool remembers=vlc_test_privacy_remember(intf->p_player);char *pattern=vlc_test_privacy_pattern(intf->p_player);vlc_player_Unlock(intf->p_player);
        QVERIFY(!remembers);QCOMPARE(QByteArray(pattern),QByteArray("owned legacy pattern"));free(pattern);
        QCOMPARE(invalidations,1);
        config_PutInt("save-recentplay",true);config_PutInt("remember-playback-pos",true);model->reload();
        QVERIFY(var_InheritBool(intf,"save-recentplay"));QCOMPARE(invalidations,2);
        model->reload();QCOMPARE(invalidations,2);
    }
    void acceptedNetworkHistoryUsesLivePolicy()
    {
        using namespace RecentNetworkHistory;
        QString error;
        QCOMPARE(record(intf,settings.get(),QStringLiteral("http://127.0.0.1/first"),error),Result::Saved);
        QVERIFY(error.isEmpty());QCOMPARE(load(intf,settings.get()).size(),1);
        QVERIFY(model->setValue("saveRecentlyPlayed",false));
        const auto stored=settings->value("OpenDialog/netMRL");
        QCOMPARE(record(intf,settings.get(),QStringLiteral("http://127.0.0.1/second"),error),Result::Ignored);
        QCOMPARE(settings->value("OpenDialog/netMRL"),stored);QVERIFY(load(intf,settings.get()).isEmpty());
        QVERIFY(model->setValue("saveRecentlyPlayed",true));QCOMPARE(load(intf,settings.get()).size(),1);
        QVERIFY(model->setValue("historyExclusion",QStringLiteral("private?")));
        QCOMPARE(record(intf,settings.get(),QStringLiteral("http://127.0.0.1/private1"),error),Result::Ignored);
        QCOMPARE(settings->value("OpenDialog/netMRL"),stored);
        for(int i=0;i<12;i++)QCOMPARE(record(intf,settings.get(),QStringLiteral("http://127.0.0.1/item%1").arg(i),error),Result::Saved);
        const auto rows=load(intf,settings.get());QCOMPARE(rows.size(),8);QCOMPARE(rows.first(),QStringLiteral("http://127.0.0.1/item11"));
        QVERIFY(clear(intf,settings.get(),error));QVERIFY(load(intf,settings.get()).isEmpty());
    }
    void networkHistorySaveFailureAndRetry()
    {
#ifdef _WIN32
        using namespace RecentNetworkHistory;
        QString error;
        QCOMPARE(record(intf,settings.get(),QStringLiteral("http://127.0.0.1/first"),error),Result::Saved);
        const auto path=settings->fileName();const auto before=settings->value("OpenDialog/netMRL").toStringList();
        HANDLE locked=CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        QVERIFY(locked!=INVALID_HANDLE_VALUE);
        const auto result=record(intf,settings.get(),QStringLiteral("http://127.0.0.1/retry"),error);CloseHandle(locked);
        QCOMPARE(result,Result::Failed);QVERIFY(!error.isEmpty());QCOMPARE(settings->value("OpenDialog/netMRL").toStringList(),before);
        QCOMPARE(record(intf,settings.get(),QStringLiteral("http://127.0.0.1/retry"),error),Result::Saved);QVERIFY(error.isEmpty());QCOMPARE(load(intf,settings.get()).first(),QStringLiteral("http://127.0.0.1/retry"));
#endif
    }
    void destroyWaitsForOwnedWorker()
    {
        processDuringClear=true;
        QVERIFY(model->clearHistory());model.reset();
        QCOMPARE(clears,1);
        QVERIFY(intf->p_player); // Dependencies remain alive after joined worker.
        vlc_player_Lock(intf->p_player);vlc_player_Unlock(intf->p_player);
    }
};
// Separate opt-in pipe mode for the companion runner's real SQLite rejection.
// Normal QtTest invocation never waits on an external process.
static int runHistoryFailurePipe()
{
    const auto config=qEnvironmentVariable("VLC_PRIVACY_PIPE_CONFIG");
    const auto settingsPath=qEnvironmentVariable("VLC_PRIVACY_PIPE_SETTINGS");
    if(config.isEmpty() || settingsPath.isEmpty() || !qEnvironmentVariableIsSet("VLC_USERDATA_PATH"))return 2;
    const bool libraryOff=qEnvironmentVariableIsSet("VLC_HISTORY_LIBRARY_OFF");
    const QList<QByteArray> args={QByteArray("--config=")+config.toUtf8(),"--no-ignore-config","--quiet",
        libraryOff ? "--no-media-library" : "--media-library","--no-ml-network-discovery","--no-metadata-network-access","--no-auto-preparse",
        "--extraintf=","--control=","--aout=dummy","--vout=dummy"};
    QList<const char *> argv;for(const auto &a:args)argv.append(a.constData());
    auto *instance=libvlc_new(argv.size(),argv.data());if(!instance)return 2;
    auto *root=vlc_test_library_object(instance);auto *intf=vlc_object_create<qt_intf_t>(root);
    if(!intf)return 2;
    intf->p_player=vlc_player_New(root,VLC_PLAYER_LOCK_NORMAL);if(!intf->p_player)return 2;
    QSettings settings(settingsPath,QSettings::IniFormat);
    settings.setValue("filedialog-path",QStringLiteral("owned old location"));
    const QStringList oldUrls{QStringLiteral("http://127.0.0.1:9/owned-old-url")};
    settings.setValue("OpenDialog/netMRL",oldUrls);settings.sync();
    int localClears=0,fileSignals=0,networkSignals=0;
    auto model=std::make_unique<PrivacyPreferencesModel>(intf,&settings,
        PrivacyPreferencesModel::LocalHistory{[]{},[&]{++localClears;return QString{};}});
    QObject::connect(model.get(),&PrivacyPreferencesModel::fileDialogHistoryCleared,[&]{++fileSignals;});
    QObject::connect(model.get(),&PrivacyPreferencesModel::networkHistoryCleared,[&]{++networkSignals;});
    auto *ml=vlc_ml_history_get(intf);
    auto *media=ml?vlc_ml_new_stream(ml,"http://127.0.0.1:9/owned-never-fetched"):nullptr;
    if(!media)return 2;
    const auto id=media->i_id;vlc_ml_release(media);
    if(vlc_ml_media_update_progress(ml,id,.5)!=VLC_SUCCESS)return 2;
    vlc_player_Lock(intf->p_player);const auto epoch=vlc_test_privacy_epoch(intf->p_player);vlc_player_Unlock(intf->p_player);
    unsigned checks=0,failures=0;
    const auto check=[&](bool passed,const char *name){++checks;failures+=!passed;printf("%s: %s\n",passed?"PASS":"FAIL",name);fflush(stdout);};
    const auto wait=[&]{QElapsedTimer timer;timer.start();while(model->busy()&&timer.elapsed()<10000){QCoreApplication::processEvents();QThread::msleep(1);}return !model->busy();};
    char command[64];printf("READY controller\n");fflush(stdout);
    if(!fgets(command,sizeof(command),stdin)||strcmp(command,"blocked\n"))return 2;
    check(model->clearHistory()&&model->busy(),"controller begins native clear asynchronously");
    check(wait()&&!model->lastError().isEmpty()&&model->status().isEmpty(),"real database failure reaches controller error without success status");
    check(localClears==0&&fileSignals==0&&networkSignals==0,"failed native clear leaves all interface stores untouched");
    vlc_player_Lock(intf->p_player);const auto after=vlc_test_privacy_epoch(intf->p_player);vlc_player_Unlock(intf->p_player);
    check(epoch==after,"failed controller clear preserves player write generation");
    check(settings.value("OpenDialog/netMRL").toStringList()==oldUrls&&settings.value("filedialog-path").toString()==QStringLiteral("owned old location"),"failed controller clear preserves saved URL and file history");
    printf("RETRY\n");fflush(stdout);
    if(!fgets(command,sizeof(command),stdin)||strcmp(command,"retry\n"))return 2;
    check(model->clearHistory()&&model->busy(),"controller accepts retry after database repair");
    check(wait()&&model->lastError().isEmpty()&&model->status()==QStringLiteral("History cleared."),"controller retry reports success after all stores finish");
    check(localClears==1&&fileSignals==1&&networkSignals==1&&!settings.contains("filedialog-path")&&!settings.contains("OpenDialog/netMRL"),"retry clears Home callback, network URLs and file location exactly once");
    check(vlc_ml_count_history(ml,nullptr,VLC_ML_HISTORY_TYPE_GLOBAL)==0
          && (vlc_ml_instance_get(intf)!=nullptr)==!libraryOff
          && var_InheritBool(intf,"media-library")==!libraryOff,
          "controller retry clears native history while preserving Library preference and availability");
    model.reset();vlc_player_Delete(intf->p_player);vlc_object_delete(intf);libvlc_release(instance);
    printf("RESULT: %u checks, %u failures\n",checks,failures);fflush(stdout);
    return failures?1:0;
}
int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    if(argc==2&&!strcmp(argv[1],"--pipe-history-failure"))return runHistoryFailurePipe();
    TestPrivacyPreferences test;
    return QTest::qExec(&test,argc,argv);
}
#include "test_privacy_preferences.moc"
