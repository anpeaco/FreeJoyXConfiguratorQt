#include "flasher.h"
#include "ui_flasher.h"

#include <QFileDialog>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QVariantMap>

#include "deviceconfig.h"
#include "global.h"
#include "style_helpers.h"
#include "firmwarelibrary.h"

#include <QDebug>

namespace {

/* QVariantMap keys for remote-release entries in the Source dropdown. */
const char *kKeyKind   = "kind";
const char *kKeyRepo   = "repo";
const char *kKeyTag    = "tag";
const char *kKeyName   = "name";
const char *kKeyUrl    = "url";
const char *kKeyLocal  = "local";

const char *kKindLocal  = "local";
const char *kKindRemote = "remote";

} // namespace


Flasher::Flasher(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Flasher)
    , m_library(new FirmwareLibrary(this))
{
    ui->setupUi(this);
    m_enterToFlash_BtnText = ui->pushButton_FlasherMode->text();
    m_flashButtonText = ui->pushButton_FlashFirmware->text();

    connect(m_library, &FirmwareLibrary::releasesUpdated,
            this, &Flasher::onReleasesUpdated);
    connect(m_library, &FirmwareLibrary::assetDownloaded,
            this, &Flasher::onAssetDownloaded);
    connect(m_library, &FirmwareLibrary::releasesError,
            this, [](const QString &msg) {
                qWarning() << "FirmwareLibrary error:" << msg;
            });

    /* Initial population from local disk + cached release list (loaded
     * by FirmwareLibrary's ctor). Then kick off a background fetch to
     * refresh against GitHub. */
    refreshSourceList();
    m_library->fetchReleases();
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
    if (isConnect == true) {
        ui->pushButton_FlasherMode->setEnabled(true);
    } else {
        ui->pushButton_FlasherMode->setEnabled(false);
    }
}

void Flasher::flasherFound(bool isFound)
{
    ui->pushButton_FlashFirmware->setEnabled(isFound);
    ui->pushButton_FlasherMode->setEnabled(!isFound);
    /* Abort is only relevant while the device is in flasher mode --
     * mirrors the Flash button's enabled state but with opposite
     * intent. */
    ui->pushButton_AbortFlash->setEnabled(isFound);
    if (isFound == true) {
        qDebug() << "Flasher found";
        freejoy_style::setRole(ui->pushButton_FlasherMode, "feedback", "success");
        ui->pushButton_FlasherMode->setText(tr("Ready to flash"));
    } else {
        freejoy_style::clearRole(ui->pushButton_FlasherMode, "feedback");
        ui->pushButton_FlasherMode->setText(m_enterToFlash_BtnText);
    }
}

void Flasher::on_pushButton_AbortFlash_clicked()
{
    /* The bootloader doesn't expose a "stop and jump to app" command
     * over HID -- it only knows the flash protocol. To exit DFU
     * without writing a new firmware, the user has to power-cycle
     * the device. Fortunately the bootloader's magic word
     * (BKP->DR4 = 0x424C on F103, RTC->BKP0R on F411) is cleared on
     * read at boot, so any reset path puts the device back into the
     * existing application:
     *
     *   1. Unplug + replug the USB cable (cleanest, always works)
     *   2. Press the BluePill / BlackPill reset button (NRST)
     *   3. Software reset via SWD / external tool (advanced)
     *
     * No automatic option from the configurator side is reliable --
     * a USB-port-cycle isn't portable across OSes (Windows can't
     * even reliably re-cycle without a hub) and the bootloader
     * isn't going to listen to a "please exit" packet we make up.
     * So: explain the situation and trust the user to do the
     * physical action. */
    QMessageBox::information(this, tr("Abort flasher mode"),
        tr("<p>The bootloader doesn't have a software-abort command, "
           "so getting the device out of flasher mode without "
           "flashing requires a physical reset:</p>"
           "<ul>"
           "<li><b>Easiest:</b> unplug + replug the USB cable.</li>"
           "<li>Or press the device's reset button (NRST on "
           "BluePill / BlackPill) if it has one.</li>"
           "</ul>"
           "<p>The bootloader's flasher-mode magic word is "
           "cleared on read, so any reset path will boot the "
           "existing application normally -- nothing on the device "
           "has been changed by entering flasher mode.</p>"));
}

void Flasher::on_pushButton_FlasherMode_clicked()
{
    emit flashModeClicked(false);
}

void Flasher::onReleasesUpdated()
{
    qDebug() << "Flasher: releases updated, rebuilding source list";
    refreshSourceList();
}

void Flasher::refreshSourceList()
{
    /* Capture the current selection so we can preserve it across the
     * rebuild if the same source is still available. */
    const QVariant prevData = ui->comboBox_FlashSource->currentData();

    QSignalBlocker bl(ui->comboBox_FlashSource);
    ui->comboBox_FlashSource->clear();

    /* Always-present "Browse for file..." entry. */
    ui->comboBox_FlashSource->addItem(tr("Browse for file..."), QVariant());

    /* Remote releases first -- newest builds bubble to the top of the
     * dropdown via FirmwareLibrary's sort. */
    const QList<FirmwareLibrary::Release> rels = m_library->releases();
    for (const auto &rel : rels) {
        for (const auto &asset : rel.assets) {
            const QString localPath = m_library->localPathFor(rel, asset);
            const bool cached = QFile::exists(localPath);

            /* Display label format: "[FreeJoyX v0.2.0] FreeJoy.bin"
             * for cached / "[Upstream v1.7.3b0] FreeJoy.bin (download)"
             * for not-yet-cached. Strip "FreeJoy-Team/" / "anpeaco/" to
             * a friendlier owner tag. */
            QString ownerTag = rel.repo;
            if (ownerTag.startsWith(QStringLiteral("FreeJoy-Team/"))) {
                ownerTag = QStringLiteral("Upstream");
            } else if (ownerTag.startsWith(QStringLiteral("anpeaco/"))) {
                ownerTag = QStringLiteral("FreeJoyX");
            }

            QString label = QStringLiteral("[%1 %2] %3")
                                .arg(ownerTag, rel.tag, asset.name);
            if (!cached) {
                label += tr(" (download)");
            }

            QVariantMap data;
            data[kKeyKind] = kKindRemote;
            data[kKeyRepo] = rel.repo;
            data[kKeyTag] = rel.tag;
            data[kKeyName] = asset.name;
            data[kKeyUrl] = asset.downloadUrl;
            data[kKeyLocal] = localPath;
            ui->comboBox_FlashSource->addItem(label, data);
        }
    }

    /* Local .bin files in recovery/ that aren't auto-downloaded asset
     * mirrors. We skip files matching the auto-download naming scheme
     * (prefix "FreeJoy-Team_" or "anpeaco_") because they show up in
     * the remote section already. */
    QDir recoveryDir(m_library->recoveryDir());
    if (!recoveryDir.exists()) {
        recoveryDir.mkpath(".");
    }
    const QStringList bins = recoveryDir.entryList(
        QStringList() << "*.bin", QDir::Files, QDir::Name);
    for (const QString &name : bins) {
        if (name.startsWith(QStringLiteral("FreeJoy-Team_")) ||
            name.startsWith(QStringLiteral("anpeaco_"))) {
            continue;
        }
        const QString fullPath = recoveryDir.absoluteFilePath(name);

        QVariantMap data;
        data[kKeyKind] = kKindLocal;
        data[kKeyLocal] = fullPath;
        ui->comboBox_FlashSource->addItem(tr("[Local] %1").arg(name), data);
    }

    /* Restore selection if the previously-selected entry still exists.
     * Compare by local path (works for both local and remote-cached
     * entries) or URL (for not-yet-cached remote entries). */
    if (prevData.isValid()) {
        const QVariantMap prev = prevData.toMap();
        const QString prevLocal = prev.value(kKeyLocal).toString();
        const QString prevUrl   = prev.value(kKeyUrl).toString();
        for (int i = 0; i < ui->comboBox_FlashSource->count(); ++i) {
            const QVariantMap d = ui->comboBox_FlashSource->itemData(i).toMap();
            if ((!prevLocal.isEmpty() && d.value(kKeyLocal).toString() == prevLocal) ||
                (!prevUrl.isEmpty()   && d.value(kKeyUrl).toString()   == prevUrl)) {
                ui->comboBox_FlashSource->setCurrentIndex(i);
                break;
            }
        }
    }
}

void Flasher::on_toolButton_OpenRecoveryDir_clicked()
{
    /* The "..." button now does double duty: open the folder AND
     * trigger a refresh of the GitHub release list, since the user
     * is probably about to drop in or notice new files either way. */
    QDir recoveryDir(m_library->recoveryDir());
    if (!recoveryDir.exists()) {
        recoveryDir.mkpath(".");
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(recoveryDir.absolutePath()));
    m_library->fetchReleases();   /* fires onReleasesUpdated when done */
    refreshSourceList();
}

void Flasher::on_pushButton_FlashFirmware_clicked()
{
    const QVariant currentData = ui->comboBox_FlashSource->currentData();

    /* "Browse for file..." -- empty data; original behaviour. */
    if (!currentData.isValid()) {
        const QString filePath = QFileDialog::getOpenFileName(
            this, tr("Open firmware binary"),
            m_library->recoveryDir() + "/",
            tr("Binary files (.bin) (*.bin)"));
        if (filePath.isEmpty()) {
            qDebug() << "User cancelled file picker";
            return;
        }
        startFlashFromFile(filePath);
        return;
    }

    const QVariantMap data = currentData.toMap();
    const QString kind = data.value(kKeyKind).toString();

    if (kind == kKindLocal) {
        startFlashFromFile(data.value(kKeyLocal).toString());
        return;
    }

    if (kind == kKindRemote) {
        /* Confirm before flashing a remote firmware -- the dropdown
         * entry might have been auto-selected as the user clicked
         * around. Recovery flashes shouldn't be one-click. */
        const QString name = data.value(kKeyName).toString();
        const QString tag  = data.value(kKeyTag).toString();
        const QString repo = data.value(kKeyRepo).toString();
        const QString localPath = data.value(kKeyLocal).toString();
        const bool cached = QFile::exists(localPath);

        const QString action = cached ? tr("Flash this firmware?") : tr("Download and flash this firmware?");
        const QMessageBox::StandardButton rc = QMessageBox::question(this,
            action,
            tr("<p>%1</p>"
               "<p>Source: <b>%2</b><br>"
               "Tag: <b>%3</b><br>"
               "Asset: <b>%4</b></p>"
               "<p>The device's current firmware will be erased and "
               "replaced. Make sure you have a backup of your config "
               "(auto-backup runs on Enter Flasher Mode -- check "
               "&lt;configs&gt;/backups/).</p>"
               "<p>Continue?</p>").arg(action, repo, tag, name),
            QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (rc != QMessageBox::Yes) {
            qDebug() << "User cancelled remote flash";
            return;
        }

        if (cached) {
            startFlashFromFile(localPath);
            return;
        }

        /* Download then flash. Disable button + set pending flag so
         * onAssetDownloaded knows to continue. */
        ui->pushButton_FlashFirmware->setEnabled(false);
        ui->pushButton_FlashFirmware->setText(tr("Downloading..."));
        m_flashAfterDownload = true;

        FirmwareLibrary::Release rel;
        rel.repo = repo;
        rel.tag = tag;
        FirmwareLibrary::Asset a;
        a.name = name;
        a.downloadUrl = data.value(kKeyUrl).toString();
        m_pendingDownloadPath = m_library->downloadAsset(rel, a);
        return;
    }

    qWarning() << "Unknown source kind:" << kind;
}

void Flasher::onAssetDownloaded(const QString &localPath, bool success)
{
    if (!m_flashAfterDownload || localPath != m_pendingDownloadPath) {
        /* Stray download (e.g. user kicked off another one in the
         * background); just refresh the dropdown so the cached state
         * surfaces. */
        refreshSourceList();
        return;
    }

    m_flashAfterDownload = false;
    m_pendingDownloadPath.clear();

    if (!success) {
        ui->pushButton_FlashFirmware->setText(m_flashButtonText);
        ui->pushButton_FlashFirmware->setEnabled(true);
        QMessageBox::warning(this, tr("Download failed"),
            tr("Couldn't download the selected firmware from GitHub. "
               "Check your internet connection and try again."));
        return;
    }

    ui->pushButton_FlashFirmware->setText(m_flashButtonText);
    refreshSourceList();   /* dropdown now shows entry as cached */
    startFlashFromFile(localPath);
}

void Flasher::startFlashFromFile(const QString &filePath)
{
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
        emit flashTerminated(true);
    } else if (status == SIZE_ERROR) {
        ui->pushButton_FlashFirmware->setText(tr("SIZE ERROR"));
        freejoy_style::setRole(ui->pushButton_FlashFirmware, "feedback", "error");
        flashDone();
        emit flashTerminated(false);
    } else if (status == CRC_ERROR) {
        ui->pushButton_FlashFirmware->setText(tr("CRC ERROR"));
        freejoy_style::setRole(ui->pushButton_FlashFirmware, "feedback", "error");
        flashDone();
        emit flashTerminated(false);
    } else if (status == ERASE_ERROR) {
        ui->pushButton_FlashFirmware->setText(tr("ERASE ERROR"));
        freejoy_style::setRole(ui->pushButton_FlashFirmware, "feedback", "warning");
        flashDone();
        emit flashTerminated(false);
    } else if (status == 666) {
        ui->pushButton_FlashFirmware->setText(tr("ERROR"));
        freejoy_style::setRole(ui->pushButton_FlashFirmware, "feedback", "error");
        flashDone();
        emit flashTerminated(false);
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
