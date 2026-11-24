import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Hmi.Backend 1.0
import Hmi.Ui 1.0

// One entry of the alarm journal. Severity is shown by the stripe colour and
// spelled out in the detail line.
Rectangle {
    id: row

    property int alarmId
    property string code
    property int severity
    property var raisedAt
    property var clearedAt
    property var acknowledgedAt
    property bool active
    property bool acknowledged
    property real value: NaN
    property real limit: NaN
    property string unit
    property int decimals

    property int timeColumnWidth: Theme.px(140)
    property int statusColumnWidth: Theme.px(230)
    property int actionColumnWidth: Theme.px(170)

    signal acknowledgeClicked(int alarmId)

    readonly property color severityColor: severity === AlarmModel.Fault ? Theme.critical : Theme.warning
    readonly property bool closed: !active && acknowledged

    implicitHeight: Theme.px(80)
    color: active && !acknowledged ? Theme.withAlpha(severityColor, 0.08) : "transparent"

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: Theme.px(8)
        width: Theme.px(6)
        radius: width / 2
        color: row.severityColor
        opacity: row.closed ? 0.35 : 1.0
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.px(28)
        anchors.rightMargin: Theme.px(16)
        spacing: Theme.px(12)
        opacity: row.closed ? 0.6 : 1.0

        Column {
            Layout.preferredWidth: row.timeColumnWidth - Theme.px(12)
            Label {
                text: Qt.formatTime(row.raisedAt, "HH:mm:ss")
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
            }
            Label {
                text: Qt.formatDate(row.raisedAt, "yyyy-MM-dd")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }
        }

        Column {
            Layout.fillWidth: true
            Label {
                width: parent.width
                text: Strings.alarmText(row.code)
                font.pixelSize: Theme.fontBody
                font.weight: row.acknowledged ? Font.Normal : Font.DemiBold
                elide: Text.ElideRight
            }
            Label {
                width: parent.width
                text: isNaN(row.value)
                      ? Strings.severityName(row.severity)
                      : qsTr("%1: value %2, limit %3").arg(Strings.severityName(row.severity))
                            .arg(Strings.quantity(row.value, row.decimals, row.unit))
                            .arg(Strings.quantity(row.limit, row.decimals, row.unit))
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideRight
            }
        }

        Column {
            Layout.preferredWidth: row.statusColumnWidth
            spacing: Theme.px(2)
            Row {
                spacing: Theme.px(8)
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Theme.px(10)
                    height: width
                    radius: width / 2
                    color: row.active ? row.severityColor : Theme.neutral
                }
                Label {
                    text: row.active ? qsTr("Active")
                                     : qsTr("Cleared %1").arg(Qt.formatTime(row.clearedAt, "HH:mm:ss"))
                    font.pixelSize: Theme.fontSmall + Theme.px(1)
                }
            }
            Label {
                text: row.acknowledged ? qsTr("Acknowledged %1").arg(Qt.formatTime(row.acknowledgedAt, "HH:mm:ss"))
                                       : qsTr("Not acknowledged")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }
        }

        Item {
            Layout.preferredWidth: row.actionColumnWidth
            Layout.fillHeight: true
            HmiButton {
                visible: !row.acknowledged
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: Theme.px(56)
                leftPadding: Theme.px(10)
                rightPadding: Theme.px(10)
                text: qsTr("Acknowledge")
                onClicked: row.acknowledgeClicked(row.alarmId)
            }
        }
    }
}
