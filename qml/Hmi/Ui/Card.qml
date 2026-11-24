import QtQuick 2.12
import QtQuick.Controls 2.12
import Hmi.Ui 1.0

// Rounded panel with an optional title row. Child items go into the body;
// `headerItems` are placed at the right end of the title row.
Rectangle {
    id: card

    property string title
    property int padding: Theme.px(16)
    property alias headerItems: headerRow.data
    default property alias content: body.data

    color: Theme.surface
    radius: Theme.radius
    border.color: Theme.border
    border.width: 1

    Label {
        id: titleLabel
        visible: card.title.length > 0
        anchors {
            left: parent.left
            right: headerRow.left
            top: parent.top
            leftMargin: card.padding
            rightMargin: Theme.px(8)
            topMargin: Theme.px(12)
        }
        height: Math.max(Theme.px(32), headerRow.height)
        verticalAlignment: Text.AlignVCenter
        text: card.title
        color: Theme.textSecondary
        font.pixelSize: Theme.fontBody
        font.weight: Font.DemiBold
        elide: Text.ElideRight
    }

    Row {
        id: headerRow
        anchors {
            right: parent.right
            rightMargin: card.padding
            verticalCenter: titleLabel.verticalCenter
        }
        spacing: Theme.px(8)
    }

    Item {
        id: body
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            top: titleLabel.visible ? titleLabel.bottom : parent.top
            leftMargin: card.padding
            rightMargin: card.padding
            bottomMargin: card.padding
            topMargin: titleLabel.visible ? Theme.px(8) : card.padding
        }
    }
}
