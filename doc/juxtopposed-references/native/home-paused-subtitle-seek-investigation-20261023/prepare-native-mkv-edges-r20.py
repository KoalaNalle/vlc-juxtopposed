from pathlib import Path
import ast
work=Path(__file__).resolve().parent
text=(work/'native-tracks-r15.py').read_text(encoding='utf-8')
prefix=text[:text.index('options=')]
prefix=prefix.replace("parser.add_argument('--case',required=True)","parser.add_argument('--scenario',type=Path,required=True)")
start=prefix.index('cases={');end=prefix.index('\ndef write',start)
prefix=prefix[:start]+"cfg=json.loads(args.scenario.read_text(encoding='utf-8-sig'));mediaPath=work/cfg['media'];audio=cfg['audio']\n"+prefix[end:]
text=prefix+(work/'native-mkv-edges-r20-tail.py.txt').read_text(encoding='utf-8')
ast.parse(text)
target=work/'native-mkv-edges-r20.py';assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')
