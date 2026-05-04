#ifndef FLASHER_H
#define FLASHER_H

#include <QWidget>

namespace Ui {
class Flasher;
}

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

public slots:
    void flasherFound(bool isFound);
    void flashStatus(int status, int percent);
private slots:
    void on_pushButton_FlasherMode_clicked();
    void on_pushButton_FlashFirmware_clicked();
    void on_toolButton_OpenRecoveryDir_clicked();
    void on_comboBox_FlashSource_aboutToShowPopup();

private:
    Ui::Flasher *ui;

    QByteArray m_fileArray;
    QString m_flashButtonText;
    QString m_enterToFlash_BtnText;
    void flashDone();

    /* Recovery firmwares: scan <appDir>/recovery/ for .bin files and
     * populate the Source dropdown. The first entry is always
     * "Browse for file..." which keeps the original Flash-via-file-
     * picker behaviour. Named entries are recovery binaries the user
     * has dropped into the folder; selecting one and clicking Flash
     * Firmware loads that .bin directly into m_fileArray. */
    QString recoveryDirPath() const;
    void refreshRecoveryList();
};

#endif // FLASHER_H
