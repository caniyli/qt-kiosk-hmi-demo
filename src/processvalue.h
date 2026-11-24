// SPDX-License-Identifier: MIT
#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>

// One measured process channel (for example the barrel temperature): the
// latest value, the operator setpoint, a high alarm limit and a fixed-size
// history for the trend chart.
//
// Display names are not stored here on purpose; the UI maps `key` to a
// translated label so the data layer stays language independent.
class ProcessValue : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString key READ key CONSTANT)
    Q_PROPERTY(QString unit READ unit CONSTANT)
    Q_PROPERTY(double minimum READ minimum CONSTANT)
    Q_PROPERTY(double maximum READ maximum CONSTANT)
    Q_PROPERTY(int decimals READ decimals CONSTANT)
    Q_PROPERTY(double step READ step CONSTANT)
    Q_PROPERTY(double value READ value NOTIFY valueChanged)
    Q_PROPERTY(bool hasValue READ hasValue NOTIFY valueChanged)
    Q_PROPERTY(double setpoint READ setpoint WRITE setSetpoint NOTIFY setpointChanged)
    Q_PROPERTY(double highLimit READ highLimit WRITE setHighLimit NOTIFY highLimitChanged)
    Q_PROPERTY(bool aboveLimit READ isAboveLimit NOTIFY aboveLimitChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)
    Q_PROPERTY(int historyCapacity READ historyCapacity CONSTANT)

public:
    struct Spec
    {
        QString key;
        QString unit;
        double minimum = 0.0;
        double maximum = 100.0;
        int decimals = 0;
        double step = 1.0; // increment used when the operator edits setpoint/limit
        double setpoint = 0.0;
        double highLimit = 100.0;
    };

    ProcessValue(const Spec &spec, int historyCapacity, QObject *parent = nullptr);

    QString key() const { return m_spec.key; }
    QString unit() const { return m_spec.unit; }
    double minimum() const { return m_spec.minimum; }
    double maximum() const { return m_spec.maximum; }
    int decimals() const { return m_spec.decimals; }
    double step() const { return m_spec.step; }

    double value() const { return m_value; }
    bool hasValue() const { return m_count > 0; }

    double setpoint() const { return m_setpoint; }
    void setSetpoint(double setpoint);

    double highLimit() const { return m_highLimit; }
    void setHighLimit(double limit);

    // The value leaves the "above limit" state only after it has dropped below
    // highLimit - hysteresis(), so a noisy signal does not toggle the alarm.
    bool isAboveLimit() const { return m_aboveLimit; }
    double hysteresis() const;

    // Records a new measurement and appends it to the history.
    void update(double value);

    QVariantList history() const;   // oldest first, for QML
    QVector<double> samples() const; // oldest first, for C++ callers and tests
    int historyCapacity() const { return m_history.size(); }

signals:
    void valueChanged();
    void setpointChanged();
    void highLimitChanged();
    void aboveLimitChanged();
    void historyChanged();

private:
    double normalize(double value) const;
    void evaluateLimit();

    Spec m_spec;
    double m_value = 0.0;
    double m_setpoint = 0.0;
    double m_highLimit = 0.0;
    bool m_aboveLimit = false;

    QVector<double> m_history; // ring buffer
    int m_head = 0;            // next write position
    int m_count = 0;
};
