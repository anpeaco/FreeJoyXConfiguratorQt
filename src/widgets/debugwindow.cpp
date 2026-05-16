#include "debugwindow.h"
#include "ui_debugwindow.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QSettings>

#include "global.h"
#include <QDebug>
#include <QStandardPaths>
DebugWindow::DebugWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DebugWindow)
{
    ui->setupUi(this);

    m_packetsCount = 0;
    m_writeToFile = false;

    QString docLoc = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docLoc.isEmpty() == false) {
        docLoc+= "/FreeJoy/";
    }
    QSettings s(docLoc + "FreeJoySettings.conf", QSettings::IniFormat);

    s.beginGroup("OtherSettings");
    ui->checkBox_WriteLog->setChecked(s.value("LogEnabled", "false").toBool());
    s.endGroup();
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

void DebugWindow::printMsg(const QString &msg)
{
    QString log(QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + ": " + msg + '\n');
    ui->textBrowser_DebugMsg->insertPlainText(log);         // append?
    ui->textBrowser_DebugMsg->moveCursor(QTextCursor::End); // works oddly with plainTextEdit

    if (m_writeToFile) {
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
            /* DO NOT qWarning() here -- the custom message handler routes
             * back into printMsg, and with m_writeToFile still true we'd
             * recurse straight back into this failing open and overflow
             * the stack. Drop silently; the textBrowser still shows the
             * original message even if disk write failed. */
            return;
        }
        QTextStream out(&file);
        out << log;
    }
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
    if (state) {
        ui->textBrowser_ButtonsPressLog->insertPlainText(
                    QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + ": " + tr("Logical button ")
                    + QString::number(buttonNumber) + tr(" pressed") + '\n');

        ui->textBrowser_ButtonsPressLog->moveCursor(QTextCursor::End);
    } else {
        ui->textBrowser_ButtonsUnpressLog->insertPlainText(
                    QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + ": " + tr("Logical button ")
                    + QString::number(buttonNumber) + tr(" unpressed") + '\n');

        ui->textBrowser_ButtonsUnpressLog->moveCursor(QTextCursor::End);
    }
}

void DebugWindow::buttonLogReset()
{
    ui->textBrowser_ButtonsPressLog->clear();
    ui->textBrowser_ButtonsUnpressLog->clear(); // need to improve
}

void DebugWindow::on_checkBox_WriteLog_clicked(bool checked)
{
    gEnv.pAppSettings->beginGroup("OtherSettings");
    gEnv.pAppSettings->setValue("LogEnabled", checked);
    gEnv.pAppSettings->endGroup();

    m_writeToFile = checked;
}
