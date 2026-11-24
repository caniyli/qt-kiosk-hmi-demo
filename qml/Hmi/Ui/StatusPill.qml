import QtQuick 2.12
import QtQuick.Controls 2.12
import Hmi.Ui 1.0

// Rounded status indicator: a dot plus a label, so state is never shown by
// colour alone. `filled` paints the whole pill (used for alarms) and
// `blinking` flashes the dot.
Rectangle {
    id: pill

    property alias text: label.text
    property color statusColor: Theme.neutral
    property bool filled: false
    property bool blinking: false
    property bool compact: false
    property bool blinkOn: true

    implicitHeight: compact ? Theme.px(28) : Theme.px(48)
    implicitWidth: row.implicitWidth + (compact ? Theme.px(20) : Theme.px(40))
    radius: height / 2
    color: filled ? statusColor : Theme.withAlpha(statusColor, 0.14)
    border.color: statusColor
    border.width: compact ? 1 : Theme.px(2)

    Row {
        id: row
        anchors.centerIn: parent
        spacing: pill.compact ? Theme.px(8) : Theme.px(12)

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: pill.compact ? Theme.px(8) : Theme.px(14)
            height: width
            radius: width / 2
            color: pill.filled ? Theme.textOnColor : pill.statusColor
            opacity: pill.blinkOn ? 1.0 : 0.2
        }

        Label {
            id: label
            anchors.verticalCenter: parent.verticalCenter
            color: pill.filled ? Theme.textOnColor : Theme.textPrimary
            font.pixelSize: pill.compact ? Theme.fontSmall : Theme.fontTitle
            font.weight: pill.compact ? Font.DemiBold : Font.Bold
        }
    }

    Timer {
        interval: 500
        repeat: true
        running: pill.blinking
        onTriggered: pill.blinkOn = !pill.blinkOn
        onRunningChanged: pill.blinkOn = true
    }
}
