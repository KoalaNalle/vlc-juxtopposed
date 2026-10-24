// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "controls_preferences_model.hpp"
#include "qt.hpp"
#include "util/checked_preferences_store.hpp"
#include <vlc_configuration.h>
#include <vlc_modules.h>
#include <vlc_plugin.h>
#include <vlc_actions.h>
#include <QSet>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QTimer>
#include <QUrl>
#include <cmath>

namespace {
struct Option { const char *key, *native; int type; bool live; };
constexpr Option options[] = {
    {"veryShortSkip", "extrashort-jump-size", VLC_VAR_INTEGER, true},
    {"shortSkip", "short-jump-size", VLC_VAR_INTEGER, true},
    {"mediumSkip", "medium-jump-size", VLC_VAR_INTEGER, true},
    {"longSkip", "long-jump-size", VLC_VAR_INTEGER, true},
    {"singleClick", "video-singleclick-action", VLC_VAR_INTEGER, true},
    {"doubleClick", "video-doubleclick-action", VLC_VAR_INTEGER, true},
    {"scrollWheel", "hotkeys-y-wheel-mode", VLC_VAR_INTEGER, false},
    {"horizontalScroll", "hotkeys-x-wheel-mode", VLC_VAR_INTEGER, false},
    {"reverseScroll", "hotkeys-wheel-reverse", VLC_VAR_BOOL, false},
    {"linkWheelSettings", "http-wheel-linked", VLC_VAR_BOOL, true},
    {"remoteScrollWheel", "http-wheel-y-mode", VLC_VAR_INTEGER, true},
    {"remoteHorizontalScroll", "http-wheel-x-mode", VLC_VAR_INTEGER, true},
    {"remoteReverseScroll", "http-wheel-reverse", VLC_VAR_BOOL, true},
};
QString text(const char *name)
{
    if (config_GetType(name) != VLC_VAR_STRING) return {};
    char *raw = config_GetPsz(name);
    const auto result = QString::fromUtf8(raw ? raw : "");
    free(raw); return result;
}
bool integer(const QVariant &input, qlonglong lo, qlonglong hi, qlonglong &out)
{
    switch (input.typeId()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Float: case QMetaType::Double: break;
    default: return false;
    }
    const double number = input.toDouble();
    if (!std::isfinite(number) || number < lo || number > hi || std::floor(number) != number) return false;
    out = qlonglong(number); return true;
}
bool wheelCode(uint32_t code)
{
    return code >= KEY_MOUSEWHEELUP && code <= KEY_MOUSEWHEELRIGHT;
}
QVariantList choices(const char *name)
{
    if (config_GetType(name) != VLC_VAR_INTEGER) return {};
    int64_t *values = nullptr; char **names = nullptr;
    const ssize_t count = config_GetIntChoices(name, &values, &names);
    QVariantList result;
    for (ssize_t i = 0; i < count; ++i) {
        result.append(QVariantMap{{QStringLiteral("value"), qlonglong(values[i])},
            {QStringLiteral("name"), QString::fromUtf8(names[i] ? names[i] : "")}});
        free(names[i]);
    }
    free(names); free(values); return result;
}
bool listed(const QVariantList &list, qlonglong value)
{
    for (const auto &entry : list) if (entry.toMap().value(QStringLiteral("value")).toLongLong() == value) return true;
    return false;
}
struct InterfaceChain { bool http = false; QString withoutHttp; };
InterfaceChain interfaces(const char *option)
{
    InterfaceChain result;
    QByteArray remaining = text(option).toUtf8();
    QByteArrayList retained;
    while (!remaining.isEmpty()) {
        char *name = nullptr; config_chain_t *options = nullptr;
        char *next = config_ChainCreate(&name, &options, remaining.constData());
        bool http = name && (!strcmp(name, "http") || !strcmp(name, "luahttp"));
        if (name && !strcmp(name, "luaintf"))
            for (auto *item = options; item; item = item->p_next)
                if (!strcmp(item->psz_name, "intf") && item->psz_value && !strcmp(item->psz_value, "http")) http = true;
        const qsizetype consumed = remaining.size() - (next ? qsizetype(strlen(next)) + 1 : 0);
        if (!http) retained.append(remaining.left(consumed));
        result.http |= http;
        free(name); config_ChainDestroy(options);
        remaining = next ? QByteArray(next) : QByteArray{}; free(next);
    }
    result.withoutHttp = QString::fromUtf8(retained.join(':'));
    return result;
}
bool remoteAvailable(qt_intf_t *intf)
{
    return intf && config_GetType("http-controls-policy-version") == VLC_VAR_INTEGER
        && config_GetInt("http-controls-policy-version") == 2
        && config_GetType("http-wheel-linked") == VLC_VAR_BOOL
        && config_GetType("http-password") == VLC_VAR_STRING
        && config_GetType("http-host") == VLC_VAR_STRING
        && config_GetType("http-port") == VLC_VAR_INTEGER
        && config_GetType("https-port") == VLC_VAR_INTEGER
        && config_GetType("extraintf") == VLC_VAR_STRING
        && config_GetType("control") == VLC_VAR_STRING
        && var_InheritBool(intf, "lua");
}
bool loopbackHost(const QString &host)
{
    return host == QStringLiteral("127.0.0.1") || host == QStringLiteral("::1")
        || host.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0;
}
}

ControlsPreferencesModel::ControlsPreferencesModel(qt_intf_t *intf, Consumers consumers, QObject *parent)
    : QObject(parent), m_intf(intf), m_consumers(std::move(consumers))
{
    unsigned count = 0;
    module_config_t *config = module_config_get(module_get_main(), &count);
    for (unsigned i = 0; i < count; ++i) {
        if (config[i].i_type != CONFIG_ITEM_KEY) continue;
        m_defaults.insert(QString::fromUtf8(config[i].psz_name),
            QString::fromUtf8(config[i].orig.psz ? config[i].orig.psz : ""));
    }
    module_config_free(config);
    for (const auto &option : options)
        if (option.type == VLC_VAR_INTEGER && !QByteArray(option.key).endsWith("Skip"))
            m_choices.insert(QString::fromLatin1(option.key), ::choices(option.native));
    m_startup = restartSnapshot();
    refresh();
    auto *serviceTimer = new QTimer(this);
    serviceTimer->setInterval(1000);
    connect(serviceTimer, &QTimer::timeout, this, [this] {
        if (m_consumers.remotePolicy) refresh();
    });
    serviceTimer->start();
}
void ControlsPreferencesModel::setError(const QString &value)
{ if (value != m_error) { m_error = value; emit lastErrorChanged(); } }
void ControlsPreferencesModel::setStatus(const QString &value)
{ if (value != m_status) { m_status = value; emit statusChanged(); } }
bool ControlsPreferencesModel::fail(const QString &error) { setError(error); return false; }
QVariantMap ControlsPreferencesModel::shortcutValues() const
{
    QVariantMap result;
    for (auto it = m_defaults.cbegin(); it != m_defaults.cend(); ++it)
        result.insert(it.key(), text(it.key().toUtf8().constData()));
    return result;
}
QVariantMap ControlsPreferencesModel::activeShortcuts() const
{
    // Native shortcut edits apply after restart. The tour must describe the
    // current input mapping, rather than advertise a newly saved inactive key.
    QVariantMap result;
    for (auto it = m_defaults.cbegin(); it != m_defaults.cend(); ++it)
        result.insert(it.key(), m_startup.value(it.key()));
    return result;
}
QVariantMap ControlsPreferencesModel::restartSnapshot() const
{
    QVariantMap result = shortcutValues();
    for (const auto &option : options) if (!option.live && config_GetType(option.native) == option.type)
        result.insert(QString::fromLatin1(option.native), qlonglong(config_GetInt(option.native)));
    // Hold only a fingerprint of restart-sensitive credentials/configuration.
    // None of these raw values are exposed to QML or diagnostic/status strings.
    QVariantMap remote;
    for (const char *key : {"extraintf", "control", "http-host", "http-password", "http-cert", "http-key"})
        remote.insert(QString::fromLatin1(key), text(key));
    for (const char *key : {"http-port", "https-port"})
        if (config_GetType(key) == VLC_VAR_INTEGER) remote.insert(QString::fromLatin1(key), qlonglong(config_GetInt(key)));
    result.insert(QStringLiteral("@remote"), QCryptographicHash::hash(
        QJsonDocument::fromVariant(remote).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256));
    return result;
}
void ControlsPreferencesModel::refresh()
{
    QVariantMap values, caps;
    bool mouse = m_intf && m_consumers.mousePolicy
        && var_Type(vlc_object_instance(m_intf), "mouse-click-policy-version") == VLC_VAR_INTEGER
        && var_GetInteger(vlc_object_instance(m_intf), "mouse-click-policy-version") == 1;
    const bool remote = m_consumers.remotePolicy && remoteAvailable(m_intf);
    const bool linked = m_intf && var_InheritBool(m_intf, "http-wheel-linked");
    for (const auto &option : options) {
        const auto key = QString::fromLatin1(option.key);
        const bool present = config_GetType(option.native) == option.type;
        const bool remoteOption = key.startsWith(QStringLiteral("remote")) || key == QStringLiteral("linkWheelSettings");
        caps[key] = m_intf && present && (remoteOption ? remote
            : key.endsWith(QStringLiteral("Skip")) ? true : mouse);
        values[key] = option.type == VLC_VAR_BOOL ? QVariant(present && config_GetInt(option.native) != 0)
            : QVariant(qlonglong(present ? config_GetInt(option.native) : -1));
    }
    values[QStringLiteral("linkWheelSettings")] = linked;
    if (linked) {
        for (const auto &pair : {qMakePair("remoteScrollWheel", "scrollWheel"),
             qMakePair("remoteHorizontalScroll", "horizontalScroll"), qMakePair("remoteReverseScroll", "reverseScroll")}) {
            values[QString::fromLatin1(pair.first)] = values.value(QString::fromLatin1(pair.second));
            caps[QString::fromLatin1(pair.first)] = remote && caps.value(QString::fromLatin1(pair.second)).toBool();
        }
    }
    if (values.value(QStringLiteral("singleClick")).toInt() == 1
        && text("mouse-pause-policy") == QStringLiteral("disabled"))
        values[QStringLiteral("singleClick")] = 0;
    values[QStringLiteral("shortcutsPreset")] = shortcutValues() == m_defaults ? tr("Defaults") : tr("Custom");
    caps[QStringLiteral("editShortcuts")] = m_intf && !m_defaults.isEmpty() && bool(m_consumers.openShortcutEditor);
    caps[QStringLiteral("resetShortcuts")] = m_intf && !m_defaults.isEmpty();
    values[QStringLiteral("webRemoteEnabled")] = interfaces("extraintf").http || interfaces("control").http;
    const bool tls = !text("http-cert").isEmpty();
    values[QStringLiteral("remotePort")] = qlonglong(config_GetInt(tls ? "https-port" : "http-port"));
    values[QStringLiteral("remotePasswordConfigured")] = !text("http-password").isEmpty();
    QString service = QStringLiteral("stopped");
    if (m_intf) {
        auto *root = vlc_object_instance(m_intf);
        if (var_Type(root, "http-service-state") == VLC_VAR_STRING) {
            char *raw = var_GetString(root, "http-service-state"); service = QString::fromUtf8(raw ? raw : ""); free(raw);
        }
    }
    if (!QStringList{QStringLiteral("stopped"), QStringLiteral("starting"), QStringLiteral("active"),
        QStringLiteral("failed"), QStringLiteral("password-required")}.contains(service)) service = QStringLiteral("unavailable");
    values[QStringLiteral("remoteState")] = service;
    caps[QStringLiteral("webRemoteEnabled")] = remote;
    caps[QStringLiteral("remotePort")] = remote;
    const bool restart = restartSnapshot() != m_startup;
    if (m_values == values && m_capabilities == caps && m_restart == restart) return;
    m_restart = restart;
    m_values = std::move(values); m_capabilities = std::move(caps);
    emit valuesChanged();
}
QVariantMap ControlsPreferencesModel::applyModes() const
{
    QVariantMap modes;
    for (const auto &option : options)
        modes[QString::fromLatin1(option.key)] = option.live ? QStringLiteral("live") : QStringLiteral("restart");
    if (m_values.value(QStringLiteral("linkWheelSettings")).toBool())
        for (const char *key : {"remoteScrollWheel", "remoteHorizontalScroll", "remoteReverseScroll"})
            modes[QString::fromLatin1(key)] = QStringLiteral("restart");
    modes[QStringLiteral("editShortcuts")] = QStringLiteral("restart");
    modes[QStringLiteral("resetShortcuts")] = QStringLiteral("restart");
    modes[QStringLiteral("webRemoteEnabled")] = QStringLiteral("restart");
    modes[QStringLiteral("remotePort")] = QStringLiteral("restart");
    return modes;
}
QVariantMap ControlsPreferencesModel::supportNotes() const
{
    return {{QStringLiteral("skip"), tr("Seconds used by the native seek shortcuts. New seek actions use saved amounts immediately.")},
        {QStringLiteral("mouse"), tr("Video clicks wait for the system double-click interval. Drags, context menus, media changes and modal dialogs cancel pending clicks.")},
        {QStringLiteral("wheel"), tr("Wheel and keyboard mappings take effect after restarting VLC. Changing Mouse wheel settings removes conflicting unmodified wheel shortcuts; other shortcuts are preserved.")},
        {QStringLiteral("remote"), tr("Use the same wheel settings is On by default. Shared wheel edits apply to Mouse and Remotes after restarting VLC. Turn it Off to restore independent remote settings; those changes apply immediately. Enabling the service or changing its port requires a restart. New remotes use localhost and require a password. A non-localhost listener requires configured HTTPS credentials.")}};
}
bool ControlsPreferencesModel::save(const QVariantMap &requested)
{
    QVariantMap changes;
    for (auto it = requested.cbegin(); it != requested.cend(); ++it) {
        const auto name = it.key().toUtf8();
        const QVariant old = it.value().typeId() == QMetaType::QString
            ? QVariant(text(name.constData())) : QVariant(qlonglong(config_GetInt(name.constData())));
        if (old != it.value()) changes.insert(it.key(), it.value());
    }
    if (changes.isEmpty()) { setError({}); refresh(); return true; }
    QString error;
    if (!CheckedPreferencesStore(m_intf).saveNative(changes, error)) return fail(error);
    setError({}); refresh(); return true;
}
bool ControlsPreferencesModel::setValue(const QString &key, const QVariant &input)
{
    setStatus({});
    if (busy()) return fail(tr("Close the Controls dialog before changing another setting."));
    if (!m_capabilities.value(key).toBool()) return fail(tr("This Controls setting is unavailable in this build."));
    if (key == QStringLiteral("webRemoteEnabled")) {
        if (input.typeId() != QMetaType::Bool) return fail(tr("This setting requires On or Off."));
        if (input.toBool() && text("http-password").isEmpty()) {
            if (!m_consumers.openRemoteSetup) return fail(tr("Set a web remote password before enabling the service."));
            m_remoteSetupActive = true; setError({}); emit remoteSetupActiveChanged(); emit busyChanged();
            m_consumers.openRemoteSetup(this);
            return true;
        }
        return saveRemoteEnabled(input.toBool());
    }
    if (key == QStringLiteral("remotePort")) {
        qlonglong port;
        if (!integer(input, 1, 65535, port)) return fail(tr("Enter a whole port number from 1 to 65535."));
        if (!save({{!text("http-cert").isEmpty() ? QStringLiteral("https-port") : QStringLiteral("http-port"), port}})) return false;
        setStatus(tr("Port saved. Restart VLC to use it.")); return true;
    }
    const Option *selected = nullptr;
    for (const auto &option : options) if (key == QLatin1String(option.key)) { selected = &option; break; }
    if (!selected) return fail(tr("Unknown Controls setting."));
    QString effectiveKey = key;
    if (m_values.value(QStringLiteral("linkWheelSettings")).toBool()) {
        if (key == QStringLiteral("remoteScrollWheel")) effectiveKey = QStringLiteral("scrollWheel");
        else if (key == QStringLiteral("remoteHorizontalScroll")) effectiveKey = QStringLiteral("horizontalScroll");
        else if (key == QStringLiteral("remoteReverseScroll")) effectiveKey = QStringLiteral("reverseScroll");
        if (effectiveKey != key)
            for (const auto &option : options) if (effectiveKey == QLatin1String(option.key)) { selected = &option; break; }
    }
    QVariant value;
    if (selected->type == VLC_VAR_BOOL) {
        if (input.typeId() != QMetaType::Bool) return fail(tr("This setting requires On or Off."));
        value = qlonglong(input.toBool());
    } else {
        qlonglong number;
        if (!integer(input, -1, 86400, number)) return fail(tr("Enter a whole number of seconds from 0 to 86400, or select a listed action."));
        if (key.endsWith(QStringLiteral("Skip"))) {
            if (number < 0) return fail(tr("A skip amount cannot be negative."));
        } else if (!listed(m_choices.value(key).toList(), number)) return fail(tr("Select a listed Controls action."));
        value = number;
    }
    QVariantMap changes{{QString::fromLatin1(selected->native), value}};
    if (key == QStringLiteral("singleClick"))
        changes.insert(QStringLiteral("mouse-pause-policy"), QStringLiteral("auto"));
    if (effectiveKey == QStringLiteral("scrollWheel") || effectiveKey == QStringLiteral("horizontalScroll") || effectiveKey == QStringLiteral("reverseScroll")) {
        const auto shortcuts = shortcutValues();
        for (auto it = shortcuts.cbegin(); it != shortcuts.cend(); ++it) {
            if (it.key().startsWith(QStringLiteral("global-"))) continue;
            QStringList retained;
            const auto tokens = it.value().toString().split('\t', Qt::SkipEmptyParts);
            bool removed = false;
            for (const auto &token : tokens) {
                const uint32_t code = vlc_str2keycode(token.toUtf8().constData());
                const bool axis = effectiveKey == QStringLiteral("reverseScroll")
                    || (effectiveKey == QStringLiteral("scrollWheel") && (code == KEY_MOUSEWHEELUP || code == KEY_MOUSEWHEELDOWN))
                    || (effectiveKey == QStringLiteral("horizontalScroll") && (code == KEY_MOUSEWHEELLEFT || code == KEY_MOUSEWHEELRIGHT));
                if (wheelCode(code) && axis) removed = true;
                else retained.append(token);
            }
            if (removed) changes.insert(it.key(), retained.join('\t'));
        }
    }
    if (!save(changes)) return false;
    if (selected->live) {
        libvlc_int_t *root = vlc_object_instance(m_intf);
        if (key == QStringLiteral("linkWheelSettings") && !value.toBool()) {
            // Restore the saved independent policy before making it active.
            // Linking never overwrites these preferences with the Mouse values.
            for (const auto &option : options) if (QByteArray(option.key).startsWith("remote")) {
                if (var_Type(root, option.native) == 0) var_Create(root, option.native, option.type);
                if (option.type == VLC_VAR_BOOL) var_SetBool(root, option.native, config_GetInt(option.native) != 0);
                else var_SetInteger(root, option.native, config_GetInt(option.native));
            }
        }
        // Publish only after checked persistence, so explicit edits supersede
        // startup CLI values at the same point as the saved setting.
        if (var_Type(root, selected->native) == 0) var_Create(root, selected->native, selected->type);
        if (selected->type == VLC_VAR_BOOL) var_SetBool(root, selected->native, value.toBool());
        else var_SetInteger(root, selected->native, value.toLongLong());
        if (key == QStringLiteral("singleClick")) var_SetString(root, "mouse-pause-policy", "auto");
        refresh();
    }
    setStatus(key == QStringLiteral("linkWheelSettings")
        ? value.toBool() ? tr("Shared wheel settings enabled. Remotes use the current Mouse mappings; pending mapping changes need a restart.")
                        : tr("Independent remote wheel settings restored. New remote gestures use them immediately.")
        : selected->live ? tr("Saved. New gestures use this setting.") : tr("Saved. Restart VLC to apply the new mappings."));
    return true;
}
bool ControlsPreferencesModel::applyShortcuts(const QVariantMap &requested)
{
    setStatus({});
    if (m_remoteSetupActive) return fail(tr("Close the web remote setup before saving shortcuts."));
    if (!m_intf || requested.size() != m_defaults.size()) return fail(tr("The shortcut editor snapshot is incomplete. Reopen it and retry."));
    const auto current = shortcutValues();
    QVariantMap changes;
    QMap<QString, QSet<uint32_t>> codes;
    for (auto it = requested.cbegin(); it != requested.cend(); ++it) {
        if (!m_defaults.contains(it.key()) || it.value().typeId() != QMetaType::QString)
            return fail(tr("The shortcut editor contains an unknown action."));
        const auto value = it.value().toString(); const auto bytes = value.toUtf8();
        if (bytes.size() > 8192 || QString::fromUtf8(bytes) != value)
            return fail(tr("This shortcut value cannot be saved."));
        const bool changed = value != current.value(it.key()).toString();
        if (changed) changes.insert(it.key(), value);
        for (const auto &token : value.split('\t', Qt::SkipEmptyParts)) {
            const auto code = vlc_str2keycode(token.toUtf8().constData());
            if (changed) {
                for (const auto character : token) if (character.unicode() < 32 || character.unicode() == 127)
                    return fail(tr("Control characters are not valid shortcuts."));
                if (code == KEY_UNSET) return fail(tr("Unrecognized shortcut for %1.").arg(it.key()));
                if (wheelCode(code)) return fail(tr("Use the Mouse controls for unmodified wheel actions."));
                if (codes[it.key()].contains(code)) return fail(tr("A shortcut is repeated for %1.").arg(it.key()));
            }
            if (code != KEY_UNSET) codes[it.key()].insert(code);
        }
    }
    // Preserve unrelated legacy conflicts. A newly edited binding must not
    // shadow another action in the same local/global namespace.
    for (auto changed = changes.cbegin(); changed != changes.cend(); ++changed) {
        QSet<uint32_t> previous;
        for (const auto &token : current.value(changed.key()).toString().split('\t', Qt::SkipEmptyParts))
            previous.insert(vlc_str2keycode(token.toUtf8().constData()));
        const auto added = codes.value(changed.key()) - previous;
        for (auto other = codes.cbegin(); other != codes.cend(); ++other) {
            if (changed.key() == other.key() || changed.key().startsWith(QStringLiteral("global-"))
                != other.key().startsWith(QStringLiteral("global-"))) continue;
            if (added.intersects(other.value()))
                return fail(tr("The shortcut for %1 conflicts with %2. Resolve the conflict before saving.").arg(changed.key(), other.key()));
        }
    }
    if (!save(changes)) return false;
    setStatus(tr("Shortcuts saved. Restart VLC to apply the new mappings."));
    return true;
}
bool ControlsPreferencesModel::resetShortcuts()
{
    setStatus({});
    if (busy()) return fail(tr("Close the Controls dialog before resetting shortcuts."));
    if (!m_capabilities.value(QStringLiteral("resetShortcuts")).toBool()) return fail(tr("The native shortcut configuration is unavailable."));
    // Native defaults are authoritative, including any shipped aliases. Reset
    // only CONFIG_ITEM_KEY fields, preserving every unrelated preference.
    if (!save(m_defaults)) return false;
    setStatus(tr("Default shortcuts saved. Restart VLC to apply them."));
    return true;
}
bool ControlsPreferencesModel::editShortcuts()
{
    setStatus({});
    if (busy()) return false;
    if (!m_capabilities.value(QStringLiteral("editShortcuts")).toBool()) return fail(tr("The shortcut editor is unavailable."));
    m_editorActive = true; setError({}); emit editorActiveChanged(); emit busyChanged();
    m_consumers.openShortcutEditor(this);
    return true;
}
void ControlsPreferencesModel::shortcutEditorClosed()
{
    if (!m_editorActive) return;
    m_editorActive = false; emit editorActiveChanged(); emit busyChanged(); refresh();
}
bool ControlsPreferencesModel::saveRemoteEnabled(bool enabled, const QString &password)
{
    QVariantMap changes;
    const auto extra = interfaces("extraintf"), control = interfaces("control");
    if (enabled) {
        if (password.isEmpty() && text("http-password").isEmpty()) return fail(tr("The web remote requires a password."));
        QString host = text("http-host");
        if (host.isEmpty()) { host = QStringLiteral("127.0.0.1"); changes[QStringLiteral("http-host")] = host; }
        if (!loopbackHost(host) && (text("http-cert").isEmpty() || text("http-key").isEmpty()))
            return fail(tr("Configure an HTTPS certificate and key before enabling a web remote on other devices."));
        if (!password.isEmpty()) changes[QStringLiteral("http-password")] = password;
        if (!extra.http && !control.http) {
            const auto before = text("extraintf");
            changes[QStringLiteral("extraintf")] = before.isEmpty() ? QStringLiteral("http") : before + QStringLiteral(":http");
        }
    } else {
        if (extra.http) changes[QStringLiteral("extraintf")] = extra.withoutHttp;
        if (control.http) changes[QStringLiteral("control")] = control.withoutHttp;
    }
    if (!save(changes)) return false;
    setStatus(enabled ? tr("Web remote enabled for the next start. Restart VLC to run it.")
        : tr("Web remote disabled for the next start. Restart VLC to stop a running service."));
    return true;
}
bool ControlsPreferencesModel::applyRemoteSetup(const QString &password)
{
    setStatus({});
    if (!m_remoteSetupActive || !m_capabilities.value(QStringLiteral("webRemoteEnabled")).toBool())
        return fail(tr("The web remote setup is no longer active."));
    const auto utf8 = password.toUtf8();
    if (password.size() < 8 || utf8.size() > 256 || QString::fromUtf8(utf8) != password)
        return fail(tr("Use a password of at least eight characters and at most 256 UTF-8 bytes."));
    for (const auto character : password) if (character.unicode() < 32 || character.unicode() == 127)
        return fail(tr("A password cannot contain control characters."));
    if (!saveRemoteEnabled(true, password)) return false;
    remoteSetupClosed();
    return true;
}
void ControlsPreferencesModel::remoteSetupClosed()
{
    if (!m_remoteSetupActive) return;
    m_remoteSetupActive = false; emit remoteSetupActiveChanged(); emit busyChanged(); refresh();
}
void ControlsPreferencesModel::reload() { refresh(); }
