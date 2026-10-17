// SPDX-License-Identifier: GPL-2.0-or-later
// Original dark design and accent swatches: Juxtopposed, CC BY 4.0.
pragma Singleton
import QtQuick

QtObject {
    id: root

    // VLCStyle supplies the actual native providers. Standalone component
    // previews retain the authored dark default without requiring a VLC engine.
    property var nativePalette: null
    property var nativeView: null
    property var nativeWindow: null
    property var preferences: null

    readonly property bool dark: nativePalette ? nativePalette.isDark : true
    readonly property int accentIndex: preferences ? preferences.values.accentIndex : 0
    // Exact fills from Global Settings 999:5047, 3046:9875 .. 3046:9891.
    readonly property var accentColors: ["#D37918", "#E15032", "#C49E23",
        "#19A234", "#18AAD3", "#185DD3", "#8E42FF", "#F052B3", "#AFAFAF"]
    readonly property color accent: accentIndex === 0 && !dark ? "#E8851A"
        : accentIndex >= 0 && accentIndex < accentColors.length
        ? accentColors[accentIndex] : nativeView ? nativeView.accent : "#D37918"
    // The Light player's play-button fill remains orange #D37918, while its
    // timeline uses Light/Accent/Main #E8851A (2425:13645).
    readonly property color playButtonAccent: accentIndex === 0 ? "#D37918" : accent
    readonly property color accentSelected: accentIndex === 0 ? (dark ? "#371F06" : "#D8BA9A")
        : dark ? Qt.darker(accent, 3.8) : Qt.rgba(accent.r, accent.g, accent.b, 0.16)
    readonly property color accentDim: dark && accentIndex === 0 ? "#AB6314"
        : Qt.darker(accent, 1.23)

    // Placeholder text uses foreground colors; dark border tokens also occur in inputs.
    function placeholderColor(darkValue) {
        return dark ? darkValue : nativeView ? nativeView.fg.secondary : "#4D4D4D"
    }

    function icon(name) {
        const variants = {"volume.svg": "volume-light.svg",
            "volume-background.svg": "volume-background-light.svg",
            "volume-fill.svg": "volume-fill-light.svg", "audio.svg": "audio-light.svg",
            "subtitles.svg": "subtitles-light.svg", "queue.svg": "queue-light.svg",
            "fullscreen.svg": "fullscreen-light.svg"}
        return !dark && name in variants ? variants[name] : name
    }

    // Keep each existing dark token byte-for-byte; light adaptations use the
    // existing native VLC palette, not unprovided Figma light-theme tokens.
    function color(value) {
        const key = String(value).toUpperCase()
        if (key === "#D37918") return accent
        if (key === "#371F06") return accentSelected
        if (key === "#AB6314") return accentDim
        const accentVariants = {
            "#4D2C09": 2.74, "#6B431C": 1.97, "#20160C": 6.59,
            "#B16614": 1.19, "#49290C": 2.89, "#331D05": 4.14
        }
        if (key in accentVariants && !(dark && accentIndex === 0))
            return dark ? Qt.darker(accent, accentVariants[key]) : accentSelected
        if (key === "#E39B60" && accentIndex !== 0)
            return Qt.lighter(accent, 1.18)
        if (dark) return value

        // Media black and modal dimming deliberately remain content neutral.
        if (key === "#000000" || key === "#80000000") return value
        // These Light tokens are confirmed by Player - Light 2425:13624.
        // Their use outside that frame is an adaptation, not a fidelity claim.
        if (key === "#969696") return "#2E2E2E"
        if (key === "#6B6B6B") return "#6B6B6B"
        if (key === "#090909") return "#DEDEDE"
        if (key === "#1B1B1B") return Qt.rgba(213/255, 213/255, 213/255, 0.75)
        if (key === "#252525") return "#ACACAC"
        const viewBg = Qt.darker(nativeView ? nativeView.bg.primary : "#FAFAFA", 1)
        const windowBg = Qt.darker(nativeWindow ? nativeWindow.bg.primary : "#EDEDED", 1)
        const primary = Qt.darker(nativeView ? nativeView.fg.primary : "#212121", 1)
        const secondary = Qt.darker(nativeView ? nativeView.fg.secondary : "#4D4D4D", 1)
        if (["#FFFFFF", "#EAEAEA", "#C6C6C6", "#BEBEBE"].includes(key))
            return primary
        if (["#969696", "#8F8F8F", "#6B6B6B", "#686868", "#626262",
             "#606060", "#555555", "#454545", "#4F4F4F"].includes(key))
            return secondary
        if (key === "#D98779") return nativeView ? nativeView.fg.negative : "#FF0000"
        if (["#050505", "#090909", "#0B0B0B", "#0E0E0E", "#0F0F0F"].includes(key))
            return viewBg
        if (["#111111", "#141414", "#151515", "#161616", "#171717",
             "#191919", "#1A1A1A", "#1B1B1B", "#1E1E1E"].includes(key))
            return windowBg
        if (["#202020", "#222222", "#242424", "#252525", "#292929",
             "#2D2D2D", "#303030", "#404040", "#494949"].includes(key))
            return nativeView ? nativeView.separator : "#E0E0E0"
        if (key === "#EE111111")
            return Qt.rgba(windowBg.r, windowBg.g, windowBg.b, 238 / 255)
        if (key === "#F2191919")
            return Qt.rgba(windowBg.r, windowBg.g, windowBg.b, 242 / 255)
        if (key === "#24FFFFFF")
            return Qt.rgba(primary.r, primary.g, primary.b, 36 / 255)
        return value
    }
}
