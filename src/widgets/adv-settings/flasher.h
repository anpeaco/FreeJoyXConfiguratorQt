#ifndef FLASHER_H
#define FLASHER_H

#include <QList>
#include <QPair>
#include <QString>
#include <QWidget>

namespace Ui {
class Flasher;
}

class FirmwareLibrary;

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

signals:
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
    /* Open the confirmation dialog with the given firmware path and, on
     * accept, emit consolidatedFlashRequested. Shared by Browse and the
     * Flash button's local + post-download paths. */
    void requestConsolidatedFlash(const QString &filePath);

    /* Path of the asset we kicked off a download for (set when the user
     * clicks Flash on a not-yet-cached remote entry). Cleared by
     * onAssetDownloaded. Non-empty -> a download is in flight. */
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

    /* Refresh the per-row "Selected firmware" info card from the picked
     * file path. */
    void refreshFirmwareInfoCard(const QString &filePath);
};

#endif // FLASHER_H
