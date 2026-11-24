import QtQuick 2.12
import QtQuick.Controls 2.12
import Hmi.Ui 1.0

// 270 degree arc gauge drawn on a Canvas: value arc, a white setpoint tick
// and a red band marking the alarm zone above the high limit. The number in
// the centre shows the exact value; the arc is animated slightly so 5 Hz
// updates look smooth. With `valid` false (no data, connection lost) the
// gauge shows a dash instead of a value that might be stale.
Item {
    id: gauge

    property real value: 0
    property bool valid: true
    property real minimum: 0
    property real maximum: 100
    property real setpoint: NaN
    property real limit: NaN
    property bool alarm: false
    property int decimals: 0
    property string unit

    property real shownValue: value
    Behavior on shownValue {
        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
    }

    readonly property real lineWidth: Math.max(6, Math.round(Math.min(width, height) * 0.075))
    readonly property real markerSize: Math.max(8, lineWidth * 0.9)
    // A 270 degree arc is about 15 % shorter than it is wide: fit the radius
    // to both dimensions and centre the arc vertically.
    readonly property real radius: Math.max(10, Math.min(width / 2, height / 1.707) - lineWidth / 2 - markerSize - 2)
    readonly property real centerX: width / 2
    readonly property real centerY: height / 2 + radius * 0.146

    implicitWidth: Theme.px(240)
    implicitHeight: Theme.px(200)

    function angle(v) {
        var span = maximum - minimum
        var fraction = span > 0 ? (v - minimum) / span : 0
        fraction = Math.max(0, Math.min(1, fraction))
        return (135 + 270 * fraction) * Math.PI / 180
    }

    onShownValueChanged: canvas.requestPaint()
    onSetpointChanged: canvas.requestPaint()
    onLimitChanged: canvas.requestPaint()
    onAlarmChanged: canvas.requestPaint()
    onValidChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var cx = gauge.centerX, cy = gauge.centerY
            var r = gauge.radius, lw = gauge.lineWidth
            var start = gauge.angle(gauge.minimum), end = gauge.angle(gauge.maximum)

            // Track
            ctx.lineCap = "round"
            ctx.lineWidth = lw
            ctx.strokeStyle = Theme.track
            ctx.beginPath()
            ctx.arc(cx, cy, r, start, end, false)
            ctx.stroke()

            // Alarm zone: thin band just outside the track
            if (!isNaN(gauge.limit) && gauge.limit < gauge.maximum) {
                var band = Math.max(2, Math.round(lw * 0.22))
                ctx.lineCap = "butt"
                ctx.lineWidth = band
                ctx.strokeStyle = Theme.critical
                ctx.beginPath()
                ctx.arc(cx, cy, r + lw / 2 + band / 2 + 2, gauge.angle(gauge.limit), end, false)
                ctx.stroke()
            }

            // Value
            if (gauge.valid && gauge.shownValue > gauge.minimum) {
                ctx.lineCap = "round"
                ctx.lineWidth = lw
                ctx.strokeStyle = gauge.alarm ? Theme.warning : Theme.accent
                ctx.beginPath()
                ctx.arc(cx, cy, r, start, gauge.angle(gauge.shownValue), false)
                ctx.stroke()
            }

            // Setpoint: radial tick across the arc, drawn on top of the value
            if (!isNaN(gauge.setpoint)) {
                var a = gauge.angle(gauge.setpoint)
                var inner = r - lw / 2 - 3
                var outer = r + lw / 2 + gauge.markerSize * 0.5
                ctx.lineCap = "round"
                ctx.lineWidth = Math.max(2, Math.round(lw * 0.25))
                ctx.strokeStyle = Theme.textPrimary
                ctx.beginPath()
                ctx.moveTo(cx + inner * Math.cos(a), cy + inner * Math.sin(a))
                ctx.lineTo(cx + outer * Math.cos(a), cy + outer * Math.sin(a))
                ctx.stroke()
            }
        }
    }

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        y: gauge.centerY - valueLabel.height * 0.62
        spacing: 0

        Label {
            id: valueLabel
            anchors.horizontalCenter: parent.horizontalCenter
            text: gauge.valid ? Strings.number(gauge.value, gauge.decimals) : "\u2013"
            color: !gauge.valid ? Theme.textMuted : gauge.alarm ? Theme.warning : Theme.textPrimary
            font.pixelSize: Theme.fontValue
            font.weight: Font.DemiBold
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Strings.unit(gauge.unit)
            color: Theme.textSecondary
            font.pixelSize: Theme.fontBody
        }
    }
}
