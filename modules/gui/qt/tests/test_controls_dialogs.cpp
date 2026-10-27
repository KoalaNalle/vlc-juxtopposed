// SPDX-License-Identifier: GPL-2.0-or-later
// Owned QWidget integration tests. These do not automate another application.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include "dialogs/preferences/controls_shortcuts.hpp"
#include "dialogs/preferences/preferences_widgets.hpp"
#include "maininterface/controls_preferences_model.hpp"
#include "util/modal_input_guard.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>
#include <QWindow>
#include <QtPlugin>
#include <memory>
#ifdef _WIN32
# include <windows.h>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif
extern "C" vlc_object_t *vlc_test_library_object(libvlc_instance_t *);

class TestControlsDialogs final : public QObject
{
    Q_OBJECT
    QTemporaryDir temp;
    libvlc_instance_t *instance = nullptr;
    qt_intf_t *intf = nullptr;
    std::unique_ptr<ControlsPreferencesModel> model;
    std::unique_ptr<ModalInputGuard> inputGuard;
    std::unique_ptr<QWindow> owner;
    QPointer<QDialog> dialog;
    QString config;
    bool blocked() const { return var_GetBool(vlc_object_instance(intf), "intf-input-blocked"); }
    static QByteArray bytes(const QString &path)
    { QFile file(path); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{}; }
    void click(QDialogButtonBox::StandardButton button)
    { dialog->findChild<QDialogButtonBox *>()->button(button)->click(); }
    KeyTableItem *action(const char *name)
    {
        auto *table = dialog->findChild<QTreeWidget *>();
        if (!table) return nullptr;
        for (int i = 0; i < table->topLevelItemCount(); ++i) {
            auto *item = static_cast<KeyTableItem *>(table->topLevelItem(i));
            if (!strcmp(item->normal.config_name, name)) return item;
        }
        return nullptr;
    }
    void openShortcuts()
    { QVERIFY(model->editShortcuts()); QVERIFY(dialog); QVERIFY(model->busy()); QVERIFY(blocked()); }
    void openRemote()
    { QVERIFY(model->setValue(QStringLiteral("webRemoteEnabled"), true)); QVERIFY(dialog); QVERIFY(model->busy()); QVERIFY(blocked()); }
private slots:
    void init()
    {
        QVERIFY(temp.isValid());
        const auto dir = QDir(temp.path()).filePath(QString::fromLatin1(QTest::currentTestFunction()));
        QVERIFY(QDir().mkpath(dir)); config = QDir(dir).filePath(QStringLiteral("vlcrc"));
        const auto userdata = QDir(dir).filePath(QStringLiteral("userdata")); QVERIFY(QDir().mkpath(userdata));
        qputenv("VLC_USERDATA_PATH", userdata.toUtf8());
#ifdef _WIN32
        SetEnvironmentVariableW(L"VLC_USERDATA_PATH", reinterpret_cast<LPCWSTR>(userdata.utf16()));
#endif
        const QList<QByteArray> args = {QByteArray("--config=") + config.toUtf8(), "--no-ignore-config", "--quiet",
            "--no-media-library", "--no-metadata-network-access", "--no-auto-preparse", "--extraintf=", "--control=",
            "--aout=dummy", "--vout=dummy"};
        QList<const char *> argv; for (const auto &arg : args) argv.append(arg.constData());
        instance = libvlc_new(argv.size(), argv.data()); QVERIFY(instance);
        intf = vlc_object_create<qt_intf_t>(vlc_test_library_object(instance)); QVERIFY(intf);
        inputGuard = std::make_unique<ModalInputGuard>(VLC_OBJECT(intf));
        owner = std::make_unique<QWindow>(); owner->resize(600, 400); owner->create();
        ControlsPreferencesModel::Consumers consumers;
        consumers.remotePolicy = true;
        consumers.openShortcutEditor = [this](ControlsPreferencesModel *model) { dialog = openControlsShortcutEditor(model, owner.get()); };
        consumers.openRemoteSetup = [this](ControlsPreferencesModel *model) { dialog = openControlsRemoteSetup(model, owner.get()); };
        model = std::make_unique<ControlsPreferencesModel>(intf, std::move(consumers));
    }
    void cleanup()
    {
        if (dialog) { dialog->reject(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }
        model.reset(); owner.reset();
        inputGuard.reset();
        if (intf) { vlc_object_delete(intf); intf = nullptr; }
        if (instance) { libvlc_release(instance); instance = nullptr; }
    }
    void completeSnapshotAndCancel()
    {
        openShortcuts();
        QCOMPARE(dialog->windowHandle()->transientParent(), owner.get());
        QCOMPARE(dialog->windowModality(), Qt::WindowModal);
        auto *selector = dialog->findChild<KeySelectorControl *>(); QVERIFY(selector);
        QCOMPARE(selector->requestedValues(), model->shortcutValues());
        auto *item = action("key-play-pause"); QVERIFY(item);
        item->set_keys(QStringLiteral("Ctrl+Alt+F12"), KeySelectorControl::HOTKEY_COL);
        click(QDialogButtonBox::Cancel);
        QTRY_VERIFY(!blocked());
        QVERIFY(!model->busy()); QVERIFY(!QFile::exists(config));
        QVERIFY(model->shortcutValues().value(QStringLiteral("key-play-pause")) != QStringLiteral("Ctrl+Alt+F12"));
    }
    void initialShortcutRowsAreReachable()
    {
        openShortcuts();
        auto *table = dialog->findChild<QTreeWidget *>(); QVERIFY(table);
        QVERIFY(table->topLevelItemCount() > 20);
        auto *first = table->topLevelItem(0); QVERIFY(first);
        QTRY_VERIFY(table->isVisible());
        QTRY_VERIFY(table->visualItemRect(first).height() > 8);
        const auto rect = table->visualItemRect(first);
        QVERIFY(table->viewport()->rect().intersects(rect));
        QCOMPARE(table->itemAt(rect.center()), first);
        QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, rect.center());
        QCOMPARE(table->currentItem(), first);
        QCOMPARE(dialog->findChild<QLineEdit *>()->text(), QString{});
        click(QDialogButtonBox::Cancel);
        QTRY_VERIFY(!model->busy());
    }
    void scopedSaveAndReopen()
    {
        config_PutInt("short-jump-size", 23);
        openShortcuts(); auto *item = action("key-play-pause"); QVERIFY(item);
        item->set_keys(QStringLiteral("Ctrl+Alt+F12"), KeySelectorControl::HOTKEY_COL);
        click(QDialogButtonBox::Save);
        QVERIFY(!model->busy()); QVERIFY(model->lastError().isEmpty());
        QCOMPARE(model->shortcutValues().value(QStringLiteral("key-play-pause")).toString(), QStringLiteral("Ctrl+Alt+F12"));
        QCOMPARE(config_GetInt("short-jump-size"), int64_t(23)); QVERIFY(model->restartRequired());
        QVERIFY(bytes(config).contains("key-play-pause=Ctrl+Alt+F12"));
        openShortcuts(); QCOMPARE(action("key-play-pause")->normal.keys, QStringLiteral("Ctrl+Alt+F12"));
        click(QDialogButtonBox::Cancel);
    }
    void failedSaveKeepsDraftAndRetries()
    {
        openShortcuts(); auto *item = action("key-play-pause"); QVERIFY(item);
        const auto old = model->shortcutValues();
        item->set_keys(QStringLiteral("Ctrl+Alt+F12"), KeySelectorControl::HOTKEY_COL);
        QVERIFY(QDir().mkpath(config)); click(QDialogButtonBox::Save);
        QVERIFY(model->busy()); QVERIFY(dialog->isVisible()); QVERIFY(!model->lastError().isEmpty());
        QCOMPARE(model->shortcutValues(), old);
        QVERIFY(!dialog->findChild<QLabel *>(QStringLiteral("shortcutEditorStatus"))->text().isEmpty());
        QVERIFY(QDir().rmdir(config)); click(QDialogButtonBox::Save);
        QVERIFY(!model->busy()); QVERIFY(model->lastError().isEmpty()); QVERIFY(QFile::exists(config));
    }
    void conflictKeepsEditorOpen()
    {
        openShortcuts();
        auto *play = action("key-play-pause"), *stop = action("key-stop"); QVERIFY(play && stop);
        play->set_keys(QStringLiteral("Ctrl+Alt+F12"), KeySelectorControl::HOTKEY_COL);
        stop->set_keys(QStringLiteral("Ctrl+Alt+F12"), KeySelectorControl::HOTKEY_COL);
        click(QDialogButtonBox::Save);
        QVERIFY(model->busy()); QVERIFY(model->lastError().contains(QStringLiteral("conflict")));
        QVERIFY(!QFile::exists(config)); click(QDialogButtonBox::Cancel); QVERIFY(!model->busy());
    }
    void remoteMaskedCancelDoesNotEnable()
    {
        openRemote(); auto *password = dialog->findChild<QLineEdit *>(QStringLiteral("webRemotePassword")); QVERIFY(password);
        QCOMPARE(password->echoMode(), QLineEdit::Password);
        QCOMPARE(dialog->windowHandle()->transientParent(), owner.get());
        password->setText(QStringLiteral("Owned fixture password")); click(QDialogButtonBox::Cancel);
        QVERIFY(password->text().isEmpty()); QVERIFY(!model->busy());
        QVERIFY(!model->values().value(QStringLiteral("webRemoteEnabled")).toBool()); QVERIFY(!QFile::exists(config));
    }
    void remoteValidationFailureRetry()
    {
        openRemote(); auto *password = dialog->findChild<QLineEdit *>(QStringLiteral("webRemotePassword")); QVERIFY(password);
        password->setText(QStringLiteral("short")); click(QDialogButtonBox::Save);
        QVERIFY(model->busy()); QVERIFY(!QFile::exists(config));
        password->setText(QStringLiteral("Owned fixture password"));
        QVERIFY(QDir().mkpath(config)); click(QDialogButtonBox::Save);
        QVERIFY(model->busy()); QVERIFY(!model->values().value(QStringLiteral("webRemoteEnabled")).toBool());
        QVERIFY(QDir().rmdir(config)); click(QDialogButtonBox::Save);
        QVERIFY(!model->busy()); QVERIFY(password->text().isEmpty());
        QVERIFY(model->values().value(QStringLiteral("webRemoteEnabled")).toBool());
        QVERIFY(model->values().value(QStringLiteral("remotePasswordConfigured")).toBool());
        QVERIFY(!model->status().contains(QStringLiteral("Owned fixture password")));
    }
    void ownerDestructionClosesEditor()
    {
        openShortcuts(); owner.reset();
        QVERIFY(!model->busy()); QVERIFY(!QFile::exists(config));
    }
    void modelDestructionClosesRemote()
    {
        openRemote(); auto *password = dialog->findChild<QLineEdit *>(QStringLiteral("webRemotePassword")); QVERIFY(password);
        password->setText(QStringLiteral("Owned fixture password")); model.reset();
        QVERIFY(!dialog->isVisible()); QVERIFY(password->text().isEmpty()); QVERIFY(!QFile::exists(config));
    }
    void nativeAndQmlModalOwnershipCompose()
    {
        QVERIFY(!blocked()); inputGuard->setQmlVisible(true); QVERIFY(blocked());
        QDialog first, second;
        first.setWindowModality(Qt::ApplicationModal); second.setWindowModality(Qt::ApplicationModal);
        first.show(); second.show(); QVERIFY(blocked());
        inputGuard->setQmlVisible(false); QVERIFY(blocked());
        second.hide(); QVERIFY(blocked()); first.hide(); QTRY_VERIFY(!blocked());
        inputGuard->setQmlVisible(true); first.show(); first.hide(); QVERIFY(blocked());
        inputGuard->setQmlVisible(false); QTRY_VERIFY(!blocked());
    }
    void popupBlocksButNonmodalAndTooltipDoNot()
    {
        QWidget normal, tooltip;
        tooltip.setWindowFlags(Qt::ToolTip);
        normal.show(); tooltip.show(); QCoreApplication::processEvents(); QVERIFY(!blocked());
        QMenu menu; menu.addAction(QStringLiteral("Owned fixture action")); menu.popup(QPoint(20, 20));
        QVERIFY(blocked()); menu.hide(); QTRY_VERIFY(!blocked());
        tooltip.hide(); normal.hide(); QVERIFY(!blocked());
    }
    void destructionAndGuardTeardownClearBlock()
    {
        auto modal = std::make_unique<QDialog>(); modal->setWindowModality(Qt::ApplicationModal); modal->show();
        QVERIFY(blocked()); modal.reset(); QTRY_VERIFY(!blocked());
        QDialog second; second.setWindowModality(Qt::ApplicationModal); second.show(); QVERIFY(blocked());
        // Another consumer holds the shared variable while this UI owner exits.
        var_Create(vlc_object_instance(intf), "intf-input-blocked", VLC_VAR_BOOL);
        inputGuard.reset(); QVERIFY(!blocked()); second.hide(); QVERIFY(!blocked());
        var_Destroy(vlc_object_instance(intf), "intf-input-blocked");
    }
    void windowsDisabledOwnerPublishesBlock()
    {
#ifdef _WIN32
        // Native Windows common dialogs disable their Qt owner without a
        // QWidget Show event. Exercise that owned HWND path without opening
        // a system file browser or reading any other drive.
        owner->show(); QCoreApplication::processEvents(); QVERIFY(!blocked());
        const auto handle = reinterpret_cast<HWND>(owner->winId());
        QVERIFY(IsWindowEnabled(handle));
        EnableWindow(handle, FALSE);
        const bool disabled = !IsWindowEnabled(handle);
        const bool published = blocked();
        EnableWindow(handle, TRUE); // restore ownership even on failed assertions
        QCoreApplication::processEvents();
        QVERIFY(disabled); QVERIFY(published); QTRY_VERIFY(!blocked());
#else
        QSKIP("Owned WM_ENABLE coverage requires Windows.");
#endif
    }
};
int main(int argc, char **argv)
{
    QApplication app(argc, argv); app.setQuitOnLastWindowClosed(false);
    TestControlsDialogs test; return QTest::qExec(&test, argc, argv);
}
#include "test_controls_dialogs.moc"
