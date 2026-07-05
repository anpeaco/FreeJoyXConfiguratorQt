/**
  ******************************************************************************
  * @file           : dfuinstalldialog.h
  * @brief          : Modal dialog for installing / reinstalling the F411
  *                   bootloader + app over the STM32 ROM USB DFU (DfuSe)
  *                   bootloader. Issue anpeaco/FreeJoyXConfiguratorQt#53.
  *
  * Built in code (no .ui) -- the layout is small and this keeps the dialog
  * self-contained while the feature is still settling. Owns a
  * DfuInstallSession (the QProcess driver for the `freejoyx-dfu` helper)
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
class QCheckBox;
class QFrame;
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
    void onLeaveClicked();          /* "Exit DFU mode" (ready state) -- manifest + reset, no flash */
    void onEraseClicked();          /* "Erase chip" (Advanced) -- destructive mass-erase, gated */
    void onTimingPresetChanged(int index);  /* Advanced: preset -> fill + lock the spinboxes */

    /* DfuInstallSession feeds. */
    void onAvailability(DfuInstallSession::Availability avail);
    void onDriverInstallFinished(bool ok, const QString &detail);
    void onLeaveFinished(bool ok, const QString &detail);
    void onEraseFinished(bool ok, const QString &detail);
    void onStageChanged(DfuInstallSession::Stage s, const QString &detail);
    void onProgress(qint64 done, qint64 total);
    void onLogLine(const QString &line);
    void onFinished(bool success, const QString &detail);

protected:
    /* Blocks the progress dialog's window-close (X) while a write is live so the
     * user can't orphan the device mid-flash; Cancel is the supported abort. */
    bool eventFilter(QObject *obj, QEvent *ev) override;

    /* While a write is in flight the dialog MUST NOT be torn down: it owns the
     * DfuInstallSession (and its QProcess), so destroying it kills the helper
     * mid-flash (its destructor calls QProcess::kill -> the configurator reports
     * "stopped unexpectedly"). The setup dialog is hidden (not closed) during the
     * write, but a stray reject() (Escape) or close (X/Alt-F4) would end exec()
     * and destroy it -- guard both while m_installing. */
    void reject() override;
    void closeEvent(QCloseEvent *event) override;

    /* The auto-filled path fields shouldn't open pre-selected: the first
     * focusable QLineEdit gets focus on show and Qt select-alls it, so the whole
     * bootloader path appears highlighted in blue (reads like an error). Drop
     * that selection on the first show and park the cursor at the end so the
     * filename is what's visible. */
    void showEvent(QShowEvent *event) override;

private:
    /* Severity of the DFU-mode detection banner (setDetectStatus). */
    enum DetectKind { DetectInfo, DetectReady, DetectWarn, DetectError };

    void buildUi();
    void buildProgressDialog();              /* the separate progress/log window shown on Install */
    void prefillBundledBinaries();
    void refreshInstallEnabled();
    /* Rebuild the top erase-warning banner to match the Boot/App selection:
     * red "config lost" when the app is (re)written, blue "app + config kept"
     * for a boot-only install. Called on construction and on each toggle. */
    void updateEraseWarning();
    void appendLog(const QString &line);
    void setControlsLocked(bool locked);     /* lock inputs while a write is in flight */
    /* Swap the detection status banner (shared makeAlertBanner look) -- blue-info
     * for neutral states, green ready, amber needs-driver, red error. Embeds all
     * action buttons (reparented, hidden); updateDfuEntryState reveals the right
     * one. Direct callers use it for transient/terminal messages (no buttons). */
    void setDetectStatus(DetectKind kind, const QString &text);

    /* The DFU-entry state machine: renders ONE of {ready / needs-driver / reboot
     * / manual} from m_dfuAvail + m_f411Connected, toggling the BOOT0 steps and
     * the active action button. Called by onAvailability + setConnectedDevice. */
    void updateDfuEntryState();

    /* Swap the current DFU-entry state widget (status banner or the plain reboot
     * prompt) into m_detectArea, deleting the previous one. The new widget must
     * already own the reparented action buttons. */
    void swapStateWidget(QWidget *w);

    /* Maps a stage + byte fraction to a coarse weighted overall percentage. */
    static int weightedProgress(DfuInstallSession::Stage s, qint64 done, qint64 total);

    DfuInstallSession *m_session = nullptr;

    QLabel         *m_instructions = nullptr;  /* manual BOOT0 steps; shown only in the Manual state */
    /* Boot/App/Both selection: the form-row labels for the two path fields.
     * Unchecking one disables its path row and drops that region from the
     * install (the helper writes only the checked region(s)). Default both on
     * == the previous behaviour. */
    QCheckBox      *m_bootCheck = nullptr;
    QCheckBox      *m_appCheck = nullptr;
    QLineEdit      *m_bootEdit = nullptr;
    QLineEdit      *m_appEdit = nullptr;
    QVBoxLayout    *m_detectArea = nullptr;    /* holds the detection status banner */
    QWidget        *m_detectBanner = nullptr;  /* current detection banner (swapped by setDetectStatus) */
    QPushButton    *m_browseBootBtn = nullptr;
    QPushButton    *m_browseAppBtn = nullptr;
    QPushButton    *m_detectBtn = nullptr;
    QPushButton    *m_driverBtn = nullptr;     /* "Install WinUSB driver"; shown only when needed */
    QPushButton    *m_leaveBtn = nullptr;      /* "Exit DFU mode"; shown only in the Ready state */
    QPushButton    *m_eraseBtn = nullptr;      /* "Erase chip (clear all)"; Advanced section, gated */
    QPushButton    *m_rebootBtn = nullptr;
    QPushButton    *m_installBtn = nullptr;
    QPushButton    *m_closeBtn = nullptr;

    QVBoxLayout    *m_rootLayout = nullptr;   /* the dialog's root layout (for the erase banner) */
    QFrame         *m_eraseWarn = nullptr;    /* top erase/keep banner; rebuilt by updateEraseWarning */

    /* Separate progress/log window, shown on Install. Same design as the
     * Upgrade-Firmware FlashProgressDialog (stage label, byte counter, centred
     * progress bar, "Status:" log) -- the stage/progress/log live here, not in
     * the setup dialog above. */
    QDialog          *m_progressDialog = nullptr;
    QVBoxLayout      *m_progLayout = nullptr;    /* progress dialog's root layout (for the result banner) */
    QLabel           *m_stageLabel = nullptr;
    QLabel           *m_bytesLabel = nullptr;   /* "<sent> / <total> bytes" during write stages */
    QProgressBar     *m_progress = nullptr;
    QPlainTextEdit   *m_log = nullptr;
    QWidget          *m_resultBanner = nullptr; /* terminal result shown as the app alert banner */
    QDialogButtonBox *m_progButtons = nullptr;
    QPushButton      *m_progCancelBtn = nullptr; /* "Cancel" during a write, "Close" after */

    /* Show the terminal outcome as the shared alert banner (green check on
     * success, red triangle on failure) at the top of the progress window,
     * instead of recolouring the stage label. clearProgressResult() removes it
     * for a retry. */
    void showProgressResult(bool success, const QString &text);
    void clearProgressResult();

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
    QSpinBox       *m_spinIdleConfirms = nullptr; /* consecutive idle reports (#80) */
    QSpinBox       *m_spinMinBlock = nullptr;  /* min per-block program window ms (#80) */

    QTimer *m_detectTimer = nullptr;  /* periodic re-probe so plugging in is noticed */
    int     m_detectTick = 0;         /* counts background polls; throttles the driver-layer check */
    bool    m_devicePresent = false;  /* probe reported Ready (WinUSB-bound, flashable) */
    bool    m_driverNeeded = false;   /* probe reported NeedsDriver (present, not bound) */

    /* DFU-entry state-machine inputs (see updateDfuEntryState). */
    DfuInstallSession::Availability m_dfuAvail =
        DfuInstallSession::Availability::Absent;   /* latest DFU detection result */
    bool    m_f411Connected = false;  /* an F411 app-mode device is connected (rebootable) */
    QString m_connName;               /* its live USB enumeration name */
    QString m_connVidPid;             /* its VID:PID */
    bool    m_installing = false;
    bool    m_bindingDriver = false;  /* an installDriver() run is in flight */
    bool    m_leaving = false;        /* a leaveDfu() run is in flight */
    bool    m_erasingChip = false;    /* an eraseChip() run is in flight */
    bool    m_firstShow = true;       /* clear the path fields' initial select-all once */
    /* True once the user used the software "Reboot into DFU" button (vs a manual
     * BOOT0 jumper). When DFU was entered by command, BOOT0 was only momentarily
     * low, so the helper's post-install leave actually exits DFU and the board
     * restarts into firmware on its own (and serial-reconnects) -- no replug. A
     * held-BOOT0 manual entry re-enters DFU on that reset, so a replug / jumper
     * removal IS needed. The completion message picks wording from this. */
    bool    m_enteredDfuViaCommand = false;
    /* Latches true after a successful install so the Install button stays
     * disabled -- the board is no longer in a fresh DFU state, and running
     * the write again over the just-finished session misbehaves. Cleared only
     * by reopening the dialog. */
    bool    m_installed = false;
};

#endif // DFUINSTALLDIALOG_H
