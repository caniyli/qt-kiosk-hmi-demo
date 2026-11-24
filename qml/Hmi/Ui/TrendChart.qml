import QtQuick 2.12
import QtQuick.Controls 2.12
import Hmi.Ui 1.0

// Line chart of the latest samples of one signal, newest at the right edge.
// Setpoint and high limit are drawn as dashed reference lines. One signal per
// chart keeps a single y axis; the page switches between signals.
Item {
    id: chart

    property var values: []
    property int capacity: 600          // samples across the full width
    property real windowSeconds: 120    // time span of `capacity` samples
    property real minimum: 0
    property real maximum: 100
    property real setpoint: NaN
    property real limit: NaN
    property int gridIntervals: 5

    readonly property real leftMargin: Theme.px(56)
    readonly property real rightMargin: Theme.px(8)
    readonly property real topMargin: Theme.px(10)
    readonly property real bottomMargin: Theme.px(30)
    readonly property real plotWidth: Math.max(1, width - leftMargin - rightMargin)
    readonly property real plotHeight: Math.max(1, height - topMargin - bottomMargin)

    // Time grid: a round step giving four to five intervals across the window.
    readonly property real tickSeconds: {
        var steps = [5, 10, 15, 20, 30, 60, 120, 300, 600]
        for (var i = 0; i < steps.length; ++i) {
            if (steps[i] >= windowSeconds / 5)
                return steps[i]
        }
        return steps[steps.length - 1]
    }
    readonly property int tickCount: Math.floor(windowSeconds / tickSeconds + 0.001) + 1

    function xForSecondsAgo(seconds) {
        return leftMargin + plotWidth * (1 - seconds / windowSeconds)
    }

    function yFor(v) {
        var span = maximum - minimum
        var fraction = span > 0 ? (v - minimum) / span : 0
        return topMargin + plotHeight * (1 - fraction)
    }

    onValuesChanged: canvas.requestPaint()
    onSetpointChanged: canvas.requestPaint()
    onLimitChanged: canvas.requestPaint()
    onMinimumChanged: canvas.requestPaint()
    onMaximumChanged: canvas.requestPaint()
    onWindowSecondsChanged: canvas.requestPaint()

    // Y axis labels
    Repeater {
        model: chart.gridIntervals + 1
        Label {
            readonly property real tick: chart.minimum + (chart.maximum - chart.minimum) * index / chart.gridIntervals
            x: 0
            y: chart.yFor(tick) - height / 2
            width: chart.leftMargin - Theme.px(10)
            horizontalAlignment: Text.AlignRight
            text: Strings.number(tick, 0)
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
        }
    }

    // Time axis labels, counted back from "now" at the right edge
    Repeater {
        model: chart.tickCount
        Label {
            readonly property real tickX: chart.xForSecondsAgo(index * chart.tickSeconds)
            x: index === 0 ? tickX - width : Math.max(chart.leftMargin, tickX - width / 2)
            y: chart.height - chart.bottomMargin + Theme.px(8)
            text: index === 0 ? qsTr("now") : "−" + Strings.number(index * chart.tickSeconds, 0) + " s"
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
        }
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var x0 = chart.leftMargin, y0 = chart.topMargin
            var w = chart.plotWidth, h = chart.plotHeight
            var i, y

            // Grid
            ctx.lineWidth = 1
            ctx.strokeStyle = Theme.grid
            for (i = 0; i <= chart.gridIntervals; ++i) {
                y = Math.round(y0 + h * i / chart.gridIntervals) + 0.5
                ctx.beginPath()
                ctx.moveTo(x0, y)
                ctx.lineTo(x0 + w, y)
                ctx.stroke()
            }
            for (i = 1; i < chart.tickCount; ++i) {
                var gx = Math.round(chart.xForSecondsAgo(i * chart.tickSeconds)) + 0.5
                ctx.beginPath()
                ctx.moveTo(gx, y0)
                ctx.lineTo(gx, y0 + h)
                ctx.stroke()
            }

            // Dashes are drawn as separate segments: in Qt 5 a pattern set
            // with setLineDash() cannot be cleared again for later strokes.
            function referenceLine(v, color, dash, gap) {
                if (isNaN(v) || v < chart.minimum || v > chart.maximum)
                    return
                var ly = Math.round(chart.yFor(v)) + 0.5
                ctx.strokeStyle = color
                ctx.lineWidth = 1.5
                ctx.beginPath()
                for (var dx = x0; dx < x0 + w; dx += dash + gap) {
                    ctx.moveTo(dx, ly)
                    ctx.lineTo(Math.min(dx + dash, x0 + w), ly)
                }
                ctx.stroke()
            }
            referenceLine(chart.limit, Theme.critical, 6, 4)
            referenceLine(chart.setpoint, Theme.textSecondary, 2, 4)

            // Signal
            var data = chart.values
            var n = data ? data.length : 0
            if (n < 2)
                return
            var step = w / Math.max(1, chart.capacity - 1)
            var first = Math.max(0, n - chart.capacity)
            var xStart = x0 + w - (n - first - 1) * step

            ctx.save()
            ctx.beginPath()
            ctx.rect(x0, y0, w, h)
            ctx.clip()

            ctx.beginPath()
            for (i = first; i < n; ++i) {
                var px = xStart + (i - first) * step
                var py = chart.yFor(data[i])
                if (i === first)
                    ctx.moveTo(px, py)
                else
                    ctx.lineTo(px, py)
            }
            ctx.lineWidth = 2
            ctx.lineJoin = "round"
            ctx.strokeStyle = Theme.accent
            ctx.stroke()

            // Soft fill under the line
            ctx.lineTo(x0 + w, y0 + h)
            ctx.lineTo(xStart, y0 + h)
            ctx.closePath()
            var gradient = ctx.createLinearGradient(0, y0, 0, y0 + h)
            gradient.addColorStop(0, Theme.withAlpha(Theme.accent, 0.22))
            gradient.addColorStop(1, Theme.withAlpha(Theme.accent, 0.0))
            ctx.fillStyle = gradient
            ctx.fill()
            ctx.restore()
        }
    }
}
