// SPDX-License-Identifier: MIT
#include <QtTest>

#include <algorithm>
#include <cmath>
#include <numeric>

#include "simulatedbackend.h"

namespace {

SimulatedBackend::Options quietOptions(quint32 seed)
{
    SimulatedBackend::Options options;
    options.seed = seed;
    options.randomEvents = false;
    options.realTime = false;
    return options;
}

// Mean of the last `count` samples of a channel.
double recentMean(const ProcessValue *value, int count)
{
    const QVector<double> samples = value->samples();
    const int n = std::min(count, static_cast<int>(samples.size()));
    return std::accumulate(samples.cend() - n, samples.cend(), 0.0) / n;
}

double recentMaxDeviation(const ProcessValue *value, int count, double target)
{
    const QVector<double> samples = value->samples();
    double deviation = 0.0;
    for (auto it = samples.cend() - std::min(count, static_cast<int>(samples.size())); it != samples.cend(); ++it)
        deviation = std::max(deviation, std::abs(*it - target));
    return deviation;
}

} // namespace

class TestSimulatedBackend : public QObject
{
    Q_OBJECT

private slots:
    void convergesToSetpoints_data()
    {
        QTest::addColumn<quint32>("seed");
        for (quint32 seed : {1u, 2u, 3u, 42u, 1234u})
            QTest::newRow(qPrintable(QStringLiteral("seed %1").arg(seed))) << seed;
    }

    void convergesToSetpoints()
    {
        QFETCH(quint32, seed);
        SimulatedBackend backend(quietOptions(seed));
        backend.start();
        QCOMPARE(backend.state(), MachineBackend::Running);

        backend.runFor(150.0);

        // Averaged over the last 10 s the values sit on their setpoints...
        QVERIFY(std::abs(recentMean(backend.temperature(), 50) - 180.0) < 1.0);
        QVERIFY(std::abs(recentMean(backend.pressure(), 50) - 6.0) < 0.05);
        QVERIFY(std::abs(recentMean(backend.speed(), 50) - 1450.0) < 5.0);
        // ...while individual samples are noisy but bounded.
        QVERIFY(recentMaxDeviation(backend.temperature(), 50, 180.0) < 3.0);
        QVERIFY(recentMaxDeviation(backend.temperature(), 50, 180.0) > 0.05);
        QVERIFY(recentMaxDeviation(backend.speed(), 50, 1450.0) < 25.0);
        QCOMPARE(backend.alarms()->count(), 0);
    }

    void followsSetpointChange()
    {
        SimulatedBackend backend(quietOptions(5));
        backend.start();
        backend.runFor(120.0);
        backend.temperature()->setSetpoint(150.0);
        backend.pressure()->setSetpoint(4.5);
        backend.speed()->setSetpoint(900.0);
        backend.runFor(150.0);

        QVERIFY(std::abs(recentMean(backend.temperature(), 50) - 150.0) < 1.0);
        QVERIFY(std::abs(recentMean(backend.pressure(), 50) - 4.5) < 0.05);
        QVERIFY(std::abs(recentMean(backend.speed(), 50) - 900.0) < 5.0);
    }

    void pressureBuildsUpWithSpeed()
    {
        SimulatedBackend backend(quietOptions(4));
        backend.start();
        backend.runFor(1.0); // motor still accelerating
        QVERIFY(backend.speed()->value() < 1450.0 * 0.5);
        QVERIFY(backend.pressure()->value() < 6.0 * 0.5);
    }

    void stopRampsDownAndCools()
    {
        SimulatedBackend backend(quietOptions(6));
        backend.start();
        backend.runFor(120.0);
        backend.stop();
        QCOMPARE(backend.state(), MachineBackend::Stopped);
        backend.runFor(60.0);

        QCOMPARE(backend.speed()->value(), 0.0);
        QVERIFY(backend.pressure()->value() < 0.1);
        QVERIFY(backend.temperature()->value() < 150.0);
        QVERIFY(backend.temperature()->value() > 22.0);
    }

    void limitRaisesAndClearsWarning()
    {
        SimulatedBackend backend(quietOptions(7));
        backend.temperature()->setHighLimit(170.0); // below the 180 degC setpoint
        backend.start();
        backend.runFor(120.0);

        AlarmModel *alarms = backend.alarms();
        QVERIFY(alarms->isActive(QStringLiteral("TEMPERATURE_HIGH")));
        QCOMPARE(alarms->at(0).severity, AlarmModel::Warning);
        QVERIFY(alarms->at(0).value > 170.0);
        QCOMPARE(alarms->at(0).limit, 170.0);
        // A warning does not stop the machine.
        QCOMPARE(backend.state(), MachineBackend::Running);

        backend.temperature()->setHighLimit(200.0);
        backend.runFor(1.0);
        QVERIFY(!alarms->isActive(QStringLiteral("TEMPERATURE_HIGH")));
        QCOMPARE(alarms->unacknowledgedCount(), 1);

        backend.acknowledgeAllAlarms();
        QCOMPARE(alarms->unacknowledgedCount(), 0);
        QCOMPARE(alarms->count(), 1); // kept as history
    }

    void faultTripsMachineUntilAcknowledged()
    {
        SimulatedBackend backend(quietOptions(8));
        backend.start();
        backend.runFor(60.0);

        backend.injectFault();
        QCOMPARE(backend.state(), MachineBackend::Alarm);
        backend.runFor(10.0); // overload relay resets after a few seconds
        QCOMPARE(backend.speed()->value(), 0.0);

        backend.start(); // refused while the fault is unacknowledged
        QCOMPARE(backend.state(), MachineBackend::Alarm);
        backend.runFor(1.0);
        QCOMPARE(backend.speed()->value(), 0.0);

        backend.acknowledgeAllAlarms();
        QCOMPARE(backend.state(), MachineBackend::Stopped);
        backend.start();
        QCOMPARE(backend.state(), MachineBackend::Running);
    }

    void randomEventsCauseOccasionalAlarms()
    {
        SimulatedBackend::Options options = quietOptions(11);
        options.randomEvents = true;
        SimulatedBackend backend(options);
        backend.start();

        // 30 minutes of operation with an operator who acknowledges alarms once
        // a minute and restarts the machine after a trip. (Until acknowledged,
        // a returning alarm re-uses its entry, so this also bounds the count.)
        for (int minute = 0; minute < 30; ++minute) {
            backend.runFor(60.0);
            if (backend.state() == MachineBackend::Alarm)
                backend.runFor(10.0);
            backend.acknowledgeAllAlarms();
            if (backend.state() == MachineBackend::Stopped)
                backend.start();
        }

        int pressureAlarms = 0;
        for (int row = 0; row < backend.alarms()->count(); ++row) {
            if (backend.alarms()->at(row).code == QLatin1String("PRESSURE_HIGH"))
                ++pressureAlarms;
        }
        // Surges happen about every 40 s and roughly half of them cross 8 bar,
        // so most minutes see one; "occasional", not constant.
        QVERIFY2(pressureAlarms >= 5, qPrintable(QString::number(pressureAlarms)));
        QVERIFY2(pressureAlarms <= 30, qPrintable(QString::number(pressureAlarms)));
    }

    void sameSeedGivesSameData()
    {
        SimulatedBackend::Options options = quietOptions(99);
        options.randomEvents = true;
        SimulatedBackend a(options);
        SimulatedBackend b(options);
        a.start();
        b.start();
        a.runFor(300.0);
        b.runFor(300.0);
        QCOMPARE(a.temperature()->samples(), b.temperature()->samples());
        QCOMPARE(a.pressure()->samples(), b.pressure()->samples());
        QCOMPARE(a.alarms()->count(), b.alarms()->count());
    }
};

QTEST_GUILESS_MAIN(TestSimulatedBackend)
#include "tst_simulatedbackend.moc"
