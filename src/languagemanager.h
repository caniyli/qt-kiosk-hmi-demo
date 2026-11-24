// SPDX-License-Identifier: MIT
#pragma once

#include <QObject>
#include <QStringList>
#include <QTranslator>

class QQmlEngine;

// Switches the UI language at runtime. English is the source language; other
// languages are loaded from compiled translations (:/i18n/hmi_<code>.qm) and
// the QML engine re-evaluates every qsTr() binding.
class LanguageManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QStringList availableLanguages READ availableLanguages CONSTANT)

public:
    explicit LanguageManager(QQmlEngine *engine, QObject *parent = nullptr);

    QString language() const { return m_language; }
    // Unknown codes or missing translation files fall back to English.
    void setLanguage(const QString &language);

    static QStringList availableLanguages();

signals:
    void languageChanged();

private:
    QQmlEngine *m_engine;
    QTranslator m_translator;
    QString m_language = QStringLiteral("en");
    bool m_installed = false;
};
