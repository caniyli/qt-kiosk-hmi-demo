// SPDX-License-Identifier: MIT
#include "languagemanager.h"

#include <QCoreApplication>
#include <QQmlEngine>
#include <QtDebug>

LanguageManager::LanguageManager(QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
}

QStringList LanguageManager::availableLanguages()
{
    return {QStringLiteral("en"), QStringLiteral("tr")};
}

void LanguageManager::setLanguage(const QString &language)
{
    QString code = availableLanguages().contains(language) ? language : QStringLiteral("en");
    if (code == m_language)
        return;

    if (m_installed) {
        QCoreApplication::removeTranslator(&m_translator);
        m_installed = false;
    }

    if (code != QLatin1String("en")) {
        const QString file = QStringLiteral(":/i18n/hmi_%1.qm").arg(code);
        if (m_translator.load(file)) {
            QCoreApplication::installTranslator(&m_translator);
            m_installed = true;
        } else {
            qWarning("LanguageManager: cannot load %s, using English", qPrintable(file));
            code = QStringLiteral("en");
        }
    }

    m_language = code;
    if (m_engine)
        m_engine->retranslate();
    emit languageChanged();
}
