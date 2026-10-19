// SPDX-License-Identifier: GPL-2.0-or-later
#include "maininterface/player_profiles_store.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <limits>
#ifdef _WIN32
#include <windows.h>
#endif

namespace {
QVariantMap snapshot(int repeat = 0)
{
    return {{QStringLiteral("playback"), QVariantMap{{QStringLiteral("repeatMode"), repeat}}},
        {QStringLiteral("audio"), QVariantMap{{QStringLiteral("preferredLanguage"), QStringLiteral("fi")}}},
        {QStringLiteral("video"), QVariantMap{{QStringLiteral("enableVideo"), true}}},
        {QStringLiteral("subtitles"), QVariantMap{{QStringLiteral("fontFamily"), QStringLiteral("Inter 日本語")}}},
        {QStringLiteral("layout"), QVariantMap{{QStringLiteral("video"), QVariantList{QVariantList{0, 3, 4}, QVariantList{}, QVariantList{33}}}}}};
}
QByteArray bytes(const QString &path) { QFile file(path); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{}; }
void saveText(QSettings &settings, const QString &text)
{ settings.setValue(PlayerProfilesStore::settingsKey(), text); settings.sync(); }
}

class TestPlayerProfilesStore final : public QObject
{
    Q_OBJECT
    QTemporaryDir temp;
    QString path;
private slots:
    void init()
    {
        QVERIFY(temp.isValid());
        path = temp.filePath(QString::fromLatin1(QTest::currentTestFunction()) + QStringLiteral(".ini"));
        QVERIFY(!QFile::exists(path));
    }
    void initialProfilesDoNotWrite()
    {
        QSettings settings(path, QSettings::IniFormat); PlayerProfilesStore store(&settings); QString error;
        QVERIFY(store.reload(snapshot(), error)); QVERIFY(store.readable());
        QCOMPARE(store.profiles().size(), 4); QCOMPARE(store.selectedId(), QStringLiteral("movies"));
        QCOMPARE(store.profiles()[2].name, QStringLiteral("Music"));
        QVERIFY(!store.pendingSwitch()); QVERIFY(!QFile::exists(path));
        QVERIFY(store.checkpoint(snapshot(), error)); QVERIFY(!QFile::exists(path));
    }
    void cloneAndCheckpointPreserveIndependentProfilesAndUnrelatedKeys()
    {
        QSettings settings(path, QSettings::IniFormat);
        settings.setValue(QStringLiteral("unrelated/sentinel"), QStringLiteral("Keep Ää 日本語")); settings.sync();
        PlayerProfilesStore store(&settings); QString error, id;
        QVERIFY(store.reload(snapshot(), error));
        QVERIFY2(store.create(QStringLiteral("Travel Ää 日本語"), snapshot(1), id, error), qPrintable(error));
        QVERIFY(!id.isEmpty()); QCOMPARE(store.selectedId(), id); QCOMPARE(store.profiles().size(), 5);
        QCOMPARE(store.profiles()[0].snapshot, snapshot(1)); QCOMPARE(store.profiles()[1].snapshot, snapshot());
        QVERIFY(store.checkpoint(snapshot(2), error));
        QCOMPARE(store.profiles()[4].snapshot, snapshot(2)); QCOMPARE(store.profiles()[0].snapshot, snapshot(1));
        QSettings fresh(path, QSettings::IniFormat); PlayerProfilesStore restored(&fresh);
        QVERIFY(restored.reload(snapshot(), error)); QCOMPARE(restored.selectedId(), id);
        QCOMPARE(restored.profiles()[4].snapshot, snapshot(2));
        QCOMPARE(fresh.value(QStringLiteral("unrelated/sentinel")).toString(), QStringLiteral("Keep Ää 日本語"));
    }
    void switchJournalKeepsPreviousSelectionUntilComplete()
    {
        QSettings settings(path, QSettings::IniFormat); PlayerProfilesStore store(&settings); QString error;
        QVERIFY(store.reload(snapshot(), error)); QVERIFY(store.beginSwitch(QStringLiteral("shows"), snapshot(1), error));
        QCOMPARE(store.selectedId(), QStringLiteral("movies")); QVERIFY(store.pendingSwitch());
        QCOMPARE(store.pendingSwitch()->before, snapshot(1)); QCOMPARE(store.pendingSwitch()->after, snapshot());
        const auto saved = bytes(path);
        QVERIFY(!store.finishSwitch(snapshot(2), error)); QVERIFY(!error.isEmpty()); QCOMPARE(bytes(path), saved);
        QVERIFY(store.finishSwitch(snapshot(), error)); QCOMPARE(store.selectedId(), QStringLiteral("shows"));
        QVERIFY(!store.pendingSwitch()); QCOMPARE(store.profiles()[0].snapshot, snapshot(1));
    }
    void incompleteRollbackCannotClearRecoveryRecord()
    {
        QSettings settings(path, QSettings::IniFormat); PlayerProfilesStore store(&settings); QString error;
        QVERIFY(store.reload(snapshot(), error)); QVERIFY(store.beginSwitch(QStringLiteral("music"), snapshot(1), error));
        const auto saved = bytes(path);
        QVERIFY(!store.cancelSwitch(snapshot(), error)); QCOMPARE(bytes(path), saved); QVERIFY(store.pendingSwitch());
        QVERIFY(store.cancelSwitch(snapshot(1), error)); QVERIFY(!store.pendingSwitch());
        QCOMPARE(store.selectedId(), QStringLiteral("movies")); QCOMPARE(store.profiles()[0].snapshot, snapshot(1));
    }
    void pendingChangeBlocksCompetingEdits()
    {
        QSettings settings(path, QSettings::IniFormat); PlayerProfilesStore store(&settings); QString error, id;
        QVERIFY(store.reload(snapshot(), error)); QVERIFY(store.beginSwitch(QStringLiteral("work"), snapshot(1), error));
        const auto saved = bytes(path);
        QVERIFY(!store.checkpoint(snapshot(2), error)); QVERIFY(!store.create(QStringLiteral("Another"), snapshot(), id, error));
        QVERIFY(id.isEmpty()); QVERIFY(!store.beginSwitch(QStringLiteral("music"), snapshot(), error));
        QCOMPARE(bytes(path), saved); QCOMPARE(store.selectedId(), QStringLiteral("movies"));
    }
    void invalidNamesDoNotPublishOrWrite()
    {
        QSettings settings(path, QSettings::IniFormat); PlayerProfilesStore store(&settings); QString error, id;
        QVERIFY(store.reload(snapshot(), error));
        for (const auto &name : {QString{}, QStringLiteral("  "), QStringLiteral("Bad\nName"), QString(129, QLatin1Char('a')),
                                QString(QChar(0xd800)), QStringLiteral("MOVIES")}) {
            QVERIFY(!store.create(name, snapshot(), id, error)); QVERIFY(!error.isEmpty()); QVERIFY(id.isEmpty());
            QCOMPARE(store.profiles().size(), 4); QVERIFY(!QFile::exists(path));
        }
    }
    void snapshotsExcludeGlobalGroupsAndUnsupportedValues()
    {
        QSettings settings(path, QSettings::IniFormat); PlayerProfilesStore store(&settings); QString error;
        QVERIFY(store.reload(snapshot(), error));
        auto invalid = snapshot(); invalid.insert(QStringLiteral("privacy"), QVariantMap{{QStringLiteral("metadata"), true}});
        QVERIFY(!store.checkpoint(invalid, error));
        invalid = snapshot(); invalid.remove(QStringLiteral("audio")); QVERIFY(!store.checkpoint(invalid, error));
        invalid = snapshot(); invalid[QStringLiteral("audio")] = QVariantMap{{QStringLiteral("opaque"), QUrl(QStringLiteral("file:///owned"))}};
        QVERIFY(!store.checkpoint(invalid, error)); QVERIFY(!QFile::exists(path));
    }
    void nonfiniteAndUnboundedSnapshotsAreRejected()
    {
        QString error;
        for (const auto number : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
            auto invalid = snapshot(); invalid[QStringLiteral("audio")] = QVariantMap{{QStringLiteral("gain"), number}};
            QVERIFY(!PlayerProfilesStore::validSnapshot(invalid, error));
        }
        auto invalid = snapshot(); QVariantList tooMany; for (int i = 0; i < 257; ++i) tooMany.push_back(i);
        invalid[QStringLiteral("layout")] = QVariantMap{{QStringLiteral("items"), tooMany}};
        QVERIFY(!PlayerProfilesStore::validSnapshot(invalid, error));
    }
    void malformedSavedDataIsPreserved()
    {
        QSettings settings(path, QSettings::IniFormat); saveText(settings, QStringLiteral("{bad"));
        const auto saved = bytes(path); PlayerProfilesStore store(&settings); QString error, id;
        QVERIFY(!store.reload(snapshot(), error)); QVERIFY(!store.readable()); QVERIFY(!error.isEmpty());
        QVERIFY(!store.create(QStringLiteral("New"), snapshot(), id, error)); QCOMPARE(bytes(path), saved);
    }
    void unknownVersionAndMissingSelectionArePreserved()
    {
        QSettings settings(path, QSettings::IniFormat); PlayerProfilesStore store(&settings); QString error, id;
        QVERIFY(store.reload(snapshot(), error)); QVERIFY(store.create(QStringLiteral("Owned"), snapshot(), id, error));
        const auto good = settings.value(PlayerProfilesStore::settingsKey()).toString();
        for (const auto &field : {QStringLiteral("version"), QStringLiteral("selected")}) {
            auto document = QJsonDocument::fromJson(good.toUtf8()).object();
            document.insert(field, field == QLatin1String("version") ? QJsonValue(2) : QJsonValue(QStringLiteral("missing")));
            saveText(settings, QString::fromUtf8(QJsonDocument(document).toJson(QJsonDocument::Compact)));
            const auto saved = bytes(path);
            QVERIFY(!store.reload(snapshot(), error)); QCOMPARE(bytes(path), saved);
        }
    }
    void staleWindowCannotOverwriteAnotherWindow()
    {
        QSettings firstSettings(path, QSettings::IniFormat), secondSettings(path, QSettings::IniFormat);
        PlayerProfilesStore first(&firstSettings), second(&secondSettings); QString error, id;
        QVERIFY(first.reload(snapshot(), error)); QVERIFY(second.reload(snapshot(), error));
        QVERIFY(first.create(QStringLiteral("First"), snapshot(), id, error)); const auto saved = bytes(path);
        QVERIFY(!second.create(QStringLiteral("Second"), snapshot(), id, error)); QVERIFY(!error.isEmpty()); QCOMPARE(bytes(path), saved);
        QVERIFY(second.reload(snapshot(), error)); QVERIFY(second.create(QStringLiteral("Second"), snapshot(), id, error));
        QCOMPARE(second.profiles().size(), 6);
        QVERIFY(!first.checkpoint(snapshot(1), error));
    }
    void concurrentSaveLockCanRetry()
    {
        QSettings settings(path, QSettings::IniFormat); PlayerProfilesStore store(&settings); QString error, id;
        QVERIFY(store.reload(snapshot(), error)); QLockFile lock(path + QStringLiteral(".player-profiles.lock"));
        QVERIFY(lock.tryLock(0)); QVERIFY(!store.create(QStringLiteral("Locked"), snapshot(), id, error));
        QVERIFY(!error.isEmpty()); QCOMPARE(store.profiles().size(), 4); lock.unlock();
        QVERIFY2(store.create(QStringLiteral("Locked"), snapshot(), id, error), qPrintable(error));
    }
    void failedDiskSavePreservesAcceptedStateAndRetries()
    {
#ifdef _WIN32
        QSettings settings(path, QSettings::IniFormat); settings.setValue(QStringLiteral("sentinel"), QStringLiteral("Keep")); settings.sync();
        PlayerProfilesStore store(&settings); QString error, id; QVERIFY(store.reload(snapshot(), error)); const auto saved = bytes(path);
        HANDLE handle = CreateFileW(path.toStdWString().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        QVERIFY(handle != INVALID_HANDLE_VALUE);
        const auto accepted = store.create(QStringLiteral("Retry"), snapshot(), id, error);
        CloseHandle(handle);
        QVERIFY(!accepted); QVERIFY(!error.isEmpty()); QVERIFY(id.isEmpty()); QCOMPARE(store.profiles().size(), 4);
        QCOMPARE(bytes(path), saved); QVERIFY2(store.create(QStringLiteral("Retry"), snapshot(), id, error), qPrintable(error));
#else
        QFAIL("This case requires the Windows owned-file sharing restriction.");
#endif
    }
    void mismatchedRecoveryRecordIsPreserved()
    {
        QSettings settings(path, QSettings::IniFormat); PlayerProfilesStore store(&settings); QString error;
        QVERIFY(store.reload(snapshot(), error)); QVERIFY(store.beginSwitch(QStringLiteral("shows"), snapshot(1), error));
        auto document = QJsonDocument::fromJson(settings.value(PlayerProfilesStore::settingsKey()).toString().toUtf8()).object();
        auto pending = document.value(QStringLiteral("pending")).toObject();
        pending.insert(QStringLiteral("after"), QJsonObject::fromVariantMap(snapshot(2))); document.insert(QStringLiteral("pending"), pending);
        saveText(settings, QString::fromUtf8(QJsonDocument(document).toJson(QJsonDocument::Compact)));
        const auto saved = bytes(path); QVERIFY(!store.reload(snapshot(), error)); QCOMPARE(bytes(path), saved);
    }
    void profileCountLimitPreservesAllAcceptedProfiles()
    {
        QSettings settings(path, QSettings::IniFormat); PlayerProfilesStore store(&settings); QString error, id;
        QVERIFY(store.reload(snapshot(), error));
        for (int i = 4; i < PlayerProfilesStore::maximumProfiles; ++i)
            QVERIFY2(store.create(QStringLiteral("Owned %1").arg(i), snapshot(), id, error), qPrintable(error));
        const auto saved = bytes(path); QVERIFY(!store.create(QStringLiteral("Overflow"), snapshot(), id, error));
        QCOMPARE(store.profiles().size(), PlayerProfilesStore::maximumProfiles); QCOMPARE(bytes(path), saved);
    }
    void pendingSwitchSurvivesColdProcessAndFinishes()
    {
        QSettings settings(path, QSettings::IniFormat); PlayerProfilesStore store(&settings); QString error;
        QVERIFY(store.reload(snapshot(), error)); QVERIFY(store.beginSwitch(QStringLiteral("shows"), snapshot(1), error));
        QProcess child; child.setProcessChannelMode(QProcess::SeparateChannels);
#ifdef _WIN32
        child.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *arguments) { arguments->flags |= CREATE_NO_WINDOW; });
#endif
        child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--owned-profile-restart"), path});
        QVERIFY(child.waitForStarted(10000)); QVERIFY(child.waitForFinished(20000));
        QCOMPARE(child.exitStatus(), QProcess::NormalExit); QCOMPARE(child.exitCode(), 0);
        QVERIFY(store.reload(snapshot(), error)); QCOMPARE(store.selectedId(), QStringLiteral("shows")); QVERIFY(!store.pendingSwitch());
        QCOMPARE(store.profiles()[0].snapshot, snapshot(1));
    }
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().size() == 3 && app.arguments()[1] == QStringLiteral("--owned-profile-restart")) {
        QSettings settings(app.arguments()[2], QSettings::IniFormat); PlayerProfilesStore store(&settings); QString error;
        if (!store.reload(snapshot(), error) || !store.pendingSwitch() || store.selectedId() != QStringLiteral("movies")
                || store.pendingSwitch()->before != snapshot(1) || store.pendingSwitch()->after != snapshot()) return 2;
        return store.finishSwitch(snapshot(), error) ? 0 : 3;
    }
    TestPlayerProfilesStore test;
    return QTest::qExec(&test, argc, argv);
}
#include "test_player_profiles_store.moc"
