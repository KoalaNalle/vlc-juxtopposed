from pathlib import Path
import ast
work=Path(__file__).resolve().parent;repo=work.parent.parent
old=repo/'build-windows/beta-tracks-subtitles-20261023'
text=(old/'run-ordinary-ui-r37.py').read_text(encoding='utf-8')
text=text.replace("repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent", "repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent;old=repo/'build-windows/beta-tracks-subtitles-20261023'")
text=text.replace("manifest=work/'inputs-r35c.json'", "manifest=work/'inputs-ordinary-r3.json'")
text=text.replace('len(inputs)==2210','len(inputs)==2232')
text=text.replace("media=work/'media-r1'", "media=old/'media-r1'")
text=text.replace("work/'media-generator-smoke-r35/colored-overlap.mkv'", "old/'media-generator-smoke-r35/colored-long.mkv'")
text=text.replace("out=work/'ordinary-ui-r37'", "out=work/'ordinary-ui-r4'")
text=text.replace("base=work/'ordinary-ui-r14/runtime'", "base=old/'ordinary-ui-r37/runtime'")
before="mkv=runtime/'plugins/demux/libmkv_plugin.dll';mkv.unlink();shutil.copyfile(work/'ordinary-mkv-r35c-bin/libmkv_plugin.dll',mkv)"
after="gui=runtime/'plugins/gui/libqt_plugin.dll';gui.unlink();shutil.copyfile(work/'ordinary-r3-bin/libqt_plugin.dll',gui)"
assert text.count(before)==1;text=text.replace(before,after)
text=text.replace("sourceManifest='build-windows/beta-tracks-subtitles-20261023/inputs-r35c.json'", "sourceManifest='build-windows/beta-caption-accessibility-20261023/inputs-ordinary-r3.json'")
text=text.replace("scope='Ordinary Windows11 audio/subtitle track and cue workflows using generated media; no physical audibility, real-service, design/DPI or whole-beta acceptance.'", "scope='Ordinary Windows11 progress value-setting through desktop accessibility, paused coarse frame colors, keyboard/pointer compatibility and resume. No private GUI, exact frame PTS, screen-reader speech, caption clearance or full-beta acceptance.'")
text=text.replace("overlapMediaSha256=extraMediaHash", "coloredMediaSha256=extraMediaHash")
ast.parse(text)
target=work/'run-ordinary-ui-r4.py';assert not target.exists()
target.write_text(text,encoding='utf-8',newline='\n')
