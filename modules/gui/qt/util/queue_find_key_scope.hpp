// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef VLC_QT_QUEUE_FIND_KEY_SCOPE_HPP
#define VLC_QT_QUEUE_FIND_KEY_SCOPE_HPP
#include <QQuickItem>

namespace QueueFindKeyScope {
// Only the redesigned Windows Queue tab advertises ownership of Ctrl+F.
inline bool ownsFind(QQuickItem *focused, Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    if (key != Qt::Key_F || modifiers != Qt::ControlModifier)
        return false;
    for (auto *owner = focused; owner; owner = owner->parentItem())
        if (owner->isVisible() && owner->property("queueFindKeyScope").toBool())
            return true;
    return false;
}
}
#endif
