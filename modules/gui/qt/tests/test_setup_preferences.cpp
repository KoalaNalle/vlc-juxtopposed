// SPDX-License-Identifier: GPL-2.0-or-later
// Real General service, native configuration and isolated Qt INIs. This fixture
// does not operate the user's consent UI or claim native preview/desktop proof.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "maininterface/general_preferences_model.hpp"
#include "maininterface/compositor.hpp"
#include "util/color_scheme_model.hpp"
#include "qt.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <limits>
#include <memory>
#ifdef _WIN32
# include <windows.h>
#endif
extern "C" vlc_object_t *vlc_test_general_instance_object(libvlc_instance_t *);

class TestSetupPreferences final : public QObject
{
    Q_OBJECT
    QTemporaryDir temp;
    QString directory, config, ini;
    libvlc_instance_t *instance = nullptr;
    qt_intf_t *intf = nullptr;
    std::unique_ptr<QSettings> settings, language;
    std::unique_ptr<ColorSchemeModel> scheme;
    std::unique_ptr<GeneralPreferencesModel> model;
    static QByteArray bytes(const QString &path)
    { QFile file(path); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{}; }
    bool open()
    {
        const QList<QByteArray> args {QByteArray("--config=")+config.toUtf8(), "--no-ignore-config",
            "--quiet", "--no-media-library", "--no-auto-preparse", "--no-metadata-network-access",
            "--no-qt-privacy-ask", "--extraintf=", "--control=", "--aout=dummy", "--vout=dummy"};
        QList<const char *> argv; for (const auto &arg : args) argv.append(arg.constData());
        instance=libvlc_new(argv.size(),argv.data()); if(!instance)return false;
        intf=vlc_object_create<qt_intf_t>(vlc_test_general_instance_object(instance)); if(!intf)return false;
        if(config_GetType("qt-privacy-ask")!=VLC_VAR_BOOL)return false;
        // Fixture input: an already decided policy in this owned native
        // configuration. --no-qt-privacy-ask suppresses a window but does not
        // change the saved decision that the real setup service requires.
        config_PutInt("qt-privacy-ask",0);
        settings=std::make_unique<QSettings>(ini,QSettings::IniFormat);settings->setFallbacksEnabled(false);
        language=std::make_unique<QSettings>(QDir(directory).filePath("language.ini"),QSettings::IniFormat);
        language->setFallbacksEnabled(false);
        scheme=std::make_unique<ColorSchemeModel>();
        scheme->setCurrentIndex(settings->value("MainWindow/color-scheme-index",0).toInt());
        model=std::make_unique<GeneralPreferencesModel>(intf,scheme.get(),settings.get(),
            GeneralPreferencesModel::Storage{language.get()});
        return true;
    }
    void close()
    {
        model.reset();scheme.reset();settings.reset();language.reset();
        if(intf){vlc_object_delete(intf);intf=nullptr;}
        if(instance){libvlc_release(instance);instance=nullptr;}
    }
#ifdef _WIN32
    struct OwnedReadLock {
        HANDLE handle=INVALID_HANDLE_VALUE;
        explicit OwnedReadLock(const QString &path) {
            handle=CreateFileW(path.toStdWString().c_str(),GENERIC_READ,FILE_SHARE_READ,
                nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        }
        void release(){if(handle!=INVALID_HANDLE_VALUE){CloseHandle(handle);handle=INVALID_HANDLE_VALUE;}}
        ~OwnedReadLock(){release();}
    };
#endif
private slots:
    void init()
    {
        QVERIFY(temp.isValid());directory=QDir(temp.path()).filePath(QString::fromLatin1(QTest::currentTestFunction()));
        QVERIFY(QDir().mkpath(directory));config=QDir(directory).filePath("vlcrc");ini=QDir(directory).filePath("interface.ini");
        const auto userdata=QDir(directory).filePath("userdata");QVERIFY(QDir().mkpath(userdata));
        qputenv("VLC_USERDATA_PATH",userdata.toUtf8());
#ifdef _WIN32
        SetEnvironmentVariableW(L"VLC_USERDATA_PATH",userdata.toStdWString().c_str());
#endif
        QVERIFY(open());
    }
    void cleanup(){close();}
    void setupReadinessPreservesConsent()
    {
        QVERIFY(model->setupRequired());QVERIFY(model->setupReady());
        config_PutInt("qt-privacy-ask",1);model->reload();
        QVERIFY(model->setupRequired());QVERIFY(!model->setupReady());
        const auto metadata=config_GetInt("metadata-network-access");
        QVERIFY(!model->finishSetup(1,6));QVERIFY(!model->lastError().isEmpty());
        QCOMPARE(config_GetInt("metadata-network-access"),metadata);
        QCOMPARE(config_GetInt("qt-privacy-ask"),1);
        QVERIFY(!QFile::exists(config));QVERIFY(!settings->contains(GeneralPreferencesModel::setupVersionKey()));
    }
    void invalidDraftDoesNotSave()
    {
        const auto before=model->values();
        const QList<QPair<QVariant,QVariant>> invalid {{true,0},{QStringLiteral("1"),0},{-1,0},{3,0},{1.5,0},
            {std::numeric_limits<double>::infinity(),0},{0,true},{0,QStringLiteral("2")},{0,-1},{0,9},{0,2.5}};
        for(const auto &choice:invalid){
            QVERIFY(!model->finishSetup(choice.first,choice.second));
            QCOMPARE(model->values(),before);QVERIFY(model->setupRequired());
            QVERIFY(!QFile::exists(config));QVERIFY(!QFile::exists(ini));
        }
    }
    void finishPersistsAndFreshInstanceRestores()
    {
        const auto metadata=config_GetInt("metadata-network-access");
        QVERIFY(model->finishSetup(1,6));QVERIFY(!model->setupRequired());QVERIFY(!model->setupReady());
        QCOMPARE(scheme->currentIndex(),1);QCOMPARE(model->values().value("accentIndex").toInt(),6);
        QCOMPARE(config_GetInt("metadata-network-access"),metadata);QCOMPARE(config_GetInt("qt-privacy-ask"),0);
        {QSettings readback(ini,QSettings::IniFormat);readback.setFallbacksEnabled(false);
            QCOMPARE(readback.value(GeneralPreferencesModel::setupVersionKey()).toInt(),1);
            QCOMPARE(readback.value("MainWindow/color-scheme-index").toInt(),1);}
        close();QVERIFY(open());
        QVERIFY(!model->setupRequired());QCOMPARE(model->values().value("theme").toInt(),1);
        QCOMPARE(model->values().value("accentIndex").toInt(),6);
        QCOMPARE(config_GetInt("metadata-network-access"),metadata);
    }
    void completionPreservesExistingEnabledMetadataChoice()
    {
        config_PutInt("metadata-network-access",1);
        QVERIFY(model->finishSetup(2,0));
        QCOMPARE(config_GetInt("metadata-network-access"),1);
        QVERIFY(!model->setupRequired());
    }
    void repeatedFinishDoesNotOverwriteLaterPreferences()
    {
        QVERIFY(model->finishSetup(1,6));QVERIFY(model->setValue("theme",2));
        const auto saved=bytes(ini);const auto native=bytes(config);
        QVERIFY(model->finishSetup(0,0));QCOMPARE(model->values().value("theme").toInt(),2);
        QCOMPARE(model->values().value("accentIndex").toInt(),6);
        QCOMPARE(bytes(ini),saved);QCOMPARE(bytes(config),native);
    }
    void blockedQtSaveCanRetryWithoutCompletion()
    {
#ifdef _WIN32
        settings->setValue("unrelated","preserved");settings->sync();const auto before=bytes(ini);
        OwnedReadLock lock(ini);QVERIFY(lock.handle!=INVALID_HANDLE_VALUE);
        QVERIFY(!model->finishSetup(1,6));QVERIFY(model->setupRequired());
        QCOMPARE(bytes(ini),before);QVERIFY(!QFile::exists(config));
        lock.release();QVERIFY(model->finishSetup(1,6));QVERIFY(!model->setupRequired());
        QCOMPARE(settings->value("unrelated").toString(),QStringLiteral("preserved"));
#else
        QSKIP("Windows file-sharing check");
#endif
    }
    void failedNativeAppearanceSaveDoesNotComplete()
    {
        QVERIFY(QDir().mkpath(config));
        QVERIFY(!model->finishSetup(1,6));QVERIFY(model->setupRequired());
        QVERIFY(!settings->contains(GeneralPreferencesModel::setupVersionKey()));
        QCOMPARE(model->values().value("theme").toInt(),1);
        QVERIFY(model->values().value("accentIndex").toInt()!=6);
        QVERIFY(QDir().rmdir(config));QVERIFY(model->finishSetup(1,6));QVERIFY(!model->setupRequired());
    }
    void failedCompletionMarkerRetainsSavedAppearanceAndRetries()
    {
#ifdef _WIN32
        std::unique_ptr<OwnedReadLock> lock;
        const auto connection=connect(model.get(),&GeneralPreferencesModel::valuesChanged,this,[&]{
            if(!lock && model->values().value("theme").toInt()==1 && model->values().value("accentIndex").toInt()==6)
                lock=std::make_unique<OwnedReadLock>(ini);
        });
        QVERIFY(!model->finishSetup(1,6));
        QVERIFY(lock && lock->handle!=INVALID_HANDLE_VALUE);disconnect(connection);
        QVERIFY(model->setupRequired());QVERIFY(model->lastError().contains("appearance choices were saved"));
        QCOMPARE(model->values().value("theme").toInt(),1);QCOMPARE(model->values().value("accentIndex").toInt(),6);
        {QSettings readback(ini,QSettings::IniFormat);readback.setFallbacksEnabled(false);
            QVERIFY(!readback.contains(GeneralPreferencesModel::setupVersionKey()));}
        lock->release();QVERIFY(model->finishSetup(1,6));QVERIFY(!model->setupRequired());
#else
        QSKIP("Windows file-sharing check");
#endif
    }
    void missingDependencyFailsWithoutWriting()
    {
        scheme.reset();model->reload();QVERIFY(!model->setupReady());
        QVERIFY(!model->finishSetup(1,6));QVERIFY(!model->lastError().isEmpty());
        QVERIFY(!QFile::exists(config));QVERIFY(!QFile::exists(ini));
    }
};
QTEST_GUILESS_MAIN(TestSetupPreferences)
#include "test_setup_preferences.moc"
