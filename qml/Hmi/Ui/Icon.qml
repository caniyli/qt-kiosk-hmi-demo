import QtQuick 2.12
import Hmi.Ui 1.0

// Small icons drawn with plain QML items, so the project ships no image
// assets. Supported names: "overview", "alarms", "settings".
Item {
    id: icon

    property string name
    property color color: Theme.textPrimary

    implicitWidth: Theme.px(28)
    implicitHeight: implicitWidth

    onColorChanged: triangle.requestPaint()

    // Four tiles
    Grid {
        visible: icon.name === "overview"
        anchors.centerIn: parent
        columns: 2
        spacing: Math.round(icon.width * 0.14)
        Repeater {
            model: 4
            Rectangle {
                width: Math.round(icon.width * 0.4)
                height: width
                radius: width * 0.25
                color: icon.color
            }
        }
    }

    // Warning triangle
    Canvas {
        id: triangle
        visible: icon.name === "alarms"
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var w = width, h = height
            var lw = Math.max(2, w * 0.09)
            ctx.strokeStyle = icon.color
            ctx.fillStyle = icon.color
            ctx.lineWidth = lw
            ctx.lineJoin = "round"
            ctx.lineCap = "round"
            ctx.beginPath()
            ctx.moveTo(w * 0.5, h * 0.08)
            ctx.lineTo(w * 0.95, h * 0.9)
            ctx.lineTo(w * 0.05, h * 0.9)
            ctx.closePath()
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(w * 0.5, h * 0.4)
            ctx.lineTo(w * 0.5, h * 0.6)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(w * 0.5, h * 0.74, lw * 0.6, 0, 2 * Math.PI)
            ctx.fill()
        }
    }

    // Three sliders
    Column {
        visible: icon.name === "settings"
        anchors.centerIn: parent
        spacing: Math.round(icon.height * 0.12)
        Repeater {
            model: [0.25, 0.75, 0.45]
            Item {
                width: icon.width
                height: Math.round(icon.height * 0.2)
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    height: Math.max(2, Math.round(icon.height * 0.08))
                    radius: height / 2
                    color: icon.color
                    opacity: 0.55
                }
                Rectangle {
                    x: modelData * (parent.width - width)
                    width: parent.height
                    height: width
                    radius: width / 2
                    color: icon.color
                }
            }
        }
    }
}
