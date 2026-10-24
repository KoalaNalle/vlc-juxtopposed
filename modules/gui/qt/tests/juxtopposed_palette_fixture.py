# SPDX-License-Identifier: GPL-2.0-or-later
"""Register the actual pure-QML palette for supplied-controller previews.

The native application binds providers in VLCStyle. These tests deliberately
exercise the authored default without importing the unavailable native plugin.
"""
from pathlib import Path
from PySide6.QtCore import QUrl
from PySide6.QtQml import qmlRegisterSingletonType

def register_juxtopposed_palette():
    source = Path(__file__).resolve().parents[1] / 'style/JuxtopposedPalette.qml'
    qmlRegisterSingletonType(QUrl.fromLocalFile(str(source)),
                            'VLC.Style', 1, 0, 'JuxtopposedPalette')
