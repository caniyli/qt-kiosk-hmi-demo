// SPDX-License-Identifier: MIT
#pragma once

#include <QDateTime>
#include <QTimer>

#include <random>

#include "machinebackend.h"

// A small plant model that produces believable data without hardware:
//   * motor speed ramps to its setpoint with limited acceleration,
//   * pressure follows the motor speed through a short lag,
//   * temperature is heated towards its setpoint while running and cools
//     down to ambient when stopped,
//   * all signals carry measurement noise plus a slow drift,
//   * optional random events: pressure surges (often crossing the high
//     limit) and, rarely, a motor overload trip.
// With a fixed seed the output is fully deterministic, which the unit tests
// and the screenshot mode rely on.
class SimulatedBackend : public MachineBackend
{
    Q_OBJECT

public:
    struct Options
    {
        quint32 seed = 0;         // 0 picks a random seed
        int intervalMs = 200;     // sample interval
        bool randomEvents = true; // pressure surges and overload trips
        bool realTime = true;     // drive the model from a timer
    };

    explicit SimulatedBackend(QObject *parent = nullptr);
    explicit SimulatedBackend(const Options &options, QObject *parent = nullptr);

    QString sourceName() const override;
    bool isSimulated() const override { return true; }

    void start() override;
    void stop() override;

    // Advances the model by `seconds` and publishes one sample.
    void step(double seconds);
    // Runs the model for `seconds` of simulated time in sample-interval steps,
    // advancing the simulation clock. Used to build up history quickly.
    void runFor(double seconds);
    // Sets the clock used for alarm time stamps while running with runFor().
    void setClock(const QDateTime &time) { m_clock = time; }
    // Simulates a motor overload: the drive trips and a fault alarm is raised.
    void injectFault();
    // Replays about seven minutes of operation ending "now", so the UI starts
    // with trend data and alarm history (used by --screenshot): running with
    // an operator acknowledging alarms once a minute, a motor overload trip,
    // acknowledgement and restart. Deterministic for a fixed seed.
    void playDemoScenario();

protected:
    QDateTime currentTime() const override { return m_clock; }

private:
    void tick();
    void publish();
    double gaussian(double sigma);
    double uniform(double low, double high);
    bool chance(double probability);

    Options m_options;
    std::mt19937 m_rng;
    std::normal_distribution<double> m_normal{0.0, 1.0};
    QTimer m_timer;
    QDateTime m_clock;

    bool m_motorOn = false;
    double m_temperature;
    double m_pressure = 0.0;
    double m_speed = 0.0;
    double m_drift = 0.0;         // slow temperature disturbance
    double m_surge = 0.0;         // decaying pressure surge
    double m_overloadTimer = 0.0; // seconds until the overload relay resets
};
