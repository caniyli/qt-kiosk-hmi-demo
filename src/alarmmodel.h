// SPDX-License-Identifier: MIT
#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QString>
#include <QVector>

#include <limits>

// Alarm journal shown on the alarm screen, newest entry first.
//
// An alarm is "active" while its condition is present and "open" while it is
// active or not yet acknowledged by the operator. If a condition returns while
// the previous occurrence is still open, that entry is re-activated instead of
// adding a new row, which keeps a flapping signal from flooding the list.
// Closed entries stay in the list as history until the capacity is reached.
class AlarmModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countsChanged)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY countsChanged)
    Q_PROPERTY(int unacknowledgedCount READ unacknowledgedCount NOTIFY countsChanged)

public:
    enum Severity {
        Warning, // process value outside its limit, machine keeps running
        Fault    // machine was stopped; needs acknowledgement before restart
    };
    Q_ENUM(Severity)

    enum Role {
        IdRole = Qt::UserRole + 1,
        CodeRole,
        SeverityRole,
        RaisedAtRole,
        ClearedAtRole,
        AcknowledgedAtRole,
        ActiveRole,
        AcknowledgedRole,
        ValueRole,
        LimitRole,
        UnitRole,
        DecimalsRole
    };

    struct Alarm
    {
        int id = 0;
        QString code;
        Severity severity = Warning;
        QDateTime raisedAt;
        QDateTime clearedAt;
        QDateTime acknowledgedAt;
        double value = std::numeric_limits<double>::quiet_NaN();
        double limit = std::numeric_limits<double>::quiet_NaN();
        QString unit;
        int decimals = 0;
        bool active = false;

        bool isAcknowledged() const { return acknowledgedAt.isValid(); }
        bool isOpen() const { return active || !isAcknowledged(); }
    };

    explicit AlarmModel(int capacity = 200, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Raises `code`; returns the id of the new or re-activated entry.
    int raise(const QString &code, Severity severity, const QDateTime &time,
              double value = std::numeric_limits<double>::quiet_NaN(),
              double limit = std::numeric_limits<double>::quiet_NaN(),
              const QString &unit = QString(), int decimals = 0);
    // Marks the active entry for `code` as cleared (condition no longer present).
    void clear(const QString &code, const QDateTime &time);
    // Returns false if `id` is unknown or already acknowledged.
    bool acknowledge(int id, const QDateTime &time);
    // Returns the number of entries that were acknowledged.
    int acknowledgeAll(const QDateTime &time);

    bool isActive(const QString &code) const;
    bool hasOpenFault() const;
    const Alarm &at(int row) const { return m_alarms.at(row); }

    int count() const { return m_alarms.size(); }
    int activeCount() const;
    int unacknowledgedCount() const;

signals:
    void countsChanged();

private:
    int findOpenRow(const QString &code) const;
    void trimToCapacity();
    void notifyRowChanged(int row);

    QVector<Alarm> m_alarms; // newest first
    int m_capacity;
    int m_nextId = 1;
};
