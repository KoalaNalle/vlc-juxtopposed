"""Check production cue-strip QML with supplied native-tick data only.
This does not execute native selected-source matching or SRT extraction.
Original visual design by Juxtopposed; GPLv2 or later test code.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
repo = Path(__file__).resolve().parents[4]
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
os.environ['QT_QPA_FONTDIR'] = str(repo / 'modules/gui/qt/pixmaps/juxtopposed')
from PySide6.QtCore import QObject, Property, Signal, Slot, QUrl, QPoint, Qt, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication, QColor
from PySide6.QtQuick import QQuickItem, QQuickView
from PySide6.QtTest import QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir', type=Path, default=Path('ui-previews/subtitle-timeline'))
args = parser.parse_args()
args.output_dir.mkdir(parents=True, exist_ok=True)
asset_hashes = {}
for name in ['timeline-chevron-left.svg', 'timeline-chevron-right.svg', 'timeline-minus.svg', 'timeline-plus.svg']:
    path = repo / 'modules/gui/qt/pixmaps/juxtopposed' / name
    asset_hashes[name] = hashlib.sha256(path.read_bytes()).hexdigest()
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))

class Controller(QObject):
    changed = Signal()
    def __init__(self):
        super().__init__()
        self._available, self._loading, self._dense = True, False, False
        self._position, self._length, self._revision = 5000000, 18000000, 1
        self._delay = 1500000
        self._status = 'Source cue snapshot with the current subtitle delay.'
        self.seeks, self.reloads, self.shifts = [], 0, []
        self.segments = [{'startTicks': 1000000, 'endTicks': 2000000, 'text': 'First'},
                         {'startTicks': 4000000, 'endTicks': 6000000, 'text': 'Second'},
                         {'startTicks': 4500000, 'endTicks': 7000000, 'text': '<font color="green">Overlap</font>'},
                         {'startTicks': 12000000, 'endTicks': 15000000, 'text': 'Third'}]
    available = Property(bool, lambda s: s._available, notify=changed)
    loading = Property(bool, lambda s: s._loading, notify=changed)
    positionTicks = Property('qlonglong', lambda s: s._position, notify=changed)
    lengthTicks = Property('qlonglong', lambda s: s._length, notify=changed)
    revision = Property(int, lambda s: s._revision, notify=changed)
    delayTicks = Property('qlonglong', lambda s: s._delay, notify=changed)
    status = Property(str, lambda s: s._status, notify=changed)
    source = Property(QUrl, lambda s: QUrl('file:///supplied-cues.srt'), notify=changed)
    @Slot('qlonglong', 'qlonglong', result='QVariantMap')
    def view(self, start, end):
        return {'segments': [] if self._dense else [
            dict(cue, cueIndex=index, revision=self._revision,
                 seekable=cue['startTicks'] >= 0 and cue['startTicks'] < self._length)
            for index, cue in enumerate(self.segments)
            if cue['endTicks'] > start and cue['startTicks'] < end], 'tooDense': self._dense}
    @Slot(int, int, result=bool)
    def seekCue(self, index, revision):
        self.seeks.append((index, revision))
        return self._available and revision == self._revision and 0 <= index < len(self.segments)
    @Slot()
    def reload(self):
        self.reloads += 1
        self._revision += 1
        self.changed.emit()
    @Slot('qlonglong', int, result=bool)
    def setDelayTicks(self, value, revision):
        self.shifts.append((value, revision))
        if not self._available or revision != self._revision:
            return False
        self._delay = value
        self._revision += 1
        self.changed.emit()
        return True

app = QGuiApplication([])
controller = Controller()
view = QQuickView()
view.setColor(QColor('#111111'))
view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setInitialProperties({'controller': controller,
    'assetBase': QUrl.fromLocalFile(str(repo / 'modules/gui/qt/pixmaps/juxtopposed') + '/')})
view.setSource(QUrl.fromLocalFile(str(repo / 'modules/gui/qt/player/qml/JuxtopposedSubtitleCueTimeline.qml')))
assert not view.errors(), [error.toString() for error in view.errors()]
root = view.rootObject()
view.resize(900, 64)
view.show()
QTest.qWait(50)

def item(name):
    pending = [root]
    while pending:
        candidate = pending.pop()
        if candidate.objectName() == name: return candidate
        pending.extend(candidate.childItems())
    raise AssertionError(name)

assert root.property('hasCues')
assert abs(item('subtitleCue0').x() - 900 / 18) < .1
assert abs(item('subtitleCue1').width() - 900 * 2 / 18) < .1
assert 0 <= item('subtitleCuePlayhead').x() <= 900
QTest.mouseClick(view, Qt.LeftButton, pos=QPoint(75, 10))
assert controller.seeks == [(0, 1)]
QTest.keyClick(view, Qt.Key_Return)
assert controller.seeks == [(0, 1), (0, 1)]
QTest.keyClick(view, Qt.Key_Space)
assert controller.seeks[-1] == (0, 1) and len(controller.seeks) == 3
QTest.mouseClick(view, Qt.LeftButton, pos=QPoint(860, 35))
assert controller.reloads == 1
assert item('subtitleCueReload').property('enabled')
assert view.grabWindow().save(str(args.output_dir / 'subtitle-source-cues-component.png'))
pixels = view.grabWindow()
assert pixels.pixelColor(150, 14).name() == '#371f06'
assert pixels.pixelColor(350, 14).name() == '#090909'
assert item('subtitleCueStrip').property('radius') == 6
assert item('subtitleCuePlayhead').width() == 8 and item('subtitleCuePlayhead').height() == 24

# Use the production buttons with a longer supplied media clock. Pan and zoom
# change only the displayed range; their semantics are an explicit adaptation.
controller._length, controller._position = 120000000, 60000000
controller.changed.emit()
QTest.qWait(20)
assert root.property('windowStartTicks') == 45000000
position_before_navigation = controller._position
QTest.mouseClick(view, Qt.LeftButton, pos=QPoint(869, 14))
assert root.property('windowSpanTicks') == 15000000
QTest.mouseClick(view, Qt.LeftButton, pos=QPoint(891, 14))
assert not root.property('followPlayback')
assert root.property('windowStartTicks') == 56250000
QTest.mouseClick(view, Qt.LeftButton, pos=QPoint(11, 14))
assert root.property('windowStartTicks') == 52500000
QTest.mouseClick(view, Qt.LeftButton, pos=QPoint(33, 14))
assert root.property('windowSpanTicks') == 30000000
assert root.property('windowStartTicks') == 45000000
assert controller._position == position_before_navigation
controller._length, controller._position = 18000000, 5000000
controller.changed.emit()
root.setProperty('followPlayback', True)
QTest.qWait(20)

# A held pointer submits one whole-source timing change on release, while a
# transition to unavailable cancels it. The native source guards are separate.
locks = []
root.requestLockUnlockAutoHide.connect(locks.append)
revision_before_drag = controller._revision
seeks_before_drag = len(controller.seeks)
QTest.mousePress(view, Qt.LeftButton, pos=QPoint(75, 14))
QTest.mouseMove(view, QPoint(120, 14))
QTest.mouseRelease(view, Qt.LeftButton, pos=QPoint(120, 14))
assert controller.shifts == [(2400000, revision_before_drag)]
assert len(controller.seeks) == seeks_before_drag
assert locks == [True, False]
QTest.mousePress(view, Qt.LeftButton, pos=QPoint(120, 14))
QTest.mouseMove(view, QPoint(170, 14))
controller._available = False
controller._revision += 1
controller.changed.emit()
QTest.qWait(10)
QTest.mouseRelease(view, Qt.LeftButton, pos=QPoint(170, 14))
assert len(controller.shifts) == 1 and locks == [True, False, True, False]
controller._available = True
controller._revision += 1
controller.changed.emit()
QTest.mouseMove(view, QPoint(280, 8))
QTest.qWait(450)
assert not messages, messages
QTest.mouseMove(view, QPoint(895, 60))
root.setProperty('windowStartTicks', 4500000)
root.setProperty('windowEndTicks', 6500000)
QTest.qWait(25)
assert item('subtitleCue0').x() == 0 and abs(item('subtitleCue0').width() - 675) < .1
assert item('subtitleCue1').x() == 0 and item('subtitleCue1').width() == 900
controller.segments = [{'startTicks': -1000000, 'endTicks': 1000000, 'text': 'Negative delay clipping'}]
controller._revision += 1
controller.changed.emit()
root.setProperty('windowStartTicks', 0)
root.setProperty('windowEndTicks', 2000000)
QTest.qWait(25)
assert item('subtitleCue0').x() == 0 and item('subtitleCue0').width() == 450
previous_seeks = len(controller.seeks)
QTest.mouseClick(view, Qt.LeftButton, pos=QPoint(120, 10))
assert len(controller.seeks) == previous_seeks  # negative effective start cannot seek
# Keep the actual position visible across a gap with no intersecting cues.
root.setProperty('windowStartTicks', 3000000)
root.setProperty('windowEndTicks', 6000000)
QTest.qWait(25)
assert not root.property('hasCues') and root.property('timelineReady')
assert item('subtitleCuePlayhead').property('visible')
# Values above 32-bit milliseconds are still given in native microsecond ticks.
long_time = 9000000000000
controller.segments = [{'startTicks': long_time, 'endTicks': long_time + 2000000, 'text': 'Long file'}]
controller._length, controller._position = long_time + 4000000, long_time + 1000000
controller._revision += 1
controller.changed.emit()
root.setProperty('windowStartTicks', long_time)
root.setProperty('windowEndTicks', long_time + 4000000)
QTest.qWait(25)
assert item('subtitleCue0').x() == 0 and item('subtitleCue0').width() == 450
controller._dense = True
controller.changed.emit()
QTest.qWait(25)
assert not root.property('hasCues') and 'Too many' in root.property('statusText')
controller._dense, controller._available, controller._loading = False, False, True
controller._status = 'Reading the selected SRT file…'
controller.changed.emit()
QTest.qWait(25)
assert item('subtitleCueLoading').property('visible') and not root.property('hasCues')
assert not item('subtitleCueReload').property('enabled')
controller._loading = False
for status in ['Select one external SRT subtitle track to see its timeline.',
               'Timeline is unavailable for embedded or image subtitles.',
               'Timeline is unavailable while subtitle FPS correction is enabled.',
               'Disable subtitle duration effects to view accurate cue timings.',
               'Could not read valid SRT cue timings. Playback subtitles are unchanged.']:
    controller._status = status
    controller.changed.emit()
    QTest.qWait(15)
    assert not root.property('hasCues') and item('subtitleCueStatus').property('text') == status
assert view.grabWindow().save(str(args.output_dir / 'subtitle-cues-unavailable-component.png'))
controller._available, controller._length = True, 0
controller.changed.emit()
QTest.qWait(15)
assert not root.property('hasCues') and 'duration' in root.property('statusText')
layouts = []
for width, scale in [(320, 1), (640, 1.5), (900, 2)]:
    root.setProperty('uiScale', scale)
    view.resize(width, int(64 * scale))
    QTest.qWait(20)
    assert item('subtitleCueStrip').width() == width
    assert item('subtitleCueStrip').height() == 28 * scale
    layouts.append({'width': width, 'scale': scale})
assert not messages, messages

view.setInitialProperties({'assetBase': QUrl.fromLocalFile(str(repo / 'modules/gui/qt/pixmaps/juxtopposed') + '/')})
view.setSource(QUrl.fromLocalFile(str(repo / 'modules/gui/qt/player/qml/JuxtopposedSubtitleTimelineToggle.qml')))
assert not view.errors(), [error.toString() for error in view.errors()]
root = view.rootObject()
view.resize(360, 52)
QTest.qWait(20)
assert root.property('text') == 'Show Subtitle Timeline on Seekbar'
assert not root.property('checked')
assert item('subtitleTimelineToggleIndicator').width() == 68
assert item('subtitleTimelineToggleIndicator').height() == 32
assert item('subtitleTimelineToggleKnob').x() == 3
QTest.mouseClick(view, Qt.LeftButton, pos=QPoint(120, 26))
assert root.property('checked') and item('subtitleTimelineToggleKnob').x() == 39
assert view.grabWindow().save(str(args.output_dir / 'subtitle-timeline-toggle-on-component.png'))
QTest.keyClick(view, Qt.Key_Space)
assert not root.property('checked') and item('subtitleTimelineToggleKnob').x() == 3
assert view.grabWindow().save(str(args.output_dir / 'subtitle-timeline-toggle-off-component.png'))
root.setProperty('enabled', False)
QTest.mouseClick(view, Qt.LeftButton, pos=QPoint(120, 26))
assert not root.property('checked')
assert not messages, messages
(args.output_dir / 'subtitle-timeline-component-verification.json').write_text(json.dumps({
    'scope': 'Actual cue-strip QML with supplied controller/native-tick values; not native source/parse/player evidence.',
    'checks': ['tick-based geometry', 'Figma28/24/8px geometry and exact base/elapsed colors', 'unchanged original four SVG icons', 'viewport pan/zoom preserves supplied clock', 'one timing write on pointer release', 'unavailable transition cancels held-pointer write', 'balanced drag auto-hide requests', 'original settings toggle pointer/Space/disabled actions', 'overlap', 'viewport clipping', 'negative-delay clipping', 'large tick values', 'cue pointer/Return/Space activation with revision', 'invalid cue cannot activate', 'Reload routing/loading state', 'continuous playhead through an empty gap', 'dense range', 'loading', 'empty/unsupported/filter/FPS states', 'unknown duration', 'scaled strip'],
    'layouts': layouts, 'qmlMessages': messages,
    'sourceFigmaNodes': ['3744:12361', '5187:24487'],
    'svgSha256': asset_hashes,
    'semanticsLimit': 'The prototype confirms toggle entry/subtitle-icon exit. Pan/zoom/drag gestures are documented adaptations; no native or complete visual acceptance follows from this supplied-controller check.'
}, indent=2), encoding='utf-8')
print('Production subtitle source-cue strip component checks passed; native source/parse tests are separate.')
