/**
  ******************************************************************************
  * @file           : flashprogressdialog.cpp
  * @brief          : See flashprogressdialog.h.
  ******************************************************************************
  */

#include "flashprogressdialog.h"
#include "ui_flashprogressdialog.h"

#include <QDateTime>
#include <QPushButton>

FlashProgressDialog::FlashProgressDialog(bool recoveryFlash, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FlashProgressDialog)
    , m_recoveryFlash(recoveryFlash)
{
    ui->setupUi(this);
    /* Application-modal + no close button. The state machine cannot be
     * orphaned -- the only way out of the dialog is the Cancel button
     * (which only fires in cancel-safe stages) or Close (only available
     * in Done / TerminalError). */
    setModal(true);
    setWindowFlags((windowFlags() & ~Qt::WindowCloseButtonHint
                                  & ~Qt::WindowContextHelpButtonHint)
                   | Qt::CustomizeWindowHint
                   | Qt::WindowTitleHint);

    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, &FlashProgressDialog::onCancelOrCloseClicked);

    refreshStageLabels();
    refreshProgressBar();
}

FlashProgressDialog::~FlashProgressDialog()
{
    delete ui;
}

void FlashProgressDialog::setStage(Stage s, const QString &detail)
{
    m_stage = s;
    if (!detail.isEmpty()) {
        appendStatus(detail);
    } else {
        appendStatus(stageLabel(s));
    }
    refreshStageLabels();
    refreshProgressBar();
}

void FlashProgressDialog::setFlashBytes(int bytesSent, int bytesTotal)
{
    m_bytesSent = bytesSent;
    m_bytesTotal = bytesTotal;
    /* Render `23456 / 49152 bytes` only during the Flash stage; other
     * stages don't have a meaningful byte counter. */
    if (m_stage == Stage::Flash && bytesTotal > 0) {
        ui->label_BytesCounter->setText(
            QStringLiteral("%1 / %2 bytes")
                .arg(bytesSent).arg(bytesTotal));
    } else {
        ui->label_BytesCounter->clear();
    }
    refreshProgressBar();
}

void FlashProgressDialog::appendStatus(const QString &line)
{
    if (line.isEmpty()) return;
    const QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->plainTextEdit_Status->appendPlainText(
        QStringLiteral("[%1] %2").arg(ts, line));
}

void FlashProgressDialog::onCancelOrCloseClicked()
{
    if (m_stage == Stage::Done || m_stage == Stage::TerminalError) {
        accept();
        return;
    }
    if (cancelEnabledForStage()) {
        emit cancelRequested();
        appendStatus(tr("Cancel requested. Waiting for the current step to wind down..."));
    }
}

void FlashProgressDialog::refreshStageLabels()
{
    ui->label_Stage->setText(stageLabel(m_stage));

    QPushButton *btn = ui->buttonBox->button(QDialogButtonBox::Cancel);
    if (!btn) return;
    if (m_stage == Stage::Done || m_stage == Stage::TerminalError) {
        btn->setText(tr("Close"));
        btn->setEnabled(true);
    } else {
        btn->setText(tr("Cancel"));
        btn->setEnabled(cancelEnabledForStage());
    }
}

void FlashProgressDialog::refreshProgressBar()
{
    int progress = stageBaseProgress(m_stage, m_recoveryFlash);
    /* Flash stage interpolates over its 80% weight using bytes_sent /
     * bytes_total. The base is the cumulative weight of prior stages
     * (5+5=10% normally; 0% for recovery flash). */
    if (m_stage == Stage::Flash && m_bytesTotal > 0) {
        const int base = m_recoveryFlash ? 0 : 10;
        const int chunk = (int)((qint64(m_bytesSent) * 80) / m_bytesTotal);
        progress = base + chunk;
    }
    /* -1 from stageBaseProgress means "leave the bar alone" (used by
     * TerminalError and RecoveryPrompt -- we don't want a regression to
     * 0% when we pause on a stuck device, or when the flow fails
     * partway). */
    if (progress < 0) return;
    if (progress > 100) progress = 100;
    ui->progressBar_Overall->setValue(progress);
}

bool FlashProgressDialog::cancelEnabledForStage() const
{
    /* Backup is the only cancel-safe stage in normal-flow. Once we've
     * sent the bootloader-run report we're committed -- the device's
     * application is gone and only a full flash can restore it.
     * RecoveryPrompt re-enables Cancel so the user can give up cleanly
     * after a timeout (the cancel emits cancelRequested; MainWindow
     * routes that to FlashSession::abortFromRecovery). */
    return m_stage == Stage::Backup || m_stage == Stage::RecoveryPrompt;
}

QString FlashProgressDialog::stageLabel(Stage s)
{
    switch (s) {
        case Stage::Idle:               return tr("Preparing...");
        case Stage::Backup:             return tr("Step 1 of 5: Saving config backup...");
        case Stage::EnteringBootloader: return tr("Step 2 of 5: Entering bootloader mode...");
        case Stage::Flash:              return tr("Step 3 of 5: Flashing firmware...");
        case Stage::WaitingForReset:    return tr("Step 4 of 5: Waiting for device to restart...");
        case Stage::Restore:            return tr("Step 5 of 5: Restoring config...");
        case Stage::Done:               return tr("Done.");
        case Stage::TerminalError:      return tr("Flash failed.");
        case Stage::RecoveryPrompt:     return tr("Device didn't return — unplug and replug to recover");
    }
    return QString();
}

int FlashProgressDialog::stageBaseProgress(Stage s, bool recoveryFlash)
{
    /* Cumulative weight up to (but not including) the named stage.
     * Total weights: Backup 5 + Bootloader 5 + Flash 80 + Reset 5 +
     * Restore 5 = 100. Recovery skips Backup, Bootloader, and Restore
     * (their weights would otherwise vanish), so we re-normalize:
     * Flash 95 + Reset 5 = 100. */
    if (recoveryFlash) {
        switch (s) {
            case Stage::Idle:
            case Stage::Backup:
            case Stage::EnteringBootloader:
            case Stage::Flash:            return 0;
            case Stage::WaitingForReset:  return 95;
            case Stage::Restore:
            case Stage::Done:             return 100;
            case Stage::TerminalError:    return -1; /* unchanged */
            case Stage::RecoveryPrompt:   return -1; /* unchanged -- pause the bar */
        }
        return 0;
    }
    switch (s) {
        case Stage::Idle:               return 0;
        case Stage::Backup:              return 0;
        case Stage::EnteringBootloader:  return 5;
        case Stage::Flash:               return 10;
        case Stage::WaitingForReset:     return 90;
        case Stage::Restore:             return 95;
        case Stage::Done:                return 100;
        case Stage::TerminalError:       return -1;
        case Stage::RecoveryPrompt:      return -1; /* unchanged -- pause the bar */
    }
    return 0;
}
