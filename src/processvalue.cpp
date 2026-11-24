// SPDX-License-Identifier: MIT
#include "processvalue.h"

#include <QtGlobal>

#include <cmath>

namespace {

bool sameValue(double a, double b)
{
    return std::abs(a - b) < 1e-9;
}

} // namespace

ProcessValue::ProcessValue(const Spec &spec, int historyCapacity, QObject *parent)
    : QObject(parent)
    , m_spec(spec)
    , m_history(qMax(2, historyCapacity), 0.0)
{
    m_setpoint = normalize(spec.setpoint);
    m_highLimit = normalize(spec.highLimit);
}

double ProcessValue::normalize(double value) const
{
    double result = qBound(m_spec.minimum, value, m_spec.maximum);
    if (m_spec.step > 0.0)
        result = std::round(result / m_spec.step) * m_spec.step;
    // Remove binary noise such as 6.1000000000000005 after the step rounding.
    const double scale = std::pow(10.0, m_spec.decimals);
    return std::round(result * scale) / scale;
}

void ProcessValue::setSetpoint(double setpoint)
{
    const double value = normalize(setpoint);
    if (sameValue(value, m_setpoint))
        return;
    m_setpoint = value;
    emit setpointChanged();
}

void ProcessValue::setHighLimit(double limit)
{
    const double value = normalize(limit);
    if (sameValue(value, m_highLimit))
        return;
    m_highLimit = value;
    emit highLimitChanged();
    evaluateLimit();
}

double ProcessValue::hysteresis() const
{
    return (m_spec.maximum - m_spec.minimum) * 0.01;
}

void ProcessValue::update(double value)
{
    m_value = value;
    m_history[m_head] = value;
    m_head = (m_head + 1) % m_history.size();
    m_count = qMin(m_count + 1, m_history.size());

    emit valueChanged();
    emit historyChanged();
    evaluateLimit();
}

QVector<double> ProcessValue::samples() const
{
    QVector<double> result;
    result.reserve(m_count);
    const int capacity = m_history.size();
    const int first = (m_head - m_count + capacity) % capacity;
    for (int i = 0; i < m_count; ++i)
        result.append(m_history.at((first + i) % capacity));
    return result;
}

QVariantList ProcessValue::history() const
{
    QVariantList result;
    const QVector<double> values = samples();
    result.reserve(values.size());
    for (double v : values)
        result.append(v);
    return result;
}

void ProcessValue::evaluateLimit()
{
    if (m_count == 0)
        return;

    const bool above = m_aboveLimit ? m_value > m_highLimit - hysteresis()
                                    : m_value > m_highLimit;
    if (above == m_aboveLimit)
        return;
    m_aboveLimit = above;
    emit aboveLimitChanged();
}
