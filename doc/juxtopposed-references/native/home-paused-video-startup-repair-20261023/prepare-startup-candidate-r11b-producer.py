from pathlib import Path
work = Path(__file__).resolve().parent
text = (work / 'prepare-startup-candidate-r11.py').read_text(encoding='utf-8')
assert text.count('\\n+') == 4
text = text.replace('\\n+', '\\n')
text = text.replace("('static void *DecoderThread( void *p_data )', '''static bool", "('''/**\n * The decoding main loop''', '''static bool")
text = text.replace("static void *DecoderThread( void *p_data )'''),", "/**\n * The decoding main loop'''),")
text = text.replace('startup-candidate-r11-source', 'startup-candidate-r11b-source')
text = text.replace("name.replace('r10', 'r11')", "name.replace('r10', 'r11b')")
text = text.replace(".replace('r10', 'r11')", ".replace('r10', 'r11b')")
target = work / 'prepare-startup-candidate-r11b.py'
assert not target.exists()
target.write_text(text, encoding='utf-8', newline='\n')
