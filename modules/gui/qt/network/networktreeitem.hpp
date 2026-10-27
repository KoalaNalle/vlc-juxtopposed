/*****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
 * Code licensed under GPLv2 or later.
 *****************************************************************************/
#ifndef NETWORKTREEITEM_HPP
#define NETWORKTREEITEM_HPP
#include <QObject>
#include "vlcmediasourcewrapper.hpp"
#include "util/shared_input_item.hpp"
#include "devicesourceprovider.hpp"

class NetworkTreeItem
{
    Q_GADGET
public:
    NetworkTreeItem() : source(nullptr), tree(nullptr), media(nullptr) {}
    NetworkTreeItem(SharedMediaSourceModel source, const SharedInputItem& item)
        : source(source), tree(source->getTree()), media(item) {}
    NetworkTreeItem(MediaTreePtr tree, const SharedInputItem& item)
        : source(nullptr), tree(tree), media(item) {}
    NetworkTreeItem(NetworkTreeItem& parent, const SharedInputItem& item)
        : source(parent.source), tree(parent.tree), media(item) {}
    NetworkTreeItem(const NetworkTreeItem&) = default;
    NetworkTreeItem& operator=(const NetworkTreeItem&) = default;
    NetworkTreeItem(NetworkTreeItem&&) = default;
    NetworkTreeItem& operator=(NetworkTreeItem&&) = default;
    operator bool() const { return tree.get() != nullptr; }
    bool isValid() {
        MediaTreeLocker lock{tree};
        input_item_node_t* node;
        return vlc_media_tree_Find(tree.get(), media.get(), &node, nullptr);
    }
    SharedMediaSourceModel source;
    MediaTreePtr tree;
    SharedInputItem media;
};
#endif
