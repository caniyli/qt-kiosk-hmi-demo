// SPDX-License-Identifier: MIT
#include <QSignalSpy>
#include <QtTest>

#include "alarmmodel.h"

class TestAlarmModel : public QObject
{
    Q_OBJECT

private:
    const QDateTime t0 = QDateTime(QDate(2026, 1, 1), QTime(8, 0));

private slots:
    void raiseAddsNewestFirst()
    {
        AlarmModel model;
        model.raise(QStringLiteral("A"), AlarmModel::Warning, t0);
        model.raise(QStringLiteral("B"), AlarmModel::Fault, t0.addSecs(1));

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(0), AlarmModel::CodeRole).toString(), QStringLiteral("B"));
        QCOMPARE(model.data(model.index(1), AlarmModel::CodeRole).toString(), QStringLiteral("A"));
        QCOMPARE(model.activeCount(), 2);
        QCOMPARE(model.unacknowledgedCount(), 2);
        QVERIFY(model.hasOpenFault());
    }

    void raisingActiveAlarmAgainIsIgnored()
    {
        AlarmModel model;
        const int first = model.raise(QStringLiteral("A"), AlarmModel::Warning, t0);
        const int second = model.raise(QStringLiteral("A"), AlarmModel::Warning, t0.addSecs(5));
        QCOMPARE(first, second);
        QCOMPARE(model.rowCount(), 1);
    }

    void returningConditionReusesOpenEntry()
    {
        AlarmModel model;
        const int id = model.raise(QStringLiteral("A"), AlarmModel::Warning, t0);
        model.clear(QStringLiteral("A"), t0.addSecs(2));
        QVERIFY(!model.isActive(QStringLiteral("A")));

        // Not acknowledged yet: the same entry becomes active again.
        QCOMPARE(model.raise(QStringLiteral("A"), AlarmModel::Warning, t0.addSecs(3)), id);
        QCOMPARE(model.rowCount(), 1);
        QVERIFY(model.isActive(QStringLiteral("A")));
    }

    void closedEntryStaysAsHistory()
    {
        AlarmModel model;
        const int id = model.raise(QStringLiteral("A"), AlarmModel::Warning, t0);
        model.clear(QStringLiteral("A"), t0.addSecs(2));
        QVERIFY(model.acknowledge(id, t0.addSecs(3)));
        QCOMPARE(model.unacknowledgedCount(), 0);

        // Closed (cleared and acknowledged): a new occurrence gets a new row.
        const int next = model.raise(QStringLiteral("A"), AlarmModel::Warning, t0.addSecs(10));
        QVERIFY(next != id);
        QCOMPARE(model.rowCount(), 2);
    }

    void acknowledgeKeepsActiveAlarmOpen()
    {
        AlarmModel model;
        const int id = model.raise(QStringLiteral("F"), AlarmModel::Fault, t0);
        QVERIFY(model.acknowledge(id, t0.addSecs(1)));
        QVERIFY(!model.acknowledge(id, t0.addSecs(2))); // already acknowledged
        QVERIFY(model.hasOpenFault());                  // still active

        model.clear(QStringLiteral("F"), t0.addSecs(5));
        QVERIFY(!model.hasOpenFault());
    }

    void acknowledgeAllReportsCountAndSignals()
    {
        AlarmModel model;
        model.raise(QStringLiteral("A"), AlarmModel::Warning, t0);
        model.raise(QStringLiteral("B"), AlarmModel::Warning, t0);
        QSignalSpy counts(&model, &AlarmModel::countsChanged);
        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);

        QCOMPARE(model.acknowledgeAll(t0.addSecs(1)), 2);
        QCOMPARE(counts.count(), 1);
        QCOMPARE(changed.count(), 2);
        QCOMPARE(model.acknowledgeAll(t0.addSecs(2)), 0);
        QVERIFY(model.data(model.index(0), AlarmModel::AcknowledgedRole).toBool());
    }

    void capacityDropsOldestClosedEntries()
    {
        AlarmModel model(3);
        for (int i = 0; i < 5; ++i) {
            const QString code = QStringLiteral("C%1").arg(i);
            const int id = model.raise(code, AlarmModel::Warning, t0.addSecs(i));
            model.clear(code, t0.addSecs(i));
            model.acknowledge(id, t0.addSecs(i));
        }
        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(model.at(0).code, QStringLiteral("C4"));
        QCOMPARE(model.at(2).code, QStringLiteral("C2"));
    }

    void capacityNeverDropsOpenEntries()
    {
        AlarmModel model(2);
        for (int i = 0; i < 4; ++i)
            model.raise(QStringLiteral("C%1").arg(i), AlarmModel::Warning, t0);
        QCOMPARE(model.rowCount(), 4);
    }
};

QTEST_GUILESS_MAIN(TestAlarmModel)
#include "tst_alarmmodel.moc"
