# SPDX-License-Identifier: GPL-2.0-or-later
# Programmatic QML contract only; actual consumers use the native Qt fixture.
from copy import deepcopy
from PySide6.QtCore import QObject,Property,Signal,Slot
SEED=dict(saveRecentlyPlayed=True,rememberPlaybackPosition=True,historyExclusion='',internetMetadata=False,submitPlayedTracks=False,lastfmUsername='',lastfmPassword='',lastfmSubmissionUrl='post.audioscrobbler.com')
class PrivacyPreferences(QObject):
    valuesChanged=Signal();lastErrorChanged=Signal();busyChanged=Signal();statusChanged=Signal()
    def __init__(self):
        super().__init__();self.state=deepcopy(SEED);self.requests=[];self.reloads=0;self.reject=False;self._busy=False;self._error='';self._status=''
    values=Property('QVariantMap',lambda s:deepcopy(s.state),notify=valuesChanged)
    capabilities=Property('QVariantMap',lambda s:{k:k in ['saveRecentlyPlayed','rememberPlaybackPosition','historyExclusion','internetMetadata','clearHistory'] for k in list(SEED)+['clearHistory']},notify=valuesChanged)
    supportNotes=Property('QVariantMap',lambda s:{k:'Owned contract help for '+k for k in list(SEED)+['clearHistory']},constant=True)
    applyModes=Property('QVariantMap',lambda s:{'internetMetadata':'restart'},constant=True)
    lastError=Property(str,lambda s:s._error,notify=lastErrorChanged)
    status=Property(str,lambda s:s._status,notify=statusChanged)
    busy=Property(bool,lambda s:s._busy,notify=busyChanged)
    restartRequired=Property(bool,lambda s:s.state['internetMetadata'],notify=valuesChanged)
    @Slot()
    def reload(self):self.reloads+=1
    @Slot(str,'QVariant',result=bool)
    def setValue(self,key,value):
        self.requests.append([key,value]);self._error='owned write rejection' if self.reject else '';self.lastErrorChanged.emit()
        if self.reject:return False
        self.state[key]=value;self.valuesChanged.emit();return True
    @Slot(result=bool)
    def clearHistory(self):
        if self._busy:return False
        self._error='';self._status='';self._busy=True;self.lastErrorChanged.emit();self.statusChanged.emit();self.busyChanged.emit();return True
    def finish(self,success):
        self._busy=False;self._error='' if success else 'owned native clear rejection';self._status='History cleared.' if success else ''
        self.lastErrorChanged.emit();self.statusChanged.emit();self.busyChanged.emit()
