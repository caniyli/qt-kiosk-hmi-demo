import QtQuick 2.12
import QtQuick.Controls 2.12
import Hmi.Ui 1.0

// Line swatch plus label for chart legends.
Row {
    id: item

    property alias text: label.text
    property color color: Theme.accent
    property bool dashed: false

    spacing: Theme.px(8)

    Row {
        anchors.verticalCenter: parent.verticalCenter
        spacing: item.dashed ? Theme.px(3) : 0
        Repeater {
            model: item.dashed ? 3 : 1
            Rectangle {
                width: item.dashed ? Theme.px(6) : Theme.px(24)
                height: Theme.px(3)
                radius: 1
                color: item.color
            }
        }
    }

    Label {
        id: label
        anchors.verticalCenter: parent.verticalCenter
        color: Theme.textSecondary
        font.pixelSize: Theme.fontSmall
    }
}
