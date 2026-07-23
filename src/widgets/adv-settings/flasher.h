#ifndef FLASHER_H
#define FLASHER_H

#include <QList>
#include <QPair>
#include <QString>
#include <QVariant>
#include <QVector>
#include <QWidget>

namespace Ui {
class Flasher;
}

class FirmwareLibrary;
class FlashConfirmationDialog;

class Flasher : public QWidget
{
    Q_OBJECT

public:
    explicit Flasher(QWidget *parent = nullptr);
    ~Flasher();

    void retranslateUi();

    /* True once HidDevice::flasherFound(true) has fired and stays true
     * until flasherFound(false). MainWindow consults this from
     * startConsolidatedFlash to dispatch a recovery flash (runBackup=false,
     * triggerBootloader=false) instead of a normal flash when the device
     * is already in bootloader mode. */
    bool isInFlasherMode() const { return m_inFlasherMode; }

    /* Open the firmware picker + Upgrade Firmware dialog for the connected
     * device. `preferredPath` (e.g. the bundled upgrade firmware) is
     * pre-selected if it matches a source. The device identity triple
     * (name/serial/versionText) is supplied by the caller so the dialog's
     * Device pane matches the main device card exactly; when left blank the
     * dialog falls back to the flasher-mode values it scraped from HID
     * enumeration (recovery path). Called by the device card's "Upgrade
     * Firmware" button (MainWindow) and the now-hidden Flasher tab button. */
    void openFlashDialog(const QString &preferredPath = QString(),
                         const QString &deviceName = QString(),
                         const QString &deviceSerial = QString(),
                         const QString &deviceVersionText = QString());

    /* Highest FreeJoyX release version applicable to `boardId` known to the
     * firmware library (network + cache). Returns true and fills the semver
     * when a release applies; false otherwise. MainWindow uses this so the
     * device-card Upgrade button reflects actually-released firmware rather
     * than the configurator's own build version. Thin delegate to
     * FirmwareLibrary::newestApplicableFirmware(). */
    bool newestFirmwareForBoard(int boardId, int &outMajor, int &outMinor,
                                int &outPatch) const;

signals:
    /* The firmware library finished a fetch (or reloaded its cache) and the
     * known release set may have changed. MainWindow re-evaluates the Upgrade
     * button so a freshly-released firmware lights it up without needing a
     * device reconnect. */
    void firmwareLibraryUpdated();

    /* Consolidated one-click flash. Emitted by the Flash button after the
     * user accepts the FlashConfirmationDialog. Carries the resolved
     * local file path of the firmware binary. MainWindow opens the
     * FlashProgressDialog and starts a FlashSession in response. */
    void consolidatedFlashRequested(const QString &filePath);

    /* Forwarded from the DfuInstallDialog's "reboot to DFU" button. MainWindow
     * obliges by sending the "system dfu" report so a live device reboots into
     * ROM USB DFU (jumper-free reinstall, anpeaco/FreeJoyX#55). */
    void systemDfuRebootRequested();

    /* Sidebar row click. Asks MainWindow to set the toolbar's
     * comboBox_HidDeviceList index -- single-source-of-truth stays on
     * the main combobox; the sidebar is just a more visible re-render
     * of the same selection. */
    void deviceSelectionRequested(int index);

public slots:
    void flasherFound(bool isFound);

    /* Sidebar feeds. setDeviceList mirrors the toolbar combobox contents
     * and lights up the same row preferredIndex selects there;
     * setCurrentDeviceIndex pushes selection back when the user changes
     * it from the toolbar combobox. */
    void setDeviceList(const QList<QPair<bool, QString>> &deviceNames, int preferredIndex);
    void setCurrentDeviceIndex(int index);

    /* Driven by HidDevice::flasherDeviceInfo. Populates the "Connected
     * flasher" info line in the Flasher tab so the user can confirm the
     * right device is in flasher mode before committing to a flash. */
    void onFlasherDeviceInfo(const QString &manufacturer,
                             const QString &product,
                             const QString &serial,
                             ushort vid,
                             ushort pid);

    /* The currently-connected app-mode device, pushed by MainWindow on
     * connect/disconnect. Cached so the DFU install dialog can offer its
     * one-click "reboot into DFU" shortcut only when an F411 is present.
     * `name` / `vidPid` are blank when nothing is connected. */
    void setConnectedDeviceInfo(bool isF411, const QString &name, const QString &vidPid);

private slots:
    void on_pushButton_BrowseFirmware_clicked();
    void on_pushButton_FlashConsolidated_clicked();
    /* Opens the modal DfuInstallDialog -- the USB-DFU install/reinstall path
     * (anpeaco/FreeJoyXConfiguratorQt#53). Independent of the HID flash flow
     * above; works on a blank/bricked chip with no connected app device. */
    void on_pushButton_DfuInstall_clicked();
    void on_toolButton_OpenRecoveryDir_clicked();
    void on_listWidget_Devices_itemActivated(class QListWidgetItem *item);
    void on_listWidget_Devices_currentRowChanged(int row);

    void onReleasesUpdated();
    void onAssetDownloaded(const QString &localPath, bool success);

private:
    /* Tier 2 -- the firmware picker lives in the FlashConfirmationDialog now.
     * on_pushButton_FlashConsolidated_clicked() opens it; these drive it:
     *  - buildSourceItems()      : the (label, data) list for the dialog combo
     *                              (same content the old inline dropdown built).
     *  - onDialogSourceSelected(): resolve a pick to a local file (downloading a
     *                              remote asset if needed) and push it back.
     *  - onDialogBrowseRequested : file-pick + add to MRU + re-populate sources.
     *  - showResolvedInDialog()  : load the .bin and call setResolvedTarget. */
    QVector<QPair<QString, QVariant>> buildSourceItems();
    void onDialogSourceSelected(const QVariant &data);
    void onDialogBrowseRequested();
    void showResolvedInDialog(const QString &localPath);

    /* The open FlashConfirmationDialog (valid only during its exec()), and the
     * file path currently resolved into it -- emitted on accept. */
    FlashConfirmationDialog *m_flashDlg = nullptr;
    QString m_dlgResolvedPath;

    /* Path of the asset we kicked off a download for (set when a not-yet-cached
     * remote entry is selected in the dialog). Cleared by onAssetDownloaded.
     * Non-empty -> a download is in flight. */
    QString m_pendingDownloadPath;

    Ui::Flasher *ui;
    FirmwareLibrary *m_library;

    /* Source dropdown holds three kinds of entries:
     *   - "Browse for file..."  (data = empty QVariant)
     *   - Local .bin file       (data = absolute path string)
     *   - Remote release asset  (data = QVariantMap with repo/tag/url/localPath)
     * refreshSourceList() rebuilds the contents from local disk + the
     * FirmwareLibrary's last-known release list. */
    void refreshSourceList();

    /* Guards against the setCurrentRow re-entrancy that would otherwise
     * turn an external selection update (driven by setCurrentDeviceIndex)
     * into another deviceSelectionRequested emission and ping-pong with
     * MainWindow. */
    bool m_suppressDeviceSelectionEmit = false;

    /* State captured from existing signals so the FlashConfirmationDialog
     * can be populated without threading extra arguments through every
     * code path.
     *  - m_inFlasherMode    : true once flasherFound(true) has fired
     *                         and stays true until flasherFound(false).
     *  - m_lastDeviceName   : the display string of the currently-
     *                         highlighted sidebar row (matches the
     *                         toolbar combobox). Pre-flash identity.
     *  - m_lastDeviceSerial : bootloader-side serial captured from
     *                         HidDevice::flasherDeviceInfo, so the
     *                         confirmation dialog has something to show
     *                         even when the app-mode params never arrived
     *                         (recovery flash). */
    bool m_inFlasherMode = false;
    QString m_lastDeviceName;
    QString m_lastDeviceSerial;

    /* Connected app-mode device summary (setConnectedDeviceInfo), consumed
     * when opening the DFU install dialog. */
    bool m_connectedIsF411 = false;
    QString m_connectedName;
    QString m_connectedVidPid;

    /* Refresh the per-row "Selected firmware" info card from the picked
     * file path. */
    void refreshFirmwareInfoCard(const QString &filePath);
};

#endif // FLASHER_H
