from pathlib import Path
import ast
work=Path(__file__).resolve().parent
basic=(work/'run-native-mkv-interval-r25.py').read_text(encoding='utf-8')
parent=(work/'run-native-mkv-cost-r28.py').read_text(encoding='utf-8')
basic_cases=basic[basic.index('configs=['):basic.index('source=read(')]
start=parent.index("for name in ['cbr-overlap'")
end=parent.index('source=read(',start)
cost=parent[start:end].replace('configs=[','cost_configs=[')
parent=parent[:start]+basic_cases+cost+'configs += cost_configs\n'+parent[end:]
parent=parent.replace('mkv-interval-r23d-source','mkv-headers-r29-source')
parent=parent.replace('source-mkv-interval-r23d','source-mkv-headers-r29')
parent=parent.replace("source['sourceCount']==2196","source['sourceCount']==2199")
parent=parent.replace('build-mkv-interval-r23d-exit','build-mkv-headers-r29-exit')
parent=parent.replace("'mkv-interval-'+phase+'-r23d-bin/", "'mkv-headers-'+phase+'-r29-bin/")
parent=parent.replace("root=work/'native-mkv-cost-r28'", "root=work/'native-mkv-headers-r30'")
# Both phases already contain the corrected interval algorithm. This comparison
# isolates header-only indexing; neither phase is an unfixed negative control.
parent=parent.replace("write(root/'comparison.json',dict(passed=False,candidateOnly=True,repairAccepted=False,allProcessesNormal=all(r['normalExit'] for r in records),records=records))",
    "write(root/'comparison.json',dict(passed=all(r['passed'] and r['normalExit'] and r['runtimeUnchanged'] and r['sourceInputsUnchanged'] and r['producerUnchanged'] and r['fixturesUnchanged'] for r in records),candidateOnly=True,repairAccepted=False,control='Full-payload interval scan versus header-only interval scan; both are positive caption controls.',allProcessesNormal=all(r['normalExit'] for r in records),records=records))")
ast.parse(parent)
assert 'r23d' not in parent and "root=work/'native-mkv-headers-r30'" in parent
target=work/'run-native-mkv-headers-r30.py'
assert not target.exists()
target.write_text(parent,encoding='utf-8',newline='\n')
