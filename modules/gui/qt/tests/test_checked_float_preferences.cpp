// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include "util/checked_preferences_store.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QTemporaryDir>
#include <QTest>
#include <limits>
extern "C" vlc_object_t *vlc_test_general_instance_object(libvlc_instance_t *);

class TestCheckedFloatPreferences final : public QObject
{
    Q_OBJECT
    QTemporaryDir m_temporary;
    QString m_output,m_config;
    libvlc_instance_t *m_instance=nullptr;
    qt_intf_t *m_intf=nullptr;
    static QByteArray bytes(const QString &path) { QFile file(path); return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray{}; }
    static bool write(const QString &path,const QByteArray &bytes) { QFile file(path); return file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(); }
    bool openNative() {
        const QByteArray argument=QByteArray("--config=")+m_config.toUtf8();
        const char *args[]={"--no-ignore-config","--no-media-library","--quiet",argument.constData()};
        m_instance=libvlc_new(ARRAY_SIZE(args),args);
        if(!m_instance)return false;
        m_intf=vlc_object_create<qt_intf_t>(vlc_test_general_instance_object(m_instance));
        return m_intf;
    }
    void closeNative() {
        if(m_intf){vlc_object_delete(m_intf);m_intf=nullptr;}
        if(m_instance){libvlc_release(m_instance);m_instance=nullptr;}
    }
private slots:
    void initTestCase() {
        QVERIFY(m_temporary.isValid());m_output=qEnvironmentVariable("VLC_FLOAT_STORE_OUTPUT");
        if(m_output.isEmpty())m_output=m_temporary.path();
        else {QVERIFY(QFileInfo(m_output).isAbsolute());QVERIFY(!QFileInfo::exists(m_output));QVERIFY(QDir().mkpath(m_output));}
        const auto userdata=QDir(m_output).filePath(QStringLiteral("userdata"));QVERIFY(QDir().mkpath(userdata));
        qputenv("VLC_USERDATA_PATH",userdata.toUtf8());
    }
    void init() {
        const auto path=QDir(m_output).filePath(QString::fromLatin1(QTest::currentTestFunction()));
        QVERIFY(QDir().mkpath(path));m_config=QDir(path).filePath(QStringLiteral("vlcrc"));
        QVERIFY(openNative());QCOMPARE(config_GetType("gain"),VLC_VAR_FLOAT);
        QCOMPARE(config_GetType("audio"),VLC_VAR_BOOL);QCOMPARE(config_GetType("qt-max-volume"),VLC_VAR_INTEGER);
    }
    void cleanup() {closeNative();QLocale::setDefault(QLocale::c());}
    void mixedMapKeepsExistingBoolIntStringContract() {
        QString error;
        QVERIFY2(CheckedPreferencesStore(m_intf).saveNative({{QStringLiteral("gain"),1.25},
            {QStringLiteral("audio"),false},{QStringLiteral("qt-max-volume"),QVariant::fromValue(137.f)},
            {QStringLiteral("audio-filter"),QStringLiteral("normvol")}},error),qPrintable(error));
        QCOMPARE(config_GetFloat("gain"),1.25f);QCOMPARE(config_GetInt("audio"),int64_t(0));
        QCOMPARE(config_GetInt("qt-max-volume"),int64_t(137));
        QVERIFY(bytes(m_config).contains("gain=1.250000"));QVERIFY(bytes(m_config).contains("qt-max-volume=137\n"));
        QVERIFY(bytes(m_config).contains("audio-filter=normvol"));
    }
    void actualSerializerRoundTripAndLocale() {
        QLocale::setDefault(QLocale(QLocale::German,QLocale::Germany));QString error;
        QVERIFY2(CheckedPreferencesStore(m_intf).saveNative({{QStringLiteral("gain"),1.234567}},error),qPrintable(error));
        QCOMPARE(config_GetFloat("gain"),float(1.234567));QVERIFY(bytes(m_config).contains("gain=1.234567"));
        QVERIFY(!bytes(m_config).contains("gain=1,234567"));
        QVERIFY2(CheckedPreferencesStore(m_intf).saveNative({{QStringLiteral("gain"),1.0}},error),qPrintable(error));
        QVERIFY(bytes(m_config).contains("#gain=1.000000"));
    }
    void nonrepresentableAndInvalidTypesRefuseBeforeAnyMutation() {
        QString error;QVERIFY(CheckedPreferencesStore(m_intf).saveNative({{QStringLiteral("gain"),1.25}},error));
        const auto disk=bytes(m_config);const float old=config_GetFloat("gain");const auto audio=config_GetInt("audio");
        const QVariant invalid[]={1.2345678,0.0000001,std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::infinity(),QStringLiteral("0.5"),true,1};
        for(const auto &value:invalid) {
            QVERIFY(!CheckedPreferencesStore(m_intf).saveNative({{QStringLiteral("audio"),!bool(audio)},
                {QStringLiteral("gain"),value}},error));
            QVERIFY(!error.isEmpty());QCOMPARE(config_GetFloat("gain"),old);QCOMPARE(config_GetInt("audio"),audio);QCOMPARE(bytes(m_config),disk);
        }
    }
    void nativeRangeClampRollsBackEntireMixedMap() {
        QString error;QVERIFY(CheckedPreferencesStore(m_intf).saveNative({{QStringLiteral("gain"),1.25},
            {QStringLiteral("audio-filter"),QStringLiteral("normvol")}},error));
        const auto disk=bytes(m_config);const auto audio=config_GetInt("audio");
        QVERIFY(!CheckedPreferencesStore(m_intf).saveNative({{QStringLiteral("gain"),9.0},
            {QStringLiteral("audio"),!bool(audio)},{QStringLiteral("audio-filter"),QStringLiteral("equalizer")}},error));
        QCOMPARE(config_GetFloat("gain"),1.25f);QCOMPARE(config_GetInt("audio"),audio);QCOMPARE(bytes(m_config),disk);
        char *filter=config_GetPsz("audio-filter");QCOMPARE(QByteArray(filter),QByteArray("normvol"));free(filter);
    }
    void physicalSaveFailureRollsBackAndRecovers() {
        QString error;QVERIFY(CheckedPreferencesStore(m_intf).saveNative({{QStringLiteral("gain"),1.25}},error));
        const auto disk=bytes(m_config);const auto audio=config_GetInt("audio");
        QVERIFY(QFile::remove(m_config));QVERIFY(QDir().mkdir(m_config));
        QVERIFY(!CheckedPreferencesStore(m_intf).saveNative({{QStringLiteral("gain"),0.5},
            {QStringLiteral("audio"),!bool(audio)}},error));
        QVERIFY(!error.isEmpty());QCOMPARE(config_GetFloat("gain"),1.25f);QCOMPARE(config_GetInt("audio"),audio);
        QVERIFY(QFileInfo(m_config).isDir());QVERIFY(QDir().rmdir(m_config));QVERIFY(write(m_config,disk));
        QVERIFY2(CheckedPreferencesStore(m_intf).saveNative({{QStringLiteral("gain"),0.5}},error),qPrintable(error));
        QCOMPARE(config_GetFloat("gain"),0.5f);QVERIFY(bytes(m_config).contains("gain=0.500000"));
    }
    void actualFreshNativeInstanceReadsTheSavedFloat() {
        QString error;QVERIFY(CheckedPreferencesStore(m_intf).saveNative({{QStringLiteral("gain"),1.234567},
            {QStringLiteral("audio"),false},{QStringLiteral("qt-max-volume"),133}},error));
        closeNative();QVERIFY(openNative());QCOMPARE(config_GetFloat("gain"),float(1.234567));
        QCOMPARE(config_GetInt("audio"),int64_t(0));QCOMPARE(config_GetInt("qt-max-volume"),int64_t(133));
    }
    void unsupportedKeyDoesNotPartiallyCommitValidFloat() {
        const auto old=config_GetFloat("gain");const auto disk=bytes(m_config);QString error;
        QVERIFY(!CheckedPreferencesStore(m_intf).saveNative({{QStringLiteral("gain"),0.5},
            {QStringLiteral("not-a-real-audio-default"),1.0}},error));
        QVERIFY(!error.isEmpty());QCOMPARE(config_GetFloat("gain"),old);QCOMPARE(bytes(m_config),disk);
    }
};
QTEST_GUILESS_MAIN(TestCheckedFloatPreferences)
#include "test_checked_float_preferences.moc"
