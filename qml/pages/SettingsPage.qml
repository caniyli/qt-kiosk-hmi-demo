import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import Hmi.Backend 1.0
import Hmi.Ui 1.0

// Setpoints, alarm limits, language and system information.
Item {
    id: page

    readonly property var channels: [Machine.temperature, Machine.pressure, Machine.speed]

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing
        spacing: Theme.spacing

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacing

            Card {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: qsTr("Setpoints")

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    Repeater {
                        model: page.channels
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: Theme.spacing
                            Column {
                                Layout.fillWidth: true
                                Label {
                                    text: Strings.channelName(modelData.key)
                                    font.pixelSize: Theme.fontBody
                                    font.weight: Font.DemiBold
                                }
                                Label {
                                    text: qsTr("Actual %1").arg(Strings.quantity(modelData.value, modelData.decimals, modelData.unit))
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSmall
                                }
                            }
                            Stepper {
                                value: modelData.setpoint
                                from: modelData.minimum
                                to: modelData.maximum
                                stepSize: modelData.step
                                decimals: modelData.decimals
                                unit: modelData.unit
                                onValueModified: modelData.setpoint = newValue
                            }
                        }
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: qsTr("High alarm limits")

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    Repeater {
                        model: page.channels
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: Theme.spacing
                            Column {
                                Layout.fillWidth: true
                                Label {
                                    text: Strings.channelName(modelData.key)
                                    font.pixelSize: Theme.fontBody
                                    font.weight: Font.DemiBold
                                }
                                Label {
                                    text: qsTr("Setpoint %1").arg(Strings.quantity(modelData.setpoint, modelData.decimals, modelData.unit))
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSmall
                                }
                            }
                            Stepper {
                                value: modelData.highLimit
                                from: modelData.minimum
                                to: modelData.maximum
                                stepSize: modelData.step
                                decimals: modelData.decimals
                                unit: modelData.unit
                                onValueModified: modelData.highLimit = newValue
                            }
                        }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.preferredWidth: Theme.px(380)
            Layout.maximumWidth: Theme.px(380)
            Layout.fillHeight: true
            spacing: Theme.spacing

            Card {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.px(140)
                title: qsTr("Language")

                RowLayout {
                    anchors.fill: parent
                    spacing: Theme.px(12)
                    // Language names are shown in their own language on purpose.
                    HmiButton {
                        Layout.fillWidth: true
                        text: "English"
                        checked: Language.language === "en"
                        onClicked: Language.language = "en"
                    }
                    HmiButton {
                        Layout.fillWidth: true
                        text: "Türkçe"
                        checked: Language.language === "tr"
                        onClicked: Language.language = "tr"
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: qsTr("System")

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Theme.px(14)

                    InfoRow {
                        Layout.fillWidth: true
                        label: qsTr("Data source")
                        value: Machine.simulated ? qsTr("Simulator") : Machine.sourceName
                    }
                    InfoRow {
                        Layout.fillWidth: true
                        label: qsTr("Connection")
                        value: Machine.connected ? qsTr("Online") : qsTr("Offline")
                        valueColor: Machine.connected ? Theme.textPrimary : Theme.critical
                    }
                    InfoRow {
                        Layout.fillWidth: true
                        label: qsTr("Display mode")
                        value: appInfo.kiosk ? qsTr("Kiosk") : qsTr("Window")
                    }
                    InfoRow {
                        Layout.fillWidth: true
                        label: qsTr("Resolution")
                        value: Window.width + " × " + Window.height
                    }
                    InfoRow {
                        Layout.fillWidth: true
                        label: qsTr("Software version")
                        value: appInfo.version
                    }
                    InfoRow {
                        Layout.fillWidth: true
                        label: qsTr("Qt version")
                        value: appInfo.qtVersion
                    }

                    Item { Layout.fillHeight: true }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Changes apply immediately and are kept after a restart.")
                        wrapMode: Text.WordWrap
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                    }
                }
            }
        }
    }
}
