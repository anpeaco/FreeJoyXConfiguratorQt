/**
  ******************************************************************************
  * @file           : flashsession.h
  * @brief          : Explicit state machine for the firmware flash flow.
  *
  * Owns the orchestration that previously lived as a constellation of
  * member bools on MainWindow (m_consolidatedFlashActive, m_upgradePending,
  * m_postUpgradeWriteInFlight, m_backupBeforeFlashPending, etc.). One state
  * variable, one signal stream for transitions.
  *
  * Slice 1 (this file): consolidated and recovery flash flows route through
  * one session. State is explicit, but timeouts on the AwaitingBootloader /
  * AwaitingApp enumerations are still implicit (the configurator just
  * waits until the device shows up). Slice 2 layers a typed
  * DeviceTransitionWatcher on top to make those waits time-bounded with a
  * recovery branch.
  *
  * Why MainWindow still handles config Read / Write rather than
  * FlashSession calling HidDevice directly: those operations come with
  * UI side-effects (tabs repopulate from gEnv.pDeviceConfig, button
  * feedback states cycle through, blockWRConfigToDevice gates dropdowns).
  * Folding that into FlashSession would make Slice 1 huge. Instead, the
  * session emits `needs*` signals; MainWindow obliges and reports back via
  * the public slots.
  *
  * Cross-thread note: FlashSession lives on the GUI thread. HidDevice
  * lives on m_thread. Worker-thread calls (enterToFlashMode,
  * flashFirmware) are still proxied through MainWindow's existing
  * doEnterFlashMode / doFlashFirmware (which set up an event loop on
  * m_threadGetSendConfig). Slice 1 doesn't change that.
  ******************************************************************************
  */

#ifndef FLASHSESSION_H
#define FLASHSESSION_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <stdint.h>

#include "devicetransitionwatcher.h"

class HidDevice;
class QTimer;

class FlashSession : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Idle,
        BackingUp,                  /* reading the device's config + writing the backup file */
        TriggeringBootloader,       /* sending "bootloader run" */
        AwaitingBootloaderEnum,     /* waiting for the device to come back as "FreeJoyX Flasher" */
        Flashing,                   /* REPORT_ID_FLASH packet exchange in flight */
        AwaitingAppEnum,            /* waiting for the freshly-flashed app to enumerate */
        Restoring,                  /* auto-writing the backed-up config */
        Done,
        Failed,
        /* Slice 3: device didn't reappear within the watcher's timeout.
         * Surface a "try unplugging" prompt and keep listening with no
         * timeout. Resumes back into AwaitingBootloaderEnum /
         * AwaitingAppEnum behaviour when the device eventually arrives.
         * abortFromRecovery() walks to Failed if the user gives up. */
        RecoveryPrompt,
    };

    struct Params {
        QString  firmwarePath;              /* required */
        bool     runBackup           = true;   /* Read+save before BL entry */
        bool     triggerBootloader   = true;   /* false = device already in BL (recovery flash) */
        bool     autoRestoreAfterFlash = true; /* false when verdict says don't restore (downgrade, no-migrator, etc.) */
        uint16_t targetFwVersion     = 0;   /* binary footer's version, for the post-flash mismatch warn */
        uint16_t reconnectVid        = 0;   /* used by captureReconnectIdentity before BL trigger */
        uint16_t reconnectPid        = 0;
    };

    explicit FlashSession(HidDevice *hid, QObject *parent = nullptr);

    State state() const { return m_state; }
    bool isActive() const { return m_state != State::Idle && m_state != State::Done && m_state != State::Failed; }
    const QString &lastErrorDetail() const { return m_lastErrorDetail; }
    const QString &backupPath() const { return m_backupPath; }
    bool versionWarned() const { return m_versionWarned; }
    bool isRecoveryFlash() const { return !m_params.triggerBootloader; }
    const Params &params() const { return m_params; }

    /* Begin a flash flow. Caller is responsible for having shown the
     * confirmation dialog and gathered user consent. Returns false if a
     * session is already active or the firmware file fails to load. */
    bool start(const Params &p);

    /* User-initiated abort. Only honoured during BackingUp -- once BL
     * entry has been triggered the device's state is diverging from ours
     * and "cancel" no longer has a clean meaning. */
    void cancelDuringBackup();

    /* User-initiated abort from RecoveryPrompt. Walks to Failed; the
     * dialog renders the terminal error so the user sees the cause
     * (unable to recover device). */
    void abortFromRecovery();

    /* User-initiated abort during Restoring (the post-flash config write).
     * Safe to interrupt: the firmware is already flashed and running; only
     * the config write-back is skipped, and the backup file is on disk.
     * Walks to Done (flash succeeded) with a "restore skipped" note. */
    void cancelDuringRestore();

public slots:
    /* MainWindow proxies these from HidDevice. The session uses them to
     * drive state transitions rather than reading HidDevice's internal
     * state directly. */
    void onParamsPacketReceived(bool firmwareCompatible);
    void onFlasherFound(bool found);
    void onFlashStatus(int status, int bytesSent, int bytesTotal);
    void onConfigReceived(bool success);   /* MainWindow already saved the backup file */
    void onConfigSent(bool success);

    /* MainWindow calls these when its proxied work for a needs*() signal
     * is done. onBackupSaved supplies the path so the dialog can show
     * it; onConfigReadFailed supplies the user's decision when the read
     * fails (continue without backup vs. abort). */
    void onBackupSaved(const QString &path);
    void onUserAcceptedNoBackup(bool acceptedNoBackup);   /* false = user cancelled flash */

signals:
    void stateChanged(FlashSession::State newState, const QString &detail);
    void flashBytesProgress(int bytesSent, int bytesTotal);
    void backupSavedPath(const QString &path);
    void versionMismatch(uint16_t reportedFwVersion, uint16_t expectedFwVersion);

    /* MainWindow obliges these by driving its existing pipelines:
     *   needsConfigRead  -> on_pushButton_ReadConfig_clicked (with the
     *                       backup-pending flag set) -> writeAutoBackup
     *   needsConfigWrite -> on_pushButton_WriteConfig_clicked
     *   needsEnterBootloader -> doEnterFlashMode  (worker-thread call)
     *   needsCaptureReconnectIdentity -> HidDevice::captureReconnectIdentity
     *   needsFlashFirmware -> doFlashFirmware  (worker-thread call)
     */
    void needsConfigRead();
    void needsConfigWrite();
    void needsEnterBootloader();
    void needsCaptureReconnectIdentity(uint16_t vid, uint16_t pid);
    void needsFlashFirmware(const QByteArray *firmware);

    /* Sole "we're done" exit. final detail is a human-readable summary
     * the dialog can append to its log. */
    void finished(bool success, const QString &finalDetail);

private:
    void setState(State s, const QString &detail = QString());
    void fail(const QString &detail);
    void advanceFromBackup();
    void advanceFromBootloaderEnum();
    void advanceFromFlashFinished();
    void advanceFromAppEnum();
    void advanceFromRestore(bool success);

    HidDevice  *m_hid = nullptr;       /* non-owning */
    State       m_state = State::Idle;
    Params      m_params;
    QByteArray  m_firmwareBytes;
    QString     m_backupPath;
    QString     m_lastErrorDetail;
    bool        m_versionWarned = false;
    bool        m_flashTriggered = false;  /* needsFlashFirmware emitted once we reach Flashing state */

    /* Slice 3: which Awaiting* state we were in when the watcher timed
     * out. RecoveryPrompt resumes back to the corresponding advance*
     * helper when the device finally arrives. */
    State       m_stuckAt = State::Idle;

    /* Slice 2: typed timeout-bounded waiter for the AwaitingBootloaderEnum
     * and AwaitingAppEnum states. Owned by the session. Wires itself to
     * HidDevice signals at construction time so it sees every transition
     * regardless of which state the session is currently in (it's the
     * session that decides whether to act on a given arrival via its
     * own onFlasherFound / onParamsPacketReceived slots). */
    DeviceTransitionWatcher *m_watcher = nullptr;

    /* Watchdog for the Restoring state. The post-flash config write relies on
     * a configSent signal coming back; if the device re-enumerates badly (e.g.
     * a duplicate VID:PID confuses post-write re-selection) that signal can be
     * lost entirely, leaving the session stuck in Restoring forever. This
     * single-shot timer guarantees an escape: on expiry the session fails
     * gracefully to a Close-able terminal state. Started on entering Restoring,
     * stopped on configSent / cancel / fail. */
    QTimer     *m_restoreTimer = nullptr;

private slots:
    void onWatcherTimedOut(DeviceTransitionWatcher::Target target);
    void onRestoreTimedOut();
};

#endif // FLASHSESSION_H
