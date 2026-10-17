// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <vlc_common.h>
#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QHash>
#include <QPointer>

// One publisher combines Qt's native modal/popup windows and the QML host.
// The VLC context must outlive this GUI-thread object.
class ModalInputGuard final : public QObject, public QAbstractNativeEventFilter
{
public:
    ModalInputGuard(vlc_object_t *context, QObject *parent = nullptr);
    ~ModalInputGuard() override;
    void setQmlVisible(bool visible);
protected:
    bool eventFilter(QObject *, QEvent *) override;
    bool nativeEventFilter(const QByteArray &, void *, qintptr *) override;
private:
    void watch(QObject *);
    void forget(QObject *);
    void setNativeDisabled(QObject *, bool);
    void publish();
    vlc_object_t *m_root;
    QPointer<QObject> m_app;
    QHash<QObject *, QMetaObject::Connection> m_windows;
    QHash<QObject *, QMetaObject::Connection> m_nativeDisabled;
    bool m_qmlVisible = false;
    bool m_registered = false;
};
