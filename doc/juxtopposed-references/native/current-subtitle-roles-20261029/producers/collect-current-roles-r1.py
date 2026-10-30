"""Preserve the identified regression client's new-Core results, with explicit scope."""
import hashlib
import json
from pathlib import Path
import re
import shutil

ROOT=Path(__file__).resolve().parents[2]
RUN=ROOT/'build-windows/beta-subtitle-roles-20261029/current-r1'
OUT=ROOT/'doc/juxtopposed-references/native/current-subtitle-roles-20261029'


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    assert not OUT.exists()
    data=json.loads((RUN/'verification.json').read_text(encoding='utf-8'))
    assert data['passed'] and len(data['cases'])==8 and data['inputsUnchanged'] and data['runtimeUnchanged'] and data['fallbackUnchanged']
    diagnostics=[]
    for i,row in enumerate(data['cases']):
        lines=(RUN/f'case-{i}/stderr.txt').read_text(encoding='utf-8',errors='replace').splitlines()
        errors=[s for s in lines if re.search(r'\berror:|Assertion failed|QFATAL|QWARN|Binding loop|ReferenceError|TypeError',s)]
        assert len(errors)==1 and errors[0].endswith('main window error: window closed')
        diagnostics.append(dict(selector=row['selector'],teardownWindowClosed=1,otherErrors=[]))
    OUT.mkdir(parents=True)
    def copy(source,target):
        dest=OUT/target;dest.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(source,dest);assert sha(source)==sha(dest)
    copy(RUN/'verification.json',Path('verification.json'))
    for i in range(8):
        for name in ['result.xml','result.txt']:
            copy(RUN/f'case-{i}'/name,Path(f'case-{i}')/name)
        for p in sorted((RUN/f'case-{i}/snapshots').glob('*.png')):
            copy(p,Path(f'case-{i}/snapshots')/p.name)
    copy(Path(__file__).with_name('run-current-subtitle-roles-r1.py'),Path('producers/run-current-subtitle-roles-r1.py'))
    copy(Path(__file__),Path('producers')/Path(__file__).name)
    copy(ROOT/'modules/gui/qt/tests/test_player_subtitle_delay.cpp',Path('test_player_subtitle_delay.cpp'))
    (OUT/'diagnostics.json').write_text(json.dumps(diagnostics,indent=2)+'\n')
    (OUT/'README.md').write_text('''# Existing subtitle regression client on current Core — 29 October 2026

Eight selected processes pass, with actual Core `4a2865915e34`, current SRT/ASS,
WebVTT38 and TTML9d modules staged from ordinary R18. Exact identities, generated
fixture hashes, commands, selected functions, snapshots and normal child exits
are recorded. All input/runtime/fallback guards pass. Each process emits one
known native test-window-close teardown diagnostic and no other captured errors.
Raw stderr remains ignored locally; the scoped classification is included here.

Cases cover category-relative changes/Reset notifications; large own and inherited
secondary delays with held-frame/resume checks; held and first-playing FPS changes
preserving an embedded secondary caption; opposing external delays where the
secondary has ended; estimator directions and cancellation; and cancellation
without publishing an adjustment. No function was skipped.

This reuses regression executable `64846efd9fab` from `test-r23`, with its exact
test source `80838e7b8198`. The existing compiled PlayerController client is not
a fresh ordinary Qt build or a full current controller-source-closure audit.
Ordinary Qt7c7 is staged but this client owns its compiled controller. These new
results qualify this identified client interacting with current Core/codecs;
they do not inherit the older CoreF8 results, establish current ordinary GUI
role behavior, or close beta/design/DPI/package gates. The regular R18 GUI's
single-track tick/FPS/Reset evidence is recorded separately. The next check is
the visible ordinary multi-track journey.

Only generated media/captures and scoped reports are included. The executable,
runtime, profiles and raw logs stay ignored. The frozen producer requires fresh
outputs and revalidated inventories before another run. Do not package this
private regression client in an ordinary release.
''',encoding='utf-8')
    files={str(p.relative_to(OUT)).replace('\\','/'):sha(p) for p in sorted(OUT.rglob('*')) if p.is_file()}
    (OUT/'manifest.json').write_text(json.dumps(files,indent=2)+'\n')
    assert all(sha(OUT/p)==h for p,h in files.items())
    print(json.dumps(dict(passed=True,files=len(files))))


if __name__=='__main__':main()
