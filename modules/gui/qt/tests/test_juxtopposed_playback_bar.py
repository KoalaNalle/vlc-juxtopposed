# Visual design and original artwork by Juxtopposed.
# Test code licensed under GPLv2 or later.
import os
import argparse
from pathlib import Path
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
os.environ['QT_QPA_FONTDIR'] = str(Path(__file__).resolve().parents[4] / 'modules/gui/qt/pixmaps/juxtopposed')

import json
import subprocess
import sys
from pathlib import Path

import PySide6
repo = Path(__file__).resolve().parents[4]
parser = argparse.ArgumentParser(description='Render and test the Juxtopposed Qt playback component.')
parser.add_argument('--output-dir', type=Path, default=Path('ui-previews'))
args = parser.parse_args()
from PySide6.QtCore import Qt, QUrl, QPoint, QObject, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication, QFontDatabase, QFont
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest, QSignalSpy
from PySide6.QtQml import QQmlExpression, qmlContext

qml = repo / 'modules/gui/qt/player/qml'
out = args.output_dir
out.mkdir(exist_ok=True, parents=True)
messages = []
def message_handler(kind, context, message):
    messages.append(message)
qInstallMessageHandler(message_handler)
app = QGuiApplication([])
from juxtopposed_palette_fixture import register_juxtopposed_palette
register_juxtopposed_palette()
font_id = QFontDatabase.addApplicationFont(str(repo / 'modules/gui/qt/pixmaps/juxtopposed/InterVariable.ttf'))
assert font_id >= 0
app.setFont(QFont(QFontDatabase.applicationFontFamilies(font_id)[0]))

# qmlformat parses the real modified files without rewriting their formatting.
qml_files = ['JuxtopposedButton.qml', 'JuxtopposedSeekBackground.qml',
             'JuxtopposedPlaybackBar.qml', 'ControlBar.qml', 'TopBar.qml',
             'Player.qml', 'TracksMenu.qml', 'TracksListPage.qml',
             'SliderBar.qml', 'PlaybackSettingsMenu.qml', 'controlbarcontrols/ArtworkInfoWidget.qml',
             'MiniPlayer.qml', '../../maininterface/qml/MainDisplay.qml',
             '../../maininterface/qml/SideNavigationPane.qml']
for name in qml_files:
    result = subprocess.run([str(Path(PySide6.__file__).parent / ('qmlformat.exe' if os.name == 'nt' else 'qmlformat')),
                             '--ignore-settings', str(qml / name)],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    assert result.returncode == 0, (name, result.stderr)
print(f'All {len(qml_files)} playback QML files parse successfully.')

view = QQuickView()
view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setInitialProperties({'assetBase': QUrl.fromLocalFile(str(repo / 'modules/gui/qt/pixmaps/juxtopposed') + '/')})
view.setSource(QUrl.fromLocalFile(str(qml / 'JuxtopposedPlaybackBar.qml')))
assert not view.errors(), [error.toString() for error in view.errors()]
root = view.rootObject()
assert root
root.setProperty('assetBase', QUrl.fromLocalFile(str(repo / 'modules/gui/qt/pixmaps/juxtopposed') + '/'))
root.setProperty('elapsed', '00:02:10')
root.setProperty('duration', '00:25:40')
root.setProperty('position', 130 / 1540)
root.setProperty('volume', 0.5)
root.setProperty('seekable', True)
view.resize(1920, 54)
view.show()
QTest.qWait(300)

def item(name):
    result = root.findChild(QObject, name)
    assert result, name
    return result

def center(obj):
    return obj.mapToScene(obj.boundingRect().center()).toPoint()

def click_signal(name, signal):
    spy = QSignalSpy(getattr(root, signal))
    QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, center(item(name)))
    QTest.qWait(15)
    assert spy.count() == 1, (name, signal, spy.count())

# Keep the clean, default frame for the visual comparison.
assert view.grabWindow().save(str(out / 'playback-bar-preview.png'))
click_signal('juxtopposedPlay', 'playPauseRequested')
click_signal('juxtopposedNext', 'nextRequested')
click_signal('juxtopposedPrevious', 'previousRequested')
click_signal('juxtopposedMute', 'muteRequested')
click_signal('juxtopposedAudio', 'audioRequested')
click_signal('juxtopposedSubtitles', 'subtitlesRequested')
click_signal('juxtopposedSettings', 'settingsRequested')
click_signal('juxtopposedQueue', 'queueRequested')
click_signal('juxtopposedDuration', 'durationRequested')

# A live seek window has a position, not a fixed media duration. The LIVE
# indicator must not offer the remaining-time toggle used by finite files.
root.setProperty('live', True)
root.setProperty('duration', 'LIVE')
root.setProperty('durationInteractive', False)
duration = item('juxtopposedDuration')
assert duration.property('text') == 'LIVE' and not duration.property('enabled')
duration_spy = QSignalSpy(root.durationRequested)
QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, center(duration))
assert duration_spy.count() == 0, 'LIVE indicator emitted a duration toggle'
assert view.grabWindow().save(str(out / 'playback-bar-live.png'))
root.setProperty('live', False)
root.setProperty('duration', '00:25:40')
root.setProperty('durationInteractive', True)
click_signal('juxtopposedDuration', 'durationRequested')

# Keyboard activation and volume changes must produce the same native hooks.
play = item('juxtopposedPlay')
play.forceActiveFocus()
spy = QSignalSpy(root.playPauseRequested)
QTest.keyClick(view, Qt.Key_Space)
assert spy.count() == 1, 'Keyboard play/pause'

# Arrow navigation on transport buttons traverses the toolbar and skips
# unavailable buttons; sliders retain their own left/right input handling.
QTest.keyClick(view, Qt.Key_Left)
assert item('juxtopposedPrevious').hasActiveFocus(), 'Left to previous'
QTest.keyClick(view, Qt.Key_Right)
assert play.hasActiveFocus(), 'Right to play'
root.setProperty('canGoNext', False)
QTest.keyClick(view, Qt.Key_Right)
assert item('juxtopposedSeek').hasActiveFocus(), 'Skip disabled next'
root.setProperty('canGoNext', True)
item('juxtopposedSettings').forceActiveFocus()
QTest.keyClick(view, Qt.Key_Right)
assert item('juxtopposedQueue').hasActiveFocus(), 'Right to queue'
QTest.keyClick(view, Qt.Key_Right)
assert item('juxtopposedPrevious').hasActiveFocus(), 'Wrap toolbar navigation'

volume = item('juxtopposedVolume')
volume.forceActiveFocus()
spy = QSignalSpy(root.volumeRequested)
QTest.keyClick(view, Qt.Key_Right)
assert spy.count() == 1 and abs(spy.at(0)[0] - 0.55) < 0.001, 'Keyboard volume'

seek = item('juxtopposedSeek')
spy = QSignalSpy(root.seekRequested)
target = seek.mapToScene(seek.boundingRect().center()).toPoint()
QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, target)
assert spy.count() and 0.45 < spy.at(0)[0] < 0.55, 'Seek midpoint'

# Updating engine values must not emit a user action, which could seek or
# change volume just because a new media file was loaded.
seekSpy = QSignalSpy(root.seekRequested)
volumeSpy = QSignalSpy(root.volumeRequested)
root.setProperty('position', 0.8)
root.setProperty('volume', 1.0)
QTest.qWait(20)
assert seekSpy.count() == 0 and volumeSpy.count() == 0, 'Engine update emitted input'
root.setProperty('playing', True)
assert str(play.property('source').toString()).endswith('pause.svg'), 'Pause state'
root.setProperty('playing', False)

root.setProperty('canGoNext', False)
spy = QSignalSpy(root.nextRequested)
QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, center(item('juxtopposedNext')))
assert spy.count() == 0, 'Disabled next action'
root.setProperty('canGoNext', True)

geometry = []
for width, scale in [(1920, 1), (1024, 1), (640, 1), (1280, 2), (960, 1.5)]:
    root.setProperty('uiScale', scale)
    root.setProperty('elapsed', '123:02:10' if width == 640 else '00:02:10')
    root.setProperty('duration', '-123:25:40' if width == 640 else '00:25:40')
    view.resize(width, round((94 if width < 860 * scale else 54) * scale))
    QTest.qWait(80)
    elements = [item(n) for n in ['juxtopposedPrevious', 'juxtopposedPlay',
                'juxtopposedNext', 'juxtopposedAudio', 'juxtopposedSubtitles',
                'juxtopposedSettings', 'juxtopposedQueue', 'juxtopposedSeekLoader']]
    bounds = []
    for obj in elements:
        p = obj.mapToScene(obj.boundingRect().topLeft())
        assert p.x() >= -0.5 and p.y() >= -0.5, (width, scale, obj.objectName(), p)
        assert p.x() + obj.width() <= width + 0.5, (width, scale, obj.objectName(), p, obj.width())
        assert p.y() + obj.height() <= view.height() + 0.5, (width, scale, obj.objectName(), p, obj.height())
        bounds.append((obj.objectName(), round(p.x(), 2), round(p.y(), 2), round(obj.width(), 2), round(obj.height(), 2)))
    # Seek and transport must be separated in small windows.
    seekBounds = bounds[-1]
    playBounds = bounds[1]
    if root.property('stacked'):
        assert seekBounds[2] + seekBounds[4] <= playBounds[2], (width, scale, bounds)
    geometry.append({'width': width, 'scale': scale, 'bounds': bounds})
    if width == 640 and scale == 1:
        assert view.grabWindow().save(str(out / 'playback-bar-narrow.png'))

# Verify every image loaded and retained its intended 24px logical icon slot.
root.setProperty('uiScale', 1)
view.resize(1920, 54)
QTest.qWait(80)
for name in ['juxtopposedPlay', 'juxtopposedNext', 'juxtopposedPrevious',
             'juxtopposedAudio', 'juxtopposedSubtitles', 'juxtopposedSettings',
             'juxtopposedQueue', 'juxtopposedMute']:
    images = [obj for obj in item(name).findChildren(QObject) if obj.metaObject().className() == 'QQuickImage']
    assert len(images) == 1, (name, images)
    image = images[0]
    expression = QQmlExpression(qmlContext(image), image, 'status === 1')
    assert expression.evaluate()[0], (name, image.property('source'))
    assert image.width() == 24 and image.height() == 24, (name, image.width(), image.height())

# Render the production seek background with actual native peak evidence.
# These are supplied component values; this does not claim desktop playback.
waveform = json.loads((repo / 'doc/juxtopposed-references/advanced/audio-drain/native-result.json').read_text())['peaks']
background = seek.property('background')
background.setProperty('waveformPeaks', waveform)
root.setProperty('position', .25)
root.setProperty('elapsed', '00:00:08')
root.setProperty('duration', '00:00:30')
QTest.qWait(30)
waveform_items = {obj.objectName(): obj for obj in background.childItems()}
bars = [waveform_items.get('juxtopposedWaveformBar' + str(i)) for i in range(300)]
assert all(bars), ('Decoded waveform bars were not constructed', len(waveform_items), messages[-5:])
assert bars[0].property('color').name().upper() == '#D37918', 'Elapsed waveform colour'
assert bars[299].property('color').name().upper() == '#4F4F4F', 'Future waveform colour'
assert max(b.height() for b in bars) - min(b.height() for b in bars) > 8, 'Source amplitudes were flattened'
assert seekSpy.count() == 0, 'Waveform/position refresh emitted a seek'
assert view.grabWindow().save(str(out / 'waveform-bar-preview.png'))
for width, scale in [(640, 1), (1280, 2), (960, 1.5)]:
    root.setProperty('uiScale', scale)
    root.setProperty('position', .1 + .02 * scale)
    view.resize(width, round(94 * scale))
    QTest.qWait(30)
    for bar in bars:
        assert bar.x() >= 0 and bar.x() + bar.width() <= background.width() + .5, 'Waveform horizontal bounds'
        assert bar.y() >= 0 and bar.y() + bar.height() <= background.height() + .5, 'Waveform vertical bounds'
    spy = QSignalSpy(root.seekRequested)
    QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, center(seek))
    assert spy.count() and .45 < spy.at(0)[0] < .55, ('Waveform seek midpoint', width, spy.count())
background.setProperty('waveformPeaks', [0.0] * 300)
QTest.qWait(15)
silent_bars = [obj for obj in background.childItems() if obj.objectName().startswith('juxtopposedWaveformBar')]
assert len(silent_bars) == 300 and all(b.height() == scale for b in silent_bars), 'Silence should have a minimal visible baseline'
background.setProperty('waveformPeaks', [])
QTest.qWait(15)
assert not background.property('hasWaveform'), 'Missing waveform must restore ordinary seeking'

import xml.etree.ElementTree as ET
resources = ET.parse(repo / 'modules/gui/qt/assets.qrc')
for resource in resources.findall(".//qresource[@prefix='/juxtopposed']/file"):
    asset = repo / 'modules/gui/qt' / resource.text
    assert asset.is_file() and asset.stat().st_size > 0, str(asset)
    assert str(resource.text) in (repo / 'modules/gui/qt/Makefile.am').read_text(), str(asset)

errors = [m for m in messages if 'Error' in m or 'Unable' in m or 'Cannot' in m or 'binding loop' in m.lower()]
assert not errors, errors
(out / 'playback-bar-verification.json').write_text(json.dumps({'geometry':geometry,'messages':messages}, indent=2))
print('Passed: nine click actions, LIVE indicator and duration-toggle guard, arrow navigation, keyboard playback/volume, seek, engine synchronization, disabled buttons, five width/scale layouts, asset geometry, real peak display/colours, waveform seeking, silence and fallback.')
print('Native VLC compilation and actual media playback still require the VLC Windows build environment.')
