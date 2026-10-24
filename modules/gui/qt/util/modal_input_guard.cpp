// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "modal_input_guard.hpp"
#include <vlc_variables.h>
#include <QApplication>
#include <QEvent>
#include <QWindow>
#include <QWidget>
#ifdef _WIN32
# include <windows.h>
#endif

namespace {
bool blocksVideoInput(QObject *object)
{
    if (auto *widget = qobject_cast<QWidget *>(object))
        return widget->isWindow() && (widget->isModal() || widget->windowType() == Qt::Popup);
    if (auto *window = qobject_cast<QWindow *>(object))
        return window->modality() != Qt::NonModal || window->type() == Qt::Popup;
    return false;
}
#ifdef _WIN32
HWND nativeHandle(QWindow *window)
{
    if (!window->isTopLevel() || window->type() == Qt::ToolTip || !window->handle())
        return nullptr;
    const auto handle = reinterpret_cast<HWND>(window->winId());
    return (GetWindowLongPtrW(handle, GWL_STYLE) & WS_CHILD) ? nullptr : handle;
}
#endif
}

ModalInputGuard::ModalInputGuard(vlc_object_t *context, QObject *parent)
    : QObject(parent), m_root(VLC_OBJECT(vlc_object_instance(context))),
      m_app(QCoreApplication::instance())
{
    m_registered = var_Create(m_root, "intf-input-blocked", VLC_VAR_BOOL) == VLC_SUCCESS;
    if (!m_registered) return;
    if (m_app) {
        m_app->installEventFilter(this);
        static_cast<QCoreApplication *>(m_app.data())->installNativeEventFilter(this);
    }
    if (qobject_cast<QApplication *>(m_app.data()))
        for (auto *widget : QApplication::topLevelWidgets())
            if (widget->isVisible() && blocksVideoInput(widget)) watch(widget);
    if (qobject_cast<QGuiApplication *>(m_app.data()))
        for (auto *window : QGuiApplication::allWindows()) {
            if (window->isVisible() && blocksVideoInput(window)) watch(window);
#ifdef _WIN32
            const auto handle = nativeHandle(window);
            if (window->isVisible() && handle && !IsWindowEnabled(handle)) setNativeDisabled(window, true);
#endif
        }
    publish();
}

ModalInputGuard::~ModalInputGuard()
{
    if (m_app) {
        m_app->removeEventFilter(this);
        static_cast<QCoreApplication *>(m_app.data())->removeNativeEventFilter(this);
    }
    for (const auto &connection : m_windows) QObject::disconnect(connection);
    for (const auto &connection : m_nativeDisabled) QObject::disconnect(connection);
    if (m_registered) {
        var_SetBool(m_root, "intf-input-blocked", false);
        var_Destroy(m_root, "intf-input-blocked");
    }
}

void ModalInputGuard::publish()
{
    if (!m_registered) return;
    const bool blocked = m_qmlVisible || !m_windows.isEmpty() || !m_nativeDisabled.isEmpty();
    if (var_GetBool(m_root, "intf-input-blocked") != blocked)
        var_SetBool(m_root, "intf-input-blocked", blocked);
}

void ModalInputGuard::setQmlVisible(bool visible)
{
    m_qmlVisible = visible;
    publish();
}

void ModalInputGuard::watch(QObject *object)
{
    if (!m_windows.contains(object))
        m_windows.insert(object, connect(object, &QObject::destroyed, this,
            [this](QObject *object) { forget(object); }));
    publish();
}

void ModalInputGuard::forget(QObject *object)
{
    const auto it = m_windows.find(object);
    if (it == m_windows.end()) return;
    QObject::disconnect(it.value());
    m_windows.erase(it);
    publish();
}

void ModalInputGuard::setNativeDisabled(QObject *object, bool disabled)
{
    if (disabled) {
        if (!m_nativeDisabled.contains(object))
            m_nativeDisabled.insert(object, connect(object, &QObject::destroyed, this,
                [this](QObject *object) { setNativeDisabled(object, false); }));
    } else {
        const auto it = m_nativeDisabled.find(object);
        if (it == m_nativeDisabled.end()) return;
        QObject::disconnect(it.value());
        m_nativeDisabled.erase(it);
    }
    publish();
}

bool ModalInputGuard::nativeEventFilter(const QByteArray &type, void *message, qintptr *result)
{
    Q_UNUSED(result);
#ifdef _WIN32
    if (type != QByteArrayLiteral("windows_generic_MSG") && type != QByteArrayLiteral("windows_dispatcher_MSG"))
        return false;
    const auto *event = static_cast<const MSG *>(message);
    if (event->message == WM_ENABLE) {
        for (auto *window : QGuiApplication::allWindows()) {
            if (nativeHandle(window) != event->hwnd) continue;
            const bool disabled = event->wParam == FALSE;
            if (!disabled || window->isVisible()) setNativeDisabled(window, disabled);
            break;
        }
    }
#else
    Q_UNUSED(type);
    Q_UNUSED(message);
#endif
    return false;
}

bool ModalInputGuard::eventFilter(QObject *object, QEvent *event)
{
    // Show is observed synchronously, before a pending native click can run
    // while Qt is entering a modal loop. Close may be rejected; wait for Hide.
    if (event->type() == QEvent::Show) {
        if (blocksVideoInput(object)) watch(object);
        else forget(object);
    } else if (event->type() == QEvent::Hide) {
        forget(object);
    }
    return QObject::eventFilter(object, event);
}
