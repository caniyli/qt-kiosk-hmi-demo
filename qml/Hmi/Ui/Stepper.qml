import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Hmi.Ui 1.0

// Numeric editor for touch screens: large -/+ buttons that repeat while held,
// so no on-screen keyboard is needed. The owner applies `valueModified` to
// its data, which keeps `value` bound to the source of truth.
//
// Quick taps arrive as double-clicks, whose second press emits doubleClicked
// instead of clicked; both are handled so no tap is lost.
RowLayout {
    id: root

    property real value: 0
    property real from: 0
    property real to: 100
    property real stepSize: 1
    property int decimals: 0
    property string unit

    signal valueModified(real newValue)

    spacing: Theme.px(8)

    function nudge(direction) {
        var next = Math.min(root.to, Math.max(root.from, root.value + direction * root.stepSize))
        if (next !== root.value)
            root.valueModified(next)
    }

    HmiButton {
        id: minus
        Layout.preferredWidth: Theme.touchTarget
        Layout.preferredHeight: Theme.touchTarget
        autoRepeat: true
        enabled: root.value > root.from
        onClicked: root.nudge(-1)
        onDoubleClicked: root.nudge(-1)
        contentItem: Item {
            Rectangle {
                anchors.centerIn: parent
                width: Theme.px(22)
                height: Theme.px(3)
                radius: height / 2
                color: minus.enabled ? Theme.textPrimary : Theme.textMuted
            }
        }
    }

    Rectangle {
        Layout.preferredWidth: Theme.px(200)
        Layout.preferredHeight: Theme.touchTarget
        radius: Theme.radius
        color: Theme.background
        border.color: Theme.border

        Label {
            anchors.centerIn: parent
            text: Strings.quantity(root.value, root.decimals, root.unit)
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
        }
    }

    HmiButton {
        id: plus
        Layout.preferredWidth: Theme.touchTarget
        Layout.preferredHeight: Theme.touchTarget
        autoRepeat: true
        enabled: root.value < root.to
        onClicked: root.nudge(1)
        onDoubleClicked: root.nudge(1)
        contentItem: Item {
            Rectangle {
                anchors.centerIn: parent
                width: Theme.px(22)
                height: Theme.px(3)
                radius: height / 2
                color: plus.enabled ? Theme.textPrimary : Theme.textMuted
            }
            Rectangle {
                anchors.centerIn: parent
                width: Theme.px(3)
                height: Theme.px(22)
                radius: width / 2
                color: plus.enabled ? Theme.textPrimary : Theme.textMuted
            }
        }
    }
}
