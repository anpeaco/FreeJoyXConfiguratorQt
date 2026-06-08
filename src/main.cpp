#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QStyleFactory>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include "infolabel.h"
#include "windowthemehelper.h"
#include "tooltippositioner.h"

// global environment
#include "global.h"
GlobalEnvironment gEnv;
#include "deviceconfig.h"
#include "devicesync.h"
#include "widgets/debugwindow.h"   // DebugWindow::LogLevel for the message handler

// Get the default Qt message handler.
static const QtMessageHandler QT_DEFAULT_MESSAGE_HANDLER = qInstallMessageHandler(nullptr);

// define QT_NO_DEBUG_OUTPUT - no debug info
void CustomMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // mutex?
    if (gEnv.pDebugWindow != nullptr) {
        // Map the Qt severity to the debug log's level so the line is tagged +
        // colour-coded (DEBUG/INFO/WARN/ERROR). Queued: the handler can fire
        // from the HID worker thread.
        DebugWindow::LogLevel level;
        switch (type) {
            case QtDebugMsg:    level = DebugWindow::LogLevel::Debug; break;
            case QtInfoMsg:     level = DebugWindow::LogLevel::Info;  break;
            case QtWarningMsg:  level = DebugWindow::LogLevel::Warn;  break;
            case QtCriticalMsg:
            case QtFatalMsg:    level = DebugWindow::LogLevel::Error; break;
            default:            level = DebugWindow::LogLevel::Info;  break;
        }
        QMetaObject::invokeMethod(gEnv.pDebugWindow, "printMsg", Qt::QueuedConnection,
                                  Q_ARG(QString, msg), Q_ARG(int, int(level)));
    }

    // Call the default handler.
    (*QT_DEFAULT_MESSAGE_HANDLER)(type, context, msg);
}


int main(int argc, char *argv[])
{
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    #endif
    //qputenv("QT_SCALE_FACTOR", "0.8");
    qRegisterMetaType<QList<QPair<bool, QString>> >();
    /* HidDevice runs on its own QThread and emits signals carrying
     * uint16_t (firmware versions, VIDs, PIDs). Qt's metatype system
     * registers `quint16` automatically, but not the C standard
     * `uint16_t` typedef -- queued connections then fail at runtime
     * with "Cannot queue arguments of type 'uint16_t'". Register it
     * here, once, before any cross-thread connect. */
    qRegisterMetaType<uint16_t>("uint16_t");

    QElapsedTimer time;
    time.start();

    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QApplication::setStyle(new InfoProxyStyle(qApp->style()));
    QApplication a(argc, argv);

    // Apply the active theme's title bar (Windows dark mode) to every dialog
    // as it is shown, not just the main window. themeChanged() seeds the
    // active-theme flag this filter reads.
    a.installEventFilter(new freejoy_style::TitleBarThemeFilter(&a));

    // Anchor every tooltip to its control and keep it inside the app window
    // (rather than Qt's default cursor-anchored placement). See tooltippositioner.h.
    a.installEventFilter(new freejoy_ui::TooltipPositioner(&a));

    QString docLoc = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docLoc.isEmpty() == false) {
        docLoc+= "/FreeJoy/";
    }

    QDir dir(docLoc);
    if (dir.exists() == false) {
        dir.mkpath(".");
        dir.mkpath("configs");
    }

    QSettings appSettings(docLoc + "FreeJoySettings.conf", QSettings::IniFormat);

    DeviceConfig deviceConfig;
    DeviceSync deviceSync;
    QTranslator translator;

    // global
    gEnv.pAppSettings = &appSettings;
    gEnv.pDeviceConfig = &deviceConfig;
    gEnv.pDeviceSync = &deviceSync;
    gEnv.pTranslator = &translator;

    qInstallMessageHandler(CustomMessageHandler);

    gEnv.pApp_start_time = &time;

    // set font size
    gEnv.pAppSettings->beginGroup("FontSettings");
    QFont defaultFont = QApplication::font();
    defaultFont.setPointSize(gEnv.pAppSettings->value("FontSize", "8").toInt());
    qApp->setFont(defaultFont);
    gEnv.pAppSettings->endGroup();

    // load language settings
    appSettings.beginGroup("LanguageSettings");
    bool ok = false;
    if (appSettings.value("Language", "english").toString() == "russian")
    {
        ok = gEnv.pTranslator->load(":/FreeJoyQt_ru");
        if (ok == false) {
            qCritical()<<"failed to load translate file";
        } else {
            qApp->installTranslator(gEnv.pTranslator);
        }
    }
    else if (appSettings.value("Language", "english").toString() == "schinese")
    {
        ok = gEnv.pTranslator->load(":/FreeJoyQt_zh_CN");
        if (ok == false) {
            qCritical()<<"failed to load translate file";
        } else {
            qApp->installTranslator(gEnv.pTranslator);
        }
    }
    else if (appSettings.value("Language", "english").toString() == "deutsch")
    {
        ok = gEnv.pTranslator->load(":/FreeJoyQt_de_DE");
        if (ok == false) {
            qCritical()<<"failed to load translate file";
        } else {
            qApp->installTranslator(gEnv.pTranslator);
        }
    }
    appSettings.endGroup();

    MainWindow w;

    qDebug() << "Application startup time =" << gEnv.pApp_start_time->elapsed() << "ms";
    w.show();

    return a.exec();
}
