// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QVariantMap>
#include <QStringList>
struct qt_intf_t;

// Native defaults owned by a player profile. This internal value service is
// deliberately not a QML key editor. Its closed registry excludes application
// appearance, input bindings, privacy, network, library and credential options.
// Raw defaults retain inactive subtitle units, exact EQ/filter chains and
// distinct native sentinel values which the presentation models simplify.
class PlayerProfilePreferences final
{
public:
    explicit PlayerProfilePreferences(qt_intf_t *intf) : m_intf(intf) {}
    static QStringList groups();
    static QStringList nativeKeys(const QString &group);
    // Profiles saved before previews existed inherit their original off policy.
    // This is an in-memory interpretation; loading never rewrites saved data.
    static QVariantMap compatibleDefaults(QVariantMap);
    bool snapshot(QVariantMap &result, QString &error) const;
    // Validates both destinations and the ability to restore every changed
    // source field before the coordinator writes its switch journal.
    bool prepare(const QVariantMap &target, QVariantMap &changes, QString &error) const;
    bool apply(const QVariantMap &target, QString &error);
private:
    qt_intf_t *m_intf;
};
