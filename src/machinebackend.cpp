// SPDX-License-Identifier: MIT
#include "machinebackend.h"

namespace {

ProcessValue::Spec temperatureSpec()
{
    ProcessValue::Spec spec;
    spec.key = QStringLiteral("temperature");
    spec.unit = QStringLiteral("°C");
    spec.minimum = 0.0;
    spec.maximum = 250.0;
    spec.decimals = 1;
    spec.step = 1.0;
    spec.setpoint = 180.0;
    spec.highLimit = 200.0;
    return spec;
}

ProcessValue::Spec pressureSpec()
{
    ProcessValue::Spec spec;
    spec.key = QStringLiteral("pressure");
    spec.unit = QStringLiteral("bar");
    spec.minimum = 0.0;
    spec.maximum = 10.0;
    spec.decimals = 2;
    spec.step = 0.1;
    spec.setpoint = 6.0;
    spec.highLimit = 8.0;
    return spec;
}

ProcessValue::Spec speedSpec()
{
    ProcessValue::Spec spec;
    spec.key = QStringLiteral("speed");
    spec.unit = QStringLiteral("rpm");
    spec.minimum = 0.0;
    spec.maximum = 2000.0;
    spec.decimals = 0;
    spec.step = 10.0;
    spec.setpoint = 1450.0;
    spec.highLimit = 1650.0;
    return spec;
}

} // namespace

MachineBackend::MachineBackend(int sampleIntervalMs, QObject *parent)
    : QObject(parent)
    , m_sampleIntervalMs(sampleIntervalMs)
    , m_temperature(new ProcessValue(temperatureSpec(), HistoryCapacity, this))
    , m_pressure(new ProcessValue(pressureSpec(), HistoryCapacity, this))
    , m_speed(new ProcessValue(speedSpec(), HistoryCapacity, this))
    , m_alarms(new AlarmModel(200, this))
{
    for (ProcessValue *value : processValues()) {
        connect(value, &ProcessValue::aboveLimitChanged, this, [this, value] { updateLimitAlarm(value); });
        connect(value, &ProcessValue::setpointChanged, this, [this, value] { writeSetpoint(value); });
    }
    connect(m_alarms, &AlarmModel::countsChanged, this, &MachineBackend::updateState);
}

void MachineBackend::acknowledgeAlarm(int alarmId)
{
    if (m_alarms->acknowledge(alarmId, currentTime()))
        alarmsAcknowledged();
}

void MachineBackend::acknowledgeAllAlarms()
{
    if (m_alarms->acknowledgeAll(currentTime()) > 0)
        alarmsAcknowledged();
}

void MachineBackend::publishSample(double temperature, double pressure, double speed)
{
    m_temperature->update(temperature);
    m_pressure->update(pressure);
    m_speed->update(speed);
}

void MachineBackend::setRunning(bool running)
{
    if (m_running == running)
        return;
    m_running = running;
    updateState();
}

void MachineBackend::setConnected(bool connected)
{
    if (m_connected == connected)
        return;
    m_connected = connected;
    emit connectedChanged();
}

void MachineBackend::setFault(const QString &code, bool active)
{
    if (active)
        m_alarms->raise(code, AlarmModel::Fault, currentTime());
    else
        m_alarms->clear(code, currentTime());
}

void MachineBackend::updateLimitAlarm(ProcessValue *value)
{
    const QString code = value->key().toUpper() + QStringLiteral("_HIGH");
    if (value->isAboveLimit()) {
        m_alarms->raise(code, AlarmModel::Warning, currentTime(), value->value(),
                        value->highLimit(), value->unit(), value->decimals());
    } else {
        m_alarms->clear(code, currentTime());
    }
}

void MachineBackend::updateState()
{
    State state = m_running ? Running : Stopped;
    if (m_alarms->hasOpenFault())
        state = Alarm;
    if (state == m_state)
        return;
    m_state = state;
    emit stateChanged();
}
