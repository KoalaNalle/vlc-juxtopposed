from pathlib import Path
from hashlib import sha256
import json

work = Path(__file__).resolve().parent
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
rows = []
for batch in ['ass-wasapi', 'formats']:
    before = read(work/('native-delay-'+batch+'-r3')/'processes.json')
    after = read(work/('native-delay-candidate-'+batch+'-r4')/'processes.json')
    before_cases = {case['case']: case for case in before['records']}
    for case in after['records']:
        old = before_cases[case['case']]
        assert old['normalExit'] and case['normalExit']
        assert old['result']['mediaSha256'] == case['result']['mediaSha256']
        old_rows = {row['step']: row for row in old['result']['observations']}
        new_rows = {row['step']: row for row in case['result']['observations']}
        comparisons = {name: dict(before=old_rows[name]['frameMarker'],
                                   candidate=new_rows[name]['frameMarker'])
                       for name in ['reference-first', 'reference-middle',
                                    'boundary-seek', 'boundary-settled']}
        rows.append(dict(case=case['case'], frames=comparisons,
                         pairedSeekFramesMatch=all(v['before'] == v['candidate']
                                                   for v in comparisons.values()),
                         candidateChecksPass=case['passed']))
result = dict(rows=rows, allPairedSeekFramesMatch=all(row['pairedSeekFramesMatch'] for row in rows),
              allCandidateChecksPass=all(row['candidateChecksPass'] for row in rows),
              producerSha256=sha256(Path(__file__).read_bytes()).hexdigest(),
              scope='Matched actual seek pictures; no absolute requested timestamp claim.',
              fullBetaAccepted=False)
out = work/'paired-frames-r4.json'
assert not out.exists()
out.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8', newline='\n')
print(json.dumps(result), flush=True)
