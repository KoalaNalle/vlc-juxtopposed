# SPDX-License-Identifier: GPL-2.0-or-later
# Owned UI contract seam; native controller and consumer tests are separate.
from copy import deepcopy
from urllib.parse import urlsplit
from PySide6.QtCore import QObject, Property, Signal, Slot

SEED = dict(buffering=0, localFiles=1000, discs=300, networkStreams=1000,
    liveCapture=300, httpProxy='', proxyAddressCustom=False,
    proxyPasswordConfigured=False, systemTrust=True, cipherPreference='NORMAL')

class NetworkPreferences(QObject):
    valuesChanged=Signal();lastErrorChanged=Signal();statusChanged=Signal();advancedOptionsRequested=Signal()
    def __init__(self):
        super().__init__();self.state=deepcopy(SEED);self.requests=[];self.reloads=0
        self.reject=False;self.unsupported=set();self._error='';self._status='';self._password=''
    values=Property('QVariantMap',lambda s:deepcopy(s.state),notify=valuesChanged)
    def caps(self):
        result={key:key not in self.unsupported for key in ['buffering','localFiles','discs','networkStreams','liveCapture','httpProxy','httpProxyPassword','systemTrust','cipherPreference','advanced']}
        for key in ['localFiles','discs','networkStreams','liveCapture']:result[key]&=self.state['buffering']==0
        result['httpProxyPassword']&=bool(urlsplit(self.state['httpProxy']).username)
        return result
    capabilities=Property('QVariantMap',caps,notify=valuesChanged)
    choices=Property('QVariantMap',lambda s:{'buffering':[dict(value=v,name=n) for v,n in [(0,'Custom'),(100,'Lowest latency'),(200,'Low latency'),(300,'Normal'),(500,'High latency'),(1000,'Higher latency')]],
        'cipherPreference':[dict(value=v,name=n) for v,n in [('PERFORMANCE','Performance'),('NORMAL','Normal'),('SECURE128','Secure 128-bit'),('SECURE256','Secure 256-bit')]]},constant=True)
    applyModes=Property('QVariantMap',lambda s:{k:'nextMedia' for k in SEED},constant=True)
    supportNotes=Property('QVariantMap',lambda s:{k:'Owned Network help for '+k for k in ['buffering','proxy','tls']},constant=True)
    lastError=Property(str,lambda s:s._error,notify=lastErrorChanged)
    status=Property(str,lambda s:s._status,notify=statusChanged)
    @Slot()
    def reload(self):self.reloads+=1
    @Slot(str,'QVariant',result=bool)
    def setValue(self,key,value):
        self.requests.append([key,value]);valid=self.caps().get(key,False)
        if key in ['localFiles','discs','networkStreams','liveCapture']:
            valid &= type(value) in (int,float) and float(value).is_integer() and 0<=value<=60000
        if self.reject or not valid:
            self._error='owned checked-save rejection';self.lastErrorChanged.emit();return False
        self._error='';self.lastErrorChanged.emit()
        if key=='buffering':
            if value:
                for name,numerator in [('localFiles',3),('discs',3),('networkStreams',10),('liveCapture',3)]:self.state[name]=value*numerator//3
        if key=='httpProxyPassword':
            self._password=value;self.state['proxyPasswordConfigured']=bool(value)
        else:self.state[key]=value
        if key=='httpProxy':
            self.state['proxyAddressCustom']=False;self._password='';self.state['proxyPasswordConfigured']=False
        self.valuesChanged.emit();return True
    @Slot(result=bool)
    def openAdvanced(self):
        self.requests.append(['openAdvanced'])
        if not self.caps()['advanced']:return False
        self.advancedOptionsRequested.emit();return True
