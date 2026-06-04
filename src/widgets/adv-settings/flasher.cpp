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
#include "dialogs/dfuinstalldialog.h"
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

    /* Tier 2: the firmware picker (source dropdown + Browse + info card) now
     * lives inside the FlashConfirmationDialog. Hide the inline widgets and
     * turn the former "Flash" button into the dialog's entry point. The hidden
     * comboBox_FlashSource is still kept populated by refreshSourceList -- it's
     * the cache the dialog reads via buildSourceItems(). */
    ui->label_FlashSource->setVisible(false);
    ui->comboBox_FlashSource->setVisible(false);
    ui->pushButton_BrowseFirmware->setVisible(false);
    ui->groupBox_FirmwareInfo->setVisible(false);
    ui->pushButton_FlashConsolidated->setText(tr("Install / Update Firmware…"));
    ui->pushButton_FlashConsolidated->setEnabled(true);
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

QVector<QPair<QString, QVariant>> Flasher::buildSourceItems()
{
    QVector<QPair<QString, QVariant>> items;

    /* Remote releases first -- newest builds bubble to the top via
     * FirmwareLibrary's sort. Label "[FreeJoyX v0.2.0] FreeJoy.bin" cached,
     * "... (download)" when not yet on disk. */
    const QList<FirmwareLibrary::Release> rels = m_library->releases();
    for (const auto &rel : rels) {
        for (const auto &asset : rel.assets) {
            const QString localPath = m_library->localPathFor(rel, asset);
            const bool cached = QFile::exists(localPath);

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
            data[kKeyKind]  = kKindRemote;
            data[kKeyRepo]  = rel.repo;
            data[kKeyTag]   = rel.tag;
            data[kKeyName]  = asset.name;
            data[kKeyUrl]   = asset.downloadUrl;
            data[kKeyLocal] = localPath;
            items.append({label, data});
        }
    }

    /* User-browsed files persisted across runs (QSettings), MRU-ordered;
     * stale entries filtered out by loadBrowsedPaths(). */
    const QStringList browsed = loadBrowsedPaths();
    for (const QString &browsedPath : browsed) {
        const QFileInfo fi(browsedPath);
        QVariantMap data;
        data[kKeyKind]  = kKindBrowsed;
        data[kKeyLocal] = browsedPath;
        items.append({tr("[Browsed] %1").arg(fi.fileName()), data});
    }

    /* Build-output folder (<exe>/firmware/): fresh dev builds. */
    QDir firmwareDir(m_library->firmwareDir());
    if (!firmwareDir.exists()) {
        firmwareDir.mkpath(".");
    }
    const QStringList builds = firmwareDir.entryList(
        QStringList() << "*.bin", QDir::Files, QDir::Time | QDir::Reversed);
    for (const QString &name : builds) {
        QVariantMap data;
        data[kKeyKind]  = kKindLocal;
        data[kKeyLocal] = firmwareDir.absoluteFilePath(name);
        items.append({tr("[Build] %1").arg(name), data});
    }

    /* Local .bin files in recovery/ that aren't auto-downloaded mirrors. */
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
        QVariantMap data;
        data[kKeyKind]  = kKindLocal;
        data[kKeyLocal] = recoveryDir.absoluteFilePath(name);
        items.append({tr("[Local] %1").arg(name), data});
    }

    return items;
}

void Flasher::refreshSourceList()
{
    /* Keep the now-hidden cache combo in sync (the dialog reads
     * buildSourceItems() directly). Preserve the current selection. */
    const QVariant prevData = ui->comboBox_FlashSource->currentData();
    QSignalBlocker bl(ui->comboBox_FlashSource);
    ui->comboBox_FlashSource->clear();
    for (const auto &it : buildSourceItems()) {
        ui->comboBox_FlashSource->addItem(it.first, it.second);
    }
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

void Flasher::setConnectedDeviceInfo(bool isF411, const QString &name,
                                     const QString &vidPid)
{
    m_connectedIsF411 = isF411;
    m_connectedName   = name;
    m_connectedVidPid = vidPid;
}

void Flasher::on_pushButton_DfuInstall_clicked()
{
    /* USB-DFU install/reinstall is fully self-contained in its own modal
     * dialog: it detects the ROM-DFU device, resolves the boot+app binaries,
     * and drives the freejoyx-flash helper. No HID device or flasher-mode
     * state is required, so this path works even on a blank or bricked chip. */
    DfuInstallDialog dlg(this);
    /* Offer the one-click reboot-to-DFU shortcut only when an F411 is
     * connected (the command does nothing otherwise); the dialog shows the
     * device name + VID:PID. */
    dlg.setConnectedDevice(m_connectedIsF411, m_connectedName, m_connectedVidPid);
    /* The dialog's "reboot connected device to DFU" button asks us to send the
     * firmware command; forward it up to MainWindow which owns the HID worker. */
    connect(&dlg, &DfuInstallDialog::rebootToDfuRequested,
            this, &Flasher::systemDfuRebootRequested);
    dlg.exec();
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
    /* Tier 2: the inline Browse button is hidden -- browsing now happens inside
     * the FlashConfirmationDialog (its Browse... -> onDialogBrowseRequested). */
}

void Flasher::on_pushButton_FlashConsolidated_clicked()
{
    /* Tier 2: this is now the "Install / Update Firmware..." entry point. The
     * FlashConfirmationDialog owns the firmware picker; we drive it (resolve /
     * download / browse) off its signals and flash on accept. */
    QString deviceName   = m_lastDeviceName;
    QString deviceSerial = m_lastDeviceSerial;
    int boardId = 0;
    uint16_t fwVer = 0;
    if (gEnv.pDeviceConfig) {
        fwVer   = gEnv.pDeviceConfig->paramsReport.firmware_version;
        boardId = gEnv.pDeviceConfig->paramsReport.board_id;
    }

    FlashConfirmationDialog dlg(deviceName, deviceSerial, boardId, fwVer,
                                m_inFlasherMode, this);
    m_flashDlg = &dlg;
    m_dlgResolvedPath.clear();
    m_pendingDownloadPath.clear();

    connect(&dlg, &FlashConfirmationDialog::sourceSelected,
            this, &Flasher::onDialogSourceSelected);
    connect(&dlg, &FlashConfirmationDialog::browseRequested,
            this, &Flasher::onDialogBrowseRequested);

    dlg.setSources(buildSourceItems());
    onDialogSourceSelected(dlg.currentSourceData());   /* resolve initial pick */

    const int rc = dlg.exec();
    m_flashDlg = nullptr;
    m_pendingDownloadPath.clear();
    if (rc != QDialog::Accepted || m_dlgResolvedPath.isEmpty()) {
        return;
    }
    emit consolidatedFlashRequested(m_dlgResolvedPath);
}

void Flasher::onAssetDownloaded(const QString &localPath, bool success)
{
    /* Stray download (e.g. a background refresh) -- ignore. */
    if (m_pendingDownloadPath.isEmpty() || localPath != m_pendingDownloadPath) {
        return;
    }
    m_pendingDownloadPath.clear();
    if (!m_flashDlg) {
        return;   /* dialog already closed */
    }
    if (!success) {
        m_dlgResolvedPath.clear();
        m_flashDlg->clearTarget(
            tr("Couldn't download the firmware from GitHub. Check your "
               "connection and try again, or pick another source."));
        return;
    }
    showResolvedInDialog(localPath);
}

void Flasher::onDialogSourceSelected(const QVariant &data)
{
    if (!m_flashDlg) {
        return;
    }
    m_pendingDownloadPath.clear();

    /* Resolve the dropdown entry to a local path. Map with kKindLocal/Browsed:
     * a file on disk. Map with kKindRemote: a GitHub asset, maybe not cached. */
    QString localPath;
    bool isRemote = false;
    FirmwareLibrary::Release rel;
    FirmwareLibrary::Asset asset;
    if (data.type() == QVariant::String) {
        localPath = data.toString();
    } else if (data.type() == QVariant::Map) {
        const QVariantMap m = data.toMap();
        localPath = m.value(kKeyLocal).toString();
        if (m.value(kKeyKind).toString() == kKindRemote) {
            isRemote = true;
            rel.repo = m.value(kKeyRepo).toString();
            rel.tag  = m.value(kKeyTag).toString();
            asset.name = m.value(kKeyName).toString();
            asset.downloadUrl = m.value(kKeyUrl).toString();
        }
    }

    if (!localPath.isEmpty() && QFile::exists(localPath)) {
        showResolvedInDialog(localPath);
        return;
    }
    if (isRemote && !asset.downloadUrl.isEmpty()) {
        m_dlgResolvedPath.clear();
        m_flashDlg->setBusy(tr("Downloading %1…").arg(asset.name));
        m_pendingDownloadPath = m_library->downloadAsset(rel, asset);
        return;
    }
    m_dlgResolvedPath.clear();
    m_flashDlg->clearTarget(
        tr("This firmware source couldn't be resolved. Pick another, or "
           "Browse for a .bin."));
}

void Flasher::onDialogBrowseRequested()
{
    if (!m_flashDlg) {
        return;
    }
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
        m_flashDlg, tr("Choose firmware binary"), defaultPath,
        tr("Firmware (*.bin);;All files (*)"));
    if (filePath.isEmpty()) {
        return;
    }

    /* Persist into the [Browsed] MRU so it's one click away next time. */
    QStringList browsed = loadBrowsedPaths();
    browsed.removeAll(filePath);
    browsed.prepend(filePath);
    while (browsed.size() > kBrowsedCap) {
        browsed.removeLast();
    }
    saveBrowsedPaths(browsed);
    refreshSourceList();

    /* Re-populate the dialog and select the just-browsed file. */
    const QVector<QPair<QString, QVariant>> items = buildSourceItems();
    int sel = 0;
    for (int i = 0; i < items.size(); ++i) {
        const QVariantMap d = items[i].second.toMap();
        if (d.value(kKeyKind).toString() == kKindBrowsed &&
            d.value(kKeyLocal).toString() == filePath) {
            sel = i;
            break;
        }
    }
    m_flashDlg->setSources(items, sel);
    onDialogSourceSelected(m_flashDlg->currentSourceData());
}

void Flasher::showResolvedInDialog(const QString &localPath)
{
    if (!m_flashDlg) {
        return;
    }
    FirmwareImage image;
    if (!image.loadFromFile(localPath)) {
        m_dlgResolvedPath.clear();
        m_flashDlg->clearTarget(
            tr("Couldn't read the firmware file:\n%1").arg(localPath));
        return;
    }
    m_dlgResolvedPath = localPath;
    m_flashDlg->setResolvedTarget(image);
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
