// SPDX-License-Identifier: MIT
#include "alarmmodel.h"

#include <algorithm>

AlarmModel::AlarmModel(int capacity, QObject *parent)
    : QAbstractListModel(parent)
    , m_capacity(qMax(1, capacity))
{
}

int AlarmModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_alarms.size();
}

QVariant AlarmModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.parent().isValid() || index.row() >= m_alarms.size())
        return {};

    const Alarm &alarm = m_alarms.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case CodeRole:
        return alarm.code;
    case IdRole:
        return alarm.id;
    case SeverityRole:
        return alarm.severity;
    case RaisedAtRole:
        return alarm.raisedAt;
    case ClearedAtRole:
        return alarm.clearedAt;
    case AcknowledgedAtRole:
        return alarm.acknowledgedAt;
    case ActiveRole:
        return alarm.active;
    case AcknowledgedRole:
        return alarm.isAcknowledged();
    case ValueRole:
        return alarm.value;
    case LimitRole:
        return alarm.limit;
    case UnitRole:
        return alarm.unit;
    case DecimalsRole:
        return alarm.decimals;
    default:
        return {};
    }
}

QHash<int, QByteArray> AlarmModel::roleNames() const
{
    return {
        {IdRole, "alarmId"},
        {CodeRole, "code"},
        {SeverityRole, "severity"},
        {RaisedAtRole, "raisedAt"},
        {ClearedAtRole, "clearedAt"},
        {AcknowledgedAtRole, "acknowledgedAt"},
        {ActiveRole, "active"},
        {AcknowledgedRole, "acknowledged"},
        {ValueRole, "value"},
        {LimitRole, "limit"},
        {UnitRole, "unit"},
        {DecimalsRole, "decimals"},
    };
}

int AlarmModel::raise(const QString &code, Severity severity, const QDateTime &time,
                      double value, double limit, const QString &unit, int decimals)
{
    const int openRow = findOpenRow(code);
    if (openRow >= 0) {
        Alarm &alarm = m_alarms[openRow];
        if (alarm.active)
            return alarm.id;
        alarm.active = true;
        alarm.clearedAt = QDateTime();
        alarm.value = value;
        alarm.limit = limit;
        notifyRowChanged(openRow);
        emit countsChanged();
        return alarm.id;
    }

    Alarm alarm;
    alarm.id = m_nextId++;
    alarm.code = code;
    alarm.severity = severity;
    alarm.raisedAt = time;
    alarm.value = value;
    alarm.limit = limit;
    alarm.unit = unit;
    alarm.decimals = decimals;
    alarm.active = true;

    beginInsertRows(QModelIndex(), 0, 0);
    m_alarms.prepend(alarm);
    endInsertRows();

    trimToCapacity();
    emit countsChanged();
    return alarm.id;
}

void AlarmModel::clear(const QString &code, const QDateTime &time)
{
    for (int row = 0; row < m_alarms.size(); ++row) {
        Alarm &alarm = m_alarms[row];
        if (alarm.code == code && alarm.active) {
            alarm.active = false;
            alarm.clearedAt = time;
            notifyRowChanged(row);
            emit countsChanged();
            return;
        }
    }
}

bool AlarmModel::acknowledge(int id, const QDateTime &time)
{
    for (int row = 0; row < m_alarms.size(); ++row) {
        Alarm &alarm = m_alarms[row];
        if (alarm.id != id)
            continue;
        if (alarm.isAcknowledged())
            return false;
        alarm.acknowledgedAt = time;
        notifyRowChanged(row);
        emit countsChanged();
        return true;
    }
    return false;
}

int AlarmModel::acknowledgeAll(const QDateTime &time)
{
    int acknowledged = 0;
    for (int row = 0; row < m_alarms.size(); ++row) {
        Alarm &alarm = m_alarms[row];
        if (alarm.isAcknowledged())
            continue;
        alarm.acknowledgedAt = time;
        notifyRowChanged(row);
        ++acknowledged;
    }
    if (acknowledged > 0)
        emit countsChanged();
    return acknowledged;
}

bool AlarmModel::isActive(const QString &code) const
{
    return std::any_of(m_alarms.cbegin(), m_alarms.cend(), [&code](const Alarm &alarm) {
        return alarm.code == code && alarm.active;
    });
}

bool AlarmModel::hasOpenFault() const
{
    return std::any_of(m_alarms.cbegin(), m_alarms.cend(), [](const Alarm &alarm) {
        return alarm.severity == Fault && alarm.isOpen();
    });
}

int AlarmModel::activeCount() const
{
    return static_cast<int>(std::count_if(m_alarms.cbegin(), m_alarms.cend(),
                                          [](const Alarm &alarm) { return alarm.active; }));
}

int AlarmModel::unacknowledgedCount() const
{
    return static_cast<int>(std::count_if(m_alarms.cbegin(), m_alarms.cend(),
                                          [](const Alarm &alarm) { return !alarm.isAcknowledged(); }));
}

int AlarmModel::findOpenRow(const QString &code) const
{
    for (int row = 0; row < m_alarms.size(); ++row) {
        const Alarm &alarm = m_alarms.at(row);
        if (alarm.code == code && alarm.isOpen())
            return row;
    }
    return -1;
}

void AlarmModel::trimToCapacity()
{
    // Drop the oldest closed entries. Open alarms are never dropped, so the
    // list may exceed the capacity while many alarms are pending.
    for (int row = m_alarms.size() - 1; row >= 0 && m_alarms.size() > m_capacity; --row) {
        if (m_alarms.at(row).isOpen())
            continue;
        beginRemoveRows(QModelIndex(), row, row);
        m_alarms.removeAt(row);
        endRemoveRows();
    }
}

void AlarmModel::notifyRowChanged(int row)
{
    const QModelIndex changed = index(row);
    emit dataChanged(changed, changed);
}
