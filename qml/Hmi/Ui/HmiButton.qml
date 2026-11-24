import QtQuick 2.12
import QtQuick.Controls 2.12
import Hmi.Ui 1.0

// Touch-sized push button. `fillColor` is the idle colour (green for Start,
// red for Stop, ...). A checked button gets an accent outline.
Button {
    id: control

    property color fillColor: Theme.surfaceRaised
    property color textColor: Theme.textPrimary

    implicitHeight: Theme.touchTarget
    implicitWidth: Math.max(Theme.px(96), contentItem.implicitWidth + leftPadding + rightPadding)
    leftPadding: Theme.px(20)
    rightPadding: Theme.px(20)
    topPadding: 0
    bottomPadding: 0
    font.pixelSize: Theme.fontBody
    font.weight: Font.DemiBold

    contentItem: Label {
        text: control.text
        font: control.font
        color: control.enabled ? control.textColor : Theme.textMuted
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: Theme.radius
        color: !control.enabled ? Theme.surface
             : control.down ? Qt.darker(control.fillColor, 1.35)
             : control.checked ? Theme.withAlpha(Theme.accent, 0.18)
             : control.fillColor
        border.width: control.checked ? Theme.px(2) : 1
        border.color: control.checked ? Theme.accent : Theme.border
    }
}
