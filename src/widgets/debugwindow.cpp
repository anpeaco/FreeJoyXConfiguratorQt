#include "debugwindow.h"
#include "ui_debugwindow.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QSettings>

#include "global.h"
#include "common_types.h"
#include "deviceconfig.h"
#include "style_helpers.h"
#include <QDebug>
#include <QStandardPaths>

namespace {

/* Stable, ASCII, translation-independent label for a button_type_t.
 * The dropdown's m_logicFunctionList in buttonlogical.h is deliberately
 * NOT reused: its labels are tr()'d UI strings ("Double tap", "POV1 Up")
 * which are unstable across locales and contain spaces that break
 * key=value parsing. Caller frees nothing -- returns a literal or a
 * heap QString constructed from a literal. */
QString buttonTypeLabel(button_type_t t)
{
    switch (t) {
        case BUTTON_NORMAL:     return QStringLiteral("NORMAL");
        case BUTTON_TOGGLE:     return QStringLiteral("TOGGLE");
        case TOGGLE_SWITCH:     return QStringLiteral("TOGGLE_SWITCH");
        case TOGGLE_SWITCH_ON:  return QStringLiteral("TOGGLE_SWITCH_ON");
        case TOGGLE_SWITCH_OFF: return QStringLiteral("TOGGLE_SWITCH_OFF");
        case POV1_UP:           return QStringLiteral("POV1_UP");
        case POV1_RIGHT:        return QStringLiteral("POV1_RIGHT");
        case POV1_DOWN:         return QStringLiteral("POV1_DOWN");
        case POV1_LEFT:         return QStringLiteral("POV1_LEFT");
        case POV1_CENTER:       return QStringLiteral("POV1_CENTER");
        case POV2_UP:           return QStringLiteral("POV2_UP");
        case POV2_RIGHT:        return QStringLiteral("POV2_RIGHT");
        case POV2_DOWN:         return QStringLiteral("POV2_DOWN");
        case POV2_LEFT:         return QStringLiteral("POV2_LEFT");
        case POV2_CENTER:       return QStringLiteral("POV2_CENTER");
        case POV3_UP:           return QStringLiteral("POV3_UP");
        case POV3_RIGHT:        return QStringLiteral("POV3_RIGHT");
        case POV3_DOWN:         return QStringLiteral("POV3_DOWN");
        case POV3_LEFT:         return QStringLiteral("POV3_LEFT");
        case POV3_CENTER:       return QStringLiteral("POV3_CENTER");
        case POV4_UP:           return QStringLiteral("POV4_UP");
        case POV4_RIGHT:        return QStringLiteral("POV4_RIGHT");
        case POV4_DOWN:         return QStringLiteral("POV4_DOWN");
        case POV4_LEFT:         return QStringLiteral("POV4_LEFT");
        case POV4_CENTER:       return QStringLiteral("POV4_CENTER");
        case ENCODER_INPUT_A:   return QStringLiteral("ENCODER_A");
        case ENCODER_INPUT_B:   return QStringLiteral("ENCODER_B");
        case RADIO_BUTTON1:     return QStringLiteral("RADIO_1");
        case RADIO_BUTTON2:     return QStringLiteral("RADIO_2");
        case RADIO_BUTTON3:     return QStringLiteral("RADIO_3");
        case RADIO_BUTTON4:     return QStringLiteral("RADIO_4");
        case SEQUENTIAL_TOGGLE: return QStringLiteral("SEQ_TOGGLE");
        case SEQUENTIAL_BUTTON: return QStringLiteral("SEQ_BUTTON");
        case LOGIC:             return QStringLiteral("LOGIC");
        case TAP:               return QStringLiteral("TAP");
        case DOUBLE_TAP:        return QStringLiteral("DOUBLE_TAP");
        default:                return QStringLiteral("TYPE_%1").arg(int(t));
    }
}

} // namespace
namespace {

// Colour for each log level. Empty (invalid) -> render with the widget's
// default palette text colour so INFO stays theme-correct (light/dark). The
// rest are mid-tones chosen to read on both themes.
QString colorForLevel(DebugWindow::LogLevel level)
{
    switch (level) {
        case DebugWindow::LogLevel::Debug:  return QStringLiteral("#7f8c8d"); // grey
        case DebugWindow::LogLevel::Info:   return QString();                 // default
        case DebugWindow::LogLevel::Warn:   return QStringLiteral("#d9822b"); // amber
        case DebugWindow::LogLevel::Error:  return QStringLiteral("#e74c3c"); // red
        case DebugWindow::LogLevel::Button: return QStringLiteral("#2e86de"); // blue
        case DebugWindow::LogLevel::Marker: return QStringLiteral("#8e44ad"); // purple
    }
    return QString();
}

// Short [TAG] prepended to each line.
QString tagForLevel(DebugWindow::LogLevel level)
{
    switch (level) {
        case DebugWindow::LogLevel::Debug:  return QStringLiteral("DEBUG");
        case DebugWindow::LogLevel::Info:   return QStringLiteral("INFO");
        case DebugWindow::LogLevel::Warn:   return QStringLiteral("WARN");
        case DebugWindow::LogLevel::Error:  return QStringLiteral("ERROR");
        case DebugWindow::LogLevel::Button: return QStringLiteral("BTN");
        case DebugWindow::LogLevel::Marker: return QStringLiteral("MARK");
    }
    return QStringLiteral("INFO");
}

} // namespace

DebugWindow::DebugWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DebugWindow)
{
    ui->setupUi(this);

    // Theme-track the monochrome glyph icons (light grey on dark, near-black
    // on light) instead of the .ui's fixed-black SVGs.
    freejoy_style::setThemedIcon(ui->pushButton_LogMarker, QStringLiteral(":/Images/icons/lucide/bookmark.svg"));
    freejoy_style::setThemedIcon(ui->pushButton_LogClear,  QStringLiteral(":/Images/icons/lucide/rotate-ccw.svg"));

    QString docLoc = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docLoc.isEmpty() == false) {
        docLoc+= "/FreeJoy/";
    }
    QSettings s(docLoc + "FreeJoySettings.conf", QSettings::IniFormat);

    s.beginGroup("OtherSettings");
    const bool logEnabled = s.value("LogEnabled", false).toBool();
    s.endGroup();

    /* The "Write log to file" checkbox now lives in Advanced Settings; this
     * widget just reads the persisted setting on creation so file logging is
     * correct even before the pane is opened. Advanced re-pushes changes via
     * setWriteToFile(). */
    m_writeToFile = logEnabled;
}

DebugWindow::~DebugWindow()
{
    delete ui;
}

void DebugWindow::retranslateUi()
{
    ui->retranslateUi(this);
}

void DebugWindow::setWriteToFile(bool on)
{
    m_writeToFile = on;
}

void DebugWindow::appendToLogFile(const QString &line)
{
    /* Shared file-write helper used by every code path that wants to
     * land a line in the on-disk log: printMsg (app log), button-state
     * change hooks (logical + physical), the marker button. Caller is
     * responsible for the leading timestamp; this just appends the
     * already-formatted line. */
    if (!m_writeToFile) return;

    /* yyyy-MM-dd, not "YYYY-MM-DDTHH:MM" -- Qt's format tokens are
     * case-sensitive (uppercase Y/D become literals; MM after T is
     * month again, not minute) and ':' is illegal in Windows file
     * names, so the previous version's QFile::open silently failed
     * EVERY time. Daily granularity is enough for log review. */
    const QString logDir = gEnv.pAppSettings->fileName().remove("FreeJoySettings.conf") + "log/";
    /* mkpath the log dir if missing; otherwise QFile::open fails on a
     * fresh install where the user enabled logging before anything
     * created <Documents>/FreeJoy/log/. */
    QDir().mkpath(logDir);
    const QString date(QDateTime::currentDateTime().toString("yyyy-MM-dd"));
    QFile file(logDir + "FJLog" + date + ".txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        /* Silent fail -- qWarning here would route through the message
         * handler back into printMsg -> appendToLogFile and recurse into
         * the same failing open. */
        return;
    }
    QTextStream out(&file);
    out << line;
}

void DebugWindow::appendLine(LogLevel level, const QString &msg)
{
    /* The one sink every log line flows through. Builds a
     *   HH:mm:ss.zzz [TAG] message
     * line, renders it colour-coded by level in the combined view, and mirrors
     * the same (plain) text to the on-disk log. */
    const QString stamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    const QString tag   = tagForLevel(level);
    const QString plain = stamp + QStringLiteral(" [") + tag + QStringLiteral("] ") + msg;

    // Only the [TAG] status code carries the level colour; the timestamp and
    // message stay in the default palette colour so the line reads normally and
    // just the status code stands out (and it's theme-correct). Escape the
    // message so qDebug output with '<' / '&' (pointers, templates) renders
    // literally. INFO has no colour -> its tag is the default colour too.
    const QString color = colorForLevel(level);
    QString tagHtml = QStringLiteral("[") + tag + QStringLiteral("]");
    if (level == LogLevel::Marker) {
        tagHtml = QStringLiteral("<b>") + tagHtml + QStringLiteral("</b>");
    }
    if (!color.isEmpty()) {
        tagHtml = QStringLiteral("<span style=\"color:%1;\">%2</span>").arg(color, tagHtml);
    }
    const QString html = stamp.toHtmlEscaped() + QStringLiteral(" ") + tagHtml
                       + QStringLiteral(" ") + msg.toHtmlEscaped();

    ui->textBrowser_Log->moveCursor(QTextCursor::End);
    ui->textBrowser_Log->insertHtml(html + QStringLiteral("<br>"));
    ui->textBrowser_Log->moveCursor(QTextCursor::End);

    appendToLogFile(plain + '\n');
}

void DebugWindow::printMsg(const QString &msg, int level)
{
    appendLine(static_cast<LogLevel>(level), msg);
}

void DebugWindow::on_pushButton_LogMarker_clicked()
{
    /* A distinctive line the user can spot (and grep for) when reviewing the
     * log after a bench session. */
    static int counter = 0;
    ++counter;
    appendLine(LogLevel::Marker, QStringLiteral("———— MARKER #%1 ————").arg(counter));
}

void DebugWindow::on_pushButton_LogClear_clicked()
{
    // Clears the in-app view only; the on-disk log file is untouched.
    ui->textBrowser_Log->clear();
}

void DebugWindow::logicalButtonState(int buttonNumber, bool state)
{
    /* Structured log line:
     *   HH:mm:ss.zzz | LBTN slot=N type=TYPE phys=P state=ON|OFF
     *
     * buttonNumber is 1-indexed (comes from the UI slot label). Look
     * up the slot's configured type and physical_num via the global
     * device config to enrich the line. Defensive against out-of-range
     * slot numbers or a null pDeviceConfig (theoretically possible
     * during startup races) -- render '?' rather than crashing. Lands in
     * the combined log under the BTN category (blue). */
    QString typeStr = QStringLiteral("?");
    QString physStr = QStringLiteral("?");
    if (gEnv.pDeviceConfig != nullptr
        && buttonNumber >= 1 && buttonNumber <= MAX_BUTTONS_NUM)
    {
        const button_t &btn = gEnv.pDeviceConfig->config.buttons[buttonNumber - 1];
        typeStr = buttonTypeLabel(btn.type);
        physStr = (btn.physical_num < 0) ? QStringLiteral("-")
                                         : QString::number(btn.physical_num + 1);
    }
    appendLine(LogLevel::Button,
               QStringLiteral("LBTN slot=") + QString::number(buttonNumber)
                   + QStringLiteral(" type=") + typeStr
                   + QStringLiteral(" phys=") + physStr
                   + QStringLiteral(" state=") + (state ? QStringLiteral("ON") : QStringLiteral("OFF")));
}

void DebugWindow::physicalButtonState(int buttonNumber, bool state)
{
    /* Physical button edge -- lands in the combined log under the BTN
     * category so the physical->logical correlation is visible in one
     * timeline during bench testing. */
    appendLine(LogLevel::Button,
               QStringLiteral("PBTN phys=") + QString::number(buttonNumber)
                   + QStringLiteral(" state=") + (state ? QStringLiteral("ON") : QStringLiteral("OFF")));
}
