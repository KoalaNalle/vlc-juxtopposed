from hashlib import file_digest
import json
from pathlib import Path
import shutil

work = Path(__file__).resolve().parent
repo = work.parents[1]
status = json.loads((work / 'build-r5-status.json').read_text())
assert status['status'] == 'terminal' and status['exitCode'] == 0 and status['sourceCleanAfter']
installed = work / 'build-r5/install/root/vlc-juxtopposed-build/win64/_win32'
runtime = work / 'runtime-r2'
assert not runtime.exists()
runtime.mkdir()
copied = {}

def digest(path):
    with path.open('rb') as stream:
        return file_digest(stream, 'sha256').hexdigest()

def copy(source, target):
    assert source.is_file(), source
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists():
        assert digest(source) == digest(target), ('Conflicting staged resources', source, target)
        return
    shutil.copyfile(source, target)
    copied[target.relative_to(runtime).as_posix()] = str(source)

def tree(source, target):
    assert source.is_dir(), source
    for path in source.rglob('*'):
        if path.is_file() and path.suffix not in ['.la', '.a']:
            copy(path, target / path.relative_to(source))

for name in ['vlc.exe', 'libvlc.dll', 'libvlccore.dll']:
    copy(installed / 'bin' / name, runtime / name)
for name in ['vlc-cache-gen.exe', 'vlc-preparser.exe']:
    copy(installed / 'libexec/vlc' / name, runtime / name)
tree(installed / 'lib/vlc/plugins', runtime / 'plugins')
for path in [installed / 'share/vlc/lua', installed / 'libexec/vlc/lua']:
    tree(path, runtime / 'lua')
tree(installed / 'share/vlc/skins2', runtime / 'skins')
tree(repo / 'share/hrtfs', runtime / 'hrtfs')
copy(repo / 'share/icons/vlc.ico', runtime / 'vlc.ico')
for source, name in [('AUTHORS', 'AUTHORS.txt'), ('COPYING', 'COPYING.txt'),
                     ('COPYING.LIB', 'COPYING.LIB.txt'), ('NEWS', 'NEWS.txt'),
                     ('README.md', 'README.txt'), ('THANKS', 'THANKS.txt'),
                     ('modules/gui/qt/pixmaps/juxtopposed/Inter-LICENSE.txt', 'Inter-LICENSE.txt'),
                     ('modules/gui/qt/pixmaps/juxtopposed/ATTRIBUTION.md', 'Juxtopposed-ATTRIBUTION.txt')]:
    copy(repo / source, runtime / name)
(runtime / 'portable').mkdir()
assert (runtime / 'plugins/gui/libqt_plugin.dll').is_file()
identities = {name: {'sha256': digest(runtime / name), 'bytes': (runtime / name).stat().st_size,
                     'source': source} for name, source in copied.items()}
fallback = json.loads((work / 'alpha-fallback.json').read_text())
old = Path(fallback['runtime'])
assert all((old / name).stat().st_size == value['bytes'] and digest(old / name) == value['sha256']
           for name, value in fallback['files'].items())
report = dict(sourceCommit=status['sourceCommit'], runtime=str(runtime), files=identities,
              alphaFallbackUnchanged=True, fileCount=len(identities),
              bytes=sum(x['bytes'] for x in identities.values()), nativeHooks=False,
              scope='Fresh install of the ordinary build, existing compatible contrib cache; no DLLs inherited from the alpha runtime.')
(work / 'runtime-r2-manifest.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps({key: report[key] for key in ['sourceCommit','fileCount','bytes','alphaFallbackUnchanged','runtime']}))
