import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Hmi.Backend 1.0
import Hmi.Ui 1.0

// Main screen: live gauges, trend of one selected signal, start/stop.
Item {
    id: page

    property ProcessValue trendChannel: Machine.temperature
    readonly property bool inAlarm: Machine.state === MachineBackend.Alarm

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing
        spacing: Theme.spacing

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: false // nested layouts fill by default
            Layout.preferredHeight: Theme.px(296)
            spacing: Theme.spacing

            GaugeCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                channel: Machine.temperature
            }
            GaugeCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                channel: Machine.pressure
            }
            GaugeCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                channel: Machine.speed
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacing

            Card {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: qsTr("Trend, last %1 s").arg(Strings.number(chart.windowSeconds, 0))
                headerItems: Repeater {
                    model: [Machine.temperature, Machine.pressure, Machine.speed]
                    HmiButton {
                        implicitHeight: Theme.px(48)
                        font.pixelSize: Theme.fontSmall + Theme.px(1)
                        leftPadding: Theme.px(14)
                        rightPadding: Theme.px(14)
                        text: Strings.channelName(modelData.key)
                        checked: page.trendChannel === modelData
                        onClicked: page.trendChannel = modelData
                    }
                }

                Row {
                    id: legend
                    anchors.left: parent.left
                    anchors.leftMargin: chart.leftMargin
                    anchors.top: parent.top
                    spacing: Theme.px(24)
                    LegendItem {
                        text: Strings.channelName(page.trendChannel.key)
                        color: Theme.accent
                    }
                    LegendItem {
                        text: qsTr("Setpoint")
                        color: Theme.textSecondary
                        dashed: true
                    }
                    LegendItem {
                        text: qsTr("High limit")
                        color: Theme.critical
                        dashed: true
                    }
                }

                TrendChart {
                    id: chart
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: legend.bottom
                    anchors.topMargin: Theme.px(6)
                    anchors.bottom: parent.bottom
                    values: page.trendChannel.history
                    capacity: page.trendChannel.historyCapacity
                    windowSeconds: page.trendChannel.historyCapacity * Machine.sampleIntervalMs / 1000
                    minimum: page.trendChannel.minimum
                    maximum: page.trendChannel.maximum
                    setpoint: page.trendChannel.setpoint
                    limit: page.trendChannel.highLimit
                }
            }

            Card {
                Layout.preferredWidth: Theme.px(340)
                Layout.fillHeight: true
                title: qsTr("Machine control")

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Theme.px(12)

                    HmiButton {
                        visible: !page.inAlarm
                        Layout.fillWidth: true
                        Layout.preferredHeight: Theme.px(84)
                        text: qsTr("Start")
                        fillColor: Theme.good
                        textColor: Theme.textOnColor
                        font.pixelSize: Theme.fontLarge
                        enabled: Machine.connected && Machine.state === MachineBackend.Stopped
                        onClicked: Machine.start()
                    }
                    HmiButton {
                        visible: !page.inAlarm
                        Layout.fillWidth: true
                        Layout.preferredHeight: Theme.px(84)
                        text: qsTr("Stop")
                        fillColor: Theme.critical
                        textColor: Theme.textOnColor
                        font.pixelSize: Theme.fontLarge
                        enabled: Machine.connected && Machine.state === MachineBackend.Running
                        onClicked: Machine.stop()
                    }

                    Label {
                        visible: page.inAlarm
                        Layout.fillWidth: true
                        text: qsTr("A fault is pending. Check the alarm list and acknowledge it before starting again.")
                        wrapMode: Text.WordWrap
                        color: Theme.textSecondary
                    }
                    HmiButton {
                        visible: page.inAlarm
                        Layout.fillWidth: true
                        Layout.preferredHeight: Theme.px(84)
                        text: qsTr("Acknowledge alarms")
                        fillColor: Theme.warning
                        textColor: Theme.textOnWarning
                        onClicked: Machine.acknowledgeAllAlarms()
                    }

                    Item { Layout.fillHeight: true }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.border
                    }
                    InfoRow {
                        Layout.fillWidth: true
                        label: qsTr("Data source")
                        value: Machine.simulated ? qsTr("Simulator") : Machine.sourceName
                    }
                    InfoRow {
                        Layout.fillWidth: true
                        label: qsTr("Connection")
                        value: Machine.connected ? qsTr("Online") : qsTr("Offline")
                        valueColor: Machine.connected ? Theme.textPrimary : Theme.critical
                    }
                    InfoRow {
                        Layout.fillWidth: true
                        label: qsTr("Unacknowledged alarms")
                        value: Machine.alarms.unacknowledgedCount
                    }
                }
            }
        }
    }
}
