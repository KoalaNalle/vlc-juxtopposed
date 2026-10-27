// SPDX-License-Identifier: GPL-2.0-or-later
#include "maininterface/sidebar_pins_store.hpp"
#include <QtTest>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSet>
#include <QSettings>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdlib.h>

namespace {
SidebarPin uriPin(const QString &kind, const QUrl &url, const QString &title = {})
{
    SidebarPin pin; pin.kind = kind; pin.mrl = url; pin.title = title; return pin;
}
SidebarPin collectionPin(const QString &kind, const QString &id, const QString &scope)
{
    SidebarPin pin; pin.kind = kind; pin.nativeId = id; pin.nativeScope = scope;
    pin.title = QStringLiteral("Owned collection"); return pin;
}
QByteArray contents(const QString &path)
{
    QFile file(path); if (!file.open(QIODevice::ReadOnly)) return {}; return file.readAll();
}
QStringList keys(const SidebarPinsStore &store)
{
    QStringList result; for (const auto &pin : store.pins()) result.push_back(pin.key); return result;
}
#ifdef _WIN32
class HeldFile
{
public:
    explicit HeldFile(const QString &path)
        : handle(CreateFileW(path.toStdWString().c_str(), GENERIC_READ, 0, nullptr,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)) {}
    ~HeldFile() { release(); }
    bool valid() const { return handle != INVALID_HANDLE_VALUE; }
    void release() { if (valid()) { CloseHandle(handle); handle = INVALID_HANDLE_VALUE; } }
private:
    HANDLE handle;
};
#endif
}

class TestSidebarPinsStore : public QObject
{
    Q_OBJECT
    QString m_root;
    int m_serial = 0;
    QString file()
    {
        const auto directory = QDir(m_root).filePath(QString::number(++m_serial));
        const auto path = QDir(directory).filePath(QStringLiteral("pins.ini"));
        if (QFileInfo::exists(path) || !QDir().mkpath(directory))
            throw std::runtime_error("test requires a fresh owned settings path");
        return path;
    }
    QUrl owned(const QString &name) const { return QUrl::fromLocalFile(QDir(m_root).filePath(name)); }
private slots:
    void initTestCase()
    {
        const auto output = qEnvironmentVariable("VLC_SIDEBAR_PINS_TEST_OUTPUT");
        QVERIFY(!output.isEmpty() && QDir::isAbsolutePath(output));
        QVERIFY(QDir().mkpath(output)); m_root = QDir(output).canonicalPath();
        QVERIFY(!m_root.isEmpty());
    }
    void mixedKindsPersistOrderAndExactLargeIds()
    {
        const auto path = file(); QSettings settings(path, QSettings::IniFormat);
        settings.setValue(QStringLiteral("unrelated/sentinel"), QStringList{QStringLiteral("Keep Ää"), QStringLiteral("日本語")}); settings.sync();
        SidebarPinsStore store(&settings); QString error;
        const QVector<SidebarPin> pins{
            uriPin(QStringLiteral("media"), owned(QStringLiteral("Audio Ää.flac")), QStringLiteral("Track 日本語")),
            uriPin(QStringLiteral("folder"), owned(QStringLiteral("Folder Ää/"))),
            collectionPin(QStringLiteral("playlist"), QStringLiteral("9007199254740993"), QStringLiteral("owned-db")),
            collectionPin(QStringLiteral("artist"), QStringLiteral("9007199254740993"), QStringLiteral("owned-db")),
            uriPin(QStringLiteral("source"), QUrl(QStringLiteral("https://example.invalid/list/?token=owned")))};
        QVERIFY2(store.add(pins, error), qPrintable(error)); QCOMPARE(store.pins().size(), 5);
        QVERIFY(store.move(store.pins()[4].key, 1, error)); const auto order = keys(store);
        QSettings fresh(path, QSettings::IniFormat); SidebarPinsStore restarted(&fresh);
        QVERIFY(restarted.readable()); QCOMPARE(keys(restarted), order);
        QCOMPARE(restarted.pins()[0].title, QStringLiteral("Track 日本語"));
        QCOMPARE(restarted.pins()[3].nativeId, QStringLiteral("9007199254740993"));
        QCOMPARE(restarted.pins()[1].mrl.query(), QStringLiteral("token=owned"));
        QCOMPARE(fresh.value(QStringLiteral("unrelated/sentinel")).toStringList(), (QStringList{QStringLiteral("Keep Ää"), QStringLiteral("日本語")}));
        const auto report = QFileInfo(path).dir().filePath(QStringLiteral("cold-read.json"));
        const auto saved = contents(path);
        QProcess child; child.start(QCoreApplication::applicationFilePath(),
            {QStringLiteral("--owned-cold-read"), path, report});
        if (!child.waitForStarted(10000)) { child.kill(); child.waitForFinished(2000); QFAIL("owned cold-reader did not start"); }
        if (!child.waitForFinished(10000)) { child.kill(); child.waitForFinished(2000); QFAIL("owned cold-reader timed out"); }
        QCOMPARE(child.exitStatus(), QProcess::NormalExit); QCOMPARE(child.exitCode(), 0);
        const auto cold = QJsonDocument::fromJson(contents(report)).array(); QCOMPARE(cold.size(), 5);
        for (qsizetype i = 0; i < cold.size(); ++i) QCOMPARE(cold[i].toObject().value(QStringLiteral("key")).toString(), order[i]);
        QCOMPARE(cold[3].toObject().value(QStringLiteral("nativeId")).toString(), QStringLiteral("9007199254740993"));
        QCOMPARE(contents(path), saved);
    }
    void duplicateAddressDoesNotWriteEvenWhenLocked()
    {
#ifndef _WIN32
        QSKIP("Windows file-sharing check");
#else
        const auto path = file(); QSettings settings(path, QSettings::IniFormat);
        SidebarPinsStore store(&settings); QString error;
        const auto a = uriPin(QStringLiteral("folder"), owned(QStringLiteral("Folder/")));
        QVERIFY(store.add({a}, error)); const auto before = contents(path); const auto order = keys(store);
        HeldFile lock(path); QVERIFY(lock.valid());
        QVERIFY2(store.add({uriPin(QStringLiteral("folder"), owned(QStringLiteral("Folder")))}, error), qPrintable(error));
        QCOMPARE(keys(store), order); lock.release(); QCOMPARE(contents(path), before);
#endif
    }
    void identicalLabelsKeepDistinctTargets()
    {
        QSettings settings(file(), QSettings::IniFormat); SidebarPinsStore store(&settings); QString error;
        QVERIFY(store.add({uriPin(QStringLiteral("media"), owned(QStringLiteral("one.flac")), QStringLiteral("Same")), uriPin(QStringLiteral("media"), owned(QStringLiteral("two.flac")), QStringLiteral("Same"))}, error));
        QCOMPARE(store.pins().size(), 2); QVERIFY(store.pins()[0].key != store.pins()[1].key);
    }
    void nativeNamespaceSeparatesCollectionKindsAndDatabases()
    {
        QSettings settings(file(), QSettings::IniFormat); SidebarPinsStore store(&settings); QString error;
        QVERIFY(store.add({collectionPin(QStringLiteral("playlist"), QStringLiteral("1"), QStringLiteral("db-a")), collectionPin(QStringLiteral("artist"), QStringLiteral("1"), QStringLiteral("db-a")), collectionPin(QStringLiteral("playlist"), QStringLiteral("1"), QStringLiteral("db-b"))}, error));
        const auto order = keys(store); QCOMPARE(QSet<QString>(order.cbegin(), order.cend()).size(), 3);
    }
    void invalidBatchDoesNotPartiallySave()
    {
        const auto path = file(); QSettings settings(path, QSettings::IniFormat); SidebarPinsStore store(&settings); QString error;
        QVERIFY(store.add({uriPin(QStringLiteral("media"), owned(QStringLiteral("first.flac")))}, error));
        const auto before = contents(path); const auto order = keys(store);
        QVERIFY(!store.add({uriPin(QStringLiteral("media"), owned(QStringLiteral("second.flac"))), uriPin(QStringLiteral("unknown"), owned(QStringLiteral("bad")))}, error));
        QVERIFY(!error.isEmpty()); QCOMPARE(keys(store), order); QCOMPARE(contents(path), before);
    }
    void failedMutationAndSameObjectRetry_data()
    {
        QTest::addColumn<QString>("action"); QTest::newRow("add") << QStringLiteral("add");
        QTest::newRow("move") << QStringLiteral("move"); QTest::newRow("remove") << QStringLiteral("remove");
    }
    void failedMutationAndSameObjectRetry()
    {
#ifndef _WIN32
        QSKIP("Windows file-sharing check");
#else
        QFETCH(QString, action); const auto path = file(); QSettings settings(path, QSettings::IniFormat); SidebarPinsStore store(&settings); QString error;
        QVERIFY(store.add({uriPin(QStringLiteral("media"), owned(QStringLiteral("one.flac"))), uriPin(QStringLiteral("folder"), owned(QStringLiteral("Folder/")))}, error));
        const auto before = contents(path); const auto order = keys(store);
        auto mutate = [&] { if (action == QStringLiteral("add")) return store.add({uriPin(QStringLiteral("media"), owned(QStringLiteral("new.flac")))}, error); if (action == QStringLiteral("move")) return store.move(order.first(), 1, error); return store.remove(order.first(), error); };
        HeldFile lock(path); QVERIFY(lock.valid()); QVERIFY(!mutate()); QVERIFY(!error.isEmpty()); QCOMPARE(keys(store), order);
        lock.release(); QCOMPARE(contents(path), before); QVERIFY2(mutate(), qPrintable(error));
        QSettings fresh(path, QSettings::IniFormat); SidebarPinsStore restarted(&fresh); QCOMPARE(keys(restarted), keys(store));
        if (action == QStringLiteral("add")) QCOMPARE(store.pins().size(), 3);
        else if (action == QStringLiteral("move")) QCOMPARE(keys(store), (QStringList{order[1], order[0]}));
        else QCOMPARE(keys(store), QStringList{order[1]});
#endif
    }
    void unreadableSavedDataBlocksWritesAndCanReload_data()
    {
        QTest::addColumn<QString>("mode"); for (const auto *name : {"future-version", "wrong-list-type", "wrong-entry", "wrong-key", "malformed-json", "wrong-text-type"}) QTest::newRow(name) << QString::fromLatin1(name);
    }
    void unreadableSavedDataBlocksWritesAndCanReload()
    {
        QFETCH(QString, mode); const auto path = file(); QSettings settings(path, QSettings::IniFormat); SidebarPinsStore store(&settings); QString error;
        QVERIFY(store.add({uriPin(QStringLiteral("media"), owned(QStringLiteral("keep.flac")))}, error)); const auto order = keys(store);
        auto value = store.pins()[0].toVariant();
        QVariantMap envelope{{QStringLiteral("version"), 1}, {QStringLiteral("pins"), QVariantList{value}}};
        if (mode == QStringLiteral("future-version")) envelope[QStringLiteral("version")] = 2;
        else if (mode == QStringLiteral("wrong-list-type")) envelope[QStringLiteral("pins")] = QStringLiteral("not a list");
        else if (mode == QStringLiteral("wrong-text-type")) { value[QStringLiteral("title")] = 123; envelope[QStringLiteral("pins")] = QVariantList{value}; }
        else { value[mode == QStringLiteral("wrong-entry") ? QStringLiteral("kind") : QStringLiteral("key")] = QStringLiteral("invalid"); envelope[QStringLiteral("pins")] = QVariantList{value}; }
        settings.setValue(SidebarPinsStore::settingsKey(), mode == QStringLiteral("malformed-json") ? QStringLiteral("{broken") : QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(envelope)).toJson(QJsonDocument::Compact))); settings.sync(); const auto corrupt = contents(path);
        QVERIFY(!store.reload(error)); QVERIFY(!error.isEmpty()); QCOMPARE(keys(store), order);
        QVERIFY(!store.add({uriPin(QStringLiteral("media"), owned(QStringLiteral("new.flac")))}, error)); QCOMPARE(contents(path), corrupt);
        settings.setValue(SidebarPinsStore::settingsKey(), QStringLiteral("{\"version\":1,\"pins\":[]}")); settings.sync();
        QVERIFY2(store.reload(error), qPrintable(error)); QVERIFY(store.add({uriPin(QStringLiteral("media"), owned(QStringLiteral("new.flac")))}, error)); QCOMPARE(store.pins().size(), 1);
    }
    void missingStoreIsHandled()
    {
        SidebarPinsStore store(nullptr); QString error; QVERIFY(!store.readable()); QVERIFY(!store.add({uriPin(QStringLiteral("media"), owned(QStringLiteral("x")))}, error)); QVERIFY(!error.isEmpty());
    }
    void canonicalAddressesRetainDriveRootsAndNoPassword()
    {
        QSettings settings(file(), QSettings::IniFormat); SidebarPinsStore store(&settings); QString error;
        QVERIFY(store.add({uriPin(QStringLiteral("folder"), QUrl(QStringLiteral("file:///E:/"))), uriPin(QStringLiteral("source"), QUrl(QStringLiteral("ftp://owned:secret@example.invalid/folder/?token=owned")))}, error));
        QCOMPARE(store.pins()[0].mrl.toString(), QStringLiteral("file:///E:/")); QVERIFY(QDir::isAbsolutePath(store.pins()[0].mrl.toLocalFile()));
        QCOMPARE(store.pins()[1].mrl.password(), QString()); QCOMPARE(store.pins()[1].mrl.userName(), QStringLiteral("owned")); QCOMPARE(store.pins()[1].mrl.query(), QStringLiteral("token=owned"));
        QVERIFY(!contents(settings.fileName()).contains("secret"));
    }
    void relativeFileAddressIsRejected()
    {
        QSettings settings(file(), QSettings::IniFormat); SidebarPinsStore store(&settings); QString error;
        QVERIFY(!store.add({uriPin(QStringLiteral("media"), QUrl(QStringLiteral("file:../relative.mp4")))}, error)); QCOMPARE(store.pins().size(), 0);
    }
    void capacityFailureIsAtomic()
    {
        const auto path = file(); QSettings settings(path, QSettings::IniFormat); SidebarPinsStore store(&settings); QString error;
        const auto before = contents(path);
        QVector<SidebarPin> pins; for (int i = 0; i < 513; ++i) pins.push_back(uriPin(QStringLiteral("media"), owned(QString::number(i) + QStringLiteral(".flac"))));
        QVERIFY(!store.add(pins, error)); QCOMPARE(store.pins().size(), 0); QCOMPARE(contents(path), before);
    }
    void settingsGroupIsPreserved()
    {
        const auto path = file(); QSettings settings(path, QSettings::IniFormat); settings.beginGroup(QStringLiteral("Owned/group")); SidebarPinsStore store(&settings); QString error;
        QVERIFY(store.add({uriPin(QStringLiteral("media"), owned(QStringLiteral("one.flac")))}, error));
        QSettings fresh(path, QSettings::IniFormat); QVERIFY(!fresh.contains(SidebarPinsStore::settingsKey())); fresh.beginGroup(QStringLiteral("Owned/group")); SidebarPinsStore restarted(&fresh); QCOMPARE(restarted.pins().size(), 1);
    }
    void organizationFallbackPinsRemainReadAndWriteConsistent()
    {
        const auto root = QDir(m_root).filePath(QStringLiteral("fallback-scope"));
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, root + QStringLiteral("/user"));
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, root + QStringLiteral("/system"));
        QSettings defaults(QSettings::IniFormat, QSettings::SystemScope, QStringLiteral("OwnedPins"), QStringLiteral("App")); SidebarPinsStore defaultStore(&defaults); QString error;
        QVERIFY(defaultStore.add({uriPin(QStringLiteral("media"), owned(QStringLiteral("default.flac")))}, error));
        QSettings user(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("OwnedPins"), QStringLiteral("App")); SidebarPinsStore userStore(&user); QCOMPARE(userStore.pins().size(), 1);
        QVERIFY(userStore.add({uriPin(QStringLiteral("media"), owned(QStringLiteral("user.flac")))}, error));
        QSettings fresh(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("OwnedPins"), QStringLiteral("App")); SidebarPinsStore restarted(&fresh); QCOMPARE(restarted.pins().size(), 2);
        QSettings system(QSettings::IniFormat, QSettings::SystemScope, QStringLiteral("OwnedPins"), QStringLiteral("App")); SidebarPinsStore unchanged(&system); QCOMPARE(unchanged.pins().size(), 1);
    }
};

int main(int argc, char **argv)
{
#ifdef _WIN32
    _set_error_mode(_OUT_TO_STDERR); SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.size() == 4 && args[1] == QStringLiteral("--owned-cold-read"))
    {
        const auto ownedRoot = qEnvironmentVariable("VLC_SIDEBAR_PINS_TEST_OUTPUT");
        const auto root = QDir(ownedRoot).canonicalPath();
        const auto source = QFileInfo(args[2]).canonicalFilePath();
        const auto parent = QFileInfo(args[3]).dir().canonicalPath();
        const auto output = QDir(parent).filePath(QFileInfo(args[3]).fileName());
        if (ownedRoot.isEmpty() || root.isEmpty() || !source.startsWith(root + QLatin1Char('/'))
            || !parent.startsWith(root + QLatin1Char('/')) || QFileInfo::exists(output)) return 2;
        QSettings settings(source, QSettings::IniFormat); settings.setFallbacksEnabled(false);
        SidebarPinsStore reader(&settings); if (!reader.readable()) return 3;
        QJsonArray rows; for (const auto &pin : reader.pins()) rows.append(QJsonObject::fromVariantMap(pin.toVariant()));
        QFile report(output); if (!report.open(QIODevice::WriteOnly | QIODevice::NewOnly)) return 4;
        return report.write(QJsonDocument(rows).toJson()) > 0 ? 0 : 5;
    }
    TestSidebarPinsStore test; return QTest::qExec(&test, argc, argv);
}
#include "test_sidebar_pins_store.moc"
