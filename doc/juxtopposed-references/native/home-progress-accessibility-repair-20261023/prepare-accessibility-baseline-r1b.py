from pathlib import Path
from hashlib import sha256
import ast,json,shutil
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
record=read(work/'native-before-r1/process.json')
assert record['normalExit'] and not record['passed'] and record['journey']['error']=='paused video picture is ready'
prior=read(work/'inputs-before-r1.json')['files'];assert all(digest(repo/p)==h for p,h in prior.items())
source=work/'seek_accessibility_journey.cpp'
text=source.read_text(encoding='utf-8')
old='''    input_item_AddOption(media,":start-paused",VLC_INPUT_OPTION_TRUSTED);
'''
assert text.count(old)==1;text=text.replace(old,'')
text=text.replace('"native player opens paused synthetic media"','"native player opens synthetic media"')
old='''    until([]{return state()==VLC_PLAYER_STATE_PAUSED&&pictures()>0&&player()->isSeekable();},"paused video picture is ready",15000);
    QTest::qWait(350);'''
new='''    until([]{return state()==VLC_PLAYER_STATE_PLAYING&&pictures()>0&&player()->isSeekable()&&nativeTime()>VLC_TICK_FROM_MS(300);},"normal video picture and published player statistics are ready",15000);
    vlc_player_Lock(native);vlc_player_Pause(native);vlc_player_Unlock(native);
    until([]{return state()==VLC_PLAYER_STATE_PAUSED;},"native playback pauses before accessibility checks");
    QTest::qWait(350);'''
assert text.count(old)==1;text=text.replace(old,new)
old='''    checks.append(QJsonObject{{"name",name},{"passed",good}});save();'''
new='''    checks.append(QJsonObject{{"name",name},{"passed",good}});save();'''
# The earlier full source is retained in source-before-r1; this correction
# changes only the private setup, not the production SliderBar.
source.write_text(text,encoding='utf-8',newline='\n')
shell=(work/'build-accessibility-r1.sh').read_text(encoding='utf-8').replace('r1','r1b')
path=work/'build-accessibility-r1b.sh';assert not path.exists();path.write_text(shell,encoding='utf-8',newline='\n')
parent=(work/'run-accessibility-r1.py').read_text(encoding='utf-8')
parent=parent.replace('inputs-before-r1.json','inputs-before-r1b.json').replace('inputs-after-r2.json','inputs-after-r2b.json')
parent=parent.replace("+'-r1'","+'-r1b'").replace("+'-r1-exit.txt'","+'-r1b-exit.txt'")
parent=parent.replace("+'-r1-bin/","+'-r1b-bin/")
ast.parse(parent)
path=work/'run-accessibility-r1b.py';assert not path.exists();path.write_text(parent,encoding='utf-8',newline='\n')
files=set(prior)
files.update('build-windows/beta-caption-accessibility-20261023/'+p for p in [Path(__file__).name,'build-accessibility-r1b.sh','run-accessibility-r1.py','run-accessibility-r1b.py'])
hashes={p:digest(repo/p) for p in sorted(files)}
root=work/'source-before-r1b';assert not root.exists();root.mkdir()
for p,h in hashes.items():
    target=root/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==h
output=dict(sourceCount=len(hashes),files=hashes,sourceSnapshot=str(root),phase='before',fullBetaAccepted=False,
    productionSourceChanged=False,previousPreflightResultSha256=digest(work/'native-before-r1/process.json'),
    correction='Initial private fixture awaited the player statistics counter at cold pause and never reached its accessibility test. New fixture follows established seek-test setup: play until a picture/statistics event, then pause. Production SliderBar unchanged; original run and producer retained.')
(work/'inputs-before-r1b.json').write_text(json.dumps(output,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(sourceCount=len(hashes),productionSourceChanged=False)))
