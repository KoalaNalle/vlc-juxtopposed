import QtQuick
import QtQuick.Controls.Basic as C
import VLC.Style
import "file:///C:/Modding/vlc-juxtopposed/build-windows/beta-subtitle-accessibility-20261023/component-r3/qml" as Dialogs
C.ApplicationWindow{
 id:window;visible:true;width:1080;height:897;property bool omitSubtitles:false
 QtObject{id:nativePalette;property bool isDark:suppliedContext.generalPreferences.values.theme!==1}
 Binding{target:JuxtopposedPalette;property:"nativePalette";value:nativePalette}
 Binding{target:JuxtopposedPalette;property:"preferences";value:suppliedContext.generalPreferences}
 QtObject{id:generalOnly;property var generalPreferences:suppliedContext.generalPreferences}
 Rectangle{id:underlay;anchors.fill:parent;color:"#173c66"}
 C.Button{id:opener;objectName:"opener";text:"Settings";onClicked:suppliedProvider.showGlobalSettingsDialog()}
 Dialogs.JuxtopposedDialogsHost{anchors.fill:parent;mainCtx:window.omitSubtitles?generalOnly:suppliedContext;dialogsProvider:suppliedProvider;appearance:JuxtopposedPalette;assetBase:"file:///C:/Modding/vlc-juxtopposed/build-windows/beta-subtitle-accessibility-20261023/component-r3/assets/";backdropSource:underlay;fallbackFocusItem:opener}
}