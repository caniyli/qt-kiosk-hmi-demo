import QtQuick 2.12
import QtQuick.Controls 2.12
import Hmi.Backend 1.0
import Hmi.Ui 1.0

// Left navigation: one large button per page.
Rectangle {
    id: rail

    property int currentPage: 0
    signal pageSelected(int page)

    color: Theme.surface

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.border
    }

    Column {
        anchors.top: parent.top
        anchors.topMargin: Theme.px(8)
        width: parent.width - 1

        NavButton {
            width: parent.width
            text: qsTr("Overview")
            iconName: "overview"
            checked: rail.currentPage === 0
            onClicked: rail.pageSelected(0)
        }
        NavButton {
            width: parent.width
            text: qsTr("Alarms")
            iconName: "alarms"
            checked: rail.currentPage === 1
            badgeCount: Machine.alarms.unacknowledgedCount
            badgeColor: Machine.state === MachineBackend.Alarm ? Theme.critical : Theme.warning
            badgeTextColor: Machine.state === MachineBackend.Alarm ? Theme.textOnColor : Theme.textOnWarning
            onClicked: rail.pageSelected(1)
        }
        NavButton {
            width: parent.width
            text: qsTr("Settings")
            iconName: "settings"
            checked: rail.currentPage === 2
            onClicked: rail.pageSelected(2)
        }
    }

    Label {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.px(12)
        anchors.horizontalCenter: parent.horizontalCenter
        text: "v" + appInfo.version
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall
    }
}
