// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "interface_preferences_model.hpp"
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include "util/checked_preferences_store.hpp"
#include <vlc_configuration.h>
#include <vlc_modules.h>
#include <QCoreApplication>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QSystemTrayIcon>
#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <cmath>

namespace {
struct Option { const char *key; const char *native; int type; };
constexpr Option options[] = {
    {"customSkin", "intf", VLC_VAR_STRING},
    {"skinFile", "skins2-last", VLC_VAR_STRING},
    {"embeddedVideo", "embedded-video", VLC_VAR_BOOL},
    {"resizeVideo", "qt-video-autoresize", VLC_VAR_BOOL},
    {"rememberGeometry", "qt-remember-window-geometry", VLC_VAR_BOOL},
    {"alwaysOnTop", "video-on-top", VLC_VAR_BOOL},
    {"windowDecorations", "qt-titlebar", VLC_VAR_BOOL},
    {"titleFileName", "qt-name-in-title", VLC_VAR_BOOL},
    {"fullscreenScreen", "qt-fullscreen-screennumber", VLC_VAR_INTEGER},
    {"compactMode", "qt-minimal-view", VLC_VAR_BOOL},
    {"controlAutoHide", "qt-controls-auto-hide", VLC_VAR_INTEGER},
    {"controlHideDelay", "qt-controls-hide-timeout", VLC_VAR_INTEGER},
    {"fullscreenControls", "qt-fs-controller", VLC_VAR_BOOL},
    {"systemTray", "qt-system-tray", VLC_VAR_BOOL},
    {"startMinimized", "qt-start-minimized", VLC_VAR_BOOL},
    {"trackPopup", "qt-notification", VLC_VAR_INTEGER},
    {"popupDuration", "qt-notification-duration", VLC_VAR_INTEGER},
    {"playlistTree", "playlist-tree", VLC_VAR_BOOL}
};
QString text(const char *key)
{
    if (config_GetType(key) != VLC_VAR_STRING) return {};
    char *raw = config_GetPsz(key);
    const auto result = QString::fromUtf8(raw ? raw : ""); free(raw); return result;
}
int number(const char *key, int fallback)
{ return config_GetType(key) == VLC_VAR_INTEGER ? config_GetInt(key) : fallback; }
bool integer(const QVariant &value, int lo, int hi, int &out)
{
    switch (value.typeId()) {
    case QMetaType::Int: case QMetaType::UInt:
    case QMetaType::LongLong: case QMetaType::ULongLong:
    case QMetaType::Double: case QMetaType::Float: break;
    default: return false;
    }
    const auto n = value.toDouble();
    if (!std::isfinite(n) || std::floor(n) != n || n < lo || n > hi) return false;
    out = int(n); return true;
}
bool validText(const QString &input)
{
    for (const auto ch : input) if (ch.unicode() < 32) return false;
    return QString::fromUtf8(input.toUtf8()) == input;
}
bool managedInterface(const QString &value)
{ return value.isEmpty() || value == QStringLiteral("qt") || value == QStringLiteral("skins2,any"); }
QGuiApplication *guiApplication()
{ return qobject_cast<QGuiApplication *>(QCoreApplication::instance()); }
}

InterfacePreferencesModel::InterfacePreferencesModel(qt_intf_t *intf, QSettings *settings, QObject *parent)
    : InterfacePreferencesModel(intf, settings, Consumers{}, parent) {}
InterfacePreferencesModel::InterfacePreferencesModel(qt_intf_t *intf, QSettings *settings,
    Consumers consumers, QObject *parent)
    : QObject(parent), m_intf(intf), m_settings(settings), m_consumers(consumers)
{
    if (auto app = guiApplication()) {
        connect(app, &QGuiApplication::screenAdded, this, [this] { refresh(); });
        connect(app, &QGuiApplication::screenRemoved, this, [this] { refresh(); });
    }
    reload(); m_startupValues = m_values; updateRestartRequired();
}
InterfacePreferencesModel::~InterfacePreferencesModel() = default;
void InterfacePreferencesModel::setError(const QString &error)
{
    if (error == m_lastError) return;
    m_lastError = error; emit lastErrorChanged();
}
bool InterfacePreferencesModel::fail(const QString &error) { setError(error); return false; }
void InterfacePreferencesModel::refresh()
{
    QVariantMap values, capabilities;
    for (const auto &option : options) {
        const auto key = QString::fromLatin1(option.key);
        const bool present = config_GetType(option.native) == option.type;
        capabilities.insert(key, m_intf && m_settings && present);
        if (option.type == VLC_VAR_STRING) values.insert(key, text(option.native));
        else if (option.type == VLC_VAR_BOOL) values.insert(key, present && config_GetInt(option.native) != 0);
        else values.insert(key, number(option.native, -1));
    }
    const auto intf = text("intf");
    values.insert(QStringLiteral("customSkin"), intf.split(',').contains(QStringLiteral("skins2")));
    const bool skins = module_exists("skins2") && managedInterface(intf);
    capabilities[QStringLiteral("customSkin")] = capabilities.value(QStringLiteral("customSkin")).toBool() && skins;
    capabilities[QStringLiteral("skinFile")] = capabilities.value(QStringLiteral("skinFile")).toBool() && skins;
    values[QStringLiteral("controlHideDelay")] = number("qt-controls-hide-timeout", -1) >= 0
        ? number("qt-controls-hide-timeout", -1) : number("mouse-hide-timeout", 1000);
    const auto gate = [&capabilities](const char *key, bool linked) {
        const auto name = QString::fromLatin1(key);
        capabilities[name] = capabilities.value(name).toBool() && linked;
    };
    gate("rememberGeometry", m_consumers.rememberGeometry);
    gate("titleFileName", m_consumers.titleFileName);
    gate("fullscreenScreen", m_consumers.fullscreenScreen && guiApplication()
        && QGuiApplication::platformName() == QStringLiteral("windows")
        && values.value(QStringLiteral("embeddedVideo")).toBool()
        && m_intf && m_intf->p_compositor
        && m_intf->p_compositor->type() != vlc::Compositor::DummyCompositor);
    gate("controlAutoHide", m_consumers.controlAutoHide);
    gate("controlHideDelay", m_consumers.controlHideDelay);
    gate("popupDuration", m_consumers.popupDuration);
    // The fullscreen bar consumer belongs to the embedded main-window player.
    // Detached video has no paired controller; keep that mode unavailable.
    gate("fullscreenControls", m_consumers.fullscreenControls
        && values.value(QStringLiteral("embeddedVideo")).toBool());
    // No actual Qt tree model exists.
    capabilities[QStringLiteral("playlistTree")] = false;
    const bool tray = guiApplication() && QSystemTrayIcon::isSystemTrayAvailable();
    gate("systemTray", tray); gate("startMinimized", tray); gate("trackPopup", tray); gate("popupDuration", tray);
    QVariantList screens;
    if (guiApplication()) {
        const auto actualScreens = QGuiApplication::screens();
        for (int i = 0; i < actualScreens.size(); ++i)
            screens.append(QVariantMap{{QStringLiteral("index"), i},
                {QStringLiteral("name"), actualScreens[i]->name()}});
    }
    if (screens != m_screens) { m_screens = screens; emit screensChanged(); }
    if (capabilities != m_capabilities) { m_capabilities = capabilities; emit capabilitiesChanged(); }
    if (values != m_values) { m_values = values; emit valuesChanged(); }
    updateRestartRequired();
}
void InterfacePreferencesModel::updateRestartRequired()
{
    // Genuine existing consumers cache these preferences at startup or output
    // creation. Restart covers every active output without partial live state.
    const bool required = !m_startupValues.isEmpty() && m_values != m_startupValues;
    if (required == m_restartRequired) return;
    m_restartRequired = required; emit restartRequiredChanged();
}
void InterfacePreferencesModel::reload()
{
    setError({});
    const bool currentRead = CheckedPreferencesStore::syncCurrentSettings(m_settings);
    refresh();
    if (!m_intf || !m_settings) setError(tr("The native Interface preferences service is unavailable."));
    else if (!currentRead) setError(tr("Some saved interface preferences could not be read."));
}
bool InterfacePreferencesModel::validateSkinFile(const QString &input, QString &normalized, QString &error) const
{
    if (input.isEmpty() || !validText(input) || input.toUtf8().size() > 32760) {
        error = tr("Choose a readable local VLC skin file."); return false;
    }
    normalized = QDir::cleanPath(QDir::fromNativeSeparators(input));
    const QFileInfo info(normalized);
    if (!info.isAbsolute() || input.contains(QStringLiteral("://")) || !info.isFile() || !info.isReadable()) {
        error = tr("The skin must be an existing readable file at an absolute local path."); return false;
    }
#ifdef _WIN32
    const auto tail = normalized.mid(normalized.size() > 1 && normalized.at(1) == ':' ? 2 : 0);
    if (tail.contains(QRegularExpression(QStringLiteral("[<>:\"|?*]")))) {
        error = tr("The skin filename contains characters Windows cannot use."); return false;
    }
    for (const auto &part : tail.split('/', Qt::SkipEmptyParts)) {
        const auto stem = part.section('.', 0, 0).toUpper();
        if (part.endsWith('.') || part.endsWith(' ') || QRegularExpression(
            QStringLiteral("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$")).match(stem).hasMatch()) {
            error = tr("The skin filename contains a reserved Windows name."); return false;
        }
    }
#endif
    const auto suffix = info.suffix().toLower();
    if (suffix != QStringLiteral("vlt") && suffix != QStringLiteral("wsz") && suffix != QStringLiteral("xml")) {
        error = tr("Choose a VLC skin (.vlt), Winamp skin (.wsz), or VLC skin XML file."); return false;
    }
    QFile file(normalized);
    if (!file.open(QIODevice::ReadOnly) || file.size() == 0) {
        error = tr("The skin file cannot be read or is empty."); return false;
    }
    // This checks a selected file, not the skins2 parser. Actual archive/theme
    // validity is determined by the genuine skins2 interface after restart.
    return true;
}
bool InterfacePreferencesModel::setValue(const QString &key, const QVariant &input)
{
    setError({});
    const Option *option = nullptr;
    for (const auto &candidate : options) if (key == QString::fromLatin1(candidate.key)) { option = &candidate; break; }
    if (!option) return fail(tr("Unknown Interface preference."));
    if (!m_intf || !m_settings) return fail(tr("The native Interface preferences service is unavailable."));
    refresh(); // Current screen count, module support and actual native state.
    if (!m_capabilities.value(key).toBool()) return fail(tr("This Interface preference is unavailable in this build or session."));
    QVariant value;
    if (key == QStringLiteral("skinFile")) {
        if (input.typeId() != QMetaType::QString) return fail(tr("Choose a readable local VLC skin file."));
        QString normalized, error;
        if (!validateSkinFile(input.toString(), normalized, error)) return fail(error);
        value = normalized;
    } else if (option->type == VLC_VAR_BOOL || key == QStringLiteral("customSkin")) {
        if (input.typeId() != QMetaType::Bool) return fail(tr("This preference requires an on or off value."));
        value = input;
        if (key == QStringLiteral("customSkin")) {
            if (input.toBool()) {
                QString normalized, error;
                if (!validateSkinFile(m_values.value(QStringLiteral("skinFile")).toString(), normalized, error)) return fail(error);
            }
            value = input.toBool() ? QStringLiteral("skins2,any") : QString{};
        }
        if (key == QStringLiteral("startMinimized") && input.toBool()
            && !m_values.value(QStringLiteral("systemTray")).toBool())
            return fail(tr("Enable the system tray before starting VLC minimized."));
        if (key == QStringLiteral("systemTray") && !input.toBool()
            && m_values.value(QStringLiteral("startMinimized")).toBool())
            return fail(tr("Turn off Start minimized before disabling the system tray."));
    } else {
        int out, lo = 0, hi = 2;
        if (key == QStringLiteral("fullscreenScreen")) {
            lo = -1; hi = m_screens.size() - 1;
        } else if (key == QStringLiteral("controlHideDelay") || key == QStringLiteral("popupDuration")) hi = 10000;
        if (!integer(input, lo, hi, out)) return fail(tr("Choose a supported value for this Interface preference."));
        value = out;
    }
    QString error;
    if (!CheckedPreferencesStore(m_intf).saveNative({{QString::fromLatin1(option->native), value}}, error)) return fail(error);
    refresh();
    return true;
}
bool InterfacePreferencesModel::chooseSkinFile()
{
    setError({});
    if (m_pickerActive) return fail(tr("The skin file picker is already open."));
    if (!m_capabilities.value(QStringLiteral("skinFile")).toBool()
        || !qobject_cast<QApplication *>(QCoreApplication::instance()))
        return fail(tr("The native skin file picker is unavailable in this session."));
    QPointer<InterfacePreferencesModel> guard(this);
    const auto initial = m_values.value(QStringLiteral("skinFile")).toString();
    m_pickerActive = true; emit pickerActiveChanged();
    if (!guard) return false;
    const auto path = QFileDialog::getOpenFileName(nullptr, tr("Choose VLC skin"),
        initial,
        tr("VLC skins (*.vlt *.wsz *.xml);;All files (*)"));
    if (!guard) return false;
    m_pickerActive = false; emit pickerActiveChanged();
    if (!guard) return false;
    if (path.isEmpty()) return false;
    return setValue(QStringLiteral("skinFile"), path);
}
