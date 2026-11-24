import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Hmi.Backend 1.0
import Hmi.Ui 1.0
import "pages"

ApplicationWindow {
    id: window

    // Selected page: 0 overview, 1 alarms, 2 settings. Also set from C++.
    property int currentPage: 0

    width: 1280
    height: 800
    visible: false // shown from C++, windowed or full screen
    title: qsTr("Machine HMI demo")
    color: Theme.background
    font.pixelSize: Theme.fontBody
    palette.windowText: Theme.textPrimary
    palette.text: Theme.textPrimary

    // An explicit background item (not only the window colour) so that item
    // grabs used by --screenshot contain it as well.
    background: Rectangle { color: Theme.background }

    // Scale all sizes relative to the 1280x800 design resolution.
    Component.onCompleted: Theme.scale = Qt.binding(function() {
        return Math.min(window.width / 1280, window.height / 800)
    })

    Shortcut {
        sequence: "Ctrl+Q"
        enabled: !appInfo.kiosk
        onActivated: Qt.quit()
    }

    header: HeaderBar {
        onAlarmsClicked: window.currentPage = 1
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        NavigationRail {
            Layout.fillHeight: true
            Layout.preferredWidth: Theme.navWidth
            currentPage: window.currentPage
            onPageSelected: window.currentPage = page
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: window.currentPage

            OverviewPage {}
            AlarmsPage {}
            SettingsPage {}
        }
    }
}
