// SPDX-License-Identifier: MIT
#include <QCommandLineParser>
#include <QCursor>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QSharedPointer>
#include <QSettings>
#include <QTimer>
#include <QVariantMap>
#include <QtQml/qqml.h>

#include <cstdio>
#include <cstdlib>

#include "alarmmodel.h"
#include "languagemanager.h"
#include "machinebackend.h"
#include "processvalue.h"
#include "simulatedbackend.h"
#ifdef HMI_HAS_MODBUS
#include "modbustcpbackend.h"
#endif

namespace {

struct Options
{
    bool kiosk = false;
    bool hideCursor = false;
    QSize windowSize{1280, 800};
    QString language;     // empty: use the stored setting
    int page = 0;         // index into kPages
    QString screenshotPath;
    quint32 seed = 0;
    QString modbusHost;
    quint16 modbusPort = 502;
    int modbusUnit = 1;
};

const QStringList kPages = {QStringLiteral("overview"), QStringLiteral("alarms"),
                            QStringLiteral("settings")};

// Fixed seed for --screenshot so the published screenshots are reproducible.
constexpr quint32 kScreenshotSeed = 3;

[[noreturn]] void fail(const QString &message)
{
    std::fprintf(stderr, "%s\n", qPrintable(message));
    std::exit(1);
}

Options parseOptions(const QGuiApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Touch-screen machine operator panel (HMI) demo. Uses simulated data unless a "
                       "Modbus TCP controller is given."));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption kiosk(QStringLiteral("kiosk"), QStringLiteral("Full screen, frameless window."));
    const QCommandLineOption windowed(QStringLiteral("windowed"),
                                      QStringLiteral("Normal window (default)."));
    const QCommandLineOption hideCursor(QStringLiteral("hide-cursor"),
                                        QStringLiteral("Hide the mouse cursor (touch screens)."));
    const QCommandLineOption size(QStringLiteral("size"), QStringLiteral("Window size, default 1280x800."),
                                  QStringLiteral("WxH"));
    const QCommandLineOption language(QStringLiteral("lang"), QStringLiteral("UI language: en or tr."),
                                      QStringLiteral("code"));
    const QCommandLineOption page(QStringLiteral("page"),
                                  QStringLiteral("Start page: overview, alarms or settings."),
                                  QStringLiteral("name"));
    const QCommandLineOption screenshot(
        QStringLiteral("screenshot"),
        QStringLiteral("Render the window to <file> after a simulated warm-up and exit. "
                       "Combine with QT_QPA_PLATFORM=offscreen on headless machines."),
        QStringLiteral("file"));
    const QCommandLineOption seed(QStringLiteral("seed"),
                                  QStringLiteral("Random seed for the simulator (0 = random)."),
                                  QStringLiteral("n"));
    const QCommandLineOption modbus(QStringLiteral("modbus"),
                                    QStringLiteral("Read data from a Modbus TCP controller instead of "
                                                   "the simulator."),
                                    QStringLiteral("host[:port]"));
    const QCommandLineOption modbusUnit(QStringLiteral("modbus-unit"),
                                        QStringLiteral("Modbus unit id, default 1."), QStringLiteral("id"));
    parser.addOptions({kiosk, windowed, hideCursor, size, language, page, screenshot, seed, modbus,
                       modbusUnit});
    parser.process(app);

    Options options;
    options.kiosk = parser.isSet(kiosk) && !parser.isSet(windowed);
    options.hideCursor = parser.isSet(hideCursor);

    if (parser.isSet(size)) {
        const QStringList parts = parser.value(size).split(QLatin1Char('x'));
        const int width = parts.value(0).toInt();
        const int height = parts.value(1).toInt();
        if (parts.size() != 2 || width < 320 || height < 240)
            fail(QStringLiteral("Invalid --size, expected for example 1280x800"));
        options.windowSize = QSize(width, height);
    }
    if (parser.isSet(language)) {
        options.language = parser.value(language);
        if (!LanguageManager::availableLanguages().contains(options.language))
            fail(QStringLiteral("Unsupported --lang, use one of: %1")
                     .arg(LanguageManager::availableLanguages().join(QStringLiteral(", "))));
    }
    if (parser.isSet(page)) {
        options.page = kPages.indexOf(parser.value(page));
        if (options.page < 0)
            fail(QStringLiteral("Unknown --page, use one of: %1").arg(kPages.join(QStringLiteral(", "))));
    }
    options.screenshotPath = parser.value(screenshot);
    options.seed = parser.value(seed).toUInt();

    if (parser.isSet(modbus)) {
        const QString value = parser.value(modbus);
        const int colon = value.lastIndexOf(QLatin1Char(':'));
        options.modbusHost = colon > 0 ? value.left(colon) : value;
        if (colon > 0) {
            bool ok = false;
            options.modbusPort = static_cast<quint16>(value.mid(colon + 1).toUInt(&ok));
            if (!ok || options.modbusPort == 0)
                fail(QStringLiteral("Invalid port in --modbus"));
        }
        options.modbusUnit = parser.isSet(modbusUnit) ? parser.value(modbusUnit).toInt() : 1;
    }
    return options;
}

MachineBackend *createBackend(const Options &options, QObject *parent)
{
    if (!options.modbusHost.isEmpty()) {
#ifdef HMI_HAS_MODBUS
        ModbusTcpBackend::Config config;
        config.host = options.modbusHost;
        config.port = options.modbusPort;
        config.unitId = options.modbusUnit;
        return new ModbusTcpBackend(config, parent);
#else
        fail(QStringLiteral("This build has no Modbus support (Qt SerialBus was not found)."));
#endif
    }

    SimulatedBackend::Options simulation;
    simulation.seed = options.seed;
    if (!options.screenshotPath.isEmpty() && options.seed == 0)
        simulation.seed = kScreenshotSeed;
    return new SimulatedBackend(simulation, parent);
}

// Persists operator settings across restarts. Setpoints are only stored for
// the simulator; a real controller owns its setpoints.
void persistSettings(QSettings *settings, MachineBackend *backend, LanguageManager *language,
                     const QString &languageOverride)
{
    language->setLanguage(languageOverride.isEmpty()
                              ? settings->value(QStringLiteral("language"), QStringLiteral("en")).toString()
                              : languageOverride);
    QObject::connect(language, &LanguageManager::languageChanged, settings, [settings, language] {
        settings->setValue(QStringLiteral("language"), language->language());
    });

    for (ProcessValue *value : backend->processValues()) {
        const QString limitKey = value->key() + QStringLiteral("/highLimit");
        value->setHighLimit(settings->value(limitKey, value->highLimit()).toDouble());
        QObject::connect(value, &ProcessValue::highLimitChanged, settings, [settings, value, limitKey] {
            settings->setValue(limitKey, value->highLimit());
        });

        if (!backend->isSimulated())
            continue;
        const QString setpointKey = value->key() + QStringLiteral("/setpoint");
        value->setSetpoint(settings->value(setpointKey, value->setpoint()).toDouble());
        QObject::connect(value, &ProcessValue::setpointChanged, settings, [settings, value, setpointKey] {
            settings->setValue(setpointKey, value->setpoint());
        });
    }
}

// QML singletons need a provider function; these hold the instances it returns.
QObject *g_machine = nullptr;
QObject *g_language = nullptr;

void registerQmlTypes(MachineBackend *machine, LanguageManager *language)
{
    g_machine = machine;
    g_language = language;

    const char *uri = "Hmi.Backend";
    const QString reason = QStringLiteral("Provided by the application");
    qmlRegisterUncreatableType<MachineBackend>(uri, 1, 0, "MachineBackend", reason);
    qmlRegisterUncreatableType<ProcessValue>(uri, 1, 0, "ProcessValue", reason);
    qmlRegisterUncreatableType<AlarmModel>(uri, 1, 0, "AlarmModel", reason);
    qmlRegisterSingletonType<MachineBackend>(uri, 1, 0, "Machine", [](QQmlEngine *, QJSEngine *) -> QObject * {
        QQmlEngine::setObjectOwnership(g_machine, QQmlEngine::CppOwnership);
        return g_machine;
    });
    qmlRegisterSingletonType<LanguageManager>(uri, 1, 0, "Language", [](QQmlEngine *, QJSEngine *) -> QObject * {
        QQmlEngine::setObjectOwnership(g_language, QQmlEngine::CppOwnership);
        return g_language;
    });
}

// Grabs the window's root item rather than using QQuickWindow::grabWindow():
// with the software renderer the latter needs platform support that the
// headless "offscreen" platform lacks, while an item grab works everywhere.
void saveScreenshot(QQuickWindow *window, const QString &path)
{
    const QSharedPointer<QQuickItemGrabResult> grab = window->contentItem()->grabToImage();
    if (!grab) {
        std::fprintf(stderr, "Could not grab the window contents\n");
        QCoreApplication::exit(1);
        return;
    }
    QObject::connect(grab.data(), &QQuickItemGrabResult::ready, window, [grab, path] {
        if (!grab->image().save(path)) {
            std::fprintf(stderr, "Could not write screenshot to %s\n", qPrintable(path));
            QCoreApplication::exit(1);
            return;
        }
        std::printf("Screenshot written to %s\n", qPrintable(path));
        QCoreApplication::quit();
    });
}

} // namespace

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);
    // Both names are needed for QSettings: ~/.config/qt-kiosk-hmi-demo/qt-kiosk-hmi-demo.conf
    QCoreApplication::setOrganizationName(QStringLiteral("qt-kiosk-hmi-demo"));
    QCoreApplication::setApplicationName(QStringLiteral("qt-kiosk-hmi-demo"));
    QCoreApplication::setApplicationVersion(QStringLiteral(HMI_VERSION));

    const Options options = parseOptions(app);
    const bool screenshotMode = !options.screenshotPath.isEmpty();

    // Headless platforms have no OpenGL; the software renderer is enough here.
    const QString platform = QGuiApplication::platformName();
    if (platform == QLatin1String("offscreen") || platform == QLatin1String("minimal"))
        QQuickWindow::setSceneGraphBackend(QStringLiteral("software"));

    // Objects exposed as QML singletons must outlive the engine, so they are
    // owned by the application object, which is destroyed after the engine.
    QQmlApplicationEngine engine;
    MachineBackend *backend = createBackend(options, &app);
    auto *language = new LanguageManager(&engine, &app);
    registerQmlTypes(backend, language);

    if (screenshotMode) {
        language->setLanguage(options.language);
        if (auto *simulator = qobject_cast<SimulatedBackend *>(backend))
            simulator->playDemoScenario();
    } else {
        persistSettings(new QSettings(&app), backend, language, options.language);
    }

    engine.rootContext()->setContextProperty(
        QStringLiteral("appInfo"),
        QVariantMap{{QStringLiteral("version"), QCoreApplication::applicationVersion()},
                    {QStringLiteral("qtVersion"), QString::fromLatin1(qVersion())},
                    {QStringLiteral("kiosk"), options.kiosk}});
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    auto *window = engine.rootObjects().isEmpty()
                       ? nullptr
                       : qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (!window)
        return 1;

    window->setProperty("currentPage", options.page);
    if (options.hideCursor)
        QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

    if (options.kiosk) {
        window->setFlags(window->flags() | Qt::FramelessWindowHint);
        window->showFullScreen();
    } else {
        window->resize(options.windowSize);
        window->show();
    }

    if (screenshotMode) {
        // Give animations and canvas items time to settle before grabbing.
        QTimer::singleShot(1500, window, [window, &options] { saveScreenshot(window, options.screenshotPath); });
    }
    return app.exec();
}
