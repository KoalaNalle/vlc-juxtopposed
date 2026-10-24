// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef VLC_QT_DETACHED_WINDOW_PLACEMENT_HPP
#define VLC_QT_DETACHED_WINDOW_PLACEMENT_HPP
#include <QMargins>
#include <QRect>
#include <QVector>
#include <algorithm>

namespace vlc::qt {
struct DetachedWindowPlacement {
    QRect client;
    int screen = -1;
};

// Keep a detached queue beside its owner when there is room. Otherwise keep
// its complete native frame inside the owner's available display area.
inline DetachedWindowPlacement detachedWindowPlacement(
    const QRect &ownerFrame, const QSize &requestedClient, const QMargins &frame,
    const QVector<QRect> &availableScreens, int ownerScreen)
{
    const auto clientSize = [&](const QRect &area) {
        return QSize(std::clamp(requestedClient.width(), 1, std::max(1, area.width()-frame.left()-frame.right())),
                     std::clamp(requestedClient.height(), 1, std::max(1, area.height()-frame.top()-frame.bottom())));
    };
    const auto place = [&](int index, int outerX) {
        const auto area=availableScreens[index];
        const auto size=clientSize(area);
        const int outerWidth=size.width()+frame.left()+frame.right();
        const int outerHeight=size.height()+frame.top()+frame.bottom();
        const int x=std::clamp(outerX,area.left(),std::max(area.left(),area.right()+1-outerWidth));
        const int y=std::clamp(ownerFrame.top(),area.top(),std::max(area.top(),area.bottom()+1-outerHeight));
        return DetachedWindowPlacement{QRect(QPoint(x+frame.left(),y+frame.top()),size),index};
    };
    QVector<int> order;
    if(ownerScreen>=0&&ownerScreen<availableScreens.size()&&availableScreens[ownerScreen].isValid())order.append(ownerScreen);
    for(int i=0;i<availableScreens.size();++i)
        if(i!=ownerScreen&&availableScreens[i].isValid())order.append(i);
    if(order.isEmpty())return {};
    // Prefer the existing right-side placement, then the left side. A candidate
    // must fit at its requested width; don't shrink it just to fit a side gap.
    for(const bool right:{true,false})for(const int i:order) {
        const auto area=availableScreens[i];
        const int width=requestedClient.width()+frame.left()+frame.right();
        const int x=right?ownerFrame.right()+1+10:ownerFrame.left()-10-width;
        if(x>=area.left()&&x+width<=area.right()+1)return place(i,x);
    }
    return place(order.front(),ownerFrame.right()+1+10);
}
}
#endif
