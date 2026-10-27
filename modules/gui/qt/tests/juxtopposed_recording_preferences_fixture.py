# SPDX-License-Identifier: GPL-2.0-or-later
# Owned presentation seam; native controller/consumer evidence is separate.
from copy import deepcopy
from PySide6.QtCore import QObject, Property, Signal, Slot, QTimer
SEED = dict(recordingDirectory='', nativeRecording=True, discoverDevices=True,
    castQuality=0, playLocally=False, includeAudio=True, includeVideo=True,
    includeSubtitles=True, encodingPreset='medium', tuning='', defaultRecordingDirectory='E:/Owned Videos')
class RecordingPreferences(QObject):
    valuesChanged=Signal();lastErrorChanged=Signal();busyChanged=Signal();advancedOptionsRequested=Signal()
    def __init__(self):
        super().__init__();self.state=deepcopy(SEED);self.requests=[];self.reloads=0
        self.reject=False;self.unsupported=set();self._error='';self._busy=False
    values=Property('QVariantMap',lambda s:deepcopy(s.state),notify=valuesChanged)
    capabilities=Property('QVariantMap',lambda s:{key:key not in s.unsupported for key in [*list(SEED)[:10],'browse','advanced']},notify=valuesChanged)
    choices=Property('QVariantMap',lambda s:{key:[dict(value=value,name=name) for value,name in entries] for key,entries in {
        'castQuality':[(0,'Automatic'),(1,'Medium'),(2,'Low'),(3,'Lowest')],
        'encodingPreset':[('medium','Medium'),('ultrafast','Ultrafast'),('fast','Fast')],
        'tuning':[('','None'),('film','Film'),('zerolatency','Zerolatency')]}.items()},constant=True)
    applyModes=Property('QVariantMap',lambda s:{key:'nextInput' if key in ['recordingDirectory','nativeRecording'] else 'live' if key=='discoverDevices' else 'nextJob' for key in SEED},constant=True)
    supportNotes=Property('QVariantMap',lambda s:{key:'Owned Recording help for '+key for key in ['recordingDirectory','nativeRecording','discoverDevices','castQuality','playLocally','streamTracks','encodingPreset','tuning','commandLineOverride']},constant=True)
    lastError=Property(str,lambda s:s._error,notify=lastErrorChanged)
    busy=Property(bool,lambda s:s._busy,notify=busyChanged)
    @Slot()
    def reload(self):self.reloads+=1
    @Slot(str,'QVariant',result=bool)
    def setValue(self,key,value):
        self.requests.append([key,value])
        if self.reject or key in self.unsupported or self._busy:
            self._error='owned checked-save rejection';self.lastErrorChanged.emit();return False
        self._error='';self.lastErrorChanged.emit();self.state[key]=value;self.valuesChanged.emit();return True
    @Slot(result=bool)
    def chooseRecordingDirectory(self):
        self.requests.append(['chooseRecordingDirectory']);self._busy=True;self.busyChanged.emit()
        QTimer.singleShot(450,self.finishPicker);return True
    def finishPicker(self):
        self._busy=False;self.busyChanged.emit();self.state['recordingDirectory']='E:/Owned Videos Ää';self.valuesChanged.emit()
    @Slot(result=bool)
    def openAdvanced(self):
        self.requests.append(['openAdvanced']);self.advancedOptionsRequested.emit();return True
