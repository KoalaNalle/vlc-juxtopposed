# SPDX-License-Identifier: GPL-2.0-or-later
"""Explicit Library model boundary only; no native roots/history/IPC changes."""
from PySide6.QtCore import QObject,Property,Signal,Slot
SEED=dict(singleInstance=False,fileManagerSingleInstance=False,enqueueInSingleInstance=None,
    folderRecursion='expand',showHiddenFiles=False,metadataSource='local',useMediaLibrary=True)
class LibraryPreferences(QObject):
    valuesChanged=Signal();capabilitiesChanged=Signal();choicesChanged=Signal();lastErrorChanged=Signal();restartRequiredChanged=Signal()
    def __init__(self):
        super().__init__();self.state=dict(SEED);self.accepted=dict(instancePolicy=False,openingQueue=False,metadataFetch=False,hiddenAttributes=False)
        self.caps={};self.module=True;self.active=True;self.legacy=False;self.file_policy=-1;self.open_mode=-1
        self.options={'folderRecursion':[dict(value=v,name=n)for v,n in[('none','Do not expand folders automatically'),('collapse','Expand opened folders only'),('expand','Include subfolders')]],
            'metadataSource':[dict(value='local',name='From file and local cache'),dict(value='network',name='Also allow internet lookups')]}
        self.notes={
         'singleInstance':'Use one application instance, not a promise to own only one window.',
         'fileManagerSingleInstance':'The saved file-manager choice stays checked while its parent is off. Legacy file-only behavior is preserved until an explicit edit.',
         'enqueueInSingleInstance':'On appends and preserves playback. Off replaces the current queue and starts new files; named playlists are unchanged. Custom means legacy Play-without-replacing.',
         'folderRecursion':'None keeps an unopened folder as one queue item; Collapse expands the opened folder; Expand includes subfolders. Media-library indexing uses its existing explicit roots.',
         'showHiddenFiles':'Affects new VLC folder listings; does not change Explorer or remove existing indexed rows.',
         'metadataSource':'Shares the Privacy internet metadata flag. Local includes embedded metadata and local cache; Internet permits lookups, not listening-history submission.',
         'useMediaLibrary':'Changes the startup default after restart. Never clears records/history, adds folders or starts first-run code.'}
        self.modes={k:'restart'for k in SEED};self.modes.update(enqueueInSingleInstance='nextExternalOpen',showHiddenFiles='nextFolderOpen')
        self.error='';self.requests=[];self.reloads=0;self.fail_next='';self.external={};self.refresh_caps()
    values=Property('QVariantMap',lambda s:s.state,notify=valuesChanged)
    capabilities=Property('QVariantMap',lambda s:s.caps,notify=capabilitiesChanged)
    choices=Property('QVariantMap',lambda s:s.options,notify=choicesChanged)
    applyModes=Property('QVariantMap',lambda s:s.modes,constant=True)
    supportNotes=Property('QVariantMap',lambda s:s.notes,notify=valuesChanged)
    libraryAvailable=Property(bool,lambda s:s.active,notify=valuesChanged)
    legacyFileManagerOnly=Property(bool,lambda s:s.legacy,notify=valuesChanged)
    nativeFileManagerPolicy=Property(int,lambda s:s.file_policy,notify=valuesChanged)
    nativeOpenMode=Property(int,lambda s:s.open_mode,notify=valuesChanged)
    lastError=Property(str,lambda s:s.error,notify=lastErrorChanged)
    restartRequired=Property(bool,lambda s:any(s.state[k]!=SEED[k]for k in['singleInstance','fileManagerSingleInstance','folderRecursion','metadataSource','useMediaLibrary']),notify=restartRequiredChanged)
    def refresh_caps(self):
        a=self.accepted;self.caps=dict(singleInstance=a['instancePolicy'],fileManagerSingleInstance=a['instancePolicy']and self.state['singleInstance'],
          enqueueInSingleInstance=a['openingQueue']and self.state['singleInstance'],folderRecursion=bool(self.options['folderRecursion']),
          showHiddenFiles=a['hiddenAttributes'],metadataSource=a['metadataFetch'],useMediaLibrary=self.module);self.capabilitiesChanged.emit()
    def feedback(self,text):self.error=text;self.lastErrorChanged.emit()
    def patch(self,**values):self.state={**self.state,**values};self.valuesChanged.emit();self.refresh_caps();self.restartRequiredChanged.emit()
    @Slot()
    def reload(self):self.reloads+=1;self.patch(**self.external);self.external={};self.feedback('')
    @Slot(str,'QVariant',result=bool)
    def setValue(self,key,value):
        assert key in SEED and self.caps[key];self.requests.append([key,value])
        valid=type(value)is str and any(value==row['value']for row in self.options[key])if key in self.options else type(value)is bool
        if not valid:self.feedback('Choose a supported native library policy.');return False
        if self.fail_next:self.feedback(self.fail_next);self.fail_next='';return False
        if key=='singleInstance':self.file_policy=int(self.state['fileManagerSingleInstance']);self.legacy=False
        elif key=='fileManagerSingleInstance':self.file_policy=int(value);self.legacy=False
        elif key=='enqueueInSingleInstance':self.open_mode=int(value)
        self.patch(**{key:value});self.feedback('');return True
