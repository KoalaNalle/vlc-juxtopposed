# SPDX-License-Identifier: GPL-2.0-or-later
"""Assert real backdrop fallback state without replacing any native service."""
from PySide6.QtCore import QObject
from PySide6.QtTest import QTest

def software_backdrop_state(window, dialog, expected_source):
    QTest.qWait(35)
    background = dialog.property('background')
    assert background is not None and background.objectName() == 'globalSettingsBackdrop'
    assert background.property('sourceItem') is expected_source
    assert background.property('active') and background.property('sourceIsSafe')
    assert not background.property('effectsAvailable') and not background.property('blurReady')
    loader = background.findChild(QObject, 'globalSettingsBackdropEffectLoader')
    assert loader is not None and not loader.property('active') and loader.property('item') is None
    assert background.property('capturedSource') is None
    assert background.property('fillOpacity') == .95
    scale = background.property('uiScale')
    assert background.property('radius') == 14 * scale and background.property('blurRadius') == 25 * scale
    assert expected_source is not window.contentItem()  # Explicit underlying sibling, never recursive window overlay.
    return dict(scope='Actual software fallback state; no RHI/shadow/native-video acceptance',
        safeExplicitUnderlyingSource=True,effectsLoaded=False,blurReady=False,fillOpacity=.95,
        cornerRadius=14 * scale,requestedBlurRadius=25 * scale)
