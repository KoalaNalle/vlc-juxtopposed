// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include "maininterface/player_profiles_model.hpp"
#include "maininterface/player_profile_layout.hpp"
#include "player/player_controlbar_model.hpp"
#include "player/control_list_model.hpp"
#include "util/checked_preferences_store.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#ifdef _WIN32
# include <windows.h>
#endif
extern "C" const char vlc_module_name[] = "player_profiles_model";
extern "C" vlc_object_t *vlc_test_general_instance_object(libvlc_instance_t *);
namespace {
QByteArray bytes(const QString &path) {QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray{};}
bool write(const QString &path,const QByteArray &value) {QFile file(path);return file.open(QIODevice::WriteOnly)&&file.write(value)==value.size();}
struct Session {
    libvlc_instance_t *instance = nullptr; qt_intf_t *intf = nullptr;
    bool open(const QString &path) {
        const QByteArray config=QByteArray("--config=")+path.toUtf8();
        const char *args[]={"--no-ignore-config","--no-media-library","--extraintf=","--control=","--no-metadata-network-access","--quiet",config.constData()};
        instance=libvlc_new(ARRAY_SIZE(args),args);if(!instance)return false;
        intf=vlc_object_create<qt_intf_t>(vlc_test_general_instance_object(instance));return intf!=nullptr;
    }
    void close() {if(intf){vlc_object_delete(intf);intf=nullptr;}if(instance){libvlc_release(instance);instance=nullptr;}}
    ~Session(){close();}
};
#ifdef _WIN32
struct RenameLock {
    HANDLE handle=INVALID_HANDLE_VALUE;
    bool lock(const QString &path) {
        handle=CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        return handle!=INVALID_HANDLE_VALUE;
    }
    void unlock(){if(handle!=INVALID_HANDLE_VALUE){CloseHandle(handle);handle=INVALID_HANDLE_VALUE;}}
    ~RenameLock(){unlock();}
};
#endif
}
class TestPlayerProfilesModel final : public QObject
{
    Q_OBJECT
    QTemporaryDir temporary; QString root,configPath,settingsPath;
    Session native;
    std::unique_ptr<QSettings> settings;
    std::unique_ptr<PlayerProfilesModel> model;
    bool nativeChange(const QVariantMap &changes) {QString error;return CheckedPreferencesStore(native.intf).saveNative(changes,error);}
    void reopenModel() {model.reset();settings=std::make_unique<QSettings>(settingsPath,QSettings::IniFormat);
        model=std::make_unique<PlayerProfilesModel>(native.intf,settings.get());}
    QVariantMap savedDefaults() {QVariantMap value;QString error;PlayerProfilePreferences(native.intf).snapshot(value,error);return value;}
    void prepareDifferentProfile() {
        QVERIFY(model->available()); QVERIFY(model->createProfile(QStringLiteral("Travel Ää 日本語")));
        QVERIFY(nativeChange({{QStringLiteral("brightness"),1.25},{QStringLiteral("repeat"),true}}));
        model->preferencesChanged();QVERIFY(model->retrySave());
        QVERIFY(model->beginLayoutEdit()); QVERIFY(model->layout()->setTimelineAbove(0,true));
        QVERIFY(model->layout()->insertTile(0,1,0,ControlListModel::JUXTO_FOLDER_NAME)); QVERIFY(model->saveLayoutDraft());
        QVERIFY(model->selectProfile(QStringLiteral("movies"))); QCOMPARE(config_GetFloat("brightness"),1.f);
    }
private slots:
    void initTestCase() {
        QVERIFY(temporary.isValid());root=qEnvironmentVariable("JUXTO_PROFILE_MODEL_OUTPUT");
        if(root.isEmpty())root=temporary.path();else{QVERIFY(QFileInfo(root).isAbsolute());QVERIFY(!QFileInfo::exists(root));QVERIFY(QDir().mkpath(root));}
        const auto userdata=QDir(root).filePath(QStringLiteral("userdata"));QVERIFY(QDir().mkpath(userdata));qputenv("VLC_USERDATA_PATH",userdata.toUtf8());
    }
    void init() {
        const auto folder=QDir(root).filePath(QString::fromLatin1(QTest::currentTestFunction()));QVERIFY(QDir().mkpath(folder));
        configPath=QDir(folder).filePath(QStringLiteral("vlcrc"));settingsPath=QDir(folder).filePath(QStringLiteral("vlc-qt.ini"));
        QVERIFY(native.open(configPath));reopenModel();
    }
    void cleanup(){model.reset();settings.reset();native.close();}
    void initialProfilesDoNotWriteAndBusyIsHonest() {
        QVERIFY(model->available());QCOMPARE(model->profiles().size(),4);QCOMPARE(model->selectedId(),QStringLiteral("movies"));
        QCOMPARE(model->selectedName(),QStringLiteral("Movies"));QVERIFY(!model->busy());QVERIFY(!model->unsaved());QVERIFY(!model->needsRecovery());
        QVERIFY(!QFile::exists(settingsPath));QVERIFY(!QFile::exists(configPath));
        model->setExternalBusy(true);QVERIFY(!model->createProfile(QStringLiteral("Blocked")));QVERIFY(!model->beginLayoutEdit());
        QVERIFY(!model->selectProfile(QStringLiteral("shows")));QVERIFY(!QFile::exists(settingsPath));
        model->setExternalBusy(false);QVERIFY(model->createProfile(QStringLiteral("Working")));QVERIFY(!model->busy());
    }
    void nativeSettingsAndLayoutFollowIndependentProfiles() {
        const auto original=model->layout()->snapshot();prepareDifferentProfile();const auto id=model->profiles().last().toMap().value(QStringLiteral("id")).toString();
        QCOMPARE(model->layout()->snapshot(),original);QSignalSpy applied(model.get(),&PlayerProfilesModel::defaultsApplied);
        QVERIFY(model->selectProfile(id));QCOMPARE(model->selectedName(),QStringLiteral("Travel Ää 日本語"));
        QCOMPARE(config_GetFloat("brightness"),1.25f);QVERIFY(config_GetInt("repeat"));QVERIFY(model->layout()->videoTimelineAbove());
        QCOMPARE(model->layout()->video()->center()->rowCount(),1);QCOMPARE(applied.size(),1);
        QVERIFY(applied[0][0].toStringList().contains(QStringLiteral("brightness")));QVERIFY(!model->needsRecovery());
        QVERIFY(model->selectProfile(QStringLiteral("shows")));QCOMPARE(config_GetFloat("brightness"),1.f);QCOMPARE(model->layout()->snapshot(),original);
        QCOMPARE(config_GetInt("metadata-network-access"),int64_t(0));QVERIFY(!model->unsaved());
    }
    void currentPreferenceEditsCheckpointWithoutInputOrProfileSwitch() {
        QSignalSpy applied(model.get(),&PlayerProfilesModel::defaultsApplied);
        QVERIFY(nativeChange({{QStringLiteral("audio-desync"),100}}));model->preferencesChanged();QVERIFY(model->unsaved());
        QTRY_VERIFY(!model->unsaved());QCOMPARE(model->selectedId(),QStringLiteral("movies"));QCOMPARE(applied.size(),0);
        reopenModel();QVERIFY(model->available());QVERIFY(model->selectProfile(QStringLiteral("shows")));QCOMPARE(config_GetInt("audio-desync"),int64_t(0));
        QVERIFY(model->selectProfile(QStringLiteral("movies")));QCOMPARE(config_GetInt("audio-desync"),int64_t(100));
    }
    void cancellingAndRejectedNamesPreserveActiveAndSavedProfiles() {
        QVERIFY(model->createProfile(QStringLiteral("Keep")));const auto before=bytes(settingsPath);const auto active=model->layout()->snapshot();
        QVERIFY(model->beginLayoutEdit());QVERIFY(model->layout()->insertTile(0,1,0,ControlListModel::STOP_BUTTON));
        QVERIFY(!model->selectProfile(QStringLiteral("movies")));QVERIFY(!model->createProfile(QStringLiteral("Other")));
        model->layout()->cancelEdit();QCOMPARE(model->layout()->snapshot(),active);QCOMPARE(bytes(settingsPath),before);
        QVERIFY(!model->createProfile(QStringLiteral(" keep ")));QVERIFY(!model->createProfile(QString{}));
        QCOMPARE(bytes(settingsPath),before);QCOMPARE(model->selectedName(),QStringLiteral("Keep"));
    }
    void failedLayoutSavePreservesDraftAndCurrentPlayerThenRetries() {
#ifdef _WIN32
        QVERIFY(model->createProfile(QStringLiteral("Keep")));const auto active=model->layout()->snapshot();const auto accepted=bytes(settingsPath);
        QVERIFY(model->beginLayoutEdit());QVERIFY(model->layout()->insertTile(0,1,0,ControlListModel::STOP_BUTTON));const auto draft=model->layout()->draftSnapshot();
        RenameLock lock;QVERIFY(lock.lock(settingsPath));QVERIFY(!model->saveLayoutDraft());
        QVERIFY(model->layout()->editing());QCOMPARE(model->layout()->draftSnapshot(),draft);QCOMPARE(model->layout()->snapshot(),active);
        QCOMPARE(bytes(settingsPath),accepted);QVERIFY(!model->busy());lock.unlock();
        QVERIFY2(model->saveLayoutDraft(),qPrintable(model->lastError()));QCOMPARE(model->layout()->snapshot(),draft);QVERIFY(!model->layout()->editing());
#else
        QFAIL("Real Windows file sharing failure required.");
#endif
    }
    void nativeSaveFailureRetainsJournalAndCanRetry() {
#ifdef _WIN32
        prepareDifferentProfile();const auto id=model->profiles().last().toMap().value(QStringLiteral("id")).toString();
        const auto before=savedDefaults();const auto layout=model->layout()->snapshot();QSignalSpy applied(model.get(),&PlayerProfilesModel::defaultsApplied);
        RenameLock lock;QVERIFY(lock.lock(configPath));QVERIFY(!model->selectProfile(id));
        QVERIFY(model->needsRecovery());QCOMPARE(model->selectedId(),QStringLiteral("movies"));QCOMPARE(savedDefaults(),before);
        QCOMPARE(model->layout()->snapshot(),layout);QCOMPARE(applied.size(),0);QVERIFY(!model->busy());
        QVERIFY(!model->createProfile(QStringLiteral("Blocked")));QVERIFY(!model->beginLayoutEdit());
        lock.unlock();QVERIFY2(model->retrySwitch(),qPrintable(model->lastError()));QVERIFY(!model->needsRecovery());QCOMPARE(model->selectedId(),id);
        QCOMPARE(config_GetFloat("brightness"),1.25f);QVERIFY(model->layout()->videoTimelineAbove());
#else
        QFAIL("Real Windows file sharing failure required.");
#endif
    }
    void nativeSaveFailureCanRestorePreviousWithoutApplyingTarget() {
#ifdef _WIN32
        prepareDifferentProfile();const auto id=model->profiles().last().toMap().value(QStringLiteral("id")).toString();const auto before=savedDefaults();
        RenameLock lock;QVERIFY(lock.lock(configPath));QVERIFY(!model->selectProfile(id));lock.unlock();
        QVERIFY(model->restorePreviousProfile());QVERIFY(!model->needsRecovery());QCOMPARE(model->selectedId(),QStringLiteral("movies"));QCOMPARE(savedDefaults(),before);
#else
        QFAIL("Real Windows file sharing failure required.");
#endif
    }
    void finishSaveFailureRetainsAppliedTargetAndRestoresExactly() {
#ifdef _WIN32
        prepareDifferentProfile();const auto id=model->profiles().last().toMap().value(QStringLiteral("id")).toString();
        const auto before=savedDefaults();const auto layout=model->layout()->snapshot();RenameLock lock;bool locked=false;
        const auto connection=connect(model.get(),&PlayerProfilesModel::defaultsApplied,this,[&](const QStringList &){
            QVERIFY(model->busy());locked=lock.lock(settingsPath);model->preferencesChanged();});
        QVERIFY(!model->selectProfile(id));disconnect(connection);QVERIFY(locked);lock.unlock();
        QVERIFY(!model->busy());QVERIFY(model->needsRecovery());QCOMPARE(model->selectedId(),QStringLiteral("movies"));
        QCOMPARE(config_GetFloat("brightness"),1.25f);QVERIFY(model->layout()->videoTimelineAbove());
        QVERIFY2(model->restorePreviousProfile(),qPrintable(model->lastError()));QCOMPARE(savedDefaults(),before);QCOMPARE(model->layout()->snapshot(),layout);
        QVERIFY(!model->needsRecovery());QCOMPARE(model->selectedId(),QStringLiteral("movies"));
#else
        QFAIL("Real Windows file sharing failure required.");
#endif
    }
    void staleWindowDoesNotOverwriteAcceptedCatalog() {
        PlayerProfilesModel other(native.intf,settings.get());QVERIFY(other.available());
        QVERIFY(model->createProfile(QStringLiteral("First window")));const auto accepted=bytes(settingsPath);
        QVERIFY(!other.createProfile(QStringLiteral("Stale window")));QCOMPARE(bytes(settingsPath),accepted);
        QCOMPARE(other.selectedId(),QStringLiteral("movies"));QVERIFY(!other.lastError().isEmpty());
    }
    void corruptSelectedLayoutIsPreservedAndCannotBeUnblocked() {
        QVERIFY(model->createProfile(QStringLiteral("Keep")));model.reset();
        auto doc=QJsonDocument::fromJson(settings->value(PlayerProfilesStore::settingsKey()).toString().toUtf8()).toVariant().toMap();
        auto profiles=doc.value(QStringLiteral("profiles")).toList();const auto selected=doc.value(QStringLiteral("selected")).toString();
        for(auto &entry:profiles){auto profile=entry.toMap();if(profile.value(QStringLiteral("id")).toString()==selected){
            auto snapshot=profile.value(QStringLiteral("snapshot")).toMap();snapshot[QStringLiteral("layout")]=QVariantMap{{QStringLiteral("version"),99}};
            profile[QStringLiteral("snapshot")]=snapshot;entry=profile;}}
        doc[QStringLiteral("profiles")]=profiles;settings->setValue(PlayerProfilesStore::settingsKey(),QString::fromUtf8(QJsonDocument::fromVariant(doc).toJson(QJsonDocument::Compact)));settings->sync();
        const auto accepted=bytes(settingsPath);reopenModel();QVERIFY(!model->available());model->setExternalBusy(true);model->setExternalBusy(false);
        QVERIFY(!model->retrySave());QVERIFY(!model->beginLayoutEdit());QCOMPARE(bytes(settingsPath),accepted);
    }
    void unfinishedSwitchReopensWithMatchingTargetLayoutAndFinishesInColdProcess() {
#ifdef _WIN32
        prepareDifferentProfile();const auto id=model->profiles().last().toMap().value(QStringLiteral("id")).toString();RenameLock lock;bool locked=false;
        const auto connection=connect(model.get(),&PlayerProfilesModel::defaultsApplied,this,[&](const QStringList &){locked=lock.lock(settingsPath);});
        QVERIFY(!model->selectProfile(id));disconnect(connection);QVERIFY(locked);lock.unlock();QVERIFY(model->needsRecovery());
        QVERIFY(write(settingsPath+QStringLiteral(".before-parent-close"),bytes(settingsPath)));
        model.reset();settings.reset();native.close();QProcess child;
        QVERIFY(write(settingsPath+QStringLiteral(".before-child"),bytes(settingsPath)));
        child.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){args->flags|=CREATE_NO_WINDOW;});
        child.start(QCoreApplication::applicationFilePath(),{QStringLiteral("--owned-profile-model-recovery"),configPath,settingsPath,id});
        QVERIFY(child.waitForStarted(10000));QVERIFY(child.waitForFinished(60000));QCOMPARE(child.exitStatus(),QProcess::NormalExit);
        QVERIFY2(child.exitCode()==0,child.readAllStandardError().constData());
        QVERIFY(write(settingsPath+QStringLiteral(".after-child"),bytes(settingsPath)));
        const auto physicalCopy=settingsPath+QStringLiteral(".physical-copy.ini");QVERIFY(QFile::copy(settingsPath,physicalCopy));
        QSettings physical(physicalCopy,QSettings::IniFormat);
        const auto physicalDocument=QJsonDocument::fromJson(physical.value(PlayerProfilesStore::settingsKey()).toString().toUtf8()).object();
        QVERIFY(!physicalDocument.contains(QStringLiteral("pending")));
        QVERIFY(native.open(configPath));QVERIFY(write(settingsPath+QStringLiteral(".after-native-open"),bytes(settingsPath)));
        reopenModel();QVERIFY(write(settingsPath+QStringLiteral(".after-parent-reopen"),bytes(settingsPath)));
        QVERIFY2(!model->needsRecovery(),qPrintable(model->lastError()));QCOMPARE(model->selectedId(),id);QVERIFY(model->layout()->videoTimelineAbove());
#else
        QFAIL("Real Windows file sharing failure required.");
#endif
    }
};
int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    if(app.arguments().value(1)==QStringLiteral("--owned-profile-model-recovery")){
        Session native;if(!native.open(app.arguments().value(2)))return 71;
        QSettings settings(app.arguments().value(3),QSettings::IniFormat);PlayerProfilesModel model(native.intf,&settings);
        if(!model.available()||!model.needsRecovery()||model.busy()||model.selectedId()!=QStringLiteral("movies")||!model.layout()->videoTimelineAbove())return 72;
        if(!model.retrySwitch()||model.needsRecovery()||model.selectedId()!=app.arguments().value(4))return 73;
        return 0;
    }
    TestPlayerProfilesModel test;return QTest::qExec(&test,argc,argv);
}
#include "test_player_profiles_model.moc"
