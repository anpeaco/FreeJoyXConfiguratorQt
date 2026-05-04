#include "flasher.h"
#include "ui_flasher.h"

#include <QFileDialog>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>

#include "deviceconfig.h"
#include "global.h"
#include "style_helpers.h"

#include <QDebug>

Flasher::Flasher(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Flasher)
{
    ui->setupUi(this);
    m_enterToFlash_BtnText = ui->pushButton_FlasherMode->text();
    m_flashButtonText = ui->pushButton_FlashFirmware->text();

    /* Populate the Source dropdown on construction; refresh whenever the
     * user opens the popup so newly-added binaries appear without a
     * restart. The first entry is always "Browse for file..." (the
     * original behaviour). */
    refreshRecoveryList();

    /* Auto-rescan on dropdown click. Qt doesn't have an aboutToShowPopup
     * signal on QComboBox by default, so install an event filter or hook
     * via the abstract item view. The simplest is to wire the same
     * refresh to the toolButton's click + on widget show. The dropdown
     * stays in sync with disk if the user uses the "..." button to open
     * the folder + drop in new files. */
}

Flasher::~Flasher()
{
    delete ui;
}

void Flasher::retranslateUi()
{
    ui->retranslateUi(this);
}

void Flasher::deviceConnected(bool isConnect)
{
    if (isConnect == true) { // disable enter to flash button if flasher connected
        ui->pushButton_FlasherMode->setEnabled(true);
    } else {
        ui->pushButton_FlasherMode->setEnabled(false);
    }
}

void Flasher::flasherFound(bool isFound)
{
    ui->pushButton_FlashFirmware->setEnabled(isFound);
    // disable enter to flash button if flasher connected
    ui->pushButton_FlasherMode->setEnabled(!isFound);
    if (isFound == true) {
        qDebug()<< "Flasher found";
        freejoy_style::setRole(ui->pushButton_FlasherMode, "feedback", "success");
        ui->pushButton_FlasherMode->setText(tr("Ready to flash"));
    } else {
        freejoy_style::clearRole(ui->pushButton_FlasherMode, "feedback");
        ui->pushButton_FlasherMode->setText(m_enterToFlash_BtnText);
    }
}

void Flasher::on_pushButton_FlasherMode_clicked()
{
    emit flashModeClicked(false);
}

QString Flasher::recoveryDirPath() const
{
    /* Recovery folder lives next to the configurator executable. Users
     * drop known-good .bin files there; the dropdown picks them up.
     * Created on first scan if missing so the "..." button always opens
     * something. */
    return QCoreApplication::applicationDirPath() + "/recovery";
}

void Flasher::refreshRecoveryList()
{
    const QString sourcePath = ui->comboBox_FlashSource->currentData().toString();

    QSignalBlocker bl(ui->comboBox_FlashSource);
    ui->comboBox_FlashSource->clear();
    /* First entry is always Browse -- carries an empty path; the click
     * handler interprets empty as "open file picker". */
    ui->comboBox_FlashSource->addItem(tr("Browse for file..."), QString());

    QDir recoveryDir(recoveryDirPath());
    if (!recoveryDir.exists()) {
        recoveryDir.mkpath(".");
    }

    const QStringList bins = recoveryDir.entryList(
        QStringList() << "*.bin", QDir::Files, QDir::Name);
    for (const QString &name : bins) {
        const QString fullPath = recoveryDir.absoluteFilePath(name);
        /* Display the bare filename (e.g. "upstream-v1.7.1b3.bin");
         * stash the full path as user data so the click handler can
         * open it directly without re-resolving. */
        ui->comboBox_FlashSource->addItem(name, fullPath);
    }

    /* Restore selection if the previously-selected file still exists. */
    if (!sourcePath.isEmpty()) {
        const int idx = ui->comboBox_FlashSource->findData(sourcePath);
        if (idx >= 0) {
            ui->comboBox_FlashSource->setCurrentIndex(idx);
        }
    }
}

void Flasher::on_toolButton_OpenRecoveryDir_clicked()
{
    /* Make sure the folder exists, then open it in the OS file manager.
     * Refresh the dropdown when the user comes back so any binaries
     * they dropped in are visible immediately. */
    QDir recoveryDir(recoveryDirPath());
    if (!recoveryDir.exists()) {
        recoveryDir.mkpath(".");
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(recoveryDir.absolutePath()));
    refreshRecoveryList();
}

void Flasher::on_comboBox_FlashSource_aboutToShowPopup()
{
    /* Hook left for future use -- Qt doesn't fire this signal natively
     * on QComboBox. Currently the dropdown rescans only when the
     * "..." button is clicked. */
    refreshRecoveryList();
}

void Flasher::on_pushButton_FlashFirmware_clicked()
{
    const QString sourcePath = ui->comboBox_FlashSource->currentData().toString();

    QString filePath;
    if (sourcePath.isEmpty()) {
        /* "Browse for file..." selected -- original behaviour. */
        filePath = QFileDialog::getOpenFileName(
            this, tr("Open firmware binary"),
            recoveryDirPath() + "/",
            tr("Binary files (.bin) (*.bin)"));
        if (filePath.isEmpty()) {
            qDebug() << "User cancelled file picker";
            return;
        }
    } else {
        /* Recovery firmware selected -- confirm before flashing because
         * recovery means "rolling back to a known-good build" and the
         * user shouldn't trip into it accidentally. */
        const QString fileName = ui->comboBox_FlashSource->currentText();
        const QMessageBox::StandardButton rc = QMessageBox::question(this,
            tr("Flash recovery firmware?"),
            tr("<p>Flash recovery firmware:</p>"
               "<p><b>%1</b></p>"
               "<p>The device's current firmware will be erased and "
               "replaced. Make sure you have a backup of your config "
               "(auto-backup runs on Enter Flasher Mode -- check "
               "&lt;configs&gt;/backups/).</p>"
               "<p>Continue?</p>").arg(fileName),
            QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (rc != QMessageBox::Yes) {
            qDebug() << "User cancelled recovery flash";
            return;
        }
        filePath = sourcePath;
    }

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        ui->pushButton_FlashFirmware->setEnabled(false);
        m_fileArray = file.readAll();
        qDebug() << "file array size =" << m_fileArray.size()
                 << "from" << filePath;
        emit startFlash(true);
    } else {
        qWarning() << "Couldn't open firmware file:" << filePath;
        QMessageBox::warning(this, tr("Couldn't open firmware"),
            tr("Couldn't read the firmware file:\n%1\n\n"
               "Check that the file exists and the configurator has "
               "permission to read it.").arg(filePath));
    }
}

const QByteArray *Flasher::fileArray() const
{
    return &m_fileArray;
}

void Flasher::flashStatus(int status, int percent)
{
    ui->progressBar_Flash->setValue(percent);

    if (percent == 1) {
        ui->pushButton_FlasherMode->setText(tr("Firmware flashing.."));
    }

    if (status == FINISHED) {
        ui->pushButton_FlashFirmware->setText(tr("Finished"));
        freejoy_style::setRole(ui->pushButton_FlashFirmware, "feedback", "success");
        flashDone();
    } else if (status == SIZE_ERROR) {
        ui->pushButton_FlashFirmware->setText(tr("SIZE ERROR"));
        freejoy_style::setRole(ui->pushButton_FlashFirmware, "feedback", "error");
        flashDone();
    } else if (status == CRC_ERROR) {
        ui->pushButton_FlashFirmware->setText(tr("CRC ERROR"));
        freejoy_style::setRole(ui->pushButton_FlashFirmware, "feedback", "error");
        flashDone();
    } else if (status == ERASE_ERROR) {
        ui->pushButton_FlashFirmware->setText(tr("ERASE ERROR"));
        freejoy_style::setRole(ui->pushButton_FlashFirmware, "feedback", "warning");
        flashDone();
    } else if (status == 666) {
        ui->pushButton_FlashFirmware->setText(tr("ERROR"));
        freejoy_style::setRole(ui->pushButton_FlashFirmware, "feedback", "error");
        flashDone();
    }
}

void Flasher::flashDone()
{
    QTimer::singleShot(1000, [&] {
        freejoy_style::clearRole(ui->pushButton_FlashFirmware, "feedback");
        ui->pushButton_FlashFirmware->setEnabled(false);
        ui->pushButton_FlashFirmware->setText(m_flashButtonText);
        ui->pushButton_FlasherMode->setEnabled(true);
        freejoy_style::clearRole(ui->pushButton_FlasherMode, "feedback");
        ui->pushButton_FlasherMode->setText(m_enterToFlash_BtnText);
        m_fileArray.clear();
        m_fileArray.shrink_to_fit();
    });
}
