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
DebugWindow::DebugWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DebugWindow)
{
    ui->setupUi(this);

    m_packetsCount = 0;

    QString docLoc = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docLoc.isEmpty() == false) {
        docLoc+= "/FreeJoy/";
    }
    QSettings s(docLoc + "FreeJoySettings.conf", QSettings::IniFormat);

    s.beginGroup("OtherSettings");
    const bool logEnabled = s.value("LogEnabled", false).toBool();
    s.endGroup();

    /* Mirror the persisted setting into BOTH the UI checkbox state AND
     * the m_writeToFile flag that printMsg actually gates on. Previous
     * code only synced the UI; m_writeToFile stayed false until the
     * user clicked the checkbox, so logging silently no-op'd until then
     * even if the UI showed it as enabled. */
    ui->checkBox_WriteLog->setChecked(logEnabled);
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

void DebugWindow::devicePacketReceived()
{
    //    if (isVisible()){
    //    }
    static int count = 0;
    static QElapsedTimer packet_timer;

    m_packetsCount++;
    if (packet_timer.hasExpired(100)) {
        ui->label_PacketsCount->setNum(m_packetsCount);
        packet_timer.start();
    }

    if (m_timer.hasExpired(5000) && m_timer.isValid()) {
        ui->label_PacketsSpeed->setText(QString::number((double(m_timer.restart()) / double(count)), 'f', 3)
                                        + tr(" ms"));
        count = 0;
    } else if (m_timer.isValid() == false) { // valid/invalid toggle keeps display correct across device connect/disconnect
        m_timer.start();
    }

    count++;
}

void DebugWindow::resetPacketsCount()
{
    m_packetsCount = 0;
    ui->label_PacketsCount->setNum(m_packetsCount);

    m_timer.invalidate();
    ui->label_PacketsSpeed->setText(tr("0 ms"));

    buttonLogReset();
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

void DebugWindow::printMsg(const QString &msg)
{
    QString log(QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + ": " + msg + '\n');
    ui->textBrowser_DebugMsg->insertPlainText(log);         // append?
    ui->textBrowser_DebugMsg->moveCursor(QTextCursor::End); // works oddly with plainTextEdit

    appendToLogFile(log);
}

void DebugWindow::on_pushButton_LogMarker_clicked()
{
    /* A distinctive line the user can grep for when reviewing the log
     * file after a bench session. Routed through printMsg so it lands
     * in both the in-app textBrowser and the on-disk log (if enabled). */
    static int counter = 0;
    ++counter;
    printMsg(QString("================ MARKER #%1 ================").arg(counter));
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
     * during startup races) -- render '?' rather than crashing. */
    const QString stamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
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
    const QString line = stamp + QStringLiteral(" | LBTN slot=") + QString::number(buttonNumber)
                       + QStringLiteral(" type=") + typeStr
                       + QStringLiteral(" phys=") + physStr
                       + QStringLiteral(" state=") + (state ? QStringLiteral("ON") : QStringLiteral("OFF"))
                       + '\n';

    ui->textBrowser_ButtonsLog->insertPlainText(line);
    ui->textBrowser_ButtonsLog->moveCursor(QTextCursor::End);

    appendToLogFile(line);
}

void DebugWindow::physicalButtonState(int buttonNumber, bool state)
{
    /* Structured log line:
     *   HH:mm:ss.zzz | PBTN phys=N state=ON|OFF
     *
     * Routed to both the merged in-app log panel and the on-disk log
     * so the physical->logical correlation is visible in the UI during
     * bench testing without tab-switching to the log file. */
    const QString stamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    const QString line = stamp + QStringLiteral(" | PBTN phys=") + QString::number(buttonNumber)
                       + QStringLiteral(" state=") + (state ? QStringLiteral("ON") : QStringLiteral("OFF"))
                       + '\n';

    ui->textBrowser_ButtonsLog->insertPlainText(line);
    ui->textBrowser_ButtonsLog->moveCursor(QTextCursor::End);

    appendToLogFile(line);
}

void DebugWindow::buttonLogReset()
{
    ui->textBrowser_ButtonsLog->clear();
}

void DebugWindow::on_checkBox_WriteLog_clicked(bool checked)
{
    gEnv.pAppSettings->beginGroup("OtherSettings");
    gEnv.pAppSettings->setValue("LogEnabled", checked);
    gEnv.pAppSettings->endGroup();

    m_writeToFile = checked;
}
