from pathlib import Path
from hashlib import file_digest
import json
import re
import shutil

root=Path(__file__).resolve().parent
repo=root.parents[1]
out=repo/'doc/juxtopposed-references/native/home-video-sections-20261028/ordinary-r1'
out.mkdir(exist_ok=False)
selection={
    'ordinary_home_r1':['04-home-after-natural-end','07-unfinished-replay-moves-to-continue','11-continue-see-all',
        '13-continue-typed-search-empty','16-continue-see-all-resumed','18-watch-again-see-all',
        '22-home-retains-paused-player','23-two-sections-before-restart'],
    'ordinary_home_restart_r1':['01-restored-both-sections','03-continue-cold-restored','05-watch-again-always-paused']}
for run,names in selection.items():
    dest=out/run;dest.mkdir()
    for filename in ('verification.json','observed-exit.json'):
        source=root/run/filename
        if source.exists(): shutil.copyfile(source,dest/filename)
    for name in names:
        for source in (root/run/'observations').glob(name+'*'):
            shutil.copyfile(source,dest/source.name)
    text='\n'.join((root/run/n).read_text(errors='replace') for n in ('vlc.log','stderr.txt'))
    diagnostics=[line for line in text.splitlines() if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError|is not a type|Cannot assign',line)]
    (dest/'diagnostics.json').write_text(json.dumps(dict(moduleCounts=re.findall(r'plug-ins loaded: (\d+) modules',text),diagnostics=diagnostics),indent=2)+'\n')
for filename in ('run-home-ui-r1.py','observe-home-exit.py','run-home-restart-r1.py','collect-home-desktop.py'):
    shutil.copyfile(root/filename,out/filename)
identities={}
for path in out.rglob('*'):
    if path.is_file():
        with path.open('rb') as stream: identities[path.relative_to(out).as_posix()]=file_digest(stream,'sha256').hexdigest()
(out/'artifact-sha256.json').write_text(json.dumps(identities,indent=2)+'\n')
print('Copied',len(identities),'owned evidence files')
