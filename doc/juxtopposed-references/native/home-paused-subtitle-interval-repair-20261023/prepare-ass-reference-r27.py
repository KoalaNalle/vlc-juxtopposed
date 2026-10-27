from pathlib import Path
import ast
work=Path(__file__).resolve().parent
parent=(work/'run-native-overlap-mask-reference-r24.py').read_text(encoding='utf-8')
parent=parent.replace("media='media-r21/colored-overlap.mkv'", "media='media-r26/ass-overlap.mkv'")
parent=parent.replace("root=work/'native-overlap-mask-reference-r24'", "root=work/'native-ass-overlap-reference-r27'")
ast.parse(parent);target=work/'run-native-ass-reference-r27.py';assert not target.exists();target.write_text(parent,encoding='utf-8',newline='\n')
