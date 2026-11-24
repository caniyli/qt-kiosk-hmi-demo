import QtQuick 2.12
import QtQuick.Controls 2.12
import Hmi.Backend 1.0
import Hmi.Ui 1.0

// Top bar: machine name and data source, machine state, alarm counter, clock.
Rectangle {
    id: bar

    property date now: new Date()
    readonly property int pendingAlarms: Machine.alarms.unacknowledgedCount
    readonly property color alarmColor: Machine.state === MachineBackend.Alarm ? Theme.critical : Theme.warning

    signal alarmsClicked()

    implicitHeight: Theme.headerHeight
    color: Theme.surface

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.border
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: Theme.px(24)
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.px(2)

        Label {
            text: qsTr("Demo machine")
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
        }
        Label {
            text: Machine.simulated ? qsTr("Simulated data") : Machine.sourceName
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }
    }

    StatusPill {
        anchors.centerIn: parent
        width: Math.max(implicitWidth, Theme.px(220))
        text: Strings.stateName(Machine.state)
        statusColor: Machine.state === MachineBackend.Running ? Theme.good
                   : Machine.state === MachineBackend.Alarm ? Theme.critical
                   : Theme.neutral
        filled: Machine.state === MachineBackend.Alarm
        blinking: Machine.state === MachineBackend.Alarm
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: Theme.px(24)
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.px(24)

        AbstractButton {
            id: alarmButton
            anchors.verticalCenter: parent.verticalCenter
            implicitHeight: Theme.px(48)
            implicitWidth: alarmRow.implicitWidth + Theme.px(36)
            onClicked: bar.alarmsClicked()

            background: Rectangle {
                radius: height / 2
                color: alarmButton.down ? Theme.surfacePressed
                     : bar.pendingAlarms > 0 ? Theme.withAlpha(bar.alarmColor, 0.16)
                     : Theme.surfaceRaised
                border.width: bar.pendingAlarms > 0 ? Theme.px(2) : 1
                border.color: bar.pendingAlarms > 0 ? bar.alarmColor : Theme.border
            }
            contentItem: Item {
                Row {
                    id: alarmRow
                    anchors.centerIn: parent
                    spacing: Theme.px(10)
                    Icon {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Theme.px(24)
                        height: width
                        name: "alarms"
                        color: bar.pendingAlarms > 0 ? bar.alarmColor : Theme.textSecondary
                    }
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: bar.pendingAlarms
                        color: bar.pendingAlarms > 0 ? Theme.textPrimary : Theme.textSecondary
                        font.pixelSize: Theme.fontTitle
                        font.weight: Font.Bold
                    }
                }
            }
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            Label {
                anchors.right: parent.right
                text: Qt.formatTime(bar.now, "HH:mm:ss")
                font.pixelSize: Theme.fontTitle
                font.weight: Font.DemiBold
            }
            Label {
                anchors.right: parent.right
                text: Qt.formatDate(bar.now, "yyyy-MM-dd")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }
        }
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: bar.now = new Date()
    }
}
