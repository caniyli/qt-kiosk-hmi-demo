import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Hmi.Ui 1.0

// "Label ..... value" line for status panels.
RowLayout {
    id: row

    property alias label: nameLabel.text
    property alias value: valueLabel.text
    property color valueColor: Theme.textPrimary

    spacing: Theme.px(12)

    Label {
        id: nameLabel
        Layout.fillWidth: true
        color: Theme.textSecondary
        font.pixelSize: Theme.fontSmall + Theme.px(1)
        elide: Text.ElideRight
    }

    Label {
        id: valueLabel
        horizontalAlignment: Text.AlignRight
        color: row.valueColor
        font.pixelSize: Theme.fontSmall + Theme.px(1)
        font.weight: Font.DemiBold
    }
}
