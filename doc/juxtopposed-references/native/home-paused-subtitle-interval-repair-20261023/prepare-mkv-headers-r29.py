from pathlib import Path
from hashlib import sha256
import json, shutil

work = Path(__file__).resolve().parent
repo = work.parent.parent
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()
prior_path = work/'mkv-interval-r23d-source/source-record.json'
prior = read(prior_path)
assert all(digest(repo/p) == h for p,h in prior['files'].items())
root = work/'mkv-headers-r29-source'
assert not root.exists()
root.mkdir()
for phase in ['before', 'after']:
    target = root/phase
    target.mkdir()
    for name,h in prior['privateSources']['after'].items():
        original = work/'mkv-interval-r23d-source/after'/name
        assert digest(original) == h
        shutil.copyfile(original, target/name)

def change(name, old, new):
    path = root/'after'/name
    text = path.read_text(encoding='utf-8')
    assert text.count(old) == 1, (name, old, text.count(old))
    path.write_text(text.replace(old,new), encoding='utf-8', newline='\n')

change('matroska_segment.hpp', 'bool *, bool *, uint64_t *);',
       'bool *, bool *, uint64_t *, bool b_payload = true);')
change('matroska_segment.cpp', 'uint64_t *pi_duration )',
       'uint64_t *pi_duration, bool b_payload )')
change('matroska_segment.cpp', 'bool                 b_cluster_timecode;',
       'bool                 b_cluster_timecode;\n        bool                 b_payload;')
change('matroska_segment.cpp',
       'duration, duration_padding, *pb_key_picture, *pb_discardable_picture, true',
       'duration, duration_padding, *pb_key_picture, *pb_discardable_picture, true, b_payload')
for kind in ['ksblock', 'kblock']:
    change('matroska_segment.cpp',
           f'{kind}.ReadData( vars.obj->es.I_O() );',
           f'{kind}.ReadData( vars.obj->es.I_O(),\n                    vars.b_payload ? SCOPE_ALL_DATA : SCOPE_PARTIAL_DATA );')
change('matroska_segment.cpp', 'if ( vars.obj->ReadMaster( kadditions ) )',
       'if ( vars.b_payload && vars.obj->ReadMaster( kadditions ) )')
change('matroska_segment.cpp', 'if( p_track->fmt.i_codec == VLC_CODEC_THEORA )',
       'if( b_payload && p_track->fmt.i_codec == VLC_CODEC_THEORA )')
change('matroska_segment_seeker.cpp',
       'ms.BlockGet( block, simpleblock, additions, &key, &discardable, &duration )',
       'ms.BlockGet( block, simpleblock, additions, &key, &discardable, &duration, false )')

make = (work/'mkv-interval-r23d.mk').read_text(encoding='utf-8')
make = make.replace('owned-mkv-interval-r23d', 'owned-mkv-headers-r29')
# These ordinary-style names derive MODULE_STRING=mkv for every compilation unit.
make = make.replace('$(addsuffix .lo,$(MKV_INTERVAL_CXX_NAMES)))\n.PHONY',
                    '$(addsuffix .lo,$(addprefix libmkv_plugin_la-,$(MKV_INTERVAL_CXX_NAMES))))\n.PHONY')
make = make.replace('$(MKV_INTERVAL_OUT)/%.lo:', '$(MKV_INTERVAL_OUT)/libmkv_plugin_la-%.lo:')
assert '$(addprefix libmkv_plugin_la-,$(MKV_INTERVAL_CXX_NAMES))' in make
makepath = work/'mkv-headers-r29.mk'
assert not makepath.exists()
makepath.write_text(make, encoding='utf-8', newline='\n')
shell = (work/'build-mkv-interval-r23d.sh').read_text(encoding='utf-8')
shell = shell.replace('mkv-interval', 'mkv-headers').replace('r23d','r29')
# Preserve object provenance alongside the compiler dependencies.
shell = shell.replace('sha256sum "$work/mkv-headers-$phase-r29-bin/libmkv_plugin.dll"',
    'sha256sum "$work/mkv-headers-$phase-r29-bin/libmkv_plugin.dll"\n'
    '    sha256sum "$out/.libs/"*.o > "$work/mkv-headers-$phase-r29-bin/object-hashes.txt"')
script = work/'build-mkv-headers-r29.sh'
assert not script.exists()
script.write_text(shell, encoding='utf-8', newline='\n')
files = set(prior['files'])
for name in [Path(__file__).name,makepath.name,script.name]:
    files.add('build-windows/beta-tracks-subtitles-20261023/'+name)
hashes = {p:digest(repo/p) for p in sorted(files)}
snapshot = work/'source-mkv-headers-r29'
assert not snapshot.exists()
snapshot.mkdir()
for p,h in hashes.items():
    dest=snapshot/p
    dest.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(repo/p,dest)
    assert digest(dest)==h
record=dict(candidateOnly=True,repairAccepted=False,sourceCount=len(hashes),files=hashes,
    sourceSnapshot=str(snapshot),previousSourceRecordSha256=digest(prior_path),
    privateSources={phase:{p.name:digest(p) for p in sorted((root/phase).iterdir()) if p.is_file()} for phase in ['before','after']},
    changedPrivateFiles=['matroska_segment.hpp','matroska_segment.cpp','matroska_segment_seeker.cpp'],
    scope='Private header-only subtitle-prefix scan versus the full-payload interval candidate. All sixteen C++ units rebuilt in both phases with ordinary-style object names. Normal playback retains full block payloads; production source/runtime untouched.')
(root/'source-record.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(sourceCount=len(hashes),candidateOnly=True,privateRoot=str(root))))
