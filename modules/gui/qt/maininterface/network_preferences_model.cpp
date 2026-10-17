// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "network_preferences_model.hpp"
#include "qt.hpp"
#include "util/checked_preferences_store.hpp"
#include <vlc_configuration.h>
#include <QUrl>
#include <algorithm>
#include <iterator>
#include <utility>
#include <cmath>

namespace {
struct CacheOption { const char *key, *native; int numerator; };
constexpr CacheOption cacheOptions[] = {
    {"localFiles", "file-caching", 3}, {"discs", "disc-caching", 3},
    {"networkStreams", "network-caching", 10}, {"liveCapture", "live-caching", 3}
};
constexpr int presets[] = {100, 200, 300, 500, 1000};
QString text(const char *name)
{
    if (config_GetType(name) != VLC_VAR_STRING) return {};
    char *raw = config_GetPsz(name);
    const auto value = QString::fromUtf8(raw ? raw : "");
    free(raw); return value;
}
bool whole(const QVariant &input, qlonglong low, qlonglong high, qlonglong &value)
{
    switch (input.typeId()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Float: case QMetaType::Double: break;
    default: return false;
    }
    const double number = input.toDouble();
    if (!std::isfinite(number) || number < low || number > high || std::floor(number) != number) return false;
    value = qlonglong(number); return true;
}
bool validText(const QString &value, qsizetype maximum)
{
    const auto utf8 = value.toUtf8();
    if (utf8.size() > maximum || QString::fromUtf8(utf8) != value) return false;
    for (const auto ch : value) if (ch.unicode() < 32 || ch.unicode() == 127) return false;
    return true;
}
QUrl proxyUrl(const QString &value) { return QUrl::fromEncoded(value.toUtf8(), QUrl::StrictMode); }
bool manualProxy(const QUrl &url)
{
    return url.isValid() && (url.scheme() == QStringLiteral("http") || url.scheme() == QStringLiteral("https"))
        && !url.host().isEmpty() && url.port(1) >= 1 && url.port(1) <= 65535
        && (url.path().isEmpty() || url.path() == QStringLiteral("/")) && !url.hasQuery() && !url.hasFragment();
}
bool sameAuthority(const QUrl &a, const QUrl &b)
{
    return manualProxy(a) && manualProxy(b) && a.scheme() == b.scheme()
        && a.host() == b.host() && a.userName() == b.userName()
        && a.port(a.scheme() == QStringLiteral("https") ? 443 : 80)
            == b.port(b.scheme() == QStringLiteral("https") ? 443 : 80);
}
QVariantList stringChoices(const char *name)
{
    QVariantList result;
    if (config_GetType(name) != VLC_VAR_STRING) return result;
    char **values = nullptr, **names = nullptr;
    const ssize_t count = config_GetPszChoices(name, &values, &names);
    for (ssize_t i = 0; i < count; ++i) {
        result.append(QVariantMap{{QStringLiteral("value"), QString::fromUtf8(values[i] ? values[i] : "")},
            {QStringLiteral("name"), QString::fromUtf8(names[i] ? names[i] : "")}});
        free(values[i]); free(names[i]);
    }
    free(values); free(names); return result;
}
}

NetworkPreferencesModel::NetworkPreferencesModel(qt_intf_t *intf, Consumers consumers, QObject *parent)
    : QObject(parent), m_intf(intf), m_consumers(consumers)
{
    m_choices[QStringLiteral("buffering")] = QVariantList{
        QVariantMap{{QStringLiteral("value"), 0}, {QStringLiteral("name"), tr("Custom")}},
        QVariantMap{{QStringLiteral("value"), 100}, {QStringLiteral("name"), tr("Lowest latency")}},
        QVariantMap{{QStringLiteral("value"), 200}, {QStringLiteral("name"), tr("Low latency")}},
        QVariantMap{{QStringLiteral("value"), 300}, {QStringLiteral("name"), tr("Normal")}},
        QVariantMap{{QStringLiteral("value"), 500}, {QStringLiteral("name"), tr("High latency")}},
        QVariantMap{{QStringLiteral("value"), 1000}, {QStringLiteral("name"), tr("Higher latency")}}};
    m_choices[QStringLiteral("cipherPreference")] = stringChoices("gnutls-priorities");
    refresh();
}
void NetworkPreferencesModel::setError(const QString &value)
{ if (value != m_error) { m_error = value; emit lastErrorChanged(); } }
void NetworkPreferencesModel::setStatus(const QString &value)
{ if (value != m_status) { m_status = value; emit statusChanged(); } }
bool NetworkPreferencesModel::fail(const QString &value) { setError(value); return false; }
void NetworkPreferencesModel::refresh()
{
    QVariantMap values, caps;
    bool caching = m_intf && m_consumers.caching;
    for (const auto &option : cacheOptions) caching &= config_GetType(option.native) == VLC_VAR_INTEGER;
    int preset = 0;
    if (!m_customEditing && caching) for (const auto candidate : presets) {
        bool same = true;
        for (const auto &option : cacheOptions) same &= config_GetInt(option.native) == candidate * option.numerator / 3;
        if (same) { preset = candidate; break; }
    }
    values[QStringLiteral("buffering")] = preset;
    caps[QStringLiteral("buffering")] = caching;
    for (const auto &option : cacheOptions) {
        const auto key = QString::fromLatin1(option.key);
        values[key] = qlonglong(config_GetType(option.native) == VLC_VAR_INTEGER ? config_GetInt(option.native) : -1);
        caps[key] = caching && preset == 0;
    }
    const bool proxy = m_intf && m_consumers.proxy && config_GetType("http-proxy") == VLC_VAR_STRING
        && config_GetType("http-proxy-pwd") == VLC_VAR_STRING
        && config_GetType("system-proxy-policy-version") == VLC_VAR_INTEGER && config_GetInt("system-proxy-policy-version") == 1;
    const auto rawProxy = text("http-proxy");
    auto url = proxyUrl(rawProxy);
    const bool embeddedPassword = !url.password().isEmpty();
    url.setPassword({});
    // Existing embedded credentials must never enter QML values or status.
    const bool customProxy = !rawProxy.isEmpty() && rawProxy.compare(QStringLiteral("direct://"), Qt::CaseInsensitive)
        && !manualProxy(url);
    values[QStringLiteral("proxyAddressCustom")] = customProxy;
    values[QStringLiteral("httpProxy")] = customProxy ? QString{}
        : embeddedPassword ? url.toString(QUrl::FullyEncoded) : rawProxy;
    values[QStringLiteral("proxyPasswordConfigured")] = embeddedPassword || !text("http-proxy-pwd").isEmpty();
    caps[QStringLiteral("httpProxy")] = proxy;
    caps[QStringLiteral("httpProxyPassword")] = proxy && manualProxy(url) && !url.userName().isEmpty();
    values[QStringLiteral("systemTrust")] = config_GetType("gnutls-system-trust") == VLC_VAR_BOOL && config_GetInt("gnutls-system-trust") != 0;
    values[QStringLiteral("cipherPreference")] = text("gnutls-priorities");
    caps[QStringLiteral("systemTrust")] = m_intf && m_consumers.tls && config_GetType("gnutls-system-trust") == VLC_VAR_BOOL;
    caps[QStringLiteral("cipherPreference")] = m_intf && m_consumers.tls && !m_choices.value(QStringLiteral("cipherPreference")).toList().isEmpty();
    caps[QStringLiteral("advanced")] = m_intf && m_consumers.advanced;
    if (values == m_values && caps == m_capabilities) return;
    m_values = std::move(values); m_capabilities = std::move(caps); emit valuesChanged();
}
QVariantMap NetworkPreferencesModel::applyModes() const
{
    QVariantMap result;
    for (const char *key : {"buffering", "localFiles", "discs", "networkStreams", "liveCapture", "httpProxy", "httpProxyPassword", "systemTrust", "cipherPreference"})
        result[QString::fromLatin1(key)] = QStringLiteral("nextMedia");
    return result;
}
QVariantMap NetworkPreferencesModel::supportNotes() const
{
    return {{QStringLiteral("buffering"), tr("Milliseconds. Presets use VLC's native buffering values. Choose Custom to edit each source type. Reopen media to apply new buffers.")},
        {QStringLiteral("proxy"), tr("Leave empty to use Windows system proxy settings. Enter an HTTP or HTTPS proxy URL, or direct:// for a direct connection. Enter the username in the URL and the password separately. Changing proxy authority clears saved credentials. Existing connections are unaffected.")},
        {QStringLiteral("tls"), tr("System trust adds Windows trusted certificate authorities. Turning it Off still verifies certificates against explicitly configured trust directories. Cipher and trust changes apply when media is opened again; existing TLS connections keep their current policy.")}};
}
bool NetworkPreferencesModel::save(const QVariantMap &requested)
{
    QVariantMap changes;
    for (auto it = requested.cbegin(); it != requested.cend(); ++it) {
        const auto name = it.key().toUtf8();
        const QVariant old = it.value().typeId() == QMetaType::QString
            ? QVariant(text(name.constData())) : QVariant(qlonglong(config_GetInt(name.constData())));
        if (old != it.value()) changes.insert(it.key(), it.value());
    }
    QString error;
    if (!changes.isEmpty() && !CheckedPreferencesStore(m_intf).saveNative(changes, error)) return fail(error);
    setError({}); refresh();
    setStatus(tr("Saved. Reopen media to apply the new network settings.")); return true;
}
bool NetworkPreferencesModel::setValue(const QString &key, const QVariant &input)
{
    setStatus({});
    if (!m_capabilities.value(key).toBool()) return fail(tr("This Network setting is unavailable in this build or mode."));
    if (key == QStringLiteral("buffering")) {
        qlonglong value;
        if (!whole(input, 0, 1000, value) || (value && std::find(std::begin(presets), std::end(presets), value) == std::end(presets)))
            return fail(tr("Select a listed buffering preset."));
        if (!value) { m_customEditing = true; setError({}); refresh(); return true; }
        QVariantMap changes;
        for (const auto &option : cacheOptions) changes[QString::fromLatin1(option.native)] = value * option.numerator / 3;
        if (!save(changes)) return false;
        m_customEditing = false; refresh(); return true;
    }
    for (const auto &option : cacheOptions) if (key == QLatin1String(option.key)) {
        qlonglong value;
        if (!whole(input, 0, 60000, value)) return fail(tr("Enter a whole number of milliseconds from 0 to 60000."));
        return save({{QString::fromLatin1(option.native), value}});
    }
    if (key == QStringLiteral("httpProxy")) {
        if (input.typeId() != QMetaType::QString || !validText(input.toString(), 4096)) return fail(tr("Enter a valid proxy address."));
        auto value = input.toString().trimmed();
        if (!value.compare(QStringLiteral("direct://"), Qt::CaseInsensitive)) value = QStringLiteral("direct://");
        const auto url = proxyUrl(value);
        if (!value.isEmpty() && value != QStringLiteral("direct://") && (!manualProxy(url)
            || url.userInfo(QUrl::FullyEncoded).contains(QLatin1Char(':'))))
            return fail(tr("Enter an HTTP or HTTPS proxy URL. Enter its password in the password field."));
        const auto previous = proxyUrl(text("http-proxy"));
        QVariantMap changes{{QStringLiteral("http-proxy"), value}};
        if (!sameAuthority(previous, url)) changes[QStringLiteral("http-proxy-pwd")] = QString{};
        else if (!previous.password().isEmpty()) changes[QStringLiteral("http-proxy-pwd")] = previous.password();
        return save(changes);
    }
    if (key == QStringLiteral("httpProxyPassword")) {
        if (input.typeId() != QMetaType::QString || !validText(input.toString(), 1024)) return fail(tr("Enter a proxy password of at most 1024 UTF-8 bytes without control characters."));
        auto url = proxyUrl(text("http-proxy")); url.setPassword({});
        return save({{QStringLiteral("http-proxy-pwd"), input.toString()},
            {QStringLiteral("http-proxy"), url.toString(QUrl::FullyEncoded)}});
    }
    if (key == QStringLiteral("systemTrust")) {
        if (input.typeId() != QMetaType::Bool) return fail(tr("This setting requires On or Off."));
        return save({{QStringLiteral("gnutls-system-trust"), qlonglong(input.toBool())}});
    }
    if (key == QStringLiteral("cipherPreference") && input.typeId() == QMetaType::QString) {
        for (const auto &entry : m_choices.value(key).toList()) if (entry.toMap().value(QStringLiteral("value")) == input)
            return save({{QStringLiteral("gnutls-priorities"), input}});
    }
    return fail(tr("Select a listed Network setting."));
}
bool NetworkPreferencesModel::openAdvanced()
{
    setStatus({});
    if (!m_capabilities.value(QStringLiteral("advanced")).toBool()) return fail(tr("Advanced Network preferences are unavailable."));
    setError({}); emit advancedOptionsRequested(); return true;
}
void NetworkPreferencesModel::reload() { setError({}); setStatus({}); refresh(); }
