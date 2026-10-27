# SPDX-License-Identifier: GPL-2.0-or-later
# Explicit owned presentation seam; native controller/consumer evidence is separate.
from copy import deepcopy
from PySide6.QtCore import QObject, Property, Signal, Slot, QTimer
SEED=dict(hardwareDecoder='any',postprocessing=-1,loopFilter=0,decodingThreads=0,
    dropLateFrames=True,realTimePriority=False,processPriority=0,encoder='any',decoder='any',
    clockSync=-1,clockJitter=5000,clockAverage=40,actualPriority='Normal',prioritySavedRealTime=False)
class PerformancePreferences(QObject):
    valuesChanged=Signal();lastErrorChanged=Signal();statusChanged=Signal();busyChanged=Signal()
    def __init__(self):
        super().__init__();self.state=deepcopy(SEED);self.requests=[];self.reloads=0
        self.reject=False;self.unsupported=set();self._error='';self._status='';self._busy=False
    values=Property('QVariantMap',lambda s:deepcopy(s.state),notify=valuesChanged)
    capabilities=Property('QVariantMap',lambda s:{k:k not in s.unsupported for k in [*list(SEED)[:12],'rescan','advanced']},notify=valuesChanged)
    choices=Property('QVariantMap',lambda s:{k:[dict(value=v,name=n) for v,n in choices] for k,choices in {
        'hardwareDecoder':[('any','Automatic'),('d3d11_filters','Direct3D 11'),('off','Off')],
        'postprocessing':[(-1,'Off'),*[(i,'Level '+str(i)) for i in range(1,7)]],
        'loopFilter':[(0,'None'),(1,'Non-reference'),(2,'B-frames'),(3,'Non-key'),(4,'All')],
        'encoder':[('any','Automatic'),('flac','FLAC')],'decoder':[('any','Automatic'),('avcodec','FFmpeg')],
        'clockSync':[(-1,'Default'),(0,'Disable'),(1,'Enable')]}.items()},constant=True)
    applyModes=Property('QVariantMap',lambda s:{k:'live' if k in ['processPriority','realTimePriority'] else 'restart' if k in ['hardwareDecoder','postprocessing','dropLateFrames','rescan'] else 'nextJob' if k=='encoder' else 'nextInput' for k in [*SEED,'rescan']},constant=True)
    supportNotes=Property('QVariantMap',lambda s:{k:'Owned Performance help for '+k for k in ['hardwareDecoder','postprocessing','loopFilter','decodingThreads','dropLateFrames','priority','encoder','decoder','clock','rescan','commandLineOverride']},constant=True)
    lastError=Property(str,lambda s:s._error,notify=lastErrorChanged)
    status=Property(str,lambda s:s._status,notify=statusChanged)
    busy=Property(bool,lambda s:s._busy,notify=busyChanged)
    @Slot()
    def reload(self):self.reloads+=1
    @Slot(str,'QVariant',result=bool)
    def setValue(self,key,value):
        self.requests.append([key,value]);valid=key not in self.unsupported and not self._busy
        limits={'decodingThreads':(0,32),'clockJitter':(0,60000),'clockAverage':(0,1000),'processPriority':(-2,2)}
        if key in limits:valid &= type(value) in (int,float) and float(value).is_integer() and limits[key][0]<=value<=limits[key][1]
        if self.reject or not valid:
            self._error='owned checked-save rejection';self.lastErrorChanged.emit();return False
        self._error='';self._status='';self.lastErrorChanged.emit();self.statusChanged.emit()
        self.state[key]=value
        if key=='realTimePriority':self.state['prioritySavedRealTime']=bool(value)
        self.valuesChanged.emit();return True
    @Slot(result=bool)
    def rescanPlugins(self):
        self.requests.append(['rescanPlugins'])
        if self._busy or self.reject:
            self._error='owned rescan rejection';self.lastErrorChanged.emit();return False
        self._busy=True;self._status='Rescanning plugins…';self.busyChanged.emit();self.statusChanged.emit()
        QTimer.singleShot(700,self.finishScan);return True
    def finishScan(self):
        self._busy=False;self._status='Plugins rescanned. Restart VLC to use the updated cache.'
        self.busyChanged.emit();self.statusChanged.emit()
