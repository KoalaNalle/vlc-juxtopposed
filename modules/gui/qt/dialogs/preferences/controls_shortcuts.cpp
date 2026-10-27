// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "controls_shortcuts.hpp"
#include "preferences_widgets.hpp"
#include "maininterface/controls_preferences_model.hpp"
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QWindow>

QDialog *openControlsShortcutEditor(ControlsPreferencesModel *model, QWindow *owner)
{
    auto *dialog = new QDialog;
    dialog->setObjectName(QStringLiteral("juxtopposedShortcutEditor"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(QObject::tr("Edit Keyboard Shortcuts"));
    dialog->resize(780, 560);
    dialog->setMinimumSize(400, 300);
    auto *layout = new QGridLayout(dialog);
    const QPointer<ControlsPreferencesModel> lifetime(model);
    auto *selector = new KeySelectorControl(dialog);
    selector->setParent(dialog);
    selector->insertInto(layout);
    auto *status = new QLabel(QObject::tr("Double click a shortcut to change it. Changes apply after restarting VLC."), dialog);
    status->setObjectName(QStringLiteral("shortcutEditorStatus"));
    status->setWordWrap(true);
    auto *registration = new QLabel(dialog);
    registration->setObjectName(QStringLiteral("shortcutGlobalRegistrationStatus"));
    registration->setTextFormat(Qt::PlainText);
    registration->setWordWrap(true);
    const auto updateRegistration = [registration, lifetime] {
        const auto text = lifetime ? lifetime->globalShortcutStatus() : QString{};
        registration->setText(text.section(QLatin1Char('\n'), 0, 2));
        registration->setAccessibleDescription(text);
        registration->setVisible(!text.isEmpty());
    };
    updateRegistration();
    QObject::connect(model, &ControlsPreferencesModel::valuesChanged, dialog, updateRegistration);
    layout->addWidget(registration, 4, 0, 1, 4);
    layout->addWidget(status, 5, 0, 1, 4);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, dialog);
    layout->addWidget(buttons, 6, 0, 1, 4);
    QObject::connect(buttons, &QDialogButtonBox::accepted, dialog, [dialog, selector, status, lifetime] {
        if (!lifetime) { dialog->reject(); return; }
        if (!lifetime->applyShortcuts(selector->requestedValues())) {
            status->setText(lifetime->lastError());
            status->setAccessibleName(lifetime->lastError());
            return;
        }
        dialog->accept();
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    QObject::connect(dialog, &QDialog::finished, dialog, [lifetime] {
        if (lifetime) lifetime->shortcutEditorClosed();
    });
    QObject::connect(model, &QObject::destroyed, dialog, &QDialog::reject);
    if (owner) {
        dialog->winId();
        dialog->windowHandle()->setTransientParent(owner);
        dialog->setWindowModality(Qt::WindowModal);
        QObject::connect(owner, &QObject::destroyed, dialog, &QDialog::reject);
    } else {
        dialog->setWindowModality(Qt::ApplicationModal);
    }
    dialog->open();
    return dialog;
}

QDialog *openControlsRemoteSetup(ControlsPreferencesModel *model, QWindow *owner)
{
    auto *dialog = new QDialog;
    dialog->setObjectName(QStringLiteral("juxtopposedRemoteSetup"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(QObject::tr("Enable Web Remote"));
    dialog->resize(440, 200);
    auto *layout = new QGridLayout(dialog);
    auto *label = new QLabel(QObject::tr("Set a password for the web remote. New remotes listen on localhost."), dialog);
    label->setWordWrap(true); layout->addWidget(label, 0, 0);
    auto *password = new QLineEdit(dialog);
    password->setObjectName(QStringLiteral("webRemotePassword"));
    password->setEchoMode(QLineEdit::Password); password->setMaxLength(256);
    password->setAccessibleName(QObject::tr("Web remote password"));
    layout->addWidget(password, 1, 0);
    auto *status = new QLabel(QObject::tr("Use at least eight characters. Restart VLC after saving to start the service."), dialog);
    status->setObjectName(QStringLiteral("webRemoteSetupStatus"));
    status->setWordWrap(true); layout->addWidget(status, 2, 0);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, dialog);
    layout->addWidget(buttons, 3, 0);
    const QPointer<ControlsPreferencesModel> lifetime(model);
    QObject::connect(buttons, &QDialogButtonBox::accepted, dialog, [dialog, password, status, lifetime] {
        if (!lifetime) { dialog->reject(); return; }
        if (!lifetime->applyRemoteSetup(password->text())) {
            status->setText(lifetime->lastError());
            status->setAccessibleName(lifetime->lastError());
            return;
        }
        password->clear(); dialog->accept();
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    QObject::connect(dialog, &QDialog::finished, dialog, [lifetime, password] {
        password->clear();
        if (lifetime) lifetime->remoteSetupClosed();
    });
    QObject::connect(model, &QObject::destroyed, dialog, &QDialog::reject);
    if (owner) {
        dialog->winId(); dialog->windowHandle()->setTransientParent(owner);
        dialog->setWindowModality(Qt::WindowModal);
        QObject::connect(owner, &QObject::destroyed, dialog, &QDialog::reject);
    } else dialog->setWindowModality(Qt::ApplicationModal);
    dialog->open(); password->setFocus();
    return dialog;
}
