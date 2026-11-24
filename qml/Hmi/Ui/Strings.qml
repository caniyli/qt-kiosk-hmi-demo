pragma Singleton
import QtQuick 2.12
import Hmi.Backend 1.0

// Texts that depend on backend data (channel keys, alarm codes, machine
// state) and locale-aware number formatting. The backend only knows keys and
// codes; translating them here keeps the data layer language independent.
QtObject {
    function channelName(key) {
        switch (key) {
        case "temperature": return qsTr("Temperature")
        case "pressure": return qsTr("Pressure")
        case "speed": return qsTr("Motor speed")
        }
        return key
    }

    function unit(symbol) {
        return symbol === "rpm" ? qsTr("rpm") : symbol
    }

    function stateName(state) {
        switch (state) {
        case MachineBackend.Running: return qsTr("Running")
        case MachineBackend.Alarm: return qsTr("Alarm")
        }
        return qsTr("Stopped")
    }

    function alarmText(code) {
        switch (code) {
        case "TEMPERATURE_HIGH": return qsTr("Temperature above high limit")
        case "PRESSURE_HIGH": return qsTr("Pressure above high limit")
        case "SPEED_HIGH": return qsTr("Motor speed above high limit")
        case "MOTOR_OVERLOAD": return qsTr("Motor overload, machine stopped")
        case "CONTROLLER_FAULT": return qsTr("Controller reports a fault")
        case "COMM_LOST": return qsTr("No connection to the controller")
        }
        return code
    }

    function severityName(severity) {
        return severity === AlarmModel.Fault ? qsTr("Fault") : qsTr("Warning")
    }

    // Formats with the decimal separator of the current UI language.
    function number(value, decimals) {
        if (value === undefined || value === null || isNaN(value))
            return "–"
        return Number(value).toLocaleString(Qt.locale(Language.language), "f", decimals)
    }

    function quantity(value, decimals, symbol) {
        return number(value, decimals) + " " + unit(symbol)
    }
}
