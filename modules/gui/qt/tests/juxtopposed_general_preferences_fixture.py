# SPDX-License-Identifier: GPL-2.0-or-later
"""Strict supplied General QObject contract; no native persistence substitute."""
from PySide6.QtCore import QObject, Property, Signal, Slot

class GeneralPreferences(QObject):
    valuesChanged = Signal()
    languagesChanged = Signal()
    lastErrorChanged = Signal()
    restartRequiredChanged = Signal()
    updatesAvailableChanged = Signal()
    def __init__(self):
        super().__init__()
        self._values = dict(language='en', theme=2, accentIndex=0, showDialogs=False,
            autoUpdates=False, developerMode=False, loggingEnabled=True, logFile='', logDetails=0)
        self._error, self._restart, self._updates = '', False, True
        self.requests, self.reloads, self.pickers = [], 0, 0
        self.fail_next = ''
    values = Property('QVariantMap', lambda s: s._values, notify=valuesChanged)
    languages = Property('QVariantList', lambda s: [dict(code='auto', name='System language'), dict(code='en', name='English'), dict(code='fi', name='Suomi'), dict(code='sv', name='Svenska')], notify=languagesChanged)
    lastError = Property(str, lambda s: s._error, notify=lastErrorChanged)
    restartRequired = Property(bool, lambda s: s._restart, notify=restartRequiredChanged)
    updatesAvailable = Property(bool, lambda s: s._updates, notify=updatesAvailableChanged)
    def error(self, value):
        self._error = value
        self.lastErrorChanged.emit()
    @Slot(str, 'QVariant', result=bool)
    def setValue(self, key, value):
        self.requests.append([key, value])
        legal = {'language': lambda v: isinstance(v, str) and v in ['auto', 'en', 'fi', 'sv'],
            'theme': lambda v: type(v) is int and 0 <= v <= 2,
            'accentIndex': lambda v: type(v) is int and 0 <= v <= 8,
            'logDetails': lambda v: type(v) is int and 0 <= v <= 2,
            'logFile': lambda v: isinstance(v, str),
            **{k: lambda v: type(v) is bool for k in ['showDialogs', 'autoUpdates', 'developerMode', 'loggingEnabled']}}
        assert key in legal and legal[key](value), (key, value, type(value))
        if self.fail_next:
            self.error(self.fail_next)
            self.fail_next = ''
            return False
        self._values = {**self._values, key: value}
        self.error('')
        if key not in ['theme', 'accentIndex']:
            self._restart = True
            self.restartRequiredChanged.emit()
        self.valuesChanged.emit()
        return True
    @Slot()
    def reload(self):
        self.reloads += 1
        self.error('')
        self.valuesChanged.emit()
    @Slot()
    def chooseLogFile(self):
        self.pickers += 1
    def patch(self, **values):
        self._values = {**self._values, **values}
        self.valuesChanged.emit()
