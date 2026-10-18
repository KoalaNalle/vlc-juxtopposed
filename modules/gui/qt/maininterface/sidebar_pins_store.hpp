// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QUrl>
#include <QString>
#include <QVariantMap>
#include <QVector>

class QSettings;

// A manual pin keeps a typed identity, independently of visit/play counts.
// URI targets keep their address across native media-library reindexing.
// Native-only collection IDs are scoped to the database that supplied them.
struct SidebarPin
{
    QString key;
    QString kind;
    QString title;
    QUrl mrl;
    QUrl artwork;
    QString nativeId;
    QString nativeScope;

    QVariantMap toVariant() const;
    static bool normalize(SidebarPin &pin, QString &error);
    static bool fromVariant(const QVariantMap &, SidebarPin &, QString &error);
};

class SidebarPinsStore final
{
public:
    explicit SidebarPinsStore(QSettings *settings);
    static constexpr int maximumPins = 512;
    static QString settingsKey();

    const QVector<SidebarPin> &pins() const { return m_pins; }
    bool readable() const { return m_readable; }
    QString readError() const { return m_readError; }
    bool reload(QString &error);
    bool add(const QVector<SidebarPin> &pins, QString &error);
    bool remove(const QString &key, QString &error);
    bool move(const QString &key, int destination, QString &error);
    int indexOf(const QString &key) const;

private:
    bool save(const QVector<SidebarPin> &, QString &error);
    QSettings *m_settings;
    QVector<SidebarPin> m_pins;
    QString m_readError;
    bool m_readable = false;
};
