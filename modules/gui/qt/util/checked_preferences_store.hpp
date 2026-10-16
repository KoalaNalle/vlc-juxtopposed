// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef VLC_QT_CHECKED_PREFERENCES_STORE_HPP
#define VLC_QT_CHECKED_PREFERENCES_STORE_HPP
#include <QString>
#include <QVariantMap>
class QSettings;
struct qt_intf_t;

// Internal typed controllers supply their own bounded option whitelist.
// This class is deliberately not a QML object or a generic settings editor.
class CheckedPreferencesStore final
{
public:
    explicit CheckedPreferencesStore(qt_intf_t *intf) : m_intf(intf) {}
    bool saveNative(const QVariantMap &changes, QString &error);
    bool saveSettings(QSettings *, const QString &key, const QVariant &value,
                      bool remove, QString &error);
    static bool syncCurrentSettings(QSettings *);
    QString configFile() const;
private:
    qt_intf_t *m_intf;
};
#endif
