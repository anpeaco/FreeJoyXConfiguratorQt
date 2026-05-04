#ifndef FLASHER_H
#define FLASHER_H

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

signals:
    void flashModeClicked(bool is_start_flash);
    void startFlash(bool is_start_flash);
    /* Fires when flashStatus reaches a terminal state (FINISHED or
     * any error variant). MainWindow uses this to start a watchdog
     * timer expecting the device to reconnect within ~15s. If it
     * doesn't, MainWindow surfaces a "Flash may have failed" dialog
     * pointing at the recovery dropdown. */
    void flashTerminated(bool success);

public slots:
    void flasherFound(bool isFound);
    void flashStatus(int status, int percent);
private slots:
    void on_pushButton_FlasherMode_clicked();
    void on_pushButton_FlashFirmware_clicked();
    void on_pushButton_AbortFlash_clicked();
    void on_toolButton_OpenRecoveryDir_clicked();

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
};

#endif // FLASHER_H
