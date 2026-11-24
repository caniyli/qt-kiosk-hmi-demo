// SPDX-License-Identifier: MIT
#pragma once

#include <QTimer>

#include "machinebackend.h"

class QModbusReply;
class QModbusTcpClient;

// Skeleton of a real data source: polls a controller over Modbus TCP.
// Only built when Qt SerialBus is available (HMI_HAS_MODBUS).
//
// Register map (all addresses zero-based, one register per value):
//   Input registers (read, FC 04)
//     0  temperature   int16, 0.1 degC
//     1  pressure      uint16, 0.01 bar
//     2  motor speed   uint16, rpm
//     3  status word   bit 0 = running, bit 1 = controller fault
//   Holding registers (read once after connect, written on change, FC 03/06)
//     0  temperature setpoint   int16, 0.1 degC
//     1  pressure setpoint      uint16, 0.01 bar
//     2  speed setpoint         uint16, rpm
//   Coils (FC 05)
//     0  run command   1 = run, 0 = stop
//     1  fault reset   written as 1 when alarms are acknowledged; the
//                      controller is expected to clear it
//
// The map is an example; adapt kRegister* constants and the scaling in
// modbustcpbackend.cpp to the actual controller.
class ModbusTcpBackend : public MachineBackend
{
    Q_OBJECT

public:
    struct Config
    {
        QString host = QStringLiteral("127.0.0.1");
        quint16 port = 502;
        int unitId = 1;
        int pollIntervalMs = 250;
        int timeoutMs = 1000;
        int reconnectDelayMs = 2000;
    };

    explicit ModbusTcpBackend(const Config &config, QObject *parent = nullptr);
    ~ModbusTcpBackend() override;

    QString sourceName() const override;

    void start() override;
    void stop() override;

protected:
    void alarmsAcknowledged() override;
    void writeSetpoint(ProcessValue *value) override;

private:
    void connectToController();
    void onDeviceStateChanged(int state);
    void poll();
    void handlePollReply(QModbusReply *reply);
    void readSetpoints();
    void writeCoil(int address, bool value);
    void writeRegister(int address, quint16 value);
    void watchReply(QModbusReply *reply);

    Config m_config;
    QModbusTcpClient *m_client;
    QTimer m_pollTimer;
    QTimer m_reconnectTimer;
    bool m_pollPending = false;
    bool m_applyingRemoteSetpoints = false;
};
