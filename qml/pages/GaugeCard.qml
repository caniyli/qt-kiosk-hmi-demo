import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Hmi.Backend 1.0
import Hmi.Ui 1.0

// Card with an arc gauge for one process value plus its setpoint and limit.
Card {
    id: card

    property ProcessValue channel: null
    readonly property bool valid: channel !== null

    title: valid ? Strings.channelName(channel.key) : ""
    headerItems: StatusPill {
        compact: true
        visible: card.valid && card.channel.aboveLimit
        text: qsTr("Above limit")
        statusColor: Theme.warning
    }

    ArcGauge {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: footer.top
        anchors.bottomMargin: Theme.px(4)
        value: card.valid ? card.channel.value : 0
        valid: card.valid && card.channel.hasValue && Machine.connected
        minimum: card.valid ? card.channel.minimum : 0
        maximum: card.valid ? card.channel.maximum : 100
        setpoint: card.valid ? card.channel.setpoint : NaN
        limit: card.valid ? card.channel.highLimit : NaN
        alarm: card.valid && card.channel.aboveLimit && Machine.connected
        decimals: card.valid ? card.channel.decimals : 0
        unit: card.valid ? card.channel.unit : ""
    }

    RowLayout {
        id: footer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: 0

        Column {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Setpoint")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: card.valid ? Strings.quantity(card.channel.setpoint, card.channel.decimals, card.channel.unit) : ""
                color: Theme.textSecondary
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
            }
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: Theme.border
        }

        Column {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("High limit")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: card.valid ? Strings.quantity(card.channel.highLimit, card.channel.decimals, card.channel.unit) : ""
                color: Theme.textSecondary
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
            }
        }
    }
}
