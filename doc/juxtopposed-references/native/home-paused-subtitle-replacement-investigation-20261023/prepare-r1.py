from pathlib import Path
import ast
work=Path(__file__).resolve().parent;repo=work.parent.parent
old=repo/'build-windows/beta-tracks-subtitles-20261023'
text=(old/'native-tracks-r15.py').read_text(encoding='utf-8')
text=text.replace("work=Path(__file__).resolve().parent;out=args.output;runtime=args.runtime", "work=repo/'build-windows/beta-tracks-subtitles-20261023';out=args.output;runtime=args.runtime")
text=text.replace("privateCoreSha256=digest(runtime/'libvlccore.dll')", "coreSha256=digest(runtime/'libvlccore.dll')")
text=text.replace("sourceManifest='build-windows/beta-tracks-subtitles-20261023/inputs-r14.json'", "sourceManifest='build-windows/beta-caption-layout-20261023/inputs-after-r1.json',runtimeRole='Ordinary Core000f / MKVf46 / GUId0ff; replay of previous track investigation after interval repair.'")
ast.parse(text)
target=work/'native-tracks-r1.py';assert not target.exists()
target.write_text(text,encoding='utf-8',newline='\n')
print('Prepared unchanged track workflow actions against the current ordinary runtime')
