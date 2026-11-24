// SPDX-License-Identifier: MIT
#include "simulatedbackend.h"

#include <algorithm>
#include <cmath>

namespace {

// Model parameters. Time constants are in seconds.
constexpr double kAmbientTemperature = 22.0;
constexpr double kHeatingLag = 20.0;
constexpr double kCoolingLag = 90.0;
constexpr double kDriftTime = 8.0;
constexpr double kDriftSigma = 0.35;   // degC, stationary standard deviation
constexpr double kTemperatureNoise = 0.15;
constexpr double kPressureLag = 1.2;
constexpr double kPressureNoise = 0.03;
constexpr double kSurgeDecay = 2.0;
constexpr double kSurgeInterval = 40.0;     // mean time between pressure surges
constexpr double kAcceleration = 400.0;     // rpm/s
constexpr double kDeceleration = 600.0;     // rpm/s
constexpr double kSpeedNoise = 4.0;
constexpr double kOverloadInterval = 900.0; // mean time between overload trips
constexpr double kOverloadResetTime = 8.0;

// Fraction of the remaining distance a first-order lag covers in `dt`.
double lagFactor(double dt, double timeConstant)
{
    return 1.0 - std::exp(-dt / timeConstant);
}

double approach(double current, double target, double maxStep)
{
    if (current < target)
        return std::min(target, current + maxStep);
    return std::max(target, current - maxStep);
}

const QString kOverloadCode = QStringLiteral("MOTOR_OVERLOAD");

} // namespace

SimulatedBackend::SimulatedBackend(QObject *parent)
    : SimulatedBackend(Options(), parent)
{
}

SimulatedBackend::SimulatedBackend(const Options &options, QObject *parent)
    : MachineBackend(options.intervalMs, parent)
    , m_options(options)
    , m_rng(options.seed != 0 ? options.seed : std::random_device{}())
    , m_clock(QDateTime::currentDateTime())
    , m_temperature(kAmbientTemperature)
{
    setConnected(true);
    publish();

    connect(&m_timer, &QTimer::timeout, this, &SimulatedBackend::tick);
    if (m_options.realTime)
        m_timer.start(m_options.intervalMs);
}

QString SimulatedBackend::sourceName() const
{
    return QStringLiteral("Simulator");
}

void SimulatedBackend::start()
{
    // Like a real drive, refuse to start while a fault is pending.
    if (state() == Alarm)
        return;
    m_motorOn = true;
    setRunning(true);
}

void SimulatedBackend::stop()
{
    m_motorOn = false;
    setRunning(false);
}

void SimulatedBackend::injectFault()
{
    m_motorOn = false;
    m_overloadTimer = kOverloadResetTime;
    setRunning(false);
    setFault(kOverloadCode, true);
}

void SimulatedBackend::playDemoScenario()
{
    const int minutesRunning = 4;
    const double tripped = 15.0;
    const double restarted = 100.0;
    setClock(QDateTime::currentDateTime().addMSecs(
        -qint64((minutesRunning * 60 + tripped + restarted) * 1000)));

    start();
    for (int minute = 0; minute < minutesRunning; ++minute) {
        runFor(60.0);
        acknowledgeAllAlarms();
        if (state() == Stopped)
            start();
    }
    injectFault();
    runFor(tripped);
    acknowledgeAllAlarms();
    start();
    runFor(restarted);
}

void SimulatedBackend::tick()
{
    m_clock = QDateTime::currentDateTime();
    step(m_options.intervalMs / 1000.0);
}

void SimulatedBackend::runFor(double seconds)
{
    const int steps = static_cast<int>(std::ceil(seconds * 1000.0 / m_options.intervalMs));
    for (int i = 0; i < steps; ++i) {
        m_clock = m_clock.addMSecs(m_options.intervalMs);
        step(m_options.intervalMs / 1000.0);
    }
}

void SimulatedBackend::step(double dt)
{
    if (dt <= 0.0)
        return;

    // Motor: ramps towards the setpoint, or to standstill, with limited acceleration.
    const double nominalSpeed = std::max(1.0, speed()->setpoint());
    const double speedTarget = m_motorOn ? speed()->setpoint() : 0.0;
    m_speed = approach(m_speed, speedTarget, (m_motorOn ? kAcceleration : kDeceleration) * dt);

    // Pressure: follows the motor speed through a short lag; surges decay.
    const double pressureTarget = pressure()->setpoint() * std::min(1.5, m_speed / nominalSpeed);
    m_pressure += (pressureTarget - m_pressure) * lagFactor(dt, kPressureLag);
    m_surge *= std::exp(-dt / kSurgeDecay);

    // Temperature: heated towards the setpoint while running, cools when stopped.
    const double temperatureTarget = m_motorOn ? temperature()->setpoint() : kAmbientTemperature;
    m_temperature += (temperatureTarget - m_temperature)
                     * lagFactor(dt, m_motorOn ? kHeatingLag : kCoolingLag);

    // Slow drift: first-order autoregressive noise with a fixed variance.
    const double a = std::exp(-dt / kDriftTime);
    m_drift = a * m_drift + gaussian(kDriftSigma * std::sqrt(1.0 - a * a));

    if (m_overloadTimer > 0.0) {
        m_overloadTimer -= dt;
        if (m_overloadTimer <= 0.0)
            setFault(kOverloadCode, false);
    }

    if (m_options.randomEvents && m_motorOn) {
        if (chance(dt / kSurgeInterval))
            m_surge += uniform(1.2, 3.0);
        if (m_speed > 0.8 * nominalSpeed && chance(dt / kOverloadInterval))
            injectFault();
    }

    publish();
}

void SimulatedBackend::publish()
{
    const bool turning = m_speed > 1.0;
    const double t = m_temperature + m_drift + gaussian(kTemperatureNoise);
    const double p = std::max(0.0, m_pressure + m_surge + (turning ? gaussian(kPressureNoise) : 0.0));
    const double s = turning ? std::max(0.0, m_speed + gaussian(kSpeedNoise)) : 0.0;
    publishSample(t, p, s);
}

double SimulatedBackend::gaussian(double sigma)
{
    return sigma * m_normal(m_rng);
}

double SimulatedBackend::uniform(double low, double high)
{
    return std::uniform_real_distribution<double>(low, high)(m_rng);
}

bool SimulatedBackend::chance(double probability)
{
    return uniform(0.0, 1.0) < probability;
}
