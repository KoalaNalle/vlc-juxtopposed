from pathlib import Path
from hashlib import sha256
import json
work=Path(__file__).resolve().parent;repo=work.parent.parent
digest=lambda p:sha256(p.read_bytes()).hexdigest()
record=json.loads((work/'inputs-before-r1b.json').read_text(encoding='utf-8-sig'))
relative='modules/gui/qt/player/qml/SliderBar.qml'
source=repo/relative;assert digest(source)==record['files'][relative]
text=source.read_text(encoding='utf-8')
point='''    readonly property real _hoveredScalingFactor: 1.8'''
new='''    // Qt's accessibility value interface writes this control's value property.
    // Keep engine/drag display updates separate from that user seek request.
    property bool _updatingDisplayValue: false
    property bool _componentReady: false
    readonly property real minimumValue: from
    readonly property real maximumValue: to

    function _setDisplayValue(position) {
        const updating = _updatingDisplayValue
        _updatingDisplayValue = true
        try {
            value = position
        } finally {
            _updatingDisplayValue = updating
        }
    }

    onValueChanged: {
        if (_updatingDisplayValue || !_componentReady)
            return
        if (!Number.isFinite(value) || !enabled || !Player.seekable
                || !fsm.started || fsm._state !== fsmReleased) {
            _setDisplayValue(Player.position)
            return
        }
        fsm.accessibilitySeek(value)
    }

'''
assert text.count(point)==1;text=text.replace(point,new+point)
point='''        signal playerUpdatePosition(real position)'''
assert text.count(point)==1;text=text.replace(point,point+'\n        signal accessibilitySeek(real position)')
point='''            "playerUpdatePosition": fsm.playerUpdatePosition,'''
assert text.count(point)==1;text=text.replace(point,point+'\n            "accessibilitySeek": fsm.accessibilitySeek,')
assert text.count('control.value = position')==3
text=text.replace('control.value = position','control._setDisplayValue(position)')
point='''                "pressControl": {'''
new='''                "accessibilitySeek": {
                    action: (position) => fsm._seekToPosition(position, 0, true)
                },
'''
assert text.count(point)==1;text=text.replace(point,new+point)
point='''    Component.onCompleted: value = Player.position'''
assert text.count(point)==1;text=text.replace(point,'''    Component.onCompleted: {
        _setDisplayValue(Player.position)
        _componentReady = true
    }''')
target=work/'candidate-r2/SliderBar.qml';assert not target.exists();target.parent.mkdir()
target.write_text(text,encoding='utf-8',newline='\n')
out=dict(sourceRelativePath=relative,beforeSha256=digest(source),afterSha256=digest(target),productionApplied=False,
    scope='Guard normal display writes and route an external accessible value write through a released-state seek event. Preserve pointer FSM, held-wrong-input protection, chapters, keyboard/step actions and native seek-speed policy. Expose normalized accessible bounds.')
(work/'slider-candidate-r2.json').write_text(json.dumps(out,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(out,indent=2))
