// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_QT_FILTERCHAIN_HPP
#define VLC_QT_FILTERCHAIN_HPP
#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_cxx_helpers.hpp>
#include <vlc_variables.h>
#include <QMap>
#include <QList>
#include <QByteArray>
#include <QString>
#include <cstring>

namespace vlc::qt {
struct FilterChainEntry
{
    QString name;
    QByteArray original;
    QMap<QString, QString> options;
};

// Preserve original option text. A colon inside a quoted option is not a separator.
inline QList<FilterChainEntry> filterChainEntries(vout_thread_t *output, const char *variable)
{
    QList<FilterChainEntry> entries;
    if (!output) return entries;
    auto chain = vlc::wrap_cptr(var_GetString(output, variable));
    while (chain && *chain)
    {
        char *name = nullptr;
        config_chain_t *options = nullptr;
        auto next = vlc::wrap_cptr(config_ChainCreate(&name, &options, chain.get()));
        FilterChainEntry entry;
        entry.name = QString::fromUtf8(name);
        const size_t length = strlen(chain.get()) - (next ? strlen(next.get()) + 1 : 0);
        entry.original = QByteArray(chain.get(), length);
        for (auto *option = options; option; option = option->p_next)
            entry.options.insert(QString::fromUtf8(option->psz_name), QString::fromUtf8(option->psz_value));
        if (!entry.name.isEmpty()) entries.push_back(std::move(entry));
        free(name);
        config_ChainDestroy(options);
        chain = std::move(next);
    }
    return entries;
}
}
#endif
