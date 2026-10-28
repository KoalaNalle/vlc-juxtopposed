"""Frozen read-only guards, hardlink-only staging and deadline/budget helpers."""
from pathlib import Path
from hashlib import sha256
from datetime import datetime, timezone
import json, os, shutil, shlex

DEADLINE = datetime.fromisoformat('2026-10-28T23:35:00+02:00')
PEAK_COMPONENTS = {'objects':80*2**20,'strippedExe':80*2**20,'linkTemporary':80*2**20,
    'generatedAndDependencies':16*2**20,'logsAndReceipts':32*2**20,
    'isolatedDatabase':64*2**20,'runtimeMetadataAndCache':32*2**20}

def digest(path):
    result=sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''): result.update(block)
    return result.hexdigest()

def save(path,value):
    Path(path).write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8',newline='\n')

def seconds_left(): return (DEADLINE-datetime.now(timezone.utc)).total_seconds()

def storage(path, additional=None):
    state=shutil.disk_usage(path); floor=(state.total+4)//5
    peak=sum(PEAK_COMPONENTS.values()) if additional is None else additional
    return dict(totalBytes=state.total,freeBytes=state.free,minimumFreeBytes=floor,
        before80HeadroomBytes=state.free-floor,peakAdditionalBytes=peak,
        fits=state.free-floor>=peak,usedPercent=100*(1-state.free/state.total))

def check_map(root,identities):
    errors=[]
    for name,expected in identities.items():
        try:
            actual=digest(Path(root)/name)
            if actual!=expected: errors.append(name)
        except OSError: errors.append(name)
    return errors

def dependencies(files,cwd):
    result=set()
    for path in files:
        text=Path(path).read_text().replace('\\\n',' ')
        # The first rule is the actual compiler include graph; -MP adds empty
        # phony header rules afterward, which are deliberately ignored.
        rule=text.splitlines()[0]
        assert ': ' in rule or ':\t' in rule, str(path)
        words=shlex.split(rule.split(':',1)[1])
        result.update(str((Path(cwd)/name).resolve()) for name in words)
    return result

def stage_subset(source,destination,identities):
    source,destination=Path(source),Path(destination)
    assert source.drive.upper()==destination.drive.upper() and not destination.exists()
    assert not check_map(source,identities)
    destination.mkdir()
    count=0
    for name in sorted(identities):
        relative=Path(name)
        assert not relative.is_absolute() and '..' not in relative.parts
        if relative.parts[0]=='portable' or relative.name=='plugins.dat': continue
        original=source/relative
        assert original.is_file() and not original.is_symlink()
        assert not getattr(original.lstat(),'st_file_attributes',0)&1024
        target=destination/relative; target.parent.mkdir(parents=True,exist_ok=True)
        os.link(original,target) # EMLINK or any other error aborts: never copy.
        count+=1
    (destination/'portable').mkdir()
    return dict(hardLinkedFiles=count,copiedLinkLimitFiles=0,policy='hardlink-only-fail-closed')
