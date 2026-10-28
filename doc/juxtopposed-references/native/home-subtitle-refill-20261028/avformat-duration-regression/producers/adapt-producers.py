"""One-use textual preparation only; never launches a compiler, VLC or decoder."""
from pathlib import Path
import ast
root=Path(__file__).resolve().parent
def changed(name,edits):
    p=root/name;s=p.read_text()
    for old,new in edits:
        assert old in s,(name,old[:100]);s=s.replace(old,new,1)
    ast.parse(s,filename=name);p.write_text(s,newline='\n')
changed('run-long.py',[
    ("choices=('control', 'candidate', 'reversed', 'header-error', 'writer-error')", "choices=('original', 'reversed')"),
    ("prep / 'runtime-config.json'", "prep / 'regression-config.json'"),
    ("(prep / 'runtime-config.json')", "(prep / 'regression-config.json')"),
    ("fixture_info = config['alternateFixtures']['reversed'] if args.case == 'reversed' else config", "fixture_info = config['longCases'][args.case]"),
    ("if args.case != 'control':", "if True:  # Both duration-regression cases use the new immutable plugin."),
    ("expected_plugin = config['controlPluginSha256'] if args.case == 'control' else config['candidatePluginSha256']", "expected_plugin = config['candidatePluginSha256']"),
    ("if args.case in ('control', 'candidate', 'reversed'):", "if args.case in ('original', 'reversed'):"),
    ("safe_digest(prep / 'runtime-config.json', errors)", "safe_digest(prep / 'regression-config.json', errors)"),
    ("assert len(registrations) == len(removals) == len(record['esDeletionOrder']) == 4", "assert len(registrations) == len(removals) == len(record['esDeletionOrder']) == 4\n        record['avDeletionOrder'] = [row['codec'] for row in record['esDeletionOrder'] if row['codec'] in ('h264', 'flac')]\n        assert record['avDeletionOrder'] == fixture_info['expectedAvDeletionOrder']"),
])
changed('analyze-long.py',[
    ("assert variant in ('control', 'candidate', 'reversed')", "assert variant in ('original', 'reversed')"),
    ("prep / 'runtime-config.json'", "prep / 'regression-config.json'"),
    ("(prep / 'runtime-config.json')", "(prep / 'regression-config.json')"),
    ("'status': 'expected-control-tail-loss' if variant == 'control' else 'passed'", "'status': 'passed'"),
    ("(strict_audio and len(packets) == 626 and output_samples == 2880528\n             if variant != 'control' else known_tail_loss)", "(strict_audio and len(packets) == 626 and output_samples == 2880528)"),
    ("assert all(value for key, value in record.items() if key.endswith('Unchanged'))", "assert all(value for key, value in record.items() if key.endswith('Unchanged'))\n    assert all(sha256(Path(p).read_bytes()).hexdigest() == h for p,h in config['toolsSha256'].items())\n    assert record['avDeletionOrder'] == config['longCases'][variant]['expectedAvDeletionOrder']"),
    ("'producerSha256': sha256(Path(__file__).read_bytes()).hexdigest(),", "'producerSha256': sha256(Path(__file__).read_bytes()).hexdigest(), 'avDeletionOrder': record['avDeletionOrder'],"),
    ("    return result\n\n\n", "    result['toolsUnchanged'] = all(sha256(Path(p).read_bytes()).hexdigest() == h for p,h in config['toolsSha256'].items())\n    assert result['toolsUnchanged']\n    return result\n\n\n"),
])
changed('run-short.py',[
    ("parser=argparse.ArgumentParser(); parser.add_argument('--execute-frozen-smoke',action='store_true',required=True)", "parser=argparse.ArgumentParser(); parser.add_argument('--execute-frozen-smoke',action='store_true',required=True); parser.add_argument('--case',choices=('mp4','mpegts','short-matroska'),required=True)"),
    ("config_digest=digest(ROOT/'smoke-config.json'); cfg=json.loads((ROOT/'smoke-config.json').read_text())", "config_digest=digest(ROOT/'regression-config.json'); cfg=json.loads((ROOT/'regression-config.json').read_text())"),
    ("out=ROOT/'runs';", "out=ROOT/('short-'+args.case+'-r1');"),
    ("fixtures=cfg['cases'];", "fixtures=[cfg['shortCases'][args.case]];"),
    ("'Generated small compatible MP4/MPEGTS and120ms Matroska real-writer smoke; not ordinaryUI/finalpackage/fullrecording acceptance.'", "'Strict generated MP4 defaultpresentation/MPEGTS/120msMatroska duration regression; not ordinaryUI/finalpackage acceptance.'"),
    ("digest(ROOT.parent/'runtime-config.json')==cfg['parentConfigSha256']", "digest(REPO/cfg['priorRuntimeConfig'])==cfg['priorRuntimeConfigSha256']"),
    ("record['storageBeforeStage']=storage_state(REPO,cfg['storageBudget'])", "record['storageBeforeStage']=storage_state(REPO,cfg['shortStorageBudget'])"),
    ("for variant in ('control','candidate'):", "for variant in ('candidate',):"),
    ("        a,b=case_summary['variants']; case_summary['controlComparison']={\n            'videoExact':a['decoded']['videoSha256']==b['decoded']['videoSha256'],\n            'audioExact':a['decoded']['audioSha256']==b['decoded']['audioSha256'],\n            'audioSamplesControl':a['decoded']['audioSamples'],'audioSamplesCandidate':b['decoded']['audioSamples']}\n        assert b['comparisonToSource']['videoFramesEqual'] and b['comparisonToSource']['videoExact']\n        # AAC/MP2 priming and container trimming are qualified via source and matched control.\n        assert b['decoded']['audioSamples']>=a['decoded']['audioSamples']\n        if case['name']=='short-matroska': assert b['comparisonToSource']['audioExact']", "        b=case_summary['variants'][0]\n        assert b['comparisonToSource']['videoFramesEqual'] and b['comparisonToSource']['videoExact']\n        assert b['comparisonToSource']['audioExact'], 'Default presentation must preserve ALL source PCM'\n        assert b['comparisonToSource']['packetCountsEqual']['audio']\n        assert b['comparisonToSource']['packetPayloadsEqual']['audio']\n        case_summary['accepted']=True"),
    ("safe_digest(ROOT/'smoke-config.json',errors)", "safe_digest(ROOT/'regression-config.json',errors)"),
    ("safe_digest(ROOT.parent/'runtime-config.json',errors)==cfg['parentConfigSha256']", "safe_digest(REPO/cfg['priorRuntimeConfig'],errors)==cfg['priorRuntimeConfigSha256']"),
])
print('Prepared and AST-validated three regression producers; no jobs launched')
