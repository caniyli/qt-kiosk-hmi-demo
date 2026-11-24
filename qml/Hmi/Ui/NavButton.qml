import QtQuick 2.12
import QtQuick.Controls 2.12
import Hmi.Ui 1.0

// Entry of the navigation rail: icon, label and an optional count badge.
AbstractButton {
    id: control

    property string iconName
    property int badgeCount: 0
    property color badgeColor: Theme.critical
    property color badgeTextColor: Theme.textOnColor

    implicitWidth: Theme.navWidth
    implicitHeight: Theme.px(100)

    background: Rectangle {
        color: control.down ? Theme.surfacePressed
             : control.checked ? Theme.surfaceRaised
             : "transparent"
        Rectangle {
            visible: control.checked
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.px(4)
            height: parent.height * 0.56
            radius: width / 2
            color: Theme.accent
        }
    }

    contentItem: Item {
        Column {
            anchors.centerIn: parent
            spacing: Theme.px(10)

            Icon {
                anchors.horizontalCenter: parent.horizontalCenter
                name: control.iconName
                color: control.checked ? Theme.textPrimary : Theme.textSecondary

                Rectangle {
                    visible: control.badgeCount > 0
                    x: parent.width - width / 2
                    y: -height / 2
                    height: Theme.px(22)
                    width: Math.max(height, badgeLabel.implicitWidth + Theme.px(10))
                    radius: height / 2
                    color: control.badgeColor
                    Label {
                        id: badgeLabel
                        anchors.centerIn: parent
                        text: control.badgeCount > 99 ? "99+" : control.badgeCount
                        color: control.badgeTextColor
                        font.pixelSize: Theme.fontSmall
                        font.weight: Font.Bold
                    }
                }
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                width: control.width - Theme.px(12)
                horizontalAlignment: Text.AlignHCenter
                text: control.text
                color: control.checked ? Theme.textPrimary : Theme.textSecondary
                font.pixelSize: Theme.fontSmall
                font.weight: control.checked ? Font.DemiBold : Font.Normal
                elide: Text.ElideRight
            }
        }
    }
}
