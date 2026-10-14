# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual production General QML: assets, choices, search, focus and feedback.

Supplied QObject state tests presentation; native persistence, pickers,
GPU appearance and full VLC acceptance remain separate.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import xml.etree.ElementTree as ET

os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'

import PySide6
from PySide6.QtCore import QObject, Property, Signal, Slot, Qt, QUrl, QPointF, QEvent, QCoreApplication, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication, QKeyEvent
from PySide6.QtQml import QQmlApplicationEngine, QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickItem
from PySide6.QtTest import QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir', type=Path, required=True)
parser.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[4])
args = parser.parse_args()
args.output_dir.mkdir(parents=True, exist_ok=False)
repo = args.repo_root.resolve()
os.environ['QT_QPA_FONTDIR'] = str(repo / 'modules/gui/qt/pixmaps/juxtopposed')
proposal = repo / 'modules/gui/qt/dialogs/dialogs'
reference = Path(__file__).resolve().parent / 'fixtures/juxtopposed_global_settings_assets.json'
freeze = json.loads(reference.read_text(encoding='utf-8'))
assert len(freeze['assets']) == 20
asset_dir = args.output_dir / 'assets'
asset_dir.mkdir()
asset_paths = {}
for name, info in freeze['assets'].items():
    source = repo / info['runtimePath']
    target_name = source.name
    assert source.stat().st_size == info['bytes'], name
    assert hashlib.sha256(source.read_bytes()).hexdigest() == info['sha256'], name
    root = ET.parse(source).getroot()
    natural = [float(root.attrib['width']), float(root.attrib['height'])]
    assert natural == [float(info['rootWidth']), float(info['rootHeight'])], name
    shutil.copyfile(source, asset_dir / target_name)
    asset_paths[name] = {'runtime': target_name, 'sha256': info['sha256'],
        'naturalSize': [int(info['rootWidth']), int(info['rootHeight'])], 'existingMatch': info['existingMatch']}
shutil.copyfile(repo / 'modules/gui/qt/pixmaps/juxtopposed/InterVariable.ttf', asset_dir / 'InterVariable.ttf')
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))

from juxtopposed_general_preferences_fixture import GeneralPreferences as Preferences
from juxtopposed_backdrop_fixture import software_backdrop_state

app = QGuiApplication([])
preferences = Preferences()
engine = QQmlApplicationEngine()
engine.rootContext().setContextProperty('preferencesAdapter', preferences)
qml_dir_url = QUrl.fromLocalFile(str(proposal / 'qml')).toString()
asset_url = QUrl.fromLocalFile(str(asset_dir.resolve()) + os.sep).toString()
driver = f'''import QtQuick
import QtQuick.Controls.Basic as C
import "{qml_dir_url}" as Settings
C.ApplicationWindow {{
    id: window; visible: true; width: 1080; height: 897; color: "#111111"
    property int advancedRequests: 0
    property int transportCount: 0
    property int fallbackRequests: 0
    property var categoryRequests: []
    QtObject {{
        id: suppliedPalette
        property bool light: false
        readonly property color accent: "#18AAD3"
        readonly property color accentSelected: "#06404C"
        readonly property color accentDim: "#127D9C"
        function color(value) {{
            if (!light) return value
            const colors = {{"#0E0E0E": "#f2f2f2", "#090909": "#fafafa", "#191919": "#e8e8e8", "#222222": "#dddddd", "#8F8F8F": "#323232", "#C6C6C6": "#202020", "#969696": "#323232", "#6B6B6B": "#565656", "#404040": "#787878", "#151515": "#ffffff", "#EAEAEA": "#202020"}}
            return colors[value] || value
        }}
    }}
    Rectangle {{ id: backdropUnderlay; objectName: "settingsTestUnderlay"; anchors.fill: parent; color: window.color }}
    C.Button {{ id: opener; objectName: "opener"; text: "Settings"; onClicked: dialog.showDialog(opener) }}
    Shortcut {{ id: transport; objectName: "transport"; sequence: "Space"; enabled: !dialog.visible; onActivated: window.transportCount++ }}
    Component {{ id: suppliedInterface; FocusScope {{
        required property var preferences
        property var appearance
        property real uiScale: 1
        property url assetBase
        property string searchText
        property var focusNavigator
        readonly property var focusTargets: [interfaceAction]
        implicitHeight: 100 * uiScale
        C.Button {{ id: interfaceAction; objectName: "suppliedInterfaceAction"; text: "Supplied page"; onClicked: preferences.setValue("showDialogs", true) }}
    }} }}
    Settings.JuxtopposedGlobalSettings {{
        id: dialog; parent: window.contentItem
        preferences: preferencesAdapter
        backdropSource: backdropUnderlay
        assetBase: "{asset_url}"
        categoryPages: ({{interface: suppliedInterface}})
        categorySearchTerms: ({{interface: ["layout", "toolbar"]}})
        onAdvancedRequested: window.advancedRequests++
        onFallbackFocusRequested: window.fallbackRequests++
        onCategoryRequested: category => window.categoryRequests.push(category)
    }}
}}'''
engine.loadData(driver.encode(), QUrl.fromLocalFile(str(args.output_dir.resolve() / 'driver.qml')))
assert engine.rootObjects(), messages
window = engine.rootObjects()[0]
dialog = window.findChild(QObject, 'juxtopposedGlobalSettings')
assert dialog, messages
cases = []

def item(name):
    result = window.findChild(QObject, name)
    if result is None:
        def visual(parent):
            for child in parent.childItems():
                if child.objectName() == name: return child
                found = visual(child)
                if found is not None: return found
            return None
        result = visual(window.contentItem())
    assert result, (name, messages)
    return result

def evaluate(code):
    expression = QQmlExpression(qmlContext(window), window, code)
    value, _ = expression.evaluate()
    assert not expression.hasError(), expression.error().toString()
    QTest.qWait(20)
    return value

def reveal(name):
    obj = item(name)
    obj.forceActiveFocus(Qt.TabFocusReason)
    QTest.qWait(30)
    return obj

def click(name):
    obj = reveal(name)
    obj.forceActiveFocus(Qt.MouseFocusReason)
    QTest.mouseClick(window, Qt.LeftButton, Qt.NoModifier, obj.mapToScene(QPointF(obj.width() / 2, obj.height() / 2)).toPoint())
    QTest.qWait(25)

def shot(name):
    QTest.qWait(30)
    assert window.grabWindow().save(str(args.output_dir / name))

def passed(name):
    cases.append(name)
    print('PASS', name, flush=True)

def type_text(text):
    for character in text:
        QGuiApplication.sendEvent(window, QKeyEvent(QEvent.KeyPress, 0, Qt.NoModifier, character))
        QGuiApplication.sendEvent(window, QKeyEvent(QEvent.KeyRelease, 0, Qt.NoModifier, character))
    QTest.qWait(20)

def select(name, index):
    obj = reveal(name)
    QTest.keyClick(window, Qt.Key_Space)
    QTest.qWait(30)
    assert item(name + 'Popup').property('visible'), (name, messages)
    QTest.keyClick(window, Qt.Key_Home)
    for _ in range(index): QTest.keyClick(window, Qt.Key_Down)
    QTest.keyClick(window, Qt.Key_Return)
    QTest.qWait(30)
    assert not item(name + 'Popup').property('visible')

QTest.qWait(100)
click('opener')
assert dialog.property('visible') and item('globalSettingsSearch').hasActiveFocus()
assert preferences.reloads == 1 and preferences.requests == [], 'Opening wrote a preference'
assert not item('transport').property('enabled')
assert [dialog.property('width'), dialog.property('height')] == [1048, 865]
backdrop_state = software_backdrop_state(window, dialog, item('settingsTestUnderlay'))
# A reference capture without a focus ring is taken after genuine pointer focus.
click('globalCategorygeneral')
shot('general-natural-component.png')
passed('Open and reload without writes; modal transport guard and natural 1048x865 geometry')

visible_assets = {}
all_objects = set(window.findChildren(QObject))
def collect_visual(parent):
    for child in parent.childItems():
        all_objects.add(child)
        collect_visual(child)
collect_visual(window.contentItem())
for obj in all_objects:
    name = obj.objectName()
    if 'Original' not in name or not isinstance(obj, QQuickItem) or not obj.isVisible(): continue
    source = obj.property('source')
    if not source: continue
    filename = Path(source.toLocalFile()).name
    assert source.isLocalFile() and (asset_dir / filename).is_file(), (name, source)
    image_status = QQmlExpression(qmlContext(obj), obj, 'Number(status)').evaluate()[0]
    assert image_status == 1, (name, image_status)
    size = [obj.width(), obj.height()]
    assert size in [[24, 24], [18, 18]], (name, size)
    assert obj.scale() == 1
    p = obj.mapToScene(QPointF(0, 0))
    visible_assets[name] = dict(file=filename, naturalSize=size, scale=obj.scale(), scene=[p.x(), p.y()], mirrored=obj.property('mirror'))
assert len(visible_assets) == 24, (len(visible_assets), visible_assets.keys())
assert len({entry['file'] for entry in visible_assets.values()}) == 18
expected_slots = {
    'globalSettingsOriginalSearch': ('global-settings-80d5a.svg', False),
    'globalSettingsOriginalClose': ('new-playlist-close.svg', True),
    'generalLanguageOriginalChevron': ('global-settings-764c9.svg', True),
    'generalLogDetailsOriginalChevron': ('global-settings-764c9.svg', True),
    'generalAdvancedOriginalChevron': ('global-settings-97863.svg', True),
    **{'general' + key + 'InfoOriginal': ('global-settings-1f9fe.svg', False) for key in ['Dialogs', 'Updates', 'Developer', 'Logging', 'LogFile', 'LogDetails']},
    **{'globalCategory' + key + 'OriginalIcon': (filename, mirror) for key, filename, mirror in [
        ('general', 'global-settings-4dae7.svg', False), ('interface', 'global-settings-1bf8a.svg', False),
        ('privacy', 'global-settings-7d179.svg', False), ('controls', 'global-settings-36bfa.svg', False),
        ('playback', 'global-settings-ad68b.svg', False), ('video', 'global-settings-ec3f6.svg', True),
        ('audio', 'audio.svg', False), ('subtitles', 'subtitles.svg', False),
        ('library', 'global-settings-cf6e1.svg', True), ('network', 'global-settings-6db55.svg', True),
        ('performance', 'global-settings-66cd9.svg', False), ('recording', 'global-settings-08f95.svg', False),
        ('extensions', 'global-settings-05bc9.svg', False)]}}
assert set(visible_assets) == set(expected_slots)
for name, (filename, mirrored) in expected_slots.items():
    assert visible_assets[name]['file'] == filename and visible_assets[name]['mirrored'] is mirrored, name
    if 'Category' in name:
        obj = item(name)
        slot = obj.parentItem()
        local = obj.mapToItem(slot, QPointF(0, 0))
        assert [local.x(), local.y()] == [4, 4], (name, local)
for category in ['general', 'interface', 'privacy', 'controls', 'playback', 'video', 'audio', 'subtitles', 'library', 'network', 'performance', 'recording', 'extensions']:
    slot = item('globalCategory' + category + 'IconSlot')
    assert [slot.width(), slot.height()] == [32, 32]
assert item('generalLanguage').width() == 320 and item('generalLanguage').height() == 36
assert item('generalBrowseLog').width() == 100 and item('generalBrowseLog').height() == 32
assert item('generalDialogs').width() == 68 and item('generalDialogs').height() == 32
passed('All 20 unchanged asset identities; all 24 actual General image instances and 18 unique visible exports have exact natural dimensions and slot geometry')

for name, native in [('generalThemeLight', 1), ('generalThemeSystem', 0), ('generalThemeDark', 2)]:
    click(name)
    assert preferences.requests[-1] == ['theme', native] and item(name).property('selected')
reveal('generalThemeDark')
QTest.keyClick(window, Qt.Key_Right)
QTest.qWait(20)
assert item('generalThemeLight').hasActiveFocus() and preferences._values['theme'] == 1
passed('Dark/Light/System actual native enum mapping and radio arrow-key focus')

for index in range(9):
    click('generalAccent' + str(index))
    assert preferences.requests[-1] == ['accentIndex', index]
    assert item('generalAccent' + str(index)).property('selected')
preferences.patch(accentIndex=-1)
QTest.qWait(20)
before = list(preferences.requests)
assert all(not item('generalAccent' + str(index)).property('selected') for index in range(9))
assert preferences.requests == before
click('generalAccent0')
QTest.keyClick(window, Qt.Key_Left)
QTest.qWait(20)
assert preferences._values['accentIndex'] == 8 and item('generalAccent8').hasActiveFocus()
passed('Nine real swatch choices; unknown custom color retained without load-time coercion; wraparound arrow selection')

select('generalLanguage', 2)
assert preferences.requests[-1] == ['language', 'fi'] and item('generalLanguage').property('currentText') == 'Suomi'
select('generalLogDetails', 2)
assert preferences.requests[-1] == ['logDetails', 2] and item('generalLogDetails').property('currentIndex') == 2
preferences.patch(logDetails=-1)
QTest.qWait(20)
assert item('generalLogDetails').property('currentIndex') == -1
assert 'custom' in item('generalLogDetails').property('displayText')
passed('Keyboard dropdown dispatch uses exact language codes and legal log-detail integers; legacy custom detail remains unselected')

for name, key in [('generalDialogs', 'showDialogs'), ('generalUpdates', 'autoUpdates'), ('generalDeveloper', 'developerMode'), ('generalLogging', 'loggingEnabled')]:
    previous = preferences._values[key]
    click(name)
    assert preferences.requests[-1] == [key, not previous]
    assert item(name).property('selected') is not previous
assert not item('generalLogFile').property('enabled') and not item('generalBrowseLog').property('enabled') and not item('generalLogDetails').property('enabled')
click('generalLogging')
assert item('generalLogFile').property('enabled') and item('generalBrowseLog').property('enabled') and item('generalLogDetails').property('enabled')
click('generalBrowseLog')
assert preferences.pickers == 1
assert item('globalSettingsRestart').property('visible')
passed('Boolean toggles dispatch strict booleans; logger-dependent controls disable; native picker method and restart feedback dispatch')

field = reveal('generalLogFile')
QTest.keyClick(window, Qt.Key_A, Qt.ControlModifier)
type_text("C:/Generated/quote's log.txt")
QTest.keyClick(window, Qt.Key_Tab)
QTest.qWait(30)
assert preferences.requests[-1] == ['logFile', "C:/Generated/quote's log.txt"]
assert item('generalBrowseLog').hasActiveFocus()
preferences.fail_next = 'The log file could not be saved.'
field = reveal('generalLogFile')
QTest.keyClick(window, Qt.Key_A, Qt.ControlModifier)
type_text('C:/rejected.txt')
QTest.keyClick(window, Qt.Key_Tab)
QTest.qWait(30)
assert field.property('text') == "C:/Generated/quote's log.txt"
assert item('globalSettingsError').property('text') == 'The log file could not be saved.'
assert item('globalSettingsError').property('visible')
preferences.fail_next = 'The language could not be saved.'
select('generalLanguage', 3)
assert item('generalLanguage').property('currentIndex') == 2 and preferences._values['language'] == 'fi'
preferences._updates = False
preferences.updatesAvailableChanged.emit()
QTest.qWait(20)
assert not item('generalUpdates').property('enabled')
before_dialogs = preferences._values['showDialogs']
preferences.fail_next = 'The dialog preference could not be saved.'
click('generalDialogs')
assert preferences._values['showDialogs'] is before_dialogs and item('generalDialogs').property('selected') is before_dialogs
before_accent = preferences._values['accentIndex']
preferences.fail_next = 'The accent could not be saved.'
click('generalAccent3')
assert preferences._values['accentIndex'] == before_accent and item('generalAccent' + str(before_accent)).property('selected')
passed('Literal log-file text commit; failed text/dropdown writes restore native current values and show error; unavailable updater disabled')

click('globalSettingsClose')
assert not dialog.property('visible') and item('opener').hasActiveFocus() and item('transport').property('enabled')
click('opener')
assert preferences.reloads == 2
assert field.property('text') == "C:/Generated/quote's log.txt" and not item('globalSettingsError').property('visible')
reveal('generalAdvanced')
QTest.keyClick(window, Qt.Key_Tab)
QTest.qWait(20)
assert item('globalSettingsSearch').hasActiveFocus(), 'Forward tab left the modal'
QTest.keyClick(window, Qt.Key_Backtab)
QTest.qWait(20)
assert item('generalAdvanced').hasActiveFocus(), 'Backward tab left the modal'
passed('Modal forward/backward focus boundary wraps; ordinary close restores opener; reopen reloads and preserves supplied native values')

search = reveal('globalSettingsSearch')
type_text('log')
QTest.qWait(30)
assert item('generalLogging').property('visible') and not item('generalThemeDark').isVisible()
assert item('generalLogDetails').isVisible() and not item('globalSettingsNoResults').property('visible')
QTest.keyClick(window, Qt.Key_A, Qt.ControlModifier)
type_text('a nonexistent setting')
QTest.qWait(30)
assert item('globalSettingsNoResults').property('visible') and not item('generalLogging').isVisible()
QTest.keyClick(window, Qt.Key_A, Qt.ControlModifier)
type_text('toolbar')
QTest.qWait(30)
assert dialog.property('selectedCategory') == 'interface'
assert item('suppliedInterfaceAction').property('enabled')
assert Path(item('globalCategoryinterfaceOriginalIcon').property('source').toLocalFile()).name == 'global-settings-f1a4a.svg'
QTest.keyClick(window, Qt.Key_A, Qt.ControlModifier)
QTest.keyClick(window, Qt.Key_Backspace)
QTest.qWait(30)
assert not item('globalCategoryprivacy').property('enabled')
click('globalCategorygeneral')
assert dialog.property('selectedCategory') == 'general'
search = reveal('globalSettingsSearch')
type_text('General')
assert item('generalThemeDark').isVisible(), 'Searching the selected category hid its whole body'
QTest.keyClick(window, Qt.Key_A, Qt.ControlModifier)
QTest.keyClick(window, Qt.Key_Backspace)
QTest.qWait(20)
passed('Search filters genuine available settings/categories, no-results state, supplied real-page contract and exact active Interface icon; no fake category fallback')

geometry = []
for width, height, scale in [(1080, 897, 1), (640, 420, 1), (960, 600, 1.5), (1280, 720, 2)]:
    window.resize(width, height)
    dialog.setProperty('uiScale', scale)
    QTest.qWait(40)
    assert dialog.property('width') <= width and dialog.property('height') <= height
    for name in ['globalSettingsSearch', 'globalSettingsClose', 'generalLanguage', 'generalDialogs', 'generalLogFile', 'generalBrowseLog', 'generalLogDetails', 'generalAdvanced']:
        obj = reveal(name)
        assert obj.hasActiveFocus(), (name, width, height, scale)
        p = obj.mapToScene(QPointF(0, 0))
        assert p.x() >= -.5 and p.y() >= -.5 and p.x() + obj.width() <= width + .5 and p.y() + obj.height() <= height + .5, (name, width, height, scale, p, obj.width(), obj.height())
        if name.startswith('general'):
            viewport = item('globalSettingsPageScroll')
            top = viewport.mapToScene(QPointF(0, 0)).y()
            assert p.y() >= top - .5 and p.y() + obj.height() <= top + viewport.height() + .5, (name, 'focused control not revealed', width, height, scale)
    for name, expected in expected_slots.items():
        obj = item(name)
        assert [obj.width(), obj.height()] in [[24, 24], [18, 18]] and obj.scale() == scale
        assert Path(obj.property('source').toLocalFile()).name == expected[0]
        effective = obj.mapRectToItem(obj.parentItem(), obj.boundingRect())
        assert abs(effective.width() - obj.width() * scale) < .01 and abs(effective.height() - obj.height() * scale) < .01
        if 'Category' in name:
            assert abs(effective.x() - 4 * scale) < .01 and abs(effective.y() - 4 * scale) < .01, (name, scale, effective)
    geometry.append({'window': [width, height], 'scale': scale, 'dialog': [dialog.property('width'), dialog.property('height')]})
    if width == 640: shot('general-narrow-focused-component.png')
    if scale == 2: shot('general-double-scale-focused-component.png')
passed('Actual focused controls remain in bounds and reveal inside scrolling viewport at natural, narrow, 150% and 200% layouts; natural SVG geometry retained')

evaluate('dialog.appearance = suppliedPalette; suppliedPalette.light = true')
assert item('generalLanguage').property('contentItem').property('color').name() == '#323232'
assert dialog.property('background').property('color').name() == '#f2f2f2'
assert item('juxtopposedGeneralSettings').property('accent').name() == '#18aad3'
shot('general-supplied-light-palette-component.png')
evaluate('dialog.appearance = null; dialog.preferences = null')
assert not item('generalLanguage').property('enabled') and not item('generalDeveloper').property('enabled')
assert not item('generalBrowseLog').property('enabled') and not item('generalUpdates').property('enabled')
evaluate('dialog.preferences = preferencesAdapter')
assert item('generalLanguage').property('enabled')
passed('Injected light/accent palette updates rendered colors; unavailable native object disables real controls without writes or Qt errors')

click('generalAdvanced')
assert not dialog.property('visible') and window.property('advancedRequests') == 1 and item('opener').hasActiveFocus()
click('opener')
QTest.keyClick(window, Qt.Key_Escape)
QTest.qWait(20)
assert not dialog.property('visible') and item('opener').hasActiveFocus()
assert window.property('transportCount') == 0
passed('Advanced closes then dispatches intentional expert preference action exactly once; Escape closes and restores focus')

# Include initialization, interaction and explicit QML teardown in one ledger.
teardown_done = []
engine.destroyed.connect(lambda: teardown_done.append(True))
window.close()
engine.deleteLater()
QCoreApplication.sendPostedEvents(None, QEvent.DeferredDelete)
QTest.qWait(50)
assert teardown_done == [True], 'The QML engine was not explicitly destroyed'
assert messages == [], messages
qmlformat = Path(PySide6.__file__).parent / ('qmlformat.exe' if os.name == 'nt' else 'qmlformat')
for qml_source in [proposal / 'qml/JuxtopposedGlobalSettings.qml', proposal / 'qml/JuxtopposedGeneralSettings.qml', proposal / 'qml/JuxtopposedSettingsBackdrop.qml', proposal / 'qml/JuxtopposedSettingsBackdropEffect.qml']:
    result = subprocess.run([str(qmlformat), '--ignore-settings', str(qml_source)], stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE, text=True, timeout=20)
    assert result.returncode == 0 and not result.stderr, (qml_source.name, result.returncode, result.stderr)
verification = {'passed': True, 'cases': cases, 'geometry': geometry, 'messages': messages,
    'assets': asset_paths, 'visibleAssetInstances': visible_assets, 'backdropSoftwareFallback': backdrop_state,
    'sourceSha256': {str(path.relative_to(repo)).replace('\\', '/'): hashlib.sha256(path.read_bytes()).hexdigest() for path in [*[proposal / 'qml/JuxtopposedGlobalSettings.qml', proposal / 'qml/JuxtopposedGeneralSettings.qml', proposal / 'qml/JuxtopposedSettingsBackdrop.qml', proposal / 'qml/JuxtopposedSettingsBackdropEffect.qml'], Path(__file__).resolve()]},
    'designFixtureSha256': hashlib.sha256(reference.read_bytes()).hexdigest(),
    'scope': 'Integrated production General QML and assets with strict supplied QObject preferences, software-rendered offscreen. Geometry, static asset identities/slots, component keyboard/pointer/focus/search/write/error behavior only.',
    'nativePersistencePickerAppearanceDiagnosticsAcceptance': False,
    'fullBetaDesignAcceptance': False}
(args.output_dir / 'verification.json').write_text(json.dumps(verification, indent=2) + '\n', encoding='utf-8', newline='\n')
print('PASS production General component checks. Native app and full design acceptance remain separate.', flush=True)
