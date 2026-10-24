// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QStringList>
struct qt_intf_t;
class QSettings;
// Accepted opens are persisted immediately. A dialog destructor never writes a
// cached list, so clear/opt-out cannot be undone by a pending widget teardown.
namespace RecentNetworkHistory {
enum class Result { Saved, Ignored, Failed };
QStringList load(qt_intf_t *,QSettings *);
Result record(qt_intf_t *,QSettings *,const QString &,QString &error);
bool clear(qt_intf_t *,QSettings *,QString &error);
}
