#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QTimer>
#include <QTranslator>

#include "hiddevice.h"
#include "reportconverter.h"

#include "advancedsettings.h"
#include "axesconfig.h"
#include "axescurvesconfig.h"
#include "buttonconfig.h"
#include "debugwindow.h"
#include "encodersconfig.h"
#include "ledconfig.h"
#include "pinconfig.h"
#include "shiftregistersconfig.h"
#include "shiftstimersconfig.h"
#include "switchbutton.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class FlashProgressDialog;
class FlashSession;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setDefaultStyleSheet();

signals:
    void getConfigDone(bool success);
    void sendConfigDone(bool success);

private slots:
    void showConnectDeviceInfo();
    void hideConnectDeviceInfo();
    void flasherConnected();
    //void getParamsPacket(uint8_t *buffer);
    void getParamsPacket(bool firmwareCompatible);

    void configReceived(bool success);
    void configSent(bool success);
    /* Old upstream firmware was read + the bytes translated into the
     * current dev_config_t shape. Surface a message dialog explaining
     * the migration and the next-step (flash + write). */
    void legacyConfigMigrated(uint16_t oldFirmwareVersion);
    void blockWRConfigToDevice(bool block);

    void hidDeviceList(const QList<QPair<bool, QString>> &deviceNames, int preferredIndex);
    void hidDeviceListChanged(int index);

    /* Receives USB identity (vid hex, pid hex, serial) for the currently
     * opened device from HidDevice and populates the device-info card
     * below the dropdown. Empty strings (on disconnect) reset the card
     * to "—". Firmware version is filled separately from getParamsPacket
     * since it arrives later via the params report. */
    void setDeviceInfo(const QString &vidHex, const QString &pidHex, const QString &serial);

    /* Fires after the post-write 5-second grace window with no match.
     * If the dropdown still has no current selection but the list does
     * have items, falls back to index 0 -- legacy behaviour for boards
     * that re-enumerate slowly or come back with a different identity. */
    void onPostWriteFallback();

    void languageChanged(const QString &language);
    void setFont();

    void finalInitialization();

    void on_pushButton_ResetAllPins_clicked();

    void on_pushButton_ReadConfig_clicked();
    void on_pushButton_WriteConfig_clicked();
    void on_pushButton_UpgradeFirmware_clicked();

    void on_pushButton_SaveToFile_clicked();
    void on_pushButton_LoadFromFile_clicked();

    void on_pushButton_ShowDebug_clicked();

    void on_pushButton_TestButton_clicked();
    void on_pushButton_TestButton_2_clicked();

    void on_pushButton_Wiki_clicked();

    void themeChanged(bool dark);

    void on_toolButton_ConfigsDir_clicked();

    /* Bridge between the Encoders tab "Enable" checkbox and the Pin
     * Config tab. Triggered by EncodersConfig::fastEncoderEnableToggleRequested
     * (which is itself forwarded from FastEncoder's Enable checkbox).
     * Inspects the pin pair for the requested encoder slot, prompts on
     * conflict (a pin is currently assigned to something else), then
     * applies FAST_ENCODER (or NOT_USED for the disable case) to both
     * pins. The existing pinIndexChanged dispatch in PinConfig
     * propagates the change back through axesSourceChanged,
     * fastEncoderSelected (-> FastEncoder labels + checkbox sync),
     * and the limit/highlight logic. */
    void onFastEncoderEnableToggleRequested(int slotIndex, bool desiredEnabled);

    /* Diagnostic dump: shows every detected FreeJoy device with full
     * VID:PID, name, serial, and an indicator marking the currently-
     * selected one. Triggered by the "Show all connected devices"
     * button on the Advanced Settings tab. */
    void onShowAllConnectedDevicesRequested();

    /* Push the currently-connected FreeJoy device VID/PIDs (excluding
     * the selected device) into AdvancedSettings so the conflict pill
     * stays in sync. Called whenever the device list changes. */
    void refreshOtherConnectedDevices();

    /* Issue anpeaco/FreeJoyXConfiguratorQt#19: orchestrate the
     * consolidated one-click flash. Opens FlashProgressDialog, then
     * drives the existing backup -> bootloader -> flash -> reset ->
     * restore chain (reuses the m_upgradePending machinery) with
     * stage transitions pushed into the dialog from existing signal
     * handlers. */
    void onConsolidatedFlashRequested(const QString &filePath);
    void onFlashProgressCancelRequested();

    /* Slice 4: shared entry that opens the progress dialog and starts a
     * FlashSession. Used by both onConsolidatedFlashRequested (Flasher
     * tab Flash button) and on_pushButton_UpgradeFirmware_clicked
     * (toolbar Upgrade button). Caller has already shown the
     * confirmation dialog; this assumes user consent. */
    void startConsolidatedFlash(const QString &filePath);
    /* onFlashStatusToDialog was the bridge from HidDevice::flashStatus
     * into FlashProgressDialog before Slice 1. FlashSession's
     * onFlashStatus + stateChanged supersede it. */

    /* FlashSession signal handlers. The session emits `needs*` for every
     * external step (read backup, enter BL, flash bytes, write restore);
     * these slots dispatch to the existing MainWindow helpers. The
     * session also emits stateChanged / flashBytesProgress /
     * backupSavedPath / versionMismatch / finished which drive the
     * FlashProgressDialog. */
    void onFlashSessionStateChanged(int newState, const QString &detail);
    void onFlashSessionFinished(bool success, const QString &finalDetail);
    void onFlashSessionNeedsConfigRead();
    void onFlashSessionNeedsConfigWrite();
    void onFlashSessionNeedsEnterBootloader();
    void onFlashSessionNeedsCaptureReconnectIdentity(uint16_t vid, uint16_t pid);
    void onFlashSessionNeedsFlashFirmware(const QByteArray *firmware);
    void onFlashSessionVersionMismatch(uint16_t reported, uint16_t expected);
    void onFlashSessionBackupSaved(const QString &path);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    Ui::MainWindow *ui;

    QThread *m_thread;
    HidDevice *m_hidDeviceWorker;

    QThread *m_threadGetSendConfig;

    PinConfig *m_pinConfig;
    ButtonConfig *m_buttonConfig;
    ShiftsTimersConfig *m_shiftsTimersConfig;
    LedConfig *m_ledConfig;
    EncodersConfig *m_encoderConfig;
    ShiftRegistersConfig *m_shiftRegConfig;
    AxesConfig *m_axesConfig;
    AxesCurvesConfig *m_axesCurvesConfig;
    AdvancedSettings *m_advSettings;

    DebugWindow *m_debugWindow = nullptr;
    bool m_debugIsEnable;

    bool m_deviceChanged;

    /* "Pending changes" badge state. m_deviceConfigSnapshot is the
     * byte image of dev_config_t at the moment of the last device sync
     * (successful Read, or post-Write flush). m_haveDeviceConfigSnapshot
     * gates the comparison so we don't flag the zero-initialised
     * dev_config_t at startup as dirty against an empty snapshot.
     * m_dirtyCheckTimer drives a 1 Hz poll that flushes the UI into
     * dev_config_t and compares against the snapshot. */
    QByteArray m_deviceConfigSnapshot;
    bool       m_haveDeviceConfigSnapshot = false;
    QTimer    *m_dirtyCheckTimer = nullptr;

    /* True between clicking Write Config and the next deviceConnected.
     * Lets hideConnectDeviceInfo show "Restarting..." instead of
     * "Disconnected" while the chip is re-enumerating after the write,
     * which is a much friendlier signal than the red "Disconnected"
     * pill (the cable wasn't pulled). Cleared on reconnect, on
     * configSent(false), and on natural cable-pull disconnects (the
     * latter via the second hideConnectDeviceInfo call from the main
     * worker loop's empty-list path -- see comment in that slot). */
    bool m_postWriteRestarting = false;

    /* One-click firmware upgrade button on the toolbar. The button
     * itself is enabled/disabled per device state; the click handler
     * routes through startConsolidatedFlash() so a single FlashSession
     * drives every flash entry point. findUpgradeFirmwarePath locates
     * the matching .bin in firmware/ for the connected device's board. */
    void    refreshUpgradeButtonState(); /* enables/disables based on connection + version + firmware availability */
    QString findUpgradeFirmwarePath(int boardId, QString *outVersion) const;

    /* The progress dialog is owned by MainWindow for the duration of a
     * flash session. Constructed in startConsolidatedFlash() and torn
     * down on dialog close. */
    FlashProgressDialog *m_flashProgress = nullptr;

    /* Whole-window lock applied during a flash session. Disables
     * everything except the Advanced Settings tab so the user can't
     * wander to other tabs and trigger actions that'd race with the
     * in-flight flash + reconnect. Released by onFlashSessionFinished. */
    bool    m_flashChainLocked = false;
    void    setFlashChainUiLocked(bool locked);

    /* Cached default text for the Read / Write Config buttons, captured
     * once in the constructor before any code path mutates them. We
     * previously used `static QString button_default_text =
     * ui->pushButton_*->text();` inside configSent / configReceived, but
     * because `static` initialises on first call, the Write button's
     * "default" got captured as "Backup OK, writing..." (set by the
     * pre-write backup branch before configSent's first run), leaving
     * the button stuck on that label after every subsequent write. */
    QString m_writeButtonDefaultText;
    QString m_readButtonDefaultText;

    /* 5-second grace window after Write Config during which the
     * dropdown does NOT auto-select index 0 if the post-write rebuild
     * can't yet match the target identity (serial / expectedVid+Pid /
     * path). Without it the configurator briefly shows the wrong
     * device every time another HID is plugged in. On timeout we fall
     * back to selecting index 0 -- last-resort behaviour for boards
     * that legitimately took longer than 5 s to re-enumerate or that
     * came back with a totally different identity. */
    QTimer m_postWriteFallbackTimer;

    QString m_cfgDirPath;

    /* Save a snapshot of the device's current config to
     * <m_cfgDirPath>/backups/. Called from configReceived's
     * m_flashSessionBackupPending branch as the BackingUp step of a
     * FlashSession. */
    QString writeAutoBackup();

    /* Worker-thread shims. doEnterFlashMode sends the "bootloader run"
     * report; doFlashFirmwareBytes streams the .bin to the bootloader's
     * REPORT_ID_FLASH receiver. Both block on m_threadGetSendConfig
     * via QEventLoop until the underlying HID exchange returns. */
    void doEnterFlashMode();
    void doFlashFirmwareBytes(const QByteArray *firmware);

    /* Owned by MainWindow; constructed in the ctor after m_hidDeviceWorker.
     * One session at a time. Slice 1 routes the consolidated flow through
     * it; later slices route the toolbar Upgrade button and the legacy
     * two-button Flasher tab path through the same session. */
    FlashSession *m_flashSession = nullptr;
    /* True while the session-owned backup Read is in flight. Cleared on
     * configReceived; lets configReceived distinguish a session-driven
     * Read from the user's manual Read Config click. */
    bool m_flashSessionBackupPending = false;

    /* Pre-write device-config backup. When the user clicks Write, the
     * configurator first reads the device's current config (no UI
     * refresh -- silent), saves it to a backup file named with the
     * device's identity (Name + serial + VID/PID), and then continues
     * with the actual write. m_pendingWriteConfig holds the user's
     * staged edits across the read+write cycle. */
    bool m_backupBeforeWritePending = false;
    dev_config_t m_pendingWriteConfig;
    /* Device identity captured at setDeviceInfo time -- used for the
     * pre-write backup filename. Cleared on disconnect. */
    QString m_currentDeviceVid;
    QString m_currentDevicePid;
    QString m_currentDeviceSerial;
    QString writePreWriteDeviceBackup();
    void doActualWriteToDevice();

    /* Toggle the config-editing tabs (Pin / Button / Shifts&Timers /
     * Axes / Curves / Shift Reg / Encoders / LED). Disabled when an
     * unrecognised-firmware device is connected, so the user can't
     * silently edit a config that doesn't correspond to anything on
     * the device. Advanced Settings + Debug stay accessible -- those
     * are configurator-side controls, not device-state. */
    void setConfigTabsEnabled(bool enabled);

    void curCfgFileChanged(const QString &fileName);
    QStringList cfgFilesList(const QString &dirPath);
    QIcon pixmapToIcon(QPixmap pixmap, const QColor &color);
    void updateColor();

    void UiReadFromConfig();
    void UiWriteToConfig();
    /* Side-effect-free portion of UiWriteToConfig: fans out
     * writeToConfig() to every tab widget and captures the breakdown
     * snapshot into dev_config_t. Used by the dirty-state poller to
     * compare the current edit state against the last device-sync'd
     * snapshot without also clearing the Windows joystick OEMName
     * registry cache, which UiWriteToConfig does as part of an actual
     * write. */
    void flushUiToConfig();

    /* Persist a byte-copy of dev_config_t as the "last device-sync'd"
     * baseline. Called after a successful Read (device -> dev_config_t)
     * and after Write Config flushes its own dev_config_t to the wire.
     * The dirty-state poller compares the live dev_config_t against
     * this snapshot. */
    void snapshotDeviceConfig();
    /* Periodic check (1 Hz via m_dirtyCheckTimer). Flushes the UI
     * into dev_config_t, memcmps against m_deviceConfigSnapshot, and
     * shows/hides label_PendingChanges accordingly. No-op until a
     * snapshot has been taken. */
    void updatePendingChangesBadge();

    /* Returns true if every logical button slot using Function = Logic
     * has its operator and (for binary operators) Source B set to real
     * values rather than the "-" UI sentinels. On false, this also pops
     * a warning naming the first incomplete slot. */
    bool confirmLogicConfigComplete();

    void loadAppConfig();
    void saveAppConfig();
};
#endif // MAINWINDOW_H
