// SPDX-License-Identifier: MIT
#include <QSignalSpy>
#include <QtTest>

#include "processvalue.h"

class TestProcessValue : public QObject
{
    Q_OBJECT

private:
    static ProcessValue::Spec pressureSpec()
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

private slots:
    void historyKeepsLatestSamplesInOrder()
    {
        ProcessValue value(pressureSpec(), 4);
        for (int i = 1; i <= 6; ++i)
            value.update(i);

        QCOMPARE(value.samples(), (QVector<double>{3, 4, 5, 6}));
        QCOMPARE(value.history().size(), 4);
        QCOMPARE(value.history().constFirst().toDouble(), 3.0);
        QCOMPARE(value.value(), 6.0);
    }

    void setpointIsClampedAndRoundedToStep()
    {
        ProcessValue value(pressureSpec(), 4);
        value.setSetpoint(6.0 + 0.1 + 0.1 + 0.1); // 6.300000000000001
        QCOMPARE(value.setpoint(), 6.3);
        value.setSetpoint(42.0);
        QCOMPARE(value.setpoint(), 10.0);
        value.setSetpoint(-1.0);
        QCOMPARE(value.setpoint(), 0.0);
    }

    void limitUsesHysteresis()
    {
        ProcessValue value(pressureSpec(), 16);
        QSignalSpy spy(&value, &ProcessValue::aboveLimitChanged);
        QCOMPARE(value.hysteresis(), 0.1);

        value.update(7.95);
        QVERIFY(!value.isAboveLimit());
        value.update(8.05);
        QVERIFY(value.isAboveLimit());
        value.update(7.95); // inside the hysteresis band: still above
        QVERIFY(value.isAboveLimit());
        value.update(7.85);
        QVERIFY(!value.isAboveLimit());
        QCOMPARE(spy.count(), 2);
    }

    void changingLimitReevaluates()
    {
        ProcessValue value(pressureSpec(), 16);
        value.update(6.0);
        value.setHighLimit(5.0);
        QVERIFY(value.isAboveLimit());
        value.setHighLimit(9.0);
        QVERIFY(!value.isAboveLimit());
    }
};

QTEST_GUILESS_MAIN(TestProcessValue)
#include "tst_processvalue.moc"
