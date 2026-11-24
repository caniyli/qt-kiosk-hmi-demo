// SPDX-License-Identifier: MIT
//
// Runs the Modbus TCP backend against an in-process QModbusTcpServer that
// plays the controller. This checks the register map and the command path;
// it is not a substitute for testing against real hardware.
#include <QModbusTcpServer>
#include <QTcpServer>
#include <QtTest>

#include <memory>

#include "modbustcpbackend.h"

namespace {

quint16 freeLocalPort()
{
    QTcpServer probe;
    if (!probe.listen(QHostAddress::LocalHost, 0))
        return 0;
    return probe.serverPort();
}

} // namespace

class TestModbusTcpBackend : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QModbusTcpServer> m_server;
    quint16 m_port = 0;

    quint16 serverValue(QModbusDataUnit::RegisterType table, quint16 address) const
    {
        quint16 value = 0;
        m_server->data(table, address, &value);
        return value;
    }

    std::unique_ptr<ModbusTcpBackend> createBackend()
    {
        ModbusTcpBackend::Config config;
        config.host = QStringLiteral("127.0.0.1");
        config.port = m_port;
        config.pollIntervalMs = 50;
        config.reconnectDelayMs = 200;
        return std::make_unique<ModbusTcpBackend>(config);
    }

private slots:
    void init()
    {
        m_port = freeLocalPort();
        QVERIFY(m_port != 0);

        m_server = std::make_unique<QModbusTcpServer>();
        QModbusDataUnitMap map;
        map.insert(QModbusDataUnit::InputRegisters, {QModbusDataUnit::InputRegisters, 0, 4});
        map.insert(QModbusDataUnit::HoldingRegisters, {QModbusDataUnit::HoldingRegisters, 0, 3});
        map.insert(QModbusDataUnit::Coils, {QModbusDataUnit::Coils, 0, 2});
        QVERIFY(m_server->setMap(map));
        m_server->setServerAddress(1);
        m_server->setConnectionParameter(QModbusDevice::NetworkAddressParameter, QStringLiteral("127.0.0.1"));
        m_server->setConnectionParameter(QModbusDevice::NetworkPortParameter, m_port);

        // Controller state: 180.5 degC, 6.25 bar, 1450 rpm, running.
        m_server->setData(QModbusDataUnit::InputRegisters, 0, 1805);
        m_server->setData(QModbusDataUnit::InputRegisters, 1, 625);
        m_server->setData(QModbusDataUnit::InputRegisters, 2, 1450);
        m_server->setData(QModbusDataUnit::InputRegisters, 3, 0x0001);
        m_server->setData(QModbusDataUnit::HoldingRegisters, 0, 1750);
        m_server->setData(QModbusDataUnit::HoldingRegisters, 1, 600);
        m_server->setData(QModbusDataUnit::HoldingRegisters, 2, 1400);

        if (!m_server->connectDevice())
            QSKIP("Cannot start the local Modbus server");
    }

    void cleanup()
    {
        m_server.reset();
    }

    void readsValuesStatusAndSetpoints()
    {
        auto backend = createBackend();
        QTRY_VERIFY(backend->isConnected());
        QTRY_COMPARE(backend->temperature()->value(), 180.5);
        QCOMPARE(backend->pressure()->value(), 6.25);
        QCOMPARE(backend->speed()->value(), 1450.0);
        QCOMPARE(backend->state(), MachineBackend::Running);

        // Setpoints are taken over from the controller once after connecting.
        QTRY_COMPARE(backend->temperature()->setpoint(), 175.0);
        QCOMPARE(backend->speed()->setpoint(), 1400.0);
    }

    void negativeTemperatureIsSigned()
    {
        m_server->setData(QModbusDataUnit::InputRegisters, 0, static_cast<quint16>(-55)); // -5.5 degC
        auto backend = createBackend();
        QTRY_COMPARE(backend->temperature()->value(), -5.5);
    }

    void writesCommandsAndSetpoints()
    {
        auto backend = createBackend();
        QTRY_VERIFY(backend->isConnected());
        QTRY_COMPARE(backend->temperature()->setpoint(), 175.0);

        // QModbusServer stores a coil switched on with FC 05 as 0xFF00, so
        // only compare against zero.
        backend->start();
        QTRY_VERIFY(serverValue(QModbusDataUnit::Coils, 0) != 0);
        backend->stop();
        QTRY_COMPARE(serverValue(QModbusDataUnit::Coils, 0), quint16(0));

        backend->temperature()->setSetpoint(190.0);
        QTRY_COMPARE(serverValue(QModbusDataUnit::HoldingRegisters, 0), quint16(1900));
        backend->pressure()->setSetpoint(5.5);
        QTRY_COMPARE(serverValue(QModbusDataUnit::HoldingRegisters, 1), quint16(550));
    }

    void controllerFaultRaisesAlarmAndResetIsSent()
    {
        auto backend = createBackend();
        QTRY_COMPARE(backend->state(), MachineBackend::Running);

        m_server->setData(QModbusDataUnit::InputRegisters, 3, 0x0002); // stopped + fault
        QTRY_COMPARE(backend->state(), MachineBackend::Alarm);
        QVERIFY(backend->alarms()->isActive(QStringLiteral("CONTROLLER_FAULT")));

        m_server->setData(QModbusDataUnit::InputRegisters, 3, 0x0000);
        QTRY_VERIFY(!backend->alarms()->isActive(QStringLiteral("CONTROLLER_FAULT")));
        QCOMPARE(backend->state(), MachineBackend::Alarm); // until acknowledged

        backend->acknowledgeAllAlarms();
        QCOMPARE(backend->state(), MachineBackend::Stopped);
        QTRY_VERIFY(serverValue(QModbusDataUnit::Coils, 1) != 0);
    }

    void connectionLossRaisesAlarmAndReconnects()
    {
        auto backend = createBackend();
        QTRY_VERIFY(backend->isConnected());

        m_server->disconnectDevice();
        QTRY_VERIFY(!backend->isConnected());
        QVERIFY(backend->alarms()->isActive(QStringLiteral("COMM_LOST")));
        QCOMPARE(backend->state(), MachineBackend::Alarm);

        QVERIFY(m_server->connectDevice());
        QTRY_VERIFY_WITH_TIMEOUT(backend->isConnected(), 10000);
        QVERIFY(!backend->alarms()->isActive(QStringLiteral("COMM_LOST")));
    }
};

QTEST_GUILESS_MAIN(TestModbusTcpBackend)
#include "tst_modbustcpbackend.moc"
