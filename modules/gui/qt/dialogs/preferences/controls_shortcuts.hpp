// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
class ControlsPreferencesModel;
class QDialog;
class QWindow;
QDialog *openControlsShortcutEditor(ControlsPreferencesModel *, QWindow *owner);
QDialog *openControlsRemoteSetup(ControlsPreferencesModel *, QWindow *owner);
