// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef VLC_QT_AUDIO_DEFAULTS_POLICY_HPP
#define VLC_QT_AUDIO_DEFAULTS_POLICY_HPP
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QRegularExpression>
#include <cmath>

namespace AudioDefaultsPolicy {
struct ChainLink { QString raw, name; bool hasOptions = false; };

// Preserve opaque module option bodies and order, including quoted ':' bytes.
// Malformed chains stay untouched and refuse a managed filter edit.
inline bool parseChain(const QString &text, QList<ChainLink> &links)
{
    links.clear();
    if (text.toUtf8().size() > 32768) return false;
    int start = 0, depth = 0; QChar quote; bool escaped = false;
    for (int i = 0; i <= text.size(); ++i) {
        const bool end = i == text.size();
        const QChar c = end ? QChar() : text[i];
        if (!end && c.unicode() < 32) return false;
        if (escaped) { if (end) return false; escaped = false; continue; }
        if (!end && c == QLatin1Char('\\')) { escaped = true; continue; }
        if (!quote.isNull()) { if (end) return false; if (c == quote) quote = {}; continue; }
        if (!end && (c == QLatin1Char('\'') || c == QLatin1Char('"'))) { quote = c; continue; }
        if (!end && c == QLatin1Char('{')) ++depth;
        else if (!end && c == QLatin1Char('}')) { if (--depth < 0) return false; }
        if (end || (c == QLatin1Char(':') && depth == 0)) {
            if (depth != 0) return false;
            const auto raw = text.mid(start, i - start);
            const auto trimmed = raw.trimmed();
            if (trimmed.isEmpty()) { if (!text.isEmpty()) return false; }
            else {
                const int brace = trimmed.indexOf(QLatin1Char('{'));
                const auto name = (brace < 0 ? trimmed : trimmed.left(brace)).trimmed();
                static const QRegularExpression safeName(QStringLiteral("^[A-Za-z0-9_-]+$"));
                if (!safeName.match(name).hasMatch() || (brace >= 0 && !trimmed.endsWith(QLatin1Char('}')))) return false;
                links.push_back({raw, name, brace >= 0});
            }
            start = i + 1;
        }
    }
    return true;
}
inline bool contains(const QList<ChainLink> &links, const QString &name)
{ for (const auto &link : links) if (link.name == name) return true; return false; }
inline bool hasOptions(const QList<ChainLink> &links, const QString &name)
{ for (const auto &link : links) if (link.name == name && link.hasOptions) return true; return false; }
inline QString changeFilter(const QList<ChainLink> &links, const QString &name, bool enabled)
{
    QStringList result; bool found = false;
    for (const auto &link : links) {
        if (link.name == name) { found = true; if (!enabled) continue; }
        result.push_back(link.raw);
    }
    if (enabled && !found) result.push_back(name);
    return result.join(QLatin1Char(':'));
}
inline bool numeric(const QVariant &value, double lo, double hi, double &out)
{
    switch (value.typeId()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong: case QMetaType::ULongLong:
    case QMetaType::Float: case QMetaType::Double: break;
    default: return false;
    }
    out = value.toDouble(); return std::isfinite(out) && out >= lo && out <= hi;
}
inline bool integer(const QVariant &value, int lo, int hi, int &out)
{
    double number; if (!numeric(value, lo, hi, number) || number != std::floor(number)) return false;
    out = int(number); return true;
}
inline bool validText(const QString &text, int bytes = 2048)
{
    if (text.toUtf8().size() > bytes || QString::fromUtf8(text.toUtf8()) != text) return false;
    for (const QChar c : text) if (c.unicode() < 32 || c.unicode() == 127) return false;
    return true;
}
}
#endif
