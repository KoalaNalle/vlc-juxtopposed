from pathlib import Path
from hashlib import sha256
import json
from PySide6.QtGui import QImage

work = Path(__file__).resolve().parent


def mask(path):
    image = QImage(str(path)).convertToFormat(QImage.Format_RGBA8888)
    assert not image.isNull()
    pixels = bytes(image.bits())
    return bytes(1 if min(pixels[i:i+3]) > 160 else 0
                 for i in range(0, len(pixels), 4))


rows = []
for result_path in sorted(work.glob('native-delay-*-r3/*/probe/result.json')):
    result = json.loads(result_path.read_text(encoding='utf-8-sig'))
    if result.get('status') != 'terminal':
        continue
    first = mask(result_path.parent/'reference-first.png')
    middle = mask(result_path.parent/'reference-middle.png')
    comparisons = {}
    for name in ['boundary-seek', 'boundary-settled', 'live-reset', 'live-positive',
                 'live-negative', 'live-zero', 'after-playing-boundary']:
        actual = mask(result_path.parent/(name+'.png'))
        comparisons[name] = dict(bright=sum(actual),
            firstMismatch=sum(a != b for a, b in zip(actual, first)),
            middleMismatch=sum(a != b for a, b in zip(actual, middle)))
    rows.append(dict(path=str(result_path.relative_to(work)),
                     referenceFirstVsMiddle=sum(a != b for a, b in zip(first, middle)),
                     comparisons=comparisons))
output = work/'mask-analysis-r3.json'
assert not output.exists()
output.write_text(json.dumps(dict(rows=rows, producerSha256=sha256(Path(__file__).read_bytes()).hexdigest()),
                            indent=2)+'\n',encoding='utf-8',newline='\n')
for row in rows:
    print(row['path'], 'first-vs-middle', row['referenceFirstVsMiddle'],
          'playing', row['comparisons']['after-playing-boundary'], flush=True)
