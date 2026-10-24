pragma Singleton
import QtQuick
import QtQuick.Window
QtObject {
    property int controlAutoHideMode: 0
    property int controlsHideTimeout: 100000
    property bool hasEmbededVideo: false
    property bool showFullscreenControls: true
    property QtObject intfMainWindow: QtObject { property int visibility: Window.Windowed }
}
