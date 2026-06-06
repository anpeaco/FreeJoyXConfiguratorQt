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
class QToolButton;
class QComboBox;
class QSpinBox;
class QWidget;
class QTimer;
class QDialogButtonBox;
class QVBoxLayout;

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
    void onTimingPresetChanged(int index);  /* Advanced: preset -> fill + lock the spinboxes */

    /* DfuInstallSession feeds. */
    void onAvailability(DfuInstallSession::Availability avail);
    void onDriverInstallFinished(bool ok, const QString &detail);
    void onStageChanged(DfuInstallSession::Stage s, const QString &detail);
    void onProgress(qint64 done, qint64 total);
    void onLogLine(const QString &line);
    void onFinished(bool success, const QString &detail);

protected:
    /* Blocks the progress dialog's window-close (X) while a write is live so the
     * user can't orphan the device mid-flash; Cancel is the supported abort. */
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    /* Severity of the DFU-mode detection banner (setDetectStatus). */
    enum DetectKind { DetectInfo, DetectReady, DetectWarn, DetectError };

    void buildUi();
    void buildProgressDialog();              /* the separate progress/log window shown on Install */
    void prefillBundledBinaries();
    void refreshInstallEnabled();
    void appendLog(const QString &line);
    void setControlsLocked(bool locked);     /* lock inputs while a write is in flight */
    /* Swap the detection status banner (shared makeAlertBanner look) -- blue-info
     * for neutral states, green ready, amber needs-driver, red error. */
    void setDetectStatus(DetectKind kind, const QString &text);

    /* Maps a stage + byte fraction to a coarse weighted overall percentage. */
    static int weightedProgress(DfuInstallSession::Stage s, qint64 done, qint64 total);

    DfuInstallSession *m_session = nullptr;

    QLabel         *m_instructions = nullptr;
    QWidget        *m_rebootRow = nullptr;     /* whole reboot option; hidden if no F411 */
    QLabel         *m_connLabel = nullptr;     /* "<name> — VID:PID" of the connected F411 */
    QLineEdit      *m_bootEdit = nullptr;
    QLineEdit      *m_appEdit = nullptr;
    QVBoxLayout    *m_detectArea = nullptr;    /* holds the detection status banner */
    QWidget        *m_detectBanner = nullptr;  /* current detection banner (swapped by setDetectStatus) */
    QPushButton    *m_browseBootBtn = nullptr;
    QPushButton    *m_browseAppBtn = nullptr;
    QPushButton    *m_detectBtn = nullptr;
    QPushButton    *m_driverBtn = nullptr;     /* "Install WinUSB driver"; shown only when needed */
    QPushButton    *m_rebootBtn = nullptr;
    QPushButton    *m_installBtn = nullptr;
    QPushButton    *m_closeBtn = nullptr;

    /* Separate progress/log window, shown on Install. Same design as the
     * Upgrade-Firmware FlashProgressDialog (stage label, byte counter, centred
     * progress bar, "Status:" log) -- the stage/progress/log live here, not in
     * the setup dialog above. */
    QDialog          *m_progressDialog = nullptr;
    QLabel           *m_stageLabel = nullptr;
    QLabel           *m_bytesLabel = nullptr;   /* "<sent> / <total> bytes" during write stages */
    QProgressBar     *m_progress = nullptr;
    QPlainTextEdit   *m_log = nullptr;
    QDialogButtonBox *m_progButtons = nullptr;
    QPushButton      *m_progCancelBtn = nullptr; /* "Cancel" during a write, "Close" after */

    /* Advanced (collapsible) DfuSe timing controls. m_advBox is hidden until the
     * cog toggle is checked. The preset combo (Baseline/Loose/Lax/Custom) fills
     * + locks the spinboxes; Custom unlocks them within sensible ranges. */
    QToolButton    *m_advToggle = nullptr;
    QWidget        *m_advBox = nullptr;
    QComboBox      *m_presetCombo = nullptr;
    QSpinBox       *m_spinDelay = nullptr;     /* inter-block delay (ms) */
    QSpinBox       *m_spinPoll = nullptr;      /* poll/erase timeout (ms) */
    QSpinBox       *m_spinXfer = nullptr;      /* transfer timeout (ms) */
    QSpinBox       *m_spinRetries = nullptr;   /* per-block retries */
    QSpinBox       *m_spinSettle = nullptr;    /* post-flash settle (ms) */

    QTimer *m_detectTimer = nullptr;  /* periodic re-probe so plugging in is noticed */
    int     m_detectTick = 0;         /* counts background polls; throttles the driver-layer check */
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
