/**
  ******************************************************************************
  * @file           : flashprogressdialog.h
  * @brief          : Application-modal progress dialog for the flash flow.
  *                   Owns the visible state machine -- stage label, weighted
  *                   overall progress bar, byte counter, status log -- and
  *                   exposes setters that MainWindow drives from the existing
  *                   backup -> bootloader -> flash -> re-enumerate -> restore
  *                   chain. Issue anpeaco/FreeJoyXConfiguratorQt#19.
  *
  *                   The dialog is a viewer + a controller:
  *                     - viewer: MainWindow pushes state transitions via the
  *                       setStage / setFlashBytes slots; the dialog renders.
  *                     - controller: the dialog disables the X button and
  *                       gates the Cancel/Close button per-stage so the user
  *                       cannot orphan device state mid-flow.
  *
  *                   Cross-version restore (legacy_migrator integration) and
  *                   the post-flash version-match check live in later slices
  *                   (#20, #21). This slice implements same-major-version
  *                   auto-restore only.
  ******************************************************************************
  */

#ifndef FLASHPROGRESSDIALOG_H
#define FLASHPROGRESSDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class FlashProgressDialog;
}

class FlashProgressDialog : public QDialog
{
    Q_OBJECT

public:
    /* Stages of the flash flow. Order matters -- the weighted progress
     * bar (5/5/80/5/5) advances monotonically through them. Recovery
     * flashes skip Backup, EnteringBootloader, and Restore by jumping
     * straight from Idle -> Flash. */
    enum class Stage {
        Idle,
        Backup,
        EnteringBootloader,
        Flash,
        WaitingForReset,
        Restore,
        Done,
        TerminalError,
        /* Slice 3: device didn't reappear within the watcher's budget.
         * Surfaces a clear "try unplugging" message; the Cancel button
         * is re-enabled so the user can abort cleanly. The watcher keeps
         * listening with no timeout. */
        RecoveryPrompt,
    };

    /* True for a "recovery flash" -- the dialog skips Backup + Bootloader
     * stages in its weighted progress and step list. Set via the
     * constructor; cannot change mid-flow. */
    explicit FlashProgressDialog(bool recoveryFlash, QWidget *parent = nullptr);
    ~FlashProgressDialog();

    Stage stage() const { return m_stage; }

public slots:
    /* Stage transitions pushed from MainWindow. setStage(Done) and
     * setStage(TerminalError, ...) cause the Cancel button to flip to
     * Close. The detail string appears in the status log. */
    void setStage(Stage s, const QString &detail = QString());

    /* Per-chunk progress update during the Flash stage. Other stages
     * ignore the byte counters but the function is safe to call from
     * any state. */
    void setFlashBytes(int bytesSent, int bytesTotal);

    /* Append a free-form line to the status log without changing
     * stage. Useful for non-stage events (e.g. "Backup saved to
     * <path>"). */
    void appendStatus(const QString &line);

signals:
    /* Fires when the user clicks Cancel during a stage that allows
     * abort (Backup only in this slice -- the other gating cases are
     * polled via cancelEnabledForStage in the dialog). MainWindow
     * consumes this to abort the pending Read. */
    void cancelRequested();

private slots:
    void onCancelOrCloseClicked();

private:
    Ui::FlashProgressDialog *ui;
    Stage m_stage = Stage::Idle;
    bool m_recoveryFlash = false;

    /* Last seen byte counters. Captured even outside the Flash stage
     * so transitions don't drop the progress bar back to zero. */
    int m_bytesSent = 0;
    int m_bytesTotal = 0;

    /* Updates the weighted overall progress bar from the current stage
     * + last-seen byte counters. */
    void refreshProgressBar();

    /* Updates the stage label / step list, enable state of the
     * Cancel button, and (on terminal stages) the Cancel-to-Close
     * relabelling. */
    void refreshStageLabels();

    /* True if Cancel should be enabled in the current stage. Only
     * Backup is cancellable -- once we've committed to bootloader
     * entry the device's state diverges from the configurator's, so
     * "cancel" can no longer be done cleanly. */
    bool cancelEnabledForStage() const;

    /* Maps Stage -> human-readable label for the stage line. */
    static QString stageLabel(Stage s);

    /* Maps Stage -> weighted progress contribution in 0..100. The flash
     * stage's 80% is interpolated by setFlashBytes; the others are
     * each fixed 5%. */
    static int stageBaseProgress(Stage s, bool recoveryFlash);

    /* Show the terminal outcome as the shared app alert banner (green check on
     * Done, red triangle on TerminalError) pinned above the stage line, instead
     * of a recoloured stage label. The stage line is hidden while the banner is
     * up. One-shot: this dialog is created fresh per flash, so there's no clear. */
    void showResult(bool success, const QString &text);

    QWidget *m_resultBanner = nullptr;   /* terminal result banner; null until shown */
};

#endif // FLASHPROGRESSDIALOG_H
