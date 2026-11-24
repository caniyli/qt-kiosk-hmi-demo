import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Hmi.Backend 1.0
import Hmi.Ui 1.0

// Alarm journal with time stamps and acknowledgement.
Item {
    id: page

    readonly property int timeColumnWidth: Theme.px(140)
    readonly property int statusColumnWidth: Theme.px(230)
    readonly property int actionColumnWidth: Theme.px(170)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing
        spacing: Theme.spacing

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing

            Column {
                Layout.fillWidth: true
                spacing: Theme.px(2)
                Label {
                    text: qsTr("Alarms")
                    font.pixelSize: Theme.fontLarge
                    font.weight: Font.DemiBold
                }
                Label {
                    text: qsTr("%1 active, %2 unacknowledged")
                          .arg(Machine.alarms.activeCount).arg(Machine.alarms.unacknowledgedCount)
                    color: Theme.textSecondary
                }
            }

            HmiButton {
                text: qsTr("Acknowledge all")
                enabled: Machine.alarms.unacknowledgedCount > 0
                fillColor: Theme.warning
                textColor: Theme.textOnWarning
                onClicked: Machine.acknowledgeAllAlarms()
            }
        }

        Card {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 0

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.px(44)
                    Layout.leftMargin: Theme.px(28)
                    Layout.rightMargin: Theme.px(16)
                    spacing: Theme.px(12)

                    Label {
                        Layout.preferredWidth: page.timeColumnWidth - Theme.px(12)
                        text: qsTr("Time")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Alarm")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                    }
                    Label {
                        Layout.preferredWidth: page.statusColumnWidth
                        text: qsTr("Status")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                    }
                    Item { Layout.preferredWidth: page.actionColumnWidth }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.border
                }

                ListView {
                    id: list
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: Machine.alarms
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: AlarmRow {
                        width: list.width
                        alarmId: model.alarmId
                        code: model.code
                        severity: model.severity
                        raisedAt: model.raisedAt
                        clearedAt: model.clearedAt
                        acknowledgedAt: model.acknowledgedAt
                        active: model.active
                        acknowledged: model.acknowledged
                        value: model.value
                        limit: model.limit
                        unit: model.unit
                        decimals: model.decimals
                        timeColumnWidth: page.timeColumnWidth
                        statusColumnWidth: page.statusColumnWidth
                        actionColumnWidth: page.actionColumnWidth
                        onAcknowledgeClicked: Machine.acknowledgeAlarm(alarmId)
                    }

                    ScrollIndicator.vertical: ScrollIndicator {}

                    Label {
                        anchors.centerIn: parent
                        visible: list.count === 0
                        text: qsTr("No alarms")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontTitle
                    }
                }
            }
        }
    }
}
