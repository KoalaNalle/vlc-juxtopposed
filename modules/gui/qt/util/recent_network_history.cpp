// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "recent_network_history.hpp"
#include "checked_preferences_store.hpp"
#include "qt.hpp"
#include <vlc_configuration.h>
#include <vlc_player.h>
#include <QSettings>
namespace RecentNetworkHistory {
QStringList load(qt_intf_t *intf,QSettings *settings)
{
    if (!intf || !settings || !var_InheritBool(intf,"save-recentplay")) return {};
    return settings->value(QStringLiteral("OpenDialog/netMRL")).toStringList().mid(0,8);
}
Result record(qt_intf_t *intf,QSettings *settings,const QString &input,QString &error)
{
    error.clear();
    if (!intf || !settings) { error=QObject::tr("Network history storage is unavailable."); return Result::Failed; }
    if (!var_InheritBool(intf,"save-recentplay")) return Result::Ignored;
    const auto value=input.trimmed();const auto utf8=value.toUtf8();
    if (value.isEmpty()) return Result::Ignored;
    if (utf8.size()>16384 || QString::fromUtf8(utf8)!=value) { error=QObject::tr("The network address is too long or has invalid text."); return Result::Failed; }
    for (const auto c:value) if (c.unicode()<32 || c.unicode()==127) { error=QObject::tr("The network address contains a control character."); return Result::Failed; }
    char *pattern=config_GetPsz("history-exclude-pattern");
    const bool excluded=vlc_player_HistoryIsExcluded(pattern,utf8.constData(),utf8.constData());
    free(pattern);
    if (excluded) return Result::Ignored;
    auto rows=settings->value(QStringLiteral("OpenDialog/netMRL")).toStringList();
    rows.removeAll(value);rows.prepend(value);rows.removeDuplicates();rows=rows.mid(0,8);
    if (!CheckedPreferencesStore(intf).saveSettings(settings,QStringLiteral("OpenDialog/netMRL"),rows,false,error)) return Result::Failed;
    return Result::Saved;
}
bool clear(qt_intf_t *intf,QSettings *settings,QString &error)
{
    return CheckedPreferencesStore(intf).saveSettings(settings,QStringLiteral("OpenDialog/netMRL"),{},true,error);
}
}
