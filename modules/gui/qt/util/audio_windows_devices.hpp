// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef VLC_QT_AUDIO_WINDOWS_DEVICES_HPP
#define VLC_QT_AUDIO_WINDOWS_DEVICES_HPP
#include <QVariantList>
// Actual read-only Core Audio endpoint enumeration. No hardware selection or
// volume modification occurs here. Capability remains off until Root verifies
// this adapter with the actual matched native MMDevice plugin.
bool enumerateAudioWindowsDevices(QVariantList &devices);
#endif
