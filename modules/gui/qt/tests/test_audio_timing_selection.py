# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual audio timing QML with supplied player, estimator and visual boundaries.

This exercises marker state across a selected-track event. Native callback
delivery and real playback require the separate native/desktop tests.
"""
import argparse
from hashlib import sha256
import json
import os
from pathlib import Path
import shutil
import time

repo = Path(__file__).resolve().parents[4]
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QML_DISABLE_DISK_CACHE='1')
os.environ['QT_QPA_FONTDIR'] = str(repo / 'modules/gui/qt/pixmaps/juxtopposed')
from PySide6.QtCore import QObject, Property, Signal, Slot, QUrl, Qt, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QJSValue, QmlAttached, qmlRegisterType, qmlRegisterSingletonInstance
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir', type=Path, required=True)
parser.add_argument('--audio-page', type=Path, help='Alternate saved production page for before/after comparisons')
args = parser.parse_args()
out = args.output_dir.resolve()
out.mkdir(parents=True, exist_ok=False)
messages = []
qInstallMessageHandler(lambda kind, context, text: messages.append(text))
app = QGuiApplication([])

class NavigationProperties(QObject):
    changed = Signal()
    def __init__(self, parent=None): super().__init__(parent); self.values = {}
    def field(name, notification):
        return Property('QVariant', lambda self: self.values.get(name),
                        lambda self, value: self.values.__setitem__(name, value), notify=notification)
    parentItem = field('parentItem', changed)
    leftItem = field('leftItem', changed)
    rightItem = field('rightItem', changed)
    cancelAction = Property(QJSValue, lambda self: self.values.get('cancelAction', QJSValue()),
                            lambda self, value: self.values.__setitem__('cancelAction', value), notify=changed)

@QmlAttached(NavigationProperties)
class Navigation(QObject):
    @staticmethod
    def qmlAttachedProperties(owner, target): return NavigationProperties(target)

class IdentifierProperties(QObject):
    changed = Signal()
    def get_id(self): return self.parent().objectName()
    def set_id(self, value): self.parent().setObjectName(value); self.changed.emit()
    id = Property(str, get_id, set_id, notify=changed)

@QmlAttached(IdentifierProperties)
class AccessibleCompat(QObject):
    @staticmethod
    def qmlAttachedProperties(owner, target): return IdentifierProperties(target)

qmlRegisterType(Navigation, 'VLC.Util', 1, 0, 'Navigation')
qmlRegisterType(AccessibleCompat, 'VLC.Util', 1, 0, 'AccessibleCompat')

class Duration(QObject):
    def __init__(self, value=0, parent=None): super().__init__(parent); self.value = value
    @Slot(float, result=float)
    def scale(self, rate): return self.value * rate

class DelayEstimator(QObject):
    heardTimeChanged = Signal()
    spottedTimeChanged = Signal()
    delayChanged = Signal()
    def __init__(self, parent=None):
        super().__init__(parent); self.heard = None; self.spotted = None; self.current = Duration(parent=self)
    isHeardTimeMarked = Property(bool, lambda self: self.heard is not None, notify=heardTimeChanged)
    isSpottedTimeMarked = Property(bool, lambda self: self.spotted is not None, notify=spottedTimeChanged)
    delay = Property(QObject, lambda self: self.current, notify=delayChanged)
    def complete(self):
        if self.heard is None or self.spotted is None: return
        self.current.value = self.heard - self.spotted
        self.delayChanged.emit(); self.cancel()
    @Slot()
    def markHeardTime(self):
        self.heard = None if self.heard is not None else time.monotonic()
        self.heardTimeChanged.emit(); self.complete()
    @Slot()
    def markSpottedTime(self):
        self.spotted = None if self.spotted is not None else time.monotonic()
        self.spottedTimeChanged.emit(); self.complete()
    @Slot()
    def cancel(self):
        if self.heard is not None: self.heard = None; self.heardTimeChanged.emit()
        if self.spotted is not None: self.spotted = None; self.spottedTimeChanged.emit()
    @Slot()
    def reset(self):
        self.cancel(); self.current.value = 0; self.delayChanged.emit()

class Player(QObject):
    inputChanged = Signal()
    rateChanged = Signal()
    seekStarted = Signal()
    playingStateChanged = Signal()
    audioDelayChanged = Signal()
    audioSelectionChanged = Signal()
    audioTimingContextChanged = Signal()
    subtitleSelectionChanged = Signal()
    metadataChanged = Signal()
    def __init__(self):
        super().__init__(); self.track = 'a'; self.current_delay = 123; self.edits = []; self.revision = 0
        self.native_rate = 1.0; self.native_state = 'playing'; self.change_during_apply = False
    rate = Property(float, lambda self: 1.0, notify=rateChanged)
    def set_audio_delay(self, value, notify=True):
        if self.current_delay != value: self.revision += 1; self.current_delay = value
        if notify: self.audioDelayChanged.emit()
    audioDelayMS = Property(int, lambda self: self.current_delay, set_audio_delay, notify=audioDelayChanged)
    @Slot(float)
    def addAudioDelay(self, value):
        if value: self.edits.append(value)
    @Slot(result=str)
    def beginAudioTiming(self): return str(self.revision)
    @Slot(str, result=bool)
    def isAudioTimingCurrent(self, token): return token == str(self.revision)
    @Slot(QObject, str, result=bool)
    def addAudioTimingIfCurrent(self, value, token):
        if self.change_during_apply: self.change_during_apply = False; self.revision += 1
        if not self.isAudioTimingCurrent(token): return False
        if value.value:
            self.edits.append(value.value * self.native_rate); self.revision += 1
        return True
    @Slot(float, str, result=bool)
    def addAudioDelayIfCurrent(self, value, token):
        # The previous page scaled against the cached GUI rate before calling.
        if not self.isAudioTimingCurrent(token): return False
        if value: self.edits.append(value); self.revision += 1
        return True
    def native_change(self, field, value):
        if getattr(self, field) != value:
            setattr(self, field, value); self.revision += 1
    def select_track(self, track):
        if self.track != track: self.track = track; self.revision += 1; self.audioSelectionChanged.emit()

class TrackMenu(QObject):
    @Slot()
    def requestBack(self): pass

player = Player()
qmlRegisterSingletonInstance(Player, 'VLC.Player', 1, 0, 'Player', player)
qmlRegisterType(DelayEstimator, 'VLC.Player', 1, 0, 'DelayEstimator')

def write(name, text):
    path = out / name; path.parent.mkdir(parents=True, exist_ok=True); path.write_text(text, encoding='utf-8')

write('imports/VLC/Util/qmldir', 'module VLC.Util\n')
write('imports/VLC/Player/qmldir', 'module VLC.Player\n')
write('imports/VLC/MainInterface/qmldir', 'module VLC.MainInterface\nsingleton MainCtx 1.0 MainCtx.qml\n')
write('imports/VLC/MainInterface/MainCtx.qml', 'pragma Singleton\nimport QtQml\nQtObject {}')
write('imports/VLC/Style/qmldir', 'module VLC.Style\nsingleton VLCStyle 1.0 VLCStyle.qml\nsingleton VLCIcons 1.0 VLCIcons.qml\nColorContext 1.0 ColorContext.qml\n')
write('imports/VLC/Style/VLCStyle.qml', '''pragma Singleton
import QtQml
QtObject { property real scale: 1; property real margin_large: 16; property real margin_xsmall: 8; property real margin_xxxsmall: 4; property real margin_xxsmall: 6; function dp(value, scale) { return value * scale } }
''')
write('imports/VLC/Style/VLCIcons.qml', 'pragma Singleton\nimport QtQml\nQtObject { property string back: ""; property string check: "" }')
write('imports/VLC/Style/ColorContext.qml', '''import QtQuick
QtObject { enum Set { Window }
 property int colorSet: 0; property color border: "black"; property QtObject fg: QtObject { property color primary: "white" } }
''')
widgets = ['SubtitleLabel', 'MenuCaption', 'SpinBoxExt', 'ActionButtonOverlay', 'TrackDelayButton', 'IconTrackButton']
write('imports/VLC/Widgets/qmldir', 'module VLC.Widgets\n'+''.join(name+' 1.0 '+name+'.qml\n' for name in widgets))
write('imports/VLC/Widgets/SubtitleLabel.qml', 'import QtQuick\nText {}')
write('imports/VLC/Widgets/MenuCaption.qml', 'import QtQuick\nText {}')
write('imports/VLC/Widgets/SpinBoxExt.qml', 'import QtQuick.Controls.Basic\nSpinBox {}')
write('imports/VLC/Widgets/ActionButtonOverlay.qml', 'import QtQuick.Controls.Basic\nButton {}')
write('imports/VLC/Widgets/TrackDelayButton.qml', 'import QtQuick.Controls.Basic\nButton { property string iconTxt: ""; property bool selected: false; function animate() {} }')
write('imports/VLC/Widgets/IconTrackButton.qml', 'import QtQuick.Controls.Basic\nButton { property string description: "" }')
sources = {}
for name in ['TracksPage.qml', 'TracksPageAudio.qml']:
    source = args.audio_page.resolve() if name == 'TracksPageAudio.qml' and args.audio_page else repo / 'modules/gui/qt/player/qml' / name
    shutil.copyfile(source, out / name)
    sources[name] = sha256(source.read_bytes()).hexdigest()
view = QQuickView()
view.engine().addImportPath(str(out / 'imports'))
menu = TrackMenu()
view.setInitialProperties({'trackMenuController': menu})
view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setSource(QUrl.fromLocalFile(str(out / 'TracksPageAudio.qml')))
assert not view.errors(), [error.toString() for error in view.errors()]
view.resize(900, 300); view.show(); QTest.qWait(50)
root = view.rootObject()
def button(label):
    queue = [root]
    while queue:
        item = queue.pop()
        if item.property('text') == label: return item
        queue.extend(item.childItems())
    raise AssertionError(label)
def click(label):
    item = button(label)
    assert item.isVisible() and item.isEnabled()
    QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, item.mapToScene(item.boundingRect().center()).toPoint())
    QTest.qWait(20)

heard, spotted = button('Sound Heard'), button('Sound Spotted')
assert not heard.property('selected') and not spotted.property('selected')
for first, second, new_track in [('Sound Heard', 'Sound Spotted', 'b'), ('Sound Spotted', 'Sound Heard', 'a')]:
    edits_before = len(player.edits)
    click(first)
    assert button(first).property('selected') and len(player.edits) == edits_before
    player.metadataChanged.emit(); player.subtitleSelectionChanged.emit(); player.select_track(player.track); QTest.qWait(10)
    assert button(first).property('selected'), 'Unrelated changes must not cancel a pending mark'
    player.select_track(new_track); QTest.qWait(10)
    assert not button(first).property('selected'), 'Changing the selected audio track kept a stale mark'
    assert player.current_delay == 123 and len(player.edits) == edits_before
    click(second)
    assert button(second).property('selected') and len(player.edits) == edits_before, 'A new track must not complete an old pair'
    click(first)
    assert len(player.edits) == edits_before + 1 and not heard.property('selected') and not spotted.property('selected')

# A no-op event and an unrelated, delayed notification do not invalidate a new pair.
click('Sound Heard')
player.set_audio_delay(player.current_delay)
player.audioTimingContextChanged.emit()
assert heard.property('selected'), 'No-op notification canceled a current mark'
player.audioDelayChanged.emit()  # Delayed notification from the previous successful correction.
assert heard.property('selected'), 'Delayed self-notification canceled a new mark'
click('Sound Spotted')

# Changed and restored delay remains a different context, even before notifications arrive.
click('Sound Heard')
edits_before = len(player.edits)
prior = player.current_delay
player.set_audio_delay(prior + 50, False); player.set_audio_delay(prior, False)
click('Sound Spotted')
assert len(player.edits) == edits_before and spotted.property('selected') and not heard.property('selected'), 'Changed-and-restored delay completed a stale pair'
player.audioDelayChanged.emit()
assert spotted.property('selected'), 'Old notification canceled the fresh mark'
click('Sound Heard')
assert len(player.edits) == edits_before + 1

# An external change delivers a notification and cancels the stale first mark.
click('Sound Heard')
player.set_audio_delay(player.current_delay + 50)
assert not heard.property('selected'), 'A changed delay kept a stale mark'

# Actual transitions, even a return to the same value, invalidate a pair
# before the GUI receives the corresponding notification.
for field, new, old in [('native_rate', 2.0, 1.0), ('native_state', 'paused', 'playing')]:
    click('Sound Heard'); edits_before = len(player.edits)
    player.native_change(field, new); player.native_change(field, old)
    click('Sound Spotted')
    assert len(player.edits) == edits_before and spotted.property('selected') and not heard.property('selected'), field + ': stale pair applied'
    click('Sound Heard')

# Same-value notifications and old queued notifications must preserve new marks.
for field, value, changed, signal in [('native_rate', 1.0, 2.0, player.rateChanged),
                                      ('native_state', 'playing', 'paused', player.playingStateChanged)]:
    click('Sound Heard'); player.native_change(field, value); signal.emit()
    assert heard.property('selected'), field + ': no-op signal canceled a current mark'
    click('Sound Spotted')
    player.native_change(field, changed)
    click('Sound Heard'); signal.emit()
    assert heard.property('selected'), field + ': delayed signal canceled a fresh mark'
    click('Sound Spotted'); player.native_change(field, value)

# The backend rechecks the token at application and uses native, not cached, rate.
click('Sound Heard'); edits_before = len(player.edits); player.change_during_apply = True
click('Sound Spotted'); assert len(player.edits) == edits_before
player.native_change('native_rate', 2.0)
click('Sound Heard'); edits_before = len(player.edits); click('Sound Spotted')
assert len(player.edits) == edits_before + 1 and player.edits[-1] > 0.025, 'native 2x rate was not used'
view.close()
assert not messages, messages
write('result.json', json.dumps(dict(passed=True, sourceSha256=sources, qmlMessages=messages,
    checks=['changed selection cancels either pending mark without applying delay',
            'metadata, subtitle selection and same audio track do not cancel',
            'new track requires a fresh pair', 'fresh same-track pair applies once in both orders',
            'no-op and delayed old notifications preserve a fresh mark',
            'changed-and-restored delay rejects stale pair before notifications',
            'changed delay notification cancels stale mark',
            'rate and state change-and-return invalidate marks before notification',
            'no-op and delayed rate/state notifications preserve fresh marks',
            'backend rejects a late change and applies native playback rate'],
    scope='Actual audio timing QML with supplied player and estimator; not native callback or ordinary desktop evidence.'), indent=2)+'\n')
print('Audio track selection timing component checks passed.')
