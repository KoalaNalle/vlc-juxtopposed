// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QString>
#include <QVariantMap>
#include <QVector>
#include <optional>

class QSettings;

struct PlayerProfile
{
    QString id;
    QString name;
    QVariantMap snapshot;
};

struct PlayerProfileSwitch
{
    QString sourceId;
    QString targetId;
    QVariantMap before;
    QVariantMap after;
};

// Internal persistence, not a generic QML preferences editor. The coordinator
// supplies snapshots from the four typed media services and native layout model;
// those owners retain field validation, availability and application semantics.
class PlayerProfilesStore final
{
public:
    explicit PlayerProfilesStore(QSettings *settings) : m_settings(settings) {}
    static constexpr int maximumProfiles = 64;
    static constexpr int maximumDocumentBytes = 1024 * 1024;
    static QString settingsKey();
    static bool validSnapshot(const QVariantMap &, QString &error);

    bool reload(const QVariantMap &initialSnapshot, QString &error);
    bool readable() const { return m_readable; }
    const QVector<PlayerProfile> &profiles() const { return m_profiles; }
    QString selectedId() const { return m_selected; }
    int indexOf(const QString &) const;
    const std::optional<PlayerProfileSwitch> &pendingSwitch() const { return m_pending; }

    bool checkpoint(const QVariantMap &current, QString &error);
    bool create(const QString &name, const QVariantMap &current, QString &newId, QString &error);
    bool beginSwitch(const QString &targetId, const QVariantMap &current, QString &error);
    bool finishSwitch(const QVariantMap &applied, QString &error);
    bool cancelSwitch(const QVariantMap &restored, QString &error);

private:
    bool save(const QVector<PlayerProfile> &, const QString &selected,
              const std::optional<PlayerProfileSwitch> &, QString &error);
    QSettings *m_settings;
    QVector<PlayerProfile> m_profiles;
    QString m_selected;
    std::optional<PlayerProfileSwitch> m_pending;
    std::optional<QString> m_source;
    bool m_readable = false;
};
