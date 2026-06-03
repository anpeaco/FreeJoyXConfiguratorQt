/**
  ******************************************************************************
  * @file           : dfuinstalldialog.h
  * @brief          : Modal dialog for installing / reinstalling the F411
  *                   bootloader + app over the STM32 ROM USB DFU (DfuSe)
  *                   bootloader. Issue anpeaco/FreeJoyXConfiguratorQt#53.
  *
  * Built in code (no .ui) -- the layout is small and this keeps the dialog
  * self-contained while the feature is still settling. Owns a
  * DfuInstallSession (the QProcess driver for the `freejoyx-flash` helper)
  * and renders its stage / progress / log stream.
  *
  * The dialog walks the user through three things:
  *   1. Getting the chip into ROM DFU. Software-triggered "reboot to DFU"
  *      (firmware anpeaco/FreeJoyX#55) is the future one-click route; until
  *      that firmware lands the dialog shows the manual BOOT0 + NRST steps.
  *   2. Confirming which bootloader + app .bin are written. Pre-filled from
  *      the bundled per-board binaries next to the executable when present;
  *      otherwise the user browses.
  *   3. Running the DfuSe write and showing progress to completion.
  *
  * Detection (DfuInstallSession::probe) gates the Install button: it stays
  * disabled until a 0483:df11 device is actually present, so the user can't
  * launch a write against a chip that isn't in DFU yet.
  ******************************************************************************
  */

#ifndef DFUINSTALLDIALOG_H
#define DFUINSTALLDIALOG_H

#include <QDialog>

#include "flash/dfuinstallsession.h"

class QLabel;
class QLineEdit;
class QProgressBar;
class QPlainTextEdit;
class QPushButton;
class QTimer;

class DfuInstallDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DfuInstallDialog(QWidget *parent = nullptr);
    ~DfuInstallDialog() override;

    /* Tell the dialog about the currently-connected app-mode device so it can
     * offer the one-click "reboot into DFU" shortcut. The reboot row is shown
     * only when an F411 is connected (the reboot command only does anything
     * there); it then displays the device name + VID:PID. Call before exec(). */
    void setConnectedDevice(bool f411Present, const QString &name,
                            const QString &vidPid);

signals:
    /* Emitted by the "reboot connected device to DFU" button. The Flasher
     * forwards this to MainWindow, which sends the "system dfu" report so a
     * live (app-mode) device reboots into ROM USB DFU without a BOOT0 jumper
     * (anpeaco/FreeJoyX#55). No-op if no FreeJoy device is connected. */
    void rebootToDfuRequested();

private slots:
    void onRebootToDfu();
    void onBrowseBoot();
    void onBrowseApp();
    void onInstallClicked();
    void onRefreshDetect();    /* silent background poll (timer) */
    void onManualRecheck();    /* user-driven verbose re-check (button) */
    void onInstallDriverClicked();  /* "Install WinUSB driver" (needs-driver state) */

    /* DfuInstallSession feeds. */
    void onAvailability(DfuInstallSession::Availability avail);
    void onDriverInstallFinished(bool ok, const QString &detail);
    void onStageChanged(DfuInstallSession::Stage s, const QString &detail);
    void onProgress(qint64 done, qint64 total);
    void onLogLine(const QString &line);
    void onFinished(bool success, const QString &detail);

private:
    void buildUi();
    void prefillBundledBinaries();
    void refreshInstallEnabled();
    void appendLog(const QString &line);
    void setControlsLocked(bool locked);     /* lock inputs while a write is in flight */

    /* Maps a stage + byte fraction to a coarse weighted overall percentage. */
    static int weightedProgress(DfuInstallSession::Stage s, qint64 done, qint64 total);

    DfuInstallSession *m_session = nullptr;

    QLabel         *m_instructions = nullptr;
    QWidget        *m_rebootRow = nullptr;     /* whole reboot option; hidden if no F411 */
    QLabel         *m_connLabel = nullptr;     /* "<name> — VID:PID" of the connected F411 */
    QLineEdit      *m_bootEdit = nullptr;
    QLineEdit      *m_appEdit = nullptr;
    QLabel         *m_detectLabel = nullptr;
    QProgressBar   *m_progress = nullptr;
    QLabel         *m_stageLabel = nullptr;
    QPlainTextEdit *m_log = nullptr;
    QPushButton    *m_browseBootBtn = nullptr;
    QPushButton    *m_browseAppBtn = nullptr;
    QPushButton    *m_detectBtn = nullptr;
    QPushButton    *m_driverBtn = nullptr;     /* "Install WinUSB driver"; shown only when needed */
    QPushButton    *m_rebootBtn = nullptr;
    QPushButton    *m_installBtn = nullptr;
    QPushButton    *m_closeBtn = nullptr;

    QTimer *m_detectTimer = nullptr;  /* periodic re-probe so plugging in is noticed */
    bool    m_devicePresent = false;  /* probe reported Ready (WinUSB-bound, flashable) */
    bool    m_driverNeeded = false;   /* probe reported NeedsDriver (present, not bound) */
    bool    m_installing = false;
    bool    m_bindingDriver = false;  /* an installDriver() run is in flight */
    /* Latches true after a successful install so the Install button stays
     * disabled -- the board is no longer in a fresh DFU state, and running
     * the write again over the just-finished session misbehaves. Cleared only
     * by reopening the dialog. */
    bool    m_installed = false;
};

#endif // DFUINSTALLDIALOG_H
