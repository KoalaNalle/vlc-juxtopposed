// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "extensions_preferences_model.hpp"
#include "mainctx.hpp"
#include "network/addonsmodel.hpp"
#include "util/checked_preferences_store.hpp"
#include "qt.hpp"
#include <vlc_configuration.h>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QPointer>

ExtensionsPreferencesModel::ExtensionsPreferencesModel(MainCtx *ctx, QObject *parent)
    : QObject(parent), m_intf(ctx ? ctx->getIntf() : nullptr), m_ctx(ctx), m_addons(new AddonsModel(this))
{
    connect(m_addons, &AddonsModel::lastErrorChanged, this, &ExtensionsPreferencesModel::lastErrorChanged);
    connect(m_addons, &AddonsModel::availabilityChanged, this, &ExtensionsPreferencesModel::refresh);
    refresh();
}
QString ExtensionsPreferencesModel::lastError() const
{ return m_error.isEmpty() ? m_addons->lastError() : m_error; }
void ExtensionsPreferencesModel::error(const QString &value)
{ if (m_error != value) { m_error = value; emit lastErrorChanged(); } }
bool ExtensionsPreferencesModel::fail(const QString &value) { error(value); return false; }
void ExtensionsPreferencesModel::refresh()
{
    const bool native = m_intf && config_GetType("qt-autoload-extensions") == VLC_VAR_BOOL;
    m_values = {{QStringLiteral("loadOnStartup"), native && config_GetInt("qt-autoload-extensions") != 0}};
    m_capabilities = {{QStringLiteral("loadOnStartup"), native},
        {QStringLiteral("add"), m_addons->available()}, {QStringLiteral("advanced"), m_intf != nullptr}};
    emit valuesChanged();
}
QVariantMap ExtensionsPreferencesModel::applyModes() const
{ return {{QStringLiteral("loadOnStartup"), QStringLiteral("nextStart")}}; }
QVariantMap ExtensionsPreferencesModel::supportNotes() const
{
    return {{QStringLiteral("loadOnStartup"), tr("Applies on the next VLC start. Automatically loads installed extension menus; it does not activate every script. Active extensions keep running. Playlist parsers and other add-on types keep their normal VLC behavior. Command-line options can override this default.")}};
}
bool ExtensionsPreferencesModel::setValue(const QString &key, const QVariant &value)
{
    error({});
    if (busy()) return fail(tr("Finish choosing the package first."));
    refresh();
    if (key != QStringLiteral("loadOnStartup") || !m_capabilities.value(key).toBool())
        return fail(tr("This Extensions setting is unavailable."));
    if (value.typeId() != QMetaType::Bool) return fail(tr("Choose On or Off."));
    QString message;
    if (!CheckedPreferencesStore(m_intf).saveNative({{QStringLiteral("qt-autoload-extensions"), value}}, message))
        return fail(message);
    refresh(); return true;
}
bool ExtensionsPreferencesModel::add()
{
    error({});
    if (!ensureReady()) return false;
    if (busy() || m_addons->busy()) return fail(tr("Wait for the current add-on operation to finish."));
    return m_addons->canInstall() ? m_addons->installSelected() : choosePackage();
}
bool ExtensionsPreferencesModel::findMore()
{ error({}); return !busy() && ensureReady() && m_addons->loadFromDefaultRepository(); }
bool ExtensionsPreferencesModel::remove()
{ error({}); return !busy() && m_addons->removeSelected(); }
bool ExtensionsPreferencesModel::retryCatalogSave()
{ error({}); return !busy() && m_addons->retryCatalogSave(); }
bool ExtensionsPreferencesModel::choosePackage()
{
    error({});
    if (!ensureReady()) return false;
    if (busy() || m_addons->busy()) return fail(tr("Wait for the current add-on operation to finish."));
    if (!m_addons->available() || !qobject_cast<QApplication *>(QCoreApplication::instance()))
        return fail(tr("The VLC package chooser is unavailable in this session."));
    m_pickerActive = true; emit busyChanged();
    QPointer<ExtensionsPreferencesModel> guard(this);
    const auto file = QFileDialog::getOpenFileName(nullptr, tr("Add VLC package"), m_lastDirectory,
        tr("VLC packages (*.vlp *.VLP)"));
    if (!guard) return false;
    m_pickerActive = false; emit busyChanged();
    if (file.isEmpty()) return false;
    m_lastDirectory = QFileInfo(file).absolutePath();
    return m_addons->addFromPackage(QUrl::fromLocalFile(file));
}
bool ExtensionsPreferencesModel::openAdvanced()
{
    error({});
    if (busy() || !m_intf) return fail(tr("Advanced Extensions preferences are unavailable."));
    emit advancedOptionsRequested(); return true;
}
void ExtensionsPreferencesModel::reload()
{
    if (busy()) { fail(tr("Finish choosing the package first.")); return; }
    error({}); ensureReady(); refresh();
}
bool ExtensionsPreferencesModel::ensureReady()
{
    if (!m_ctx) return fail(tr("The add-on manager is unavailable in this session."));
    if (m_addons->getCtx() != m_ctx) m_addons->setCtx(m_ctx);
    else if (!m_addons->available()) m_addons->reloadCatalog();
    return m_addons->available();
}
