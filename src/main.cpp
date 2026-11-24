// SPDX-License-Identifier: MIT
#include <QCommandLineParser>
#include <QCursor>
#include <QDir>
#include <QElapsedTimer>
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
#include <QVector>
#include <QtQml/qqml.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>

#include "alarmmodel.h"
#include "languagemanager.h"
#include "machinebackend.h"
#include "processvalue.h"
#include "simulatedbackend.h"
#ifdef HMI_HAS_MODBUS
#include "modbustcpbackend.h"
#endif

namespace {

// --record saves this many frames per second of recorded time; the default
// frame count covers the whole demo script (see DemoRecorder).
constexpr int kRecordFps = 10;
constexpr int kRecordFrames = 115;

struct Options
{
    bool kiosk = false;
    bool hideCursor = false;
    QSize windowSize{1280, 800};
    QString language;     // empty: use the stored setting
    int page = 0;         // index into kPages
    QString screenshotPath;
    QString recordDir;
    int recordFrames = kRecordFrames;
    quint32 seed = 0;
    QString modbusHost;
    quint16 modbusPort = 502;
    int modbusUnit = 1;
};

const QStringList kPages = {QStringLiteral("overview"), QStringLiteral("alarms"),
                            QStringLiteral("settings")};

// Fixed seed for --screenshot and --record so the published images are reproducible.
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
    const QCommandLineOption record(
        QStringLiteral("record"),
        QStringLiteral("Play a short scripted demo (motor overload, acknowledgement, language switch, "
                       "restart) after the same warm-up as --screenshot, save it as PNG frames at "
                       "%1 fps in <dir> and exit.").arg(kRecordFps),
        QStringLiteral("dir"));
    const QCommandLineOption frames(QStringLiteral("frames"),
                                    QStringLiteral("Number of frames for --record, default %1.")
                                        .arg(kRecordFrames),
                                    QStringLiteral("n"));
    const QCommandLineOption seed(QStringLiteral("seed"),
                                  QStringLiteral("Random seed for the simulator (0 = random)."),
                                  QStringLiteral("n"));
    const QCommandLineOption modbus(QStringLiteral("modbus"),
                                    QStringLiteral("Read data from a Modbus TCP controller instead of "
                                                   "the simulator."),
                                    QStringLiteral("host[:port]"));
    const QCommandLineOption modbusUnit(QStringLiteral("modbus-unit"),
                                        QStringLiteral("Modbus unit id, default 1."), QStringLiteral("id"));
    parser.addOptions({kiosk, windowed, hideCursor, size, language, page, screenshot, record, frames,
                       seed, modbus, modbusUnit});
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
    options.recordDir = parser.value(record);
    if (parser.isSet(frames)) {
        bool ok = false;
        options.recordFrames = parser.value(frames).toInt(&ok);
        if (!ok || options.recordFrames < 1)
            fail(QStringLiteral("Invalid --frames, expected a positive number"));
    }
    if (!options.recordDir.isEmpty()) {
        if (!options.screenshotPath.isEmpty())
            fail(QStringLiteral("--record and --screenshot cannot be combined"));
        if (parser.isSet(modbus))
            fail(QStringLiteral("--record plays a simulator scenario and cannot be combined with --modbus"));
    }
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
    if ((!options.screenshotPath.isEmpty() || !options.recordDir.isEmpty()) && options.seed == 0)
        simulation.seed = kScreenshotSeed;
    // --record steps the simulator itself, in step with the recorded frames.
    simulation.realTime = options.recordDir.isEmpty();
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

// --record: plays a short scripted demo and saves each frame as
// <dir>/frame-0000.png, frame-0001.png, ... (scripts/make-demo-gif.sh turns
// them into docs/demo.gif). Recorded time advances 1/kRecordFps s per frame;
// the simulator is stepped from here and the script is timed in frames, so
// values and events are the same in every run (only clock times differ), even
// when grabbing is slower than real time. QML animations and timers (gauge
// smoothing, the blinking state indicator, the clock) follow the wall clock,
// so frames are paced to it as long as grabbing keeps up.
class DemoRecorder
{
public:
    DemoRecorder(QQuickWindow *window, SimulatedBackend *simulator, LanguageManager *language,
                 const QString &dir, int frameCount)
        : m_window(window)
        , m_simulator(simulator)
        , m_dir(dir)
        , m_frameCount(frameCount)
    {
        // Seconds into the recording and what happens then.
        m_script = {
            {1.5, [simulator] { simulator->injectFault(); }},          // motor overload trip
            {3.5, [window] { window->setProperty("currentPage", 1); }}, // alarm journal
            {5.0, [simulator] { simulator->acknowledgeAllAlarms(); }},
            {6.0, [window] { window->setProperty("currentPage", 2); }}, // settings
            {6.8, [language] { language->setLanguage(QStringLiteral("tr")); }},
            {9.0, [window] { window->setProperty("currentPage", 0); }}, // overview
            {10.0, [simulator] { simulator->start(); }},                // relay reset 8 s after the trip
        };
    }

    void start()
    {
        if (!QDir().mkpath(m_dir)) {
            std::fprintf(stderr, "Could not create directory %s\n", qPrintable(m_dir));
            QCoreApplication::exit(1);
            return;
        }
        m_wallClock.start();
        captureFrame();
    }

private:
    void captureFrame()
    {
        const int samplesDue = m_frame * 1000 / (kRecordFps * m_simulator->sampleIntervalMs());
        for (; m_samples < samplesDue; ++m_samples)
            m_simulator->runFor(m_simulator->sampleIntervalMs() / 1000.0);
        for (; m_nextStep < m_script.size() && qRound(m_script[m_nextStep].first * kRecordFps) <= m_frame;
             ++m_nextStep)
            m_script[m_nextStep].second();

        // Only one grab is in flight; replacing the previous result frees its image.
        m_grab = m_window->contentItem()->grabToImage();
        if (!m_grab) {
            std::fprintf(stderr, "Could not grab the window contents\n");
            QCoreApplication::exit(1);
            return;
        }
        QObject::connect(m_grab.data(), &QQuickItemGrabResult::ready, m_window, [this] { saveFrame(); });
    }

    void saveFrame()
    {
        const QString path =
            QDir(m_dir).filePath(QStringLiteral("frame-%1.png").arg(m_frame, 4, 10, QLatin1Char('0')));
        if (!m_grab->image().save(path)) {
            std::fprintf(stderr, "Could not write frame to %s\n", qPrintable(path));
            QCoreApplication::exit(1);
            return;
        }
        if (++m_frame == m_frameCount) {
            std::printf("%d frames written to %s\n", m_frameCount, qPrintable(m_dir));
            QCoreApplication::quit();
            return;
        }
        const qint64 wait = qint64(m_frame) * 1000 / kRecordFps - m_wallClock.elapsed();
        QTimer::singleShot(int(std::max<qint64>(0, wait)), m_window, [this] { captureFrame(); });
    }

    QQuickWindow *m_window;
    SimulatedBackend *m_simulator;
    QString m_dir;
    int m_frameCount;
    QVector<QPair<double, std::function<void()>>> m_script;
    QSharedPointer<QQuickItemGrabResult> m_grab;
    QElapsedTimer m_wallClock;
    int m_frame = 0;
    int m_samples = 0;
    int m_nextStep = 0;
};

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
    const bool recordMode = !options.recordDir.isEmpty();

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

    // The demo modes start from the seeded scenario and do not touch the stored settings.
    if (screenshotMode || recordMode) {
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

    // Give animations and canvas items time to settle before grabbing.
    std::unique_ptr<DemoRecorder> recorder;
    if (screenshotMode) {
        QTimer::singleShot(1500, window, [window, &options] { saveScreenshot(window, options.screenshotPath); });
    } else if (recordMode) {
        // --record excludes --modbus, so the backend is the simulator.
        recorder = std::make_unique<DemoRecorder>(window, qobject_cast<SimulatedBackend *>(backend), language,
                                                  options.recordDir, options.recordFrames);
        QTimer::singleShot(1500, window, [&recorder] { recorder->start(); });
    }
    return app.exec();
}
