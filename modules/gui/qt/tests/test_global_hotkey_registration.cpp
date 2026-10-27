// SPDX-License-Identifier: GPL-2.0-or-later
// Real Windows registration/lifetime checks; no synthetic key delivery or GUI.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc/vlc.h>
#include <vlc_interface.h>
#include <vlc_actions.h>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include <vector>
#include <windows.h>
extern "C" vlc_object_t *vlc_test_library_object(libvlc_instance_t *);

class TestGlobalHotkeyRegistration final : public QObject
{
    Q_OBJECT
    struct Engine { libvlc_instance_t *instance; vlc_object_t *root; };
    QTemporaryDir m_temp;
    std::vector<Engine> m_engines;
    int open(const QByteArray &play = {}, const QByteArray &stop = {})
    {
        const QList<QByteArray> args = {"--ignore-config", "--quiet", "--no-media-library",
            "--no-metadata-network-access", "--no-auto-preparse", "--extraintf=", "--control=",
            "--aout=dummy", "--vout=dummy", "--global-key-play-pause=" + play,
            "--global-key-stop=" + stop};
        QList<const char *> argv;
        for (const auto &arg : args) argv.append(arg.constData());
        auto *instance = libvlc_new(argv.size(), argv.data());
        if (!instance) return -1;
        auto *root = vlc_test_library_object(instance);
        const int index = int(m_engines.size());
        m_engines.push_back({instance, root});
        if (intf_Create(vlc_object_instance(root), "globalhotkeys") != VLC_SUCCESS) return -1;
        return index;
    }
    QJsonObject report(int index) const
    {
        if (index < 0 || var_Type(m_engines[index].root, "global-hotkeys-status") != VLC_VAR_STRING) return {};
        char *raw = var_GetString(m_engines[index].root, "global-hotkeys-status");
        const auto result = QJsonDocument::fromJson(raw ? QByteArray(raw) : QByteArray{}).object();
        free(raw); return result;
    }
    QString state(int index) const { return report(index).value("state").toString(); }
    void close(int index)
    {
        if (!m_engines[index].instance) return;
        libvlc_release(m_engines[index].instance);
        m_engines[index] = {};
    }
    void closeAll()
    {
        for (size_t i = 0; i < m_engines.size(); ++i) close(int(i));
        m_engines.clear();
    }
private slots:
    void initTestCase()
    {
        QVERIFY(m_temp.isValid());
        qputenv("VLC_USERDATA_PATH", m_temp.path().toUtf8());
        SetEnvironmentVariableW(L"VLC_USERDATA_PATH", reinterpret_cast<LPCWSTR>(m_temp.path().utf16()));
        const auto probe = open(); QVERIFY(probe >= 0);
        QTRY_COMPARE_WITH_TIMEOUT(report(probe).value("version").toInt(), 1, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(state(probe), QStringLiteral("inactive"), 3000);
        closeAll();
    }
    void cleanup() { closeAll(); }
    void noBindingsAreInactive()
    {
        const auto player = open(); QVERIFY(player >= 0);
        QTRY_COMPARE_WITH_TIMEOUT(state(player), QStringLiteral("inactive"), 3000);
        QCOMPARE(report(player).value("registered").toInt(), 0);
        QCOMPARE(report(player).value("failed").toInt(), 0);
        QVERIFY(report(player).value("errors").toArray().isEmpty());
    }
    void collisionsAndReleaseStayPerPlayer()
    {
        const auto first = open("Ctrl+Alt+Shift+F8\tCtrl+Alt+Shift+F9"); QVERIFY(first >= 0);
        QTRY_COMPARE_WITH_TIMEOUT(state(first), QStringLiteral("active"), 3000);
        QCOMPARE(report(first).value("registered").toInt(), 2);
        const auto second = open("Ctrl+Alt+Shift+F8", "Ctrl+Alt+Shift+F10"); QVERIFY(second >= 0);
        QTRY_COMPARE_WITH_TIMEOUT(state(second), QStringLiteral("partial"), 3000);
        QCOMPARE(report(second).value("registered").toInt(), 1);
        QCOMPARE(report(second).value("failed").toInt(), 1);
        const auto error = report(second).value("errors").toArray().first().toObject();
        QCOMPARE(error.value("code").toInt(), int(ERROR_HOTKEY_ALREADY_REGISTERED));
        QCOMPARE(error.value("key").toInteger(), qint64(vlc_str2keycode("Ctrl+Alt+Shift+F8")));
        QCOMPARE(error.value("action").toInt(), int(ACTIONID_PLAY_PAUSE));
        QVERIFY(!error.value("unsupported").toBool());
        QCOMPARE(report(first).value("registered").toInt(), 2);
        close(first);
        // An initially rejected mapping does not silently become active later.
        QCOMPARE(state(second), QStringLiteral("partial"));
        const auto reopened = open("Ctrl+Alt+Shift+F8\tCtrl+Alt+Shift+F9"); QVERIFY(reopened >= 0);
        QTRY_COMPARE_WITH_TIMEOUT(state(reopened), QStringLiteral("active"), 3000);
        QCOMPARE(report(reopened).value("registered").toInt(), 2);
    }
    void multipleBindingsBalanceOwnedAtoms()
    {
        if (GlobalFindAtomA("stop")) QSKIP("An existing global atom prevents an ownership-only assertion.");
        const auto player = open({}, "Ctrl+Alt+Shift+F8\tCtrl+Alt+Shift+F9"); QVERIFY(player >= 0);
        QTRY_COMPARE_WITH_TIMEOUT(state(player), QStringLiteral("active"), 3000);
        QVERIFY(GlobalFindAtomA("stop"));
        close(player);
        QCOMPARE(GlobalFindAtomA("stop"), ATOM(0));
    }
    void unsupportedBindingsAreExplicitAndReportIsBounded()
    {
        const auto reserved = open("Ctrl+Alt+F12"); QVERIFY(reserved >= 0);
        QTRY_COMPARE_WITH_TIMEOUT(state(reserved), QStringLiteral("failed"), 3000);
        QCOMPARE(report(reserved).value("registered").toInt(), 0);
        QCOMPARE(report(reserved).value("failed").toInt(), 1);
        QVERIFY(report(reserved).value("errors").toArray().first().toObject().value("unsupported").toBool());
        close(reserved);
        QByteArray keys;
        for (char letter = 'A'; letter <= 'Z'; ++letter) {
            if (!keys.isEmpty()) keys += '\t';
            keys += "Meta+"; keys += letter;
        }
        for (int i = 1; i <= 11; ++i) keys += "\tMeta+F" + QByteArray::number(i);
        const auto player = open(keys); QVERIFY(player >= 0);
        QTRY_COMPARE_WITH_TIMEOUT(state(player), QStringLiteral("failed"), 3000);
        QCOMPARE(report(player).value("registered").toInt(), 0);
        QCOMPARE(report(player).value("failed").toInt(), 37);
        QCOMPARE(report(player).value("errors").toArray().size(), 32);
        QCOMPARE(report(player).value("omitted").toInt(), 5);
        for (const auto &value : report(player).value("errors").toArray())
            QVERIFY(value.toObject().value("unsupported").toBool());
    }
public:
    ~TestGlobalHotkeyRegistration() override { closeAll(); }
};
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    TestGlobalHotkeyRegistration test;
    return QTest::qExec(&test, argc, argv);
}
#include "test_global_hotkey_registration.moc"
