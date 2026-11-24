pragma Singleton
import QtQuick 2.12

// Colours and sizes for the whole UI. Sizes are given for the 1280x800 design
// resolution and multiplied by `scale`, which the main window keeps in sync
// with its real size, so the layout works on other panels as well.
//
// Colour is reserved for machine state and alarms; process values use a
// single accent colour.
QtObject {
    property real scale: 1.0
    function px(value) { return Math.round(value * scale) }

    readonly property color background: "#0d1115"
    readonly property color surface: "#161c23"
    readonly property color surfaceRaised: "#1f2730"
    readonly property color surfacePressed: "#2c3743"
    readonly property color border: "#2a333d"
    readonly property color track: "#27313b"
    readonly property color grid: "#222b34"

    readonly property color textPrimary: "#e8edf2"
    readonly property color textSecondary: "#9eabb8"
    readonly property color textMuted: "#6b7885"
    readonly property color textOnColor: "#ffffff"
    readonly property color textOnWarning: "#1a1405"

    readonly property color accent: "#4a94ea"
    readonly property color good: "#0ca30c"
    readonly property color warning: "#fab219"
    readonly property color critical: "#d03b3b"
    readonly property color neutral: "#7d8a97"

    readonly property int radius: px(10)
    readonly property int spacing: px(16)
    readonly property int touchTarget: px(64)
    readonly property int headerHeight: px(72)
    readonly property int navWidth: px(120)

    readonly property int fontSmall: px(14)
    readonly property int fontBody: px(18)
    readonly property int fontTitle: px(22)
    readonly property int fontLarge: px(28)
    readonly property int fontValue: px(44)

    function withAlpha(color, alpha) { return Qt.rgba(color.r, color.g, color.b, alpha) }
}
