// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QSettings>

namespace vlc::qt {
inline constexpr char castDiscoverySettingsKey[] = "discover-cast-devices";
inline bool castDiscoveryEnabled(const QSettings *settings)
{
    return !settings || settings->value(QLatin1String(castDiscoverySettingsKey), true).toBool();
}
}
