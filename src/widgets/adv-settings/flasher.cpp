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
#include <QMenu>
#include <QAction>
#include <QCursor>

#include <QFileInfo>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLocale>
#include <QSettings>
#include <QSignalBlocker>

#include "common_defines.h"
#include "deviceconfig.h"
#include "dialogs/flashconfirmationdialog.h"
#include "firmwareimage.h"
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
const char *kKindBrowsed = "browsed";

/* QSettings keys for persisting browsed firmware paths. The list is
 * MRU-ordered (most-recently-browsed first) and capped at kBrowsedCap
 * entries; entries that no longer exist on disk are filtered out at
 * dropdown-refresh time. */
const char *kSettingsGroup        = "Flasher";
const char *kSettingsBrowsedPaths = "BrowsedFirmwarePaths";
constexpr int kBrowsedCap = 10;

QStringList loadBrowsedPaths()
{
    if (!gEnv.pAppSettings) return {};
    gEnv.pAppSettings->beginGroup(kSettingsGroup);
    const QStringList raw = gEnv.pAppSettings->value(kSettingsBrowsedPaths).toStringList();
    gEnv.pAppSettings->endGroup();
    QStringList alive;
    for (const QString &p : raw) {
        if (!p.isEmpty() && QFile::exists(p)) {
            alive.append(p);
        }
    }
    return alive;
}

void saveBrowsedPaths(const QStringList &paths)
{
    if (!gEnv.pAppSettings) return;
    gEnv.pAppSettings->beginGroup(kSettingsGroup);
    gEnv.pAppSettings->setValue(kSettingsBrowsedPaths, paths);
    gEnv.pAppSettings->endGroup();
}

} // namespace


Flasher::Flasher(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Flasher)
    , m_library(new FirmwareLibrary(this))
{
    ui->setupUi(this);

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

    /* Refresh the binary info card whenever the user changes the source
     * dropdown. Uses the untyped overload because the connect-PMF form
     * is ambiguous for currentIndexChanged in Qt 5.15. */
    connect(ui->comboBox_FlashSource,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int) {
                refreshFirmwareInfoCard(QString());
                /* Enable the Flash button whenever a real source is
                 * selected (data is non-empty). */
                const QVariant d = ui->comboBox_FlashSource->currentData();
                ui->pushButton_FlashConsolidated->setEnabled(d.isValid());
            });

    /* The legacy two-button flow (Enter Flasher Mode + Flash Firmware +
     * Abort Flasher Mode) is subsumed by the single Flash button. Hide
     * the three legacy buttons in place rather than removing them from
     * the .ui XML so the layout doesn't need a re-design. */
    ui->pushButton_FlasherMode->setVisible(false);
    ui->pushButton_FlashFirmware->setVisible(false);
    ui->pushButton_AbortFlash->setVisible(false);
    /* progressBar_Flash is also unused -- FlashProgressDialog renders
     * progress now. Hide it to free vertical space. */
    ui->progressBar_Flash->setVisible(false);
}

Flasher::~Flasher()
{
    delete ui;
}

void Flasher::retranslateUi()
{
    ui->retranslateUi(this);
}

void Flasher::flasherFound(bool isFound)
{
    /* Track flasher-mode state so the confirmation dialog can classify
     * a flash as Recovery vs normal, and so MainWindow's
     * startConsolidatedFlash can dispatch the right session params. */
    m_inFlasherMode = isFound;
    /* Hide the device-info strip once flasher mode ends. Re-shown by
     * onFlasherDeviceInfo when DFU is detected. */
    if (!isFound) {
        ui->frame_FlasherDeviceInfo->setVisible(false);
        ui->label_FlasherDeviceInfo->clear();
    }
    if (isFound) {
        qDebug() << "Flasher found";
    }
}

void Flasher::onFlasherDeviceInfo(const QString &manufacturer,
                                  const QString &product,
                                  const QString &serial,
                                  ushort vid,
                                  ushort pid)
{
    /* Issue #18: hold the bootloader-side serial too so a Recovery
     * flash dialog can show *some* identifier even when the app-mode
     * paramsReport was never received. */
    m_lastDeviceSerial = serial;
    /* Builds a single-line summary the user can scan to confirm the
     * right device is in flasher mode. Format mirrors the device-info
     * card on the main window: VID:PID hex + serial + product +
     * manufacturer. */
    const QString vidStr = QString::number(vid, 16).rightJustified(4, QChar('0')).toUpper();
    const QString pidStr = QString::number(pid, 16).rightJustified(4, QChar('0')).toUpper();

    QStringList parts;
    if (!product.isEmpty())      parts << QStringLiteral("<b>%1</b>").arg(product);
    if (!manufacturer.isEmpty()) parts << manufacturer;
    parts << QStringLiteral("VID:PID <code>%1:%2</code>").arg(vidStr, pidStr);
    if (!serial.isEmpty())       parts << QStringLiteral("SN <code>%1</code>").arg(serial);

    const QString summary = tr("Connected flasher: ") + parts.join(QStringLiteral(" &middot; "));
    ui->label_FlasherDeviceInfo->setText(summary);
    ui->frame_FlasherDeviceInfo->setVisible(true);
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

    /* Browse is a separate action button now (pushButton_BrowseFirmware);
     * the dropdown only lists actual firmware sources. */

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

    /* User-browsed files persisted across runs (QSettings). Listed
     * before Build/Recovery so the most-recently-flashed file is one
     * click away. Stale entries (file deleted, drive unmounted) get
     * filtered out by loadBrowsedPaths(). */
    const QStringList browsed = loadBrowsedPaths();
    for (const QString &browsedPath : browsed) {
        const QFileInfo fi(browsedPath);
        QVariantMap data;
        data[kKeyKind] = kKindBrowsed;
        data[kKeyLocal] = browsedPath;
        ui->comboBox_FlashSource->addItem(
            tr("[Browsed] %1").arg(fi.fileName()), data);
    }

    /* Build-output folder (<exe>/firmware/): fresh dev builds the user
     * has dropped here. Listed before recovery/ so they bubble to the
     * top of local entries. Tagged "[Build]" to distinguish from
     * recovery fallbacks. */
    QDir firmwareDir(m_library->firmwareDir());
    if (!firmwareDir.exists()) {
        firmwareDir.mkpath(".");
    }
    const QStringList builds = firmwareDir.entryList(
        QStringList() << "*.bin", QDir::Files, QDir::Time | QDir::Reversed);
    for (const QString &name : builds) {
        const QString fullPath = firmwareDir.absoluteFilePath(name);
        QVariantMap data;
        data[kKeyKind] = kKindLocal;
        data[kKeyLocal] = fullPath;
        ui->comboBox_FlashSource->addItem(tr("[Build] %1").arg(name), data);
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
    /* The "..." button gives the user a way to access both watched
     * folders + trigger a GitHub refresh in one place. Pop a small
     * QMenu so the action is explicit -- one click on the button
     * shows the menu, one more click chooses an action. */
    QMenu menu(this);

    QAction *openFirmware = menu.addAction(
        tr("Open firmware (build outputs) folder"));
    QAction *openRecovery = menu.addAction(
        tr("Open recovery (fallback builds) folder"));
    menu.addSeparator();
    QAction *refresh = menu.addAction(
        tr("Refresh from GitHub"));
    /* "Clear browsed list" is offered only when there's something to
     * clear -- saves the user from a confusing no-op. */
    QAction *clearBrowsed = nullptr;
    if (!loadBrowsedPaths().isEmpty()) {
        clearBrowsed = menu.addAction(tr("Clear [Browsed] firmware history"));
    }

    QAction *picked = menu.exec(QCursor::pos());
    if (!picked) return;

    if (picked == openFirmware) {
        QDir d(m_library->firmwareDir());
        if (!d.exists()) d.mkpath(".");
        QDesktopServices::openUrl(QUrl::fromLocalFile(d.absolutePath()));
    } else if (picked == openRecovery) {
        QDir d(m_library->recoveryDir());
        if (!d.exists()) d.mkpath(".");
        QDesktopServices::openUrl(QUrl::fromLocalFile(d.absolutePath()));
    } else if (clearBrowsed != nullptr && picked == clearBrowsed) {
        saveBrowsedPaths({});
        refreshSourceList();
        return;     /* fall-through to refreshSourceList below would also work,
                     * but skip the redundant rebuild */
    } else if (picked == refresh) {
        m_library->fetchReleases();
    }
    /* Refresh local list either way -- if the user opens a folder
     * they're probably about to drop a file in, and the next dropdown
     * open should reflect it. (For network-fetch the list rebuilds via
     * onReleasesUpdated when fetchReleases finishes.) */
    refreshSourceList();
}

void Flasher::on_pushButton_BrowseFirmware_clicked()
{
    /* Default the file picker to firmware/ if it has any .bin files
     * (most likely place for fresh dev builds), then recovery/, then
     * applicationDirPath() (next to the exe). Filter is permissive:
     * .bin first, then All files for builds named differently. */
    QString defaultPath = QCoreApplication::applicationDirPath();
    QDir firmwareDir(m_library->firmwareDir());
    QDir recoveryDir(m_library->recoveryDir());
    if (firmwareDir.exists() &&
        !firmwareDir.entryList(QStringList() << "*.bin", QDir::Files).isEmpty()) {
        defaultPath = firmwareDir.absolutePath();
    } else if (recoveryDir.exists() &&
               !recoveryDir.entryList(QStringList() << "*.bin", QDir::Files).isEmpty()) {
        defaultPath = recoveryDir.absolutePath();
    }

    const QString filePath = QFileDialog::getOpenFileName(
        this, tr("Choose firmware binary"),
        defaultPath,
        tr("Firmware (*.bin);;All files (*)"));
    if (filePath.isEmpty()) {
        qDebug() << "User cancelled file picker";
        return;
    }

    /* Persist into the [Browsed] section of the Source dropdown so the
     * user can flash this file again without re-browsing. MRU-ordered;
     * deduped; capped at kBrowsedCap entries. */
    QStringList browsed = loadBrowsedPaths();
    browsed.removeAll(filePath);            // dedupe
    browsed.prepend(filePath);              // newest first
    while (browsed.size() > kBrowsedCap) {
        browsed.removeLast();
    }
    saveBrowsedPaths(browsed);
    refreshSourceList();
    /* Select the just-added [Browsed] entry in the dropdown so the user
     * has visible confirmation of what they're about to flash. */
    for (int i = 0; i < ui->comboBox_FlashSource->count(); ++i) {
        const QVariantMap d = ui->comboBox_FlashSource->itemData(i).toMap();
        if (d.value(kKeyKind).toString() == kKindBrowsed &&
            d.value(kKeyLocal).toString() == filePath) {
            ui->comboBox_FlashSource->setCurrentIndex(i);
            break;
        }
    }

    requestConsolidatedFlash(filePath);
}

void Flasher::on_pushButton_FlashConsolidated_clicked()
{
    const QVariant currentData = ui->comboBox_FlashSource->currentData();
    if (!currentData.isValid()) {
        QMessageBox::information(this, tr("Pick a firmware source"),
            tr("Select a firmware build from the Source dropdown, or "
               "click <b>Browse...</b> to pick a .bin from disk."));
        return;
    }

    /* Resolve the dropdown entry to a local path. Three kinds:
     *   - String:  legacy local-path entry.
     *   - Map with kKindLocal: build / recovery folder entry.
     *   - Map with kKindRemote: GitHub asset, may or may not be cached. */
    QString localPath;
    bool isRemote = false;
    FirmwareLibrary::Release rel;
    FirmwareLibrary::Asset asset;
    if (currentData.type() == QVariant::String) {
        localPath = currentData.toString();
    } else if (currentData.type() == QVariant::Map) {
        const QVariantMap data = currentData.toMap();
        localPath = data.value(kKeyLocal).toString();
        if (data.value(kKeyKind).toString() == kKindRemote) {
            isRemote = true;
            rel.repo = data.value(kKeyRepo).toString();
            rel.tag  = data.value(kKeyTag).toString();
            asset.name = data.value(kKeyName).toString();
            asset.downloadUrl = data.value(kKeyUrl).toString();
        }
    }

    /* Already on disk -> straight to confirmation + flash. */
    if (!localPath.isEmpty() && QFile::exists(localPath)) {
        requestConsolidatedFlash(localPath);
        return;
    }

    /* Remote-not-cached -> ask, then download, then flash on the
     * onAssetDownloaded callback. */
    if (isRemote && !asset.downloadUrl.isEmpty()) {
        const QMessageBox::StandardButton rc = QMessageBox::question(this,
            tr("Download and flash this firmware?"),
            tr("<p>The selected firmware isn't downloaded yet.</p>"
               "<p>Source: <b>%1</b><br>"
               "Tag: <b>%2</b><br>"
               "Asset: <b>%3</b></p>"
               "<p>Continue?</p>").arg(rel.repo, rel.tag, asset.name),
            QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (rc != QMessageBox::Yes) {
            qDebug() << "User cancelled remote-asset download";
            return;
        }
        ui->pushButton_FlashConsolidated->setEnabled(false);
        ui->pushButton_FlashConsolidated->setText(tr("Downloading..."));
        m_pendingDownloadPath = m_library->downloadAsset(rel, asset);
        return;
    }

    QMessageBox::warning(this, tr("Firmware unavailable"),
        tr("The selected firmware source couldn't be resolved to a file. "
           "Try refreshing the source list or pick a different entry."));
}

void Flasher::onAssetDownloaded(const QString &localPath, bool success)
{
    /* Stray downloads (user-initiated refresh on a different entry, or a
     * download that overlapped a previous click) -- just refresh the
     * dropdown so the cached state surfaces. */
    if (m_pendingDownloadPath.isEmpty() || localPath != m_pendingDownloadPath) {
        refreshSourceList();
        return;
    }
    m_pendingDownloadPath.clear();
    ui->pushButton_FlashConsolidated->setText(tr("Flash"));
    ui->pushButton_FlashConsolidated->setEnabled(true);

    if (!success) {
        QMessageBox::warning(this, tr("Download failed"),
            tr("Couldn't download the selected firmware from GitHub. "
               "Check your internet connection and try again."));
        return;
    }
    refreshSourceList();   /* dropdown now shows the entry as cached */
    requestConsolidatedFlash(localPath);
}

void Flasher::requestConsolidatedFlash(const QString &filePath)
{
    /* Open the FlashConfirmationDialog so the user sees board + version
     * + verdict before committing. The dialog classifies the flash as
     * Recovery vs normal based on m_inFlasherMode, so its warnings match
     * the device's actual state. */
    FirmwareImage image;
    if (!image.loadFromFile(filePath)) {
        QMessageBox::warning(this, tr("Couldn't open firmware"),
            tr("Couldn't read the firmware file:\n%1\n\n"
               "Check that the file exists and the configurator has "
               "permission to read it.").arg(filePath));
        return;
    }
    FlashConfirmationDialog::Inputs in;
    if (gEnv.pDeviceConfig) {
        in.deviceFwVersion = gEnv.pDeviceConfig->paramsReport.firmware_version;
        in.deviceBoardId = gEnv.pDeviceConfig->paramsReport.board_id;
    }
    in.deviceInRecoveryMode = m_inFlasherMode;
    in.deviceName = m_lastDeviceName;
    in.deviceSerial = m_lastDeviceSerial;
    in.image = &image;
    FlashConfirmationDialog dlg(in, this);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    /* MainWindow's onConsolidatedFlashRequested detects the device's
     * current mode (app vs BL) and dispatches FlashSession with the
     * right (runBackup, triggerBootloader, autoRestore) parameters. */
    emit consolidatedFlashRequested(filePath);
}

/* --- Device sidebar (issue anpeaco/FreeJoyXConfiguratorQt#17) --------- */

void Flasher::setDeviceList(const QList<QPair<bool, QString>> &deviceNames, int preferredIndex)
{
    /* Mirror MainWindow's existing combobox population. The first
     * element of each pair flags an "old-firmware flash-only" device
     * (we surface it as RECOVERY rather than the previous "ONLY FLASH"
     * prefix); the second element is the human-readable name. */
    QSignalBlocker bl(ui->listWidget_Devices);
    ui->listWidget_Devices->clear();
    for (int i = 0; i < deviceNames.size(); ++i) {
        QString label = deviceNames[i].second;
        if (deviceNames[i].first) {
            label = tr("[RECOVERY] %1").arg(label);
        }
        ui->listWidget_Devices->addItem(label);
    }
    /* Match MainWindow's preferredIndex / placeholder behaviour: a
     * negative preferred means "no selection yet". */
    if (preferredIndex >= 0 && preferredIndex < deviceNames.size()) {
        ui->listWidget_Devices->setCurrentRow(preferredIndex);
    } else {
        ui->listWidget_Devices->setCurrentRow(-1);
    }
}

void Flasher::setCurrentDeviceIndex(int index)
{
    /* External selection update from MainWindow's combobox -- suppress
     * the re-emit so we don't bounce the signal back. */
    if (ui->listWidget_Devices->currentRow() == index) {
        return;
    }
    m_suppressDeviceSelectionEmit = true;
    ui->listWidget_Devices->setCurrentRow(index);
    m_suppressDeviceSelectionEmit = false;
}

void Flasher::on_listWidget_Devices_currentRowChanged(int row)
{
    /* Capture the row's display string so the confirmation dialog
     * (#18) has a device-name to render even when the user hasn't
     * changed selection in this session. */
    if (row >= 0 && row < ui->listWidget_Devices->count()) {
        m_lastDeviceName = ui->listWidget_Devices->item(row)->text();
    } else {
        m_lastDeviceName.clear();
    }
    if (m_suppressDeviceSelectionEmit) {
        return;
    }
    emit deviceSelectionRequested(row);
}

void Flasher::on_listWidget_Devices_itemActivated(QListWidgetItem *item)
{
    /* Double-click / enter on a row -- treat the same as currentRow
     * change. Belt-and-suspenders so re-activating the already-selected
     * row still re-asserts selection on MainWindow (useful if some
     * other state has drifted). */
    if (!item) return;
    if (m_suppressDeviceSelectionEmit) return;
    emit deviceSelectionRequested(ui->listWidget_Devices->row(item));
}

/* --- Firmware info card (issue anpeaco/FreeJoyXConfiguratorQt#17) ----- */

void Flasher::refreshFirmwareInfoCard(const QString &filePath)
{
    /* Resolve which file to introspect:
     *   1. Explicit filePath argument (e.g. after Browse).
     *   2. Source combobox's local-file entry data (string).
     *   3. Source combobox's remote entry's cached local path (in the
     *      QVariantMap).
     *   4. Nothing selected -- reset the card to placeholders.
     *
     * Slice 4 limits the card to filename + size + raw filename-derived
     * board guess. Full parsed metadata (board + version from
     * FirmwareImage's footer / heuristic) lands in slice 5 (#18). */
    QString resolved = filePath;
    if (resolved.isEmpty()) {
        const QVariant data = ui->comboBox_FlashSource->currentData();
        if (data.type() == QVariant::String) {
            resolved = data.toString();
        } else if (data.type() == QVariant::Map) {
            resolved = data.toMap().value(kKeyLocal).toString();
        }
    }

    if (resolved.isEmpty() || !QFileInfo::exists(resolved)) {
        const QString dash = QStringLiteral("-");
        ui->label_FwInfoFile->setText(dash);
        ui->label_FwInfoBoard->setText(dash);
        ui->label_FwInfoVersion->setText(dash);
        ui->label_FwInfoSize->setText(dash);
        ui->label_FwInfoVerdict->clear();
        return;
    }

    const QFileInfo fi(resolved);
    ui->label_FwInfoFile->setText(fi.fileName());
    ui->label_FwInfoSize->setText(QLocale::system().formattedDataSize(fi.size()));

    /* Board + version straight from the binary. FreeJoyX bins carry an
     * embedded ID footer (image_id.c, FreeJoyX#23); footer-less / foreign
     * bins fall back to FirmwareImage's vector-table heuristic, which still
     * tells F103 from F411 by the reset-handler address. Replaces the old
     * filename-substring guess that left everything "Unknown" / "-". */
    freejoy_style::clearRole(ui->label_FwInfoVerdict, "role");
    ui->label_FwInfoVerdict->clear();

    FirmwareImage image;
    if (!image.loadFromFile(resolved)) {
        ui->label_FwInfoBoard->setText(tr("Unknown"));
        ui->label_FwInfoVersion->setText(QStringLiteral("-"));
        return;
    }

    /* Annotate the board with how it was identified so the user can gauge
     * confidence: a footer read is authoritative; the heuristic is a best
     * guess from the vector table on a binary with no footer. */
    QString boardText = image.boardName();
    if (image.board() != FirmwareImage::Board::Unknown
        && image.source() == FirmwareImage::Source::VectorTableHeuristic) {
        boardText += QChar(' ') + tr("(detected)");
    }
    ui->label_FwInfoBoard->setText(boardText);

    /* Version from the footer; legacy / foreign binaries carry none. */
    const QString ver = image.versionLabel();
    ui->label_FwInfoVersion->setText(ver.isEmpty() ? tr("Legacy binary") : ver);

    /* Surface a board mismatch against the connected device right here so a
     * wrong-board pick is obvious at selection time. The FlashConfirmationDialog
     * still hard-refuses a mismatch before any bytes are written -- this is an
     * earlier, softer heads-up. */
    if (gEnv.pDeviceConfig && image.board() != FirmwareImage::Board::Unknown) {
        const uint8_t devBoard = gEnv.pDeviceConfig->paramsReport.board_id;
        const uint8_t imgBoard =
            (image.board() == FirmwareImage::Board::F103BluePill)
                ? uint8_t(BOARD_ID_F103_BLUEPILL) : uint8_t(BOARD_ID_F411_BLACKPILL);
        if (devBoard != 0 && devBoard != imgBoard) {
            ui->label_FwInfoVerdict->setText(
                tr("This firmware is for a different board than the "
                   "connected device."));
            freejoy_style::setRole(ui->label_FwInfoVerdict, "role", "status-warning");
        }
    }
}
