# SPDX-License-Identifier: GPL-2.0-or-later
# Owned QML contract seam. Actual consumers are tested by the native fixtures.
from copy import deepcopy
from PySide6.QtCore import QObject, Property, Signal, Slot

SEED = dict(veryShortSkip=3, shortSkip=10, mediumSkip=60, longSkip=300,
    singleClick=1, doubleClick=2, scrollWheel=0, horizontalScroll=2, reverseScroll=False,
    linkWheelSettings=True, remoteScrollWheel=0, remoteHorizontalScroll=2, remoteReverseScroll=False,
    webRemoteEnabled=False, remotePort=8080, remotePasswordConfigured=False,
    remoteState='stopped', shortcutsPreset='Defaults')

class ControlsPreferences(QObject):
    valuesChanged = Signal(); lastErrorChanged = Signal(); busyChanged = Signal(); statusChanged = Signal()
    def __init__(self):
        super().__init__(); self.state = deepcopy(SEED); self.requests = []; self.reloads = 0
        self.reject = False; self._busy = False; self._error = ''; self._status = ''
        self._restart = False; self.editor = ''; self.unsupported = set()
        self._global_status = ''; self._global_warning = False
        self.independent = {k:v for k,v in SEED.items() if k in self.wheel_pairs}
    wheel_pairs = dict(remoteScrollWheel='scrollWheel', remoteHorizontalScroll='horizontalScroll', remoteReverseScroll='reverseScroll')
    values = Property('QVariantMap', lambda s: deepcopy(s.state), notify=valuesChanged)
    capabilities = Property('QVariantMap', lambda s: {key:key not in s.unsupported for key in list(SEED) + ['editShortcuts','resetShortcuts']}, notify=valuesChanged)
    supportNotes = Property('QVariantMap', lambda s: {key:'Owned Controls contract help' for key in ['skip','mouse','wheel','remote']}, constant=True)
    applyModes = Property('QVariantMap', lambda s: {key:'restart' if s.state['linkWheelSettings'] else 'live' for key in s.wheel_pairs}, notify=valuesChanged)
    choices = Property('QVariantMap', lambda s: {key:[dict(value=v,name=str(v)) for v in ([0,1,2] if key in ['singleClick','doubleClick'] else [-1,0,2,3])] for key in ['singleClick','doubleClick','scrollWheel','horizontalScroll','remoteScrollWheel','remoteHorizontalScroll']}, constant=True)
    lastError = Property(str, lambda s:s._error, notify=lastErrorChanged)
    status = Property(str, lambda s:s._status, notify=statusChanged)
    busy = Property(bool, lambda s:s._busy, notify=busyChanged)
    restartRequired = Property(bool, lambda s:s._restart, notify=valuesChanged)
    globalShortcutStatus = Property(str, lambda s:s._global_status, notify=valuesChanged)
    globalShortcutWarning = Property(bool, lambda s:s._global_warning, notify=valuesChanged)
    @Slot()
    def reload(self): self.reloads += 1
    @Slot(str, 'QVariant', result=bool)
    def setValue(self, key, value):
        self.requests.append([key,value]); self._error = ''
        valid = key in self.state and not self._busy and key not in self.unsupported
        if key.endswith('Skip'): valid &= type(value) in (int,float) and float(value).is_integer() and 0 <= value <= 86400
        if key == 'remotePort': valid &= type(value) in (int,float) and float(value).is_integer() and 1 <= value <= 65535
        if self.reject or not valid:
            self._error = 'owned checked-save rejection'; self.lastErrorChanged.emit(); return False
        self.lastErrorChanged.emit()
        if key == 'webRemoteEnabled' and value and not self.state['remotePasswordConfigured']:
            self.editor = 'remote'; self._busy = True; self.busyChanged.emit(); return True
        effective = self.wheel_pairs.get(key,key) if self.state['linkWheelSettings'] else key
        self.state[effective] = value
        if key in self.wheel_pairs and not self.state['linkWheelSettings']:self.independent[key] = value
        if self.state['linkWheelSettings']:
            for remote,physical in self.wheel_pairs.items():self.state[remote] = self.state[physical]
        elif key == 'linkWheelSettings':self.state.update(self.independent)
        self._restart |= effective in ['scrollWheel','horizontalScroll','reverseScroll','webRemoteEnabled','remotePort']
        self.valuesChanged.emit(); return True
    @Slot(result=bool)
    def editShortcuts(self):
        if self._busy:return False
        self.requests.append(['editShortcuts']); self.editor = 'shortcuts'; self._busy = True; self.busyChanged.emit(); return True
    @Slot(result=bool)
    def resetShortcuts(self):
        if self._busy:return False
        self.requests.append(['resetShortcuts']); self.state['shortcutsPreset'] = 'Defaults'; self._restart = True; self.valuesChanged.emit(); return True
    def finish(self, accepted=False):
        if accepted and self.editor == 'remote':
            self.state['webRemoteEnabled'] = True; self.state['remotePasswordConfigured'] = True; self._restart = True
        if accepted and self.editor == 'shortcuts':
            self.state['shortcutsPreset'] = 'Custom'; self._restart = True
        self.editor = ''; self._busy = False; self.valuesChanged.emit(); self.busyChanged.emit()
