/**
  ******************************************************************************
  * @file           : flashsession.cpp
  * @brief          : See flashsession.h.
  ******************************************************************************
  */

#include "flashsession.h"
#include "hiddevice.h"
#include "common_defines.h"

#include <QDebug>
#include <QFile>
#include <QTimer>

namespace {
/* Mirrors the anonymous enum in src/global.h that HidDevice::flashStatus
 * actually emits. HidDevice translates the raw bootloader wire codes
 * (0xF000-family) into these small-integer enum values before signalling,
 * so flashsession must compare against the enum values -- NOT the wire
 * codes. Mixing them up means every IN_PROCESS (=99) emission falls into
 * the "unknown error" default and the whole flash is reported as failed. */
constexpr int kFlashStatusFinished   = 0;     // global.h: FINISHED
constexpr int kFlashStatusSizeError  = 1;     // global.h: SIZE_ERROR
constexpr int kFlashStatusCrcError   = 2;     // global.h: CRC_ERROR
constexpr int kFlashStatusEraseError = 3;     // global.h: ERASE_ERROR
constexpr int kFlashStatusInProcess  = 99;    // global.h: IN_PROCESS
/* HidDevice::flashFirmwareToDevice emits this on the inner hid_read
 * error path (cable pulled, device crashed, hidapi handle invalidated)
 * and on the outer 50 s loop timeout. Not a bootloader-reported status
 * -- the configurator synthesises it so consumers see a terminal
 * flashStatus and the session can walk to Failed instead of hanging
 * in Flashing forever. The numeric value 666 is historical; treat it
 * as "connection lost / packet flow stopped". */
constexpr int kFlashStatusConnectionLost = 666;

/* Slice 2: timeout budgets for the two AwaitingX states. The bootloader
 * usually comes back within ~1 s of the "bootloader run" trigger; the
 * app re-enumerates within ~2-3 s of a NVIC_SystemReset. The budgets
 * here are generous slack to absorb USB hub latency and Windows
 * driver-cache rebinding. Past the budget we transition to Failed
 * (Slice 2) -- Slice 3 will route through RecoveryPrompt instead. */
constexpr int kBootloaderEnumTimeoutMs = 10000;
constexpr int kAppEnumTimeoutMs        = 15000;

/* Watchdog for the Restoring state (post-flash config write-back). A normal
 * restore is a pre-write backup read (~5 s worst case) plus the write
 * (~5 s worst case), so ~10 s working; 15 s leaves margin without trapping the
 * user when a completion signal is lost entirely. */
constexpr int kRestoreTimeoutMs        = 15000;
}

FlashSession::FlashSession(HidDevice *hid, QObject *parent)
    : QObject(parent), m_hid(hid)
{
    m_watcher = new DeviceTransitionWatcher(hid, this);
    connect(m_watcher, &DeviceTransitionWatcher::timedOut,
            this, &FlashSession::onWatcherTimedOut);

    m_restoreTimer = new QTimer(this);
    m_restoreTimer->setSingleShot(true);
    connect(m_restoreTimer, &QTimer::timeout,
            this, &FlashSession::onRestoreTimedOut);
}

bool FlashSession::start(const Params &p)
{
    if (isActive()) {
        qWarning() << "FlashSession::start ignored -- session already active in state" << int(m_state);
        return false;
    }
    if (p.firmwarePath.isEmpty()) {
        fail(tr("No firmware path supplied to FlashSession."));
        return false;
    }

    /* Slurp the .bin up front so the worker-thread call into
     * HidDevice::flashFirmware gets a stable pointer. The buffer lives
     * as long as the session does -- HidDevice keeps a raw pointer to
     * it for the duration of flashFirmwareToDevice's packet loop. */
    QFile f(p.firmwarePath);
    if (!f.open(QIODevice::ReadOnly)) {
        fail(tr("Couldn't open firmware file: %1").arg(p.firmwarePath));
        return false;
    }
    m_firmwareBytes = f.readAll();
    f.close();
    if (m_firmwareBytes.isEmpty()) {
        fail(tr("Firmware file is empty: %1").arg(p.firmwarePath));
        return false;
    }

    m_params = p;
    m_backupPath.clear();
    m_lastErrorDetail.clear();
    m_versionWarned = false;
    m_flashTriggered = false;

    qDebug() << "FlashSession::start path=" << p.firmwarePath
             << "runBackup=" << p.runBackup
             << "triggerBL=" << p.triggerBootloader
             << "autoRestore=" << p.autoRestoreAfterFlash
             << "targetFw=" << QString::number(p.targetFwVersion, 16);

    if (m_params.runBackup) {
        setState(State::BackingUp, tr("Reading current device configuration..."));
        emit needsConfigRead();
    } else {
        /* Skip directly to bootloader entry, or further if we're already
         * in flasher mode (recovery flash). */
        advanceFromBackup();
    }
    return true;
}

void FlashSession::cancelDuringBackup()
{
    if (m_state != State::BackingUp) {
        qDebug() << "FlashSession::cancel ignored in state" << int(m_state);
        return;
    }
    fail(tr("Cancelled by user."));
}

void FlashSession::onConfigReceived(bool success)
{
    if (m_state != State::BackingUp) return;   /* not our read */
    if (success) {
        /* MainWindow will call onBackupSaved next once the file is written. */
        return;
    }
    /* Read failed. The host (MainWindow) prompts the user "continue
     * without backup?" -- we don't make that policy decision here.
     * Pause until onUserAcceptedNoBackup arrives. */
}

void FlashSession::onBackupSaved(const QString &path)
{
    if (m_state != State::BackingUp) return;
    m_backupPath = path;
    emit backupSavedPath(path);
    advanceFromBackup();
}

void FlashSession::onUserAcceptedNoBackup(bool acceptedNoBackup)
{
    if (m_state != State::BackingUp) return;
    if (acceptedNoBackup) {
        advanceFromBackup();
    } else {
        fail(tr("Cancelled: read of current device configuration failed and user declined to proceed without a backup."));
    }
}

void FlashSession::advanceFromBackup()
{
    if (!m_params.triggerBootloader) {
        /* Recovery flash: device is already in BL. Jump to Flashing. */
        setState(State::Flashing, tr("Sending firmware..."));
        m_flashTriggered = true;
        emit needsFlashFirmware(&m_firmwareBytes);
        return;
    }

    setState(State::TriggeringBootloader, tr("Rebooting device into bootloader mode..."));
    if (m_params.reconnectVid != 0 || m_params.reconnectPid != 0) {
        emit needsCaptureReconnectIdentity(m_params.reconnectVid, m_params.reconnectPid);
    }
    /* MainWindow's doEnterFlashMode is blocking on m_threadGetSendConfig --
     * by the time it returns, the device has been told to reset into BL.
     * The detection-thread polling will surface HidDevice::flasherFound(true)
     * when the BL re-enumerates. */
    emit needsEnterBootloader();

    /* After the BL entry trigger returns we sit in AwaitingBootloaderEnum
     * waiting for HidDevice::flasherFound -> onFlasherFound(true). The
     * state transition gates on that signal rather than on
     * needsEnterBootloader's return so a slow re-enumerate doesn't race. */
    if (m_state == State::TriggeringBootloader) {
        setState(State::AwaitingBootloaderEnum, tr("Waiting for device to reappear as flasher..."));
        if (m_watcher) m_watcher->expect(DeviceTransitionWatcher::Target::Bootloader,
                                         kBootloaderEnumTimeoutMs);
    }
}

void FlashSession::onFlasherFound(bool found)
{
    if (!found) return;
    /* Normal arrival from the Awaiting* state, or recovery arrival after
     * the user unplugged + replugged following a timeout. Both resume
     * into advanceFromBootloaderEnum(). */
    const bool fromAwaiting = (m_state == State::AwaitingBootloaderEnum ||
                               m_state == State::TriggeringBootloader);
    const bool fromRecovery = (m_state == State::RecoveryPrompt &&
                               m_stuckAt == State::AwaitingBootloaderEnum);
    if (!fromAwaiting && !fromRecovery) return;
    if (m_watcher) m_watcher->stop();
    if (fromRecovery) {
        m_stuckAt = State::Idle;
    }
    advanceFromBootloaderEnum();
}

void FlashSession::advanceFromBootloaderEnum()
{
    setState(State::Flashing, tr("Sending firmware..."));
    m_flashTriggered = true;
    emit needsFlashFirmware(&m_firmwareBytes);
}

void FlashSession::onWatcherTimedOut(DeviceTransitionWatcher::Target target)
{
    /* Slice 3: timeouts transition to RecoveryPrompt and keep listening
     * (no timeout this time). Mitigates issue anpeaco/FreeJoyX#36's stuck-
     * device case from the configurator side -- the prompt asks the user
     * to unplug + replug, and the watcher resumes when the device
     * reappears. If the user gives up, abortFromRecovery() walks to
     * Failed. */
    if (target == DeviceTransitionWatcher::Target::Bootloader &&
        m_state == State::AwaitingBootloaderEnum) {
        m_stuckAt = State::AwaitingBootloaderEnum;
        setState(State::RecoveryPrompt,
            tr("Bootloader didn't reappear within %1 s. Try unplugging and replugging "
               "the device -- the flash will resume automatically once the bootloader "
               "comes back. If unplugging doesn't help, click Cancel and try again.")
               .arg(kBootloaderEnumTimeoutMs / 1000));
        /* Keep listening with no timeout -- user is in control now. */
        if (m_watcher) m_watcher->expect(DeviceTransitionWatcher::Target::Bootloader, 0);
        return;
    }
    if ((target == DeviceTransitionWatcher::Target::AppCompatible ||
         target == DeviceTransitionWatcher::Target::AppAny) &&
        m_state == State::AwaitingAppEnum) {
        m_stuckAt = State::AwaitingAppEnum;
        QString detail = tr("Device didn't return after %1 s. The flash itself likely "
                            "succeeded -- the device just hasn't re-enumerated yet. "
                            "Try unplugging and replugging now. The configurator will "
                            "resume automatically once the device comes back.")
                            .arg(kAppEnumTimeoutMs / 1000);
        if (!m_backupPath.isEmpty()) {
            detail += QStringLiteral("\n") + tr("Your backup is at: %1").arg(m_backupPath);
        }
        setState(State::RecoveryPrompt, detail);
        if (m_watcher) m_watcher->expect(DeviceTransitionWatcher::Target::AppAny, 0);
        return;
    }
    /* Stale timeout (state moved on before the timer was stopped) -- ignore. */
}

void FlashSession::abortFromRecovery()
{
    if (m_state != State::RecoveryPrompt) {
        qDebug() << "FlashSession::abortFromRecovery ignored in state" << int(m_state);
        return;
    }
    QString detail = tr("Recovery cancelled by user. The device may need manual "
                        "recovery via STM32 Cube Programmer + ST-Link.");
    if (!m_backupPath.isEmpty()) {
        detail += QStringLiteral("\n") + tr("Your backup is at: %1").arg(m_backupPath);
    }
    fail(detail);
}

void FlashSession::onFlashStatus(int status, int bytesSent, int bytesTotal)
{
    if (m_state != State::Flashing) return;

    if (status == kFlashStatusInProcess) {
        emit flashBytesProgress(bytesSent, bytesTotal);
        return;
    }
    if (status == kFlashStatusFinished) {
        emit flashBytesProgress(bytesTotal, bytesTotal);
        advanceFromFlashFinished();
        return;
    }

    /* Terminal error. The first three are reported by the bootloader
     * itself; the connection-lost variant is synthesised by HidDevice
     * when the HID transfer stops without a terminal bootloader code. */
    QString detail;
    switch (status) {
        case kFlashStatusSizeError:
            detail = tr("Bootloader reported SIZE error -- the firmware "
                        "image exceeds the device's app region.");
            break;
        case kFlashStatusCrcError:
            detail = tr("Bootloader reported CRC error -- the transferred "
                        "bytes do not match the expected checksum. Try the "
                        "flash again; if it keeps failing the firmware file "
                        "may be corrupted.");
            break;
        case kFlashStatusEraseError:
            detail = tr("Bootloader reported ERASE error -- the device "
                        "flash could not be erased. This usually means the "
                        "device's flash is write-protected; recovery via "
                        "STM32 Cube Programmer + ST-Link may be required.");
            break;
        case kFlashStatusConnectionLost:
            detail = tr("Lost contact with the device mid-flash. The "
                        "bootloader's response stopped coming back -- the "
                        "USB cable may have been disconnected, or the "
                        "device's bootloader may have crashed.\n\n"
                        "Unplug and replug the device, then retry the flash. "
                        "If the device doesn't enumerate at all after "
                        "replugging, recover via STM32 Cube Programmer + "
                        "ST-Link.");
            break;
        default:
            detail = tr("Bootloader reported unknown error (status=0x%1).")
                        .arg(status, 4, 16, QChar('0'));
            break;
    }
    fail(detail);
}

void FlashSession::advanceFromFlashFinished()
{
    /* Always wait for the app to come back -- a successful FINISHED from
     * the bootloader only means the bytes landed and the CRC matched.
     * The boot loop, USB enumeration, and firmware start-up are separate
     * concerns. Without observing the app's first params packet we
     * can't tell a clean upgrade from a freshly-flashed brick. The
     * watcher's RecoveryPrompt branch surfaces "device didn't come back"
     * if AppAny doesn't fire within kAppEnumTimeoutMs. The auto-restore
     * branch lives in advanceFromAppEnum, which skips the Write for
     * recovery flashes (autoRestoreAfterFlash=false). */
    setState(State::AwaitingAppEnum, tr("Flash complete. Waiting for device to restart..."));
    if (m_watcher) m_watcher->expect(DeviceTransitionWatcher::Target::AppAny,
                                     kAppEnumTimeoutMs);
}

void FlashSession::onParamsPacketReceived(bool firmwareCompatible)
{
    /* Normal arrival, or recovery arrival after the user unplugged +
     * replugged following an AwaitingAppEnum timeout. */
    const bool fromAwaiting = (m_state == State::AwaitingAppEnum);
    const bool fromRecovery = (m_state == State::RecoveryPrompt &&
                               m_stuckAt == State::AwaitingAppEnum);
    if (!fromAwaiting && !fromRecovery) return;
    if (m_watcher) m_watcher->stop();
    if (fromRecovery) {
        m_stuckAt = State::Idle;
    }
    if (!firmwareCompatible) {
        /* Device came back but the binary reports a wire-format the
         * configurator doesn't speak. Surface as success-without-restore --
         * the flash itself worked, the device is alive, but auto-restore
         * isn't safe. */
        setState(State::Done, tr("Device returned with incompatible firmware -- auto-restore skipped."));
        emit finished(true, tr("Flash complete. Device runs new firmware but the configurator can't auto-restore (incompatible wire format)."));
        return;
    }
    advanceFromAppEnum();
}

void FlashSession::advanceFromAppEnum()
{
    /* Version-mismatch warning (issue #21). One-shot. */
    if (!m_versionWarned && m_params.targetFwVersion != 0) {
        m_versionWarned = true;
        /* MainWindow listens for this and appends to the dialog status log. */
        emit versionMismatch(/*reported=*/0, m_params.targetFwVersion);
    }

    if (!m_params.autoRestoreAfterFlash) {
        setState(State::Done, tr("Device factory-reset. Backup remains available for manual restore."));
        emit finished(true, tr("Flash complete; auto-restore skipped per verdict."));
        return;
    }

    setState(State::Restoring, tr("Device back online. Writing saved configuration..."));
    /* Arm the restore watchdog: if the write-back never reports a configSent
     * (lost completion signal -- e.g. duplicate VID:PID confusing post-write
     * re-selection), this guarantees we don't sit in Restoring forever. */
    if (m_restoreTimer) m_restoreTimer->start(kRestoreTimeoutMs);
    emit needsConfigWrite();
}

void FlashSession::onConfigSent(bool success)
{
    if (m_state != State::Restoring) return;
    if (m_restoreTimer) m_restoreTimer->stop();
    advanceFromRestore(success);
}

void FlashSession::onRestoreTimedOut()
{
    /* Single-shot; only meaningful if we're still waiting on the restore. */
    if (m_state != State::Restoring) return;
    QString detail = tr("The device did not confirm the restored configuration "
                        "in time. The firmware flashed successfully, but the "
                        "config write-back could not be verified.");
    if (!m_backupPath.isEmpty()) {
        detail += QStringLiteral("\n") + tr("Your backup is at: %1").arg(m_backupPath);
    }
    fail(detail);
}

void FlashSession::cancelDuringRestore()
{
    if (m_state != State::Restoring) return;
    if (m_restoreTimer) m_restoreTimer->stop();
    /* The flash itself succeeded; the user just chose not to wait for the
     * config write-back. Treat as success-with-restore-skipped (a late
     * configSent is harmlessly ignored by onConfigSent's state guard). */
    QString detail = tr("Restore cancelled. The firmware was flashed "
                        "successfully; your previous configuration was not "
                        "written back.");
    if (!m_backupPath.isEmpty()) {
        detail += QStringLiteral("\n") + tr("Your backup is at: %1").arg(m_backupPath);
    }
    setState(State::Done, detail);
    emit finished(true, detail);
}

void FlashSession::advanceFromRestore(bool success)
{
    if (success) {
        setState(State::Done, tr("Configuration restored. Flash complete."));
        emit finished(true, tr("Configuration restored. Flash complete."));
    } else {
        /* Flash succeeded but the post-flash restore write failed. The
         * backup file is still on disk; the user can load it manually. */
        QString detail = tr("Flash completed but the post-flash config write failed.");
        if (!m_backupPath.isEmpty()) {
            detail += QStringLiteral("\n") + tr("Your backup is at: %1").arg(m_backupPath);
        }
        setState(State::Failed, detail);
        emit finished(false, detail);
    }
}

void FlashSession::setState(State s, const QString &detail)
{
    if (s == m_state && detail.isEmpty()) return;
    qDebug() << "FlashSession" << int(m_state) << "->" << int(s)
             << (detail.isEmpty() ? "" : detail.toUtf8().constData());
    m_state = s;
    emit stateChanged(s, detail);
}

void FlashSession::fail(const QString &detail)
{
    if (m_watcher) m_watcher->stop();
    if (m_restoreTimer) m_restoreTimer->stop();
    m_lastErrorDetail = detail;
    setState(State::Failed, detail);
    emit finished(false, detail);
}
