// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "sidebar_pins_store.hpp"
#include "medialibrary/mlqmltypes.hpp"
#include <QAbstractListModel>
#include <QPointer>
#include <QQueue>
#include <QSet>
#include <QTimer>
#include <functional>
class MediaLib;
class QDialog;
class QWindow;

// One owner serializes manual pin additions and saves before publishing model changes.
class SidebarPinsModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool canRetry READ canRetry NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged FINAL)
public:
    enum Role { KeyRole = Qt::UserRole + 1, KindRole, TitleRole, ArtworkRole, AvailableRole };
    SidebarPinsModel(MediaLib *, QSettings *, QString databaseFile, QObject *parent = nullptr);
    ~SidebarPinsModel() override;
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return int(m_rows.size()); }
    bool busy() const;
    bool canRetry() const { return bool(m_retry); }
    QString error() const { return m_error; }
    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE bool pinItem(MLItemId);
    Q_INVOKABLE bool pinUrl(const QUrl &, const QString &kind, const QString &title = {});
    Q_INVOKABLE bool pinUrls(const QVariantList &);
    Q_INVOKABLE bool remove(const QString &key);
    Q_INVOKABLE bool move(const QString &key, int destination);
    Q_INVOKABLE void activate(const QString &key, int reason = Qt::OtherFocusReason);
    Q_INVOKABLE void cancelActivation();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void retry();
    Q_INVOKABLE void dismissError();
    Q_INVOKABLE void chooseFiles(QWindow *owner);
    Q_INVOKABLE void chooseFolder(QWindow *owner);
    Q_INVOKABLE void chooseSource(QWindow *owner);
signals:
    void countChanged();
    void stateChanged();
    void mediaRequested(const QUrl &url, const QString &title);
    void collectionRequested(const QString &kind, MLItemId itemId, const QString &title, int reason);
    void browseRequested(const QVariant &tree, int reason);
private:
    struct Addition { MLItemId id; QVector<SidebarPin> pins; bool resolved = false; };
    static void libraryEvent(void *, const vlc_ml_event_t *);
    bool enqueue(Addition);
    void drain();
    void finishAddition();
    void beginRefresh();
    void reloadStore();
    void publishAppend();
    void fail(const QString &, std::function<void()> retry = {});
    void openPicker(QDialog *, QWindow *);
    bool available(const SidebarPin &) const;
    int indexOf(const QString &) const;
    MediaLib *m_library;
    SidebarPinsStore m_store;
    QString m_databaseFile;
    QVector<SidebarPin> m_rows;
    QHash<QString, bool> m_available;
    QQueue<Addition> m_queue;
    QSet<quint64> m_tasks;
    vlc_ml_event_callback_t *m_listener = nullptr;
    QTimer m_refreshTimer;
    QPointer<QDialog> m_picker;
    std::function<void()> m_retry;
    QString m_error;
    bool m_adding = false, m_refreshing = false, m_failedAddition = false;
    quint64 m_activation = 0, m_generation = 0;
};
