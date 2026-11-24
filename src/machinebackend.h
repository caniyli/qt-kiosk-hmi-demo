// SPDX-License-Identifier: MIT
#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVector>

#include "alarmmodel.h"
#include "processvalue.h"

// The interface between the UI and a machine data source.
//
// The QML code only talks to this class. A data source (the simulator, a
// Modbus TCP client, an MQTT subscriber, ...) derives from it and
//   * calls publishSample() whenever new measurements arrive,
//   * calls setRunning(), setConnected() and setFault() to report status,
//   * implements start()/stop() and, if the source owns the setpoints,
//     writeSetpoint().
// Limit alarms, the alarm journal, trend history and the machine state shown
// on screen are handled here, so every data source behaves the same in the UI.
class MachineBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(bool simulated READ isSimulated CONSTANT)
    Q_PROPERTY(QString sourceName READ sourceName CONSTANT)
    Q_PROPERTY(int sampleIntervalMs READ sampleIntervalMs CONSTANT)
    Q_PROPERTY(ProcessValue *temperature READ temperature CONSTANT)
    Q_PROPERTY(ProcessValue *pressure READ pressure CONSTANT)
    Q_PROPERTY(ProcessValue *speed READ speed CONSTANT)
    Q_PROPERTY(AlarmModel *alarms READ alarms CONSTANT)

public:
    enum State {
        Stopped,
        Running,
        Alarm // a fault alarm is active or not yet acknowledged
    };
    Q_ENUM(State)

    static constexpr int HistoryCapacity = 600;

    State state() const { return m_state; }
    bool isConnected() const { return m_connected; }
    int sampleIntervalMs() const { return m_sampleIntervalMs; }

    virtual QString sourceName() const = 0;
    virtual bool isSimulated() const { return false; }

    ProcessValue *temperature() const { return m_temperature; }
    ProcessValue *pressure() const { return m_pressure; }
    ProcessValue *speed() const { return m_speed; }
    QVector<ProcessValue *> processValues() const { return {m_temperature, m_pressure, m_speed}; }
    AlarmModel *alarms() const { return m_alarms; }

    Q_INVOKABLE virtual void start() = 0;
    Q_INVOKABLE virtual void stop() = 0;
    Q_INVOKABLE void acknowledgeAlarm(int alarmId);
    Q_INVOKABLE void acknowledgeAllAlarms();

signals:
    void stateChanged();
    void connectedChanged();

protected:
    explicit MachineBackend(int sampleIntervalMs, QObject *parent = nullptr);

    // Feeds one set of measurements: updates values, trend history and limit alarms.
    void publishSample(double temperature, double pressure, double speed);
    void setRunning(bool running);
    void setConnected(bool connected);
    void setFault(const QString &code, bool active);

    // Time stamp used for alarms. The simulator overrides it to replay history.
    virtual QDateTime currentTime() const { return QDateTime::currentDateTime(); }
    // Called after the operator acknowledged at least one alarm.
    virtual void alarmsAcknowledged() {}
    // Called when the operator changed a setpoint.
    virtual void writeSetpoint(ProcessValue *value) { Q_UNUSED(value) }

private:
    void updateLimitAlarm(ProcessValue *value);
    void updateState();

    int m_sampleIntervalMs;
    State m_state = Stopped;
    bool m_running = false;
    bool m_connected = false;

    ProcessValue *m_temperature;
    ProcessValue *m_pressure;
    ProcessValue *m_speed;
    AlarmModel *m_alarms;
};
