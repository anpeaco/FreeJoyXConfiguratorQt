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

    void deviceConnected(bool isConnect);

    const QByteArray *fileArray() const;

    /* Public entry point for the one-click Upgrade Firmware flow on
     * the main view. Loads the .bin into m_fileArray and emits
     * startFlash(true) so MainWindow's existing pipeline picks up.
     * Same semantics as the private startFlashFromFile -- exposed so
     * callers outside the Flasher tab can reuse the flash path. */
    void triggerFlashFromPath(const QString &filePath) {
        startFlashFromFile(filePath);
    }

signals:
    void flashModeClicked(bool is_start_flash);
    void startFlash(bool is_start_flash);
    /* Fires when flashStatus reaches a terminal state (FINISHED or
     * any error variant). MainWindow uses this to start a watchdog
     * timer expecting the device to reconnect within ~15s. If it
     * doesn't, MainWindow surfaces a "Flash may have failed" dialog
     * pointing at the recovery dropdown. */
    void flashTerminated(bool success);

    /* Issue anpeaco/FreeJoyXConfiguratorQt#17: sidebar row click. Asks
     * MainWindow to set the toolbar's comboBox_HidDeviceList index --
     * single-source-of-truth stays on the main combobox; the sidebar
     * is just a more visible re-render of the same selection. */
    void deviceSelectionRequested(int index);

public slots:
    void flasherFound(bool isFound);
    void flashStatus(int status, int bytes_sent, int bytes_total);

    /* Sidebar feeds (issue #17). setDeviceList mirrors the toolbar
     * combobox contents and lights up the same row preferredIndex
     * selects there; setCurrentDeviceIndex pushes selection back when
     * the user changes it from the toolbar combobox. */
    void setDeviceList(const QList<QPair<bool, QString>> &deviceNames, int preferredIndex);
    void setCurrentDeviceIndex(int index);
    /* Driven by HidDevice::flasherDeviceInfo. Populates the
     * "Connected flasher" info line in the Flasher tab so the user
     * can confirm the right device is in flasher mode before
     * committing to a flash. */
    void onFlasherDeviceInfo(const QString &manufacturer,
                             const QString &product,
                             const QString &serial,
                             ushort vid,
                             ushort pid);

private slots:
    void on_pushButton_FlasherMode_clicked();
    void on_pushButton_FlashFirmware_clicked();
    void on_pushButton_AbortFlash_clicked();
    void on_pushButton_BrowseFirmware_clicked();
    void on_toolButton_OpenRecoveryDir_clicked();
    void on_listWidget_Devices_itemActivated(class QListWidgetItem *item);
    void on_listWidget_Devices_currentRowChanged(int row);

    void onReleasesUpdated();
    void onAssetDownloaded(const QString &localPath, bool success);

private:
    Ui::Flasher *ui;
    FirmwareLibrary *m_library;

    QByteArray m_fileArray;
    QString m_flashButtonText;
    QString m_enterToFlash_BtnText;
    void flashDone();

    /* Source dropdown holds three kinds of entries:
     *   - "Browse for file..."  (data = empty QVariant)
     *   - Local .bin file       (data = absolute path string)
     *   - Remote release asset  (data = QVariantMap with repo/tag/url/localPath)
     * refreshSourceList() rebuilds the contents from local disk + the
     * FirmwareLibrary's last-known release list. */
    void refreshSourceList();

    /* When the user picks a remote entry and clicks Flash, we trigger
     * an async download via FirmwareLibrary. These hold the pending
     * state so onAssetDownloaded knows to start the flash on success. */
    bool m_flashAfterDownload = false;
    QString m_pendingDownloadPath;
    void startFlashFromFile(const QString &filePath);

    /* Issue anpeaco/FreeJoyXConfiguratorQt#17: guards against the
     * setCurrentRow re-entrancy that would otherwise turn an external
     * selection update (driven by setCurrentDeviceIndex) into another
     * deviceSelectionRequested emission and ping-pong with MainWindow. */
    bool m_suppressDeviceSelectionEmit = false;

    /* Refresh the per-row "Selected firmware" info card from the picked
     * file path. Slice 4 (#17) wires this to filename / size only; the
     * full parsed binary metadata lands in slice 5 (#18) once
     * FirmwareImage is integrated. */
    void refreshFirmwareInfoCard(const QString &filePath);
};

#endif // FLASHER_H
