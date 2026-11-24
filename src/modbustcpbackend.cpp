// SPDX-License-Identifier: MIT
#include "modbustcpbackend.h"

#include <QModbusDataUnit>
#include <QModbusReply>
#include <QModbusTcpClient>
#include <QtDebug>

#include <cmath>

namespace {

constexpr int kInputRegisterCount = 4;
constexpr int kStatusRunningBit = 0x0001;
constexpr int kStatusFaultBit = 0x0002;

constexpr int kRunCoil = 0;
constexpr int kFaultResetCoil = 1;

const QString kCommLostCode = QStringLiteral("COMM_LOST");
const QString kControllerFaultCode = QStringLiteral("CONTROLLER_FAULT");

// Scaling between engineering units and raw registers, indexed like the
// holding/input registers: temperature, pressure, speed.
constexpr double kScale[] = {10.0, 100.0, 1.0};
constexpr bool kSigned[] = {true, false, false};

double fromRaw(int index, quint16 raw)
{
    const double value = kSigned[index] ? static_cast<qint16>(raw) : raw;
    return value / kScale[index];
}

quint16 toRaw(int index, double value)
{
    const long scaled = std::lround(value * kScale[index]);
    return static_cast<quint16>(kSigned[index] ? static_cast<qint16>(scaled) : scaled);
}

} // namespace

ModbusTcpBackend::ModbusTcpBackend(const Config &config, QObject *parent)
    : MachineBackend(config.pollIntervalMs, parent)
    , m_config(config)
    , m_client(new QModbusTcpClient(this))
{
    m_client->setConnectionParameter(QModbusDevice::NetworkAddressParameter, m_config.host);
    m_client->setConnectionParameter(QModbusDevice::NetworkPortParameter, m_config.port);
    m_client->setTimeout(m_config.timeoutMs);
    m_client->setNumberOfRetries(1);
    connect(m_client, &QModbusDevice::stateChanged, this,
            [this](QModbusDevice::State state) { onDeviceStateChanged(state); });

    m_pollTimer.setInterval(m_config.pollIntervalMs);
    connect(&m_pollTimer, &QTimer::timeout, this, &ModbusTcpBackend::poll);

    m_reconnectTimer.setSingleShot(true);
    m_reconnectTimer.setInterval(m_config.reconnectDelayMs);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &ModbusTcpBackend::connectToController);

    QTimer::singleShot(0, this, &ModbusTcpBackend::connectToController);
}

ModbusTcpBackend::~ModbusTcpBackend()
{
    m_pollTimer.stop();
    m_reconnectTimer.stop();
    m_client->disconnect(this);
    m_client->disconnectDevice();
}

QString ModbusTcpBackend::sourceName() const
{
    return QStringLiteral("Modbus TCP %1:%2").arg(m_config.host).arg(m_config.port);
}

void ModbusTcpBackend::start()
{
    if (state() != Alarm)
        writeCoil(kRunCoil, true);
}

void ModbusTcpBackend::stop()
{
    writeCoil(kRunCoil, false);
}

void ModbusTcpBackend::alarmsAcknowledged()
{
    writeCoil(kFaultResetCoil, true);
}

void ModbusTcpBackend::writeSetpoint(ProcessValue *value)
{
    // Setpoints read from the controller must not be echoed back to it.
    if (m_applyingRemoteSetpoints)
        return;
    const int index = processValues().indexOf(value);
    if (index >= 0)
        writeRegister(index, toRaw(index, value->setpoint()));
}

void ModbusTcpBackend::connectToController()
{
    if (m_client->state() == QModbusDevice::UnconnectedState && !m_client->connectDevice())
        m_reconnectTimer.start();
}

void ModbusTcpBackend::onDeviceStateChanged(int state)
{
    if (state == QModbusDevice::ConnectedState) {
        setConnected(true);
        setFault(kCommLostCode, false);
        readSetpoints();
        poll();
        m_pollTimer.start();
    } else if (state == QModbusDevice::UnconnectedState) {
        m_pollTimer.stop();
        m_pollPending = false;
        setConnected(false);
        setFault(kCommLostCode, true);
        m_reconnectTimer.start();
    }
}

void ModbusTcpBackend::poll()
{
    if (m_pollPending || m_client->state() != QModbusDevice::ConnectedState)
        return;

    const QModbusDataUnit request(QModbusDataUnit::InputRegisters, 0, kInputRegisterCount);
    QModbusReply *reply = m_client->sendReadRequest(request, m_config.unitId);
    if (!reply) {
        qWarning("Modbus: read request failed: %s", qPrintable(m_client->errorString()));
        return;
    }
    m_pollPending = true;
    if (reply->isFinished())
        handlePollReply(reply);
    else
        connect(reply, &QModbusReply::finished, this, [this, reply] { handlePollReply(reply); });
}

void ModbusTcpBackend::handlePollReply(QModbusReply *reply)
{
    m_pollPending = false;
    reply->deleteLater();
    if (reply->error() != QModbusDevice::NoError) {
        qWarning("Modbus: poll failed: %s", qPrintable(reply->errorString()));
        return;
    }

    const QModbusDataUnit unit = reply->result();
    if (static_cast<int>(unit.valueCount()) < kInputRegisterCount)
        return;

    const quint16 status = unit.value(3);
    setRunning(status & kStatusRunningBit);
    setFault(kControllerFaultCode, status & kStatusFaultBit);
    publishSample(fromRaw(0, unit.value(0)), fromRaw(1, unit.value(1)), fromRaw(2, unit.value(2)));
}

void ModbusTcpBackend::readSetpoints()
{
    const QModbusDataUnit request(QModbusDataUnit::HoldingRegisters, 0, 3);
    QModbusReply *reply = m_client->sendReadRequest(request, m_config.unitId);
    if (!reply)
        return;

    auto apply = [this, reply] {
        reply->deleteLater();
        if (reply->error() != QModbusDevice::NoError) {
            qWarning("Modbus: reading setpoints failed: %s", qPrintable(reply->errorString()));
            return;
        }
        const QModbusDataUnit unit = reply->result();
        const QVector<ProcessValue *> values = processValues();
        m_applyingRemoteSetpoints = true;
        for (int i = 0; i < values.size() && i < static_cast<int>(unit.valueCount()); ++i)
            values.at(i)->setSetpoint(fromRaw(i, unit.value(i)));
        m_applyingRemoteSetpoints = false;
    };
    if (reply->isFinished())
        apply();
    else
        connect(reply, &QModbusReply::finished, this, apply);
}

void ModbusTcpBackend::writeCoil(int address, bool value)
{
    if (m_client->state() != QModbusDevice::ConnectedState)
        return;
    QModbusDataUnit unit(QModbusDataUnit::Coils, address, 1);
    unit.setValue(0, value ? 1 : 0);
    watchReply(m_client->sendWriteRequest(unit, m_config.unitId));
}

void ModbusTcpBackend::writeRegister(int address, quint16 value)
{
    if (m_client->state() != QModbusDevice::ConnectedState)
        return;
    QModbusDataUnit unit(QModbusDataUnit::HoldingRegisters, address, 1);
    unit.setValue(0, value);
    watchReply(m_client->sendWriteRequest(unit, m_config.unitId));
}

void ModbusTcpBackend::watchReply(QModbusReply *reply)
{
    if (!reply) {
        qWarning("Modbus: write request failed: %s", qPrintable(m_client->errorString()));
        return;
    }
    auto finish = [reply] {
        if (reply->error() != QModbusDevice::NoError)
            qWarning("Modbus: write failed: %s", qPrintable(reply->errorString()));
        reply->deleteLater();
    };
    if (reply->isFinished())
        finish();
    else
        connect(reply, &QModbusReply::finished, this, finish);
}
