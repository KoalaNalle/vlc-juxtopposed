// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QVariantMap>
class MainCtx;
class AddonsModel;
struct qt_intf_t;
Q_MOC_INCLUDE("network/addonsmodel.hpp")

class ExtensionsPreferencesModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AddonsModel* addons READ addons CONSTANT FINAL)
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap capabilities READ capabilities NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap applyModes READ applyModes CONSTANT FINAL)
    Q_PROPERTY(QVariantMap supportNotes READ supportNotes CONSTANT FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
public:
    explicit ExtensionsPreferencesModel(MainCtx *, QObject *parent = nullptr);
    AddonsModel *addons() const { return m_addons; }
    QVariantMap values() const { return m_values; }
    QVariantMap capabilities() const { return m_capabilities; }
    QVariantMap applyModes() const;
    QVariantMap supportNotes() const;
    QString lastError() const;
    bool busy() const { return m_pickerActive; }
    Q_INVOKABLE bool setValue(const QString &, const QVariant &);
    Q_INVOKABLE bool add();
    Q_INVOKABLE bool choosePackage();
    Q_INVOKABLE bool findMore();
    Q_INVOKABLE bool remove();
    Q_INVOKABLE bool retryCatalogSave();
    Q_INVOKABLE bool openAdvanced();
    Q_INVOKABLE void reload();
    bool ensureReady();
signals:
    void valuesChanged();
    void lastErrorChanged();
    void busyChanged();
    void advancedOptionsRequested();
private:
    void refresh();
    bool fail(const QString &);
    void error(const QString &);
    qt_intf_t *m_intf;
    MainCtx *m_ctx;
    AddonsModel *m_addons;
    QVariantMap m_values, m_capabilities;
    QString m_error, m_lastDirectory;
    bool m_pickerActive = false;
};
