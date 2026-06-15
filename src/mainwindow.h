#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
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
#include "gpioexpanderconfig.h"
#include "shiftstimersconfig.h"
#include "loadingoverlay.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QGroupBox;
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
    /* Paint the device Version row from the current paramsReport. Called on
     * every params packet so the FreeJoyX semver (which lands in the second
     * params half, a packet after connect) is shown instead of a stale 0.0.0. */
    void updateVersionLabel(bool firmwareCompatible);

    void configReceived(bool success);
    void configSent(bool success);
    /* Old upstream firmware was read + the bytes translated into the
     * current dev_config_t shape. Surface a message dialog explaining
     * the migration and the next step (Upgrade Firmware on the device
     * panel, which backs up + flashes + writes the migrated config back). */
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
    void on_pushButton_FixWindowsCache_clicked();

    void on_pushButton_SaveToFile_clicked();
    void on_pushButton_LoadFromFile_clicked();

    void on_pushButton_ShowDebug_clicked();

    void on_pushButton_TestButton_clicked();
    void on_pushButton_TestButton_2_clicked();

    void on_pushButton_Wiki_clicked();

    void themeChanged(bool dark);

    /* Slot for AdvancedSettings::saveDirectoryChanged. Updates m_cfgDirPath
     * and refreshes the configs combo. The Advanced tab's "Default save
     * directory" surface is the only place this gets changed at runtime now. */
    void applySaveDirectoryChange(const QString &path);

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

    freejoy_ui::LoadingOverlay *m_loadingOverlay = nullptr;  // modal mask during read/write

    QThread *m_threadGetSendConfig;

    PinConfig *m_pinConfig;
    ButtonConfig *m_buttonConfig;
    ShiftsTimersConfig *m_shiftsTimersConfig;
    LedConfig *m_ledConfig;
    EncodersConfig *m_encoderConfig;
    ShiftRegistersConfig *m_shiftRegConfig;
    GpioExpanderConfig *m_gpioExpConfig;
    QGroupBox *m_shiftRegGroup = nullptr;   // "Shift Registers" group on the SR tab
    QGroupBox *m_expanderGroup = nullptr;   // "Port Expanders" group on the SR tab
    AxesConfig *m_axesConfig;
    AxesCurvesConfig *m_axesCurvesConfig;
    AdvancedSettings *m_advSettings = nullptr;

    DebugWindow *m_debugWindow = nullptr;
    bool m_debugIsEnable;

    /* Active theme, tracked so the App-card theme toggle (toolButton_ThemeToggle)
     * can flip to the opposite theme and paint the matching sun/moon glyph.
     * Updated by themeChanged(). */
    bool m_darkThemeActive = true;

    /* Board id currently shown on the Device card's Board row (0 = none/unknown).
     * Cached so themeChanged() can re-render the rich-text board label -- the
     * F411 CPU-icon ink tracks the theme. */
    int m_deviceCardBoardId = 0;

    /* Packet stats shown in the Device info card (relocated out of the debug
     * pane so they're visible whenever a device is connected). m_packetsReceived
     * counts USB reports since connect; the rate is the mean inter-packet time
     * sampled over a 5 s window. Reset on (dis)connect. */
    int m_packetsReceived = 0;
    int m_packetRateSamples = 0;
    QElapsedTimer m_packetRateTimer;
    void onDevicePacketReceived();
    void resetPacketStats();

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
    // True only in the normal compatible-connected state (green pill) -- gates
    // the "Connected • unsaved" amber decoration the dirty poll applies. False
    // for disconnected / restarting / legacy / incompatible / flasher.
    bool       m_deviceConnectedOk = false;
    bool       m_pillUnsaved = false;   // last-applied pill dirty decoration (transition guard)

    /* Guards the 1 Hz dirty poll (updatePendingChangesBadge ->
     * uiHasUnsavedDeviceEdits -> flushUiToConfig) from firing while a
     * config load is mid-flight. A load stages freshly-read bytes into
     * dev_config_t and only renders them via UiReadFromConfig() once the
     * load returns. If the load opens a modal (e.g. the cross-board
     * convert prompt) its nested event loop lets the timer tick -- and
     * flushUiToConfig would then write the still-stale UI back over the
     * just-loaded config, blanking it. Set across the whole
     * reset+load+UiReadFromConfig sequence at every file-load entry point. */
    bool       m_configLoadInProgress = false;

    /* True from the moment a device Read is kicked (on_pushButton_ReadConfig_clicked)
     * until configReceived() lands. The worker thread fills dev_config_t in place
     * as fragments arrive, so the 1 Hz dirty poll must not flush the (still-stale)
     * UI over it or compare against the half-filled struct -- otherwise the
     * Write-config "pending changes" dot flickers on during the read. Cleared at
     * the top of configReceived() (which the worker always emits, success or
     * fail) so it can't stick. Distinct from m_configLoadInProgress, which is a
     * synchronous RAII guard for file loads. */
    bool       m_deviceReadInProgress = false;

    /* Auto-read-on-connect: when true (default), a compatible device
     * connecting triggers an automatic Read of its stored config into the
     * UI. Gated by the dirty check (prompt before discarding unsaved edits),
     * suppressed during post-write re-enumeration and active flash sessions.
     * Persisted under OtherSettings/AutoReadOnConnect; toggled from the
     * Advanced Settings tab via AdvancedSettings::autoReadOnConnectChanged. */
    bool m_autoReadOnConnect = true;

    /* Suppress auto-read-on-connect briefly after a flash session ends. A
     * freshly-flashed device (especially F411) can re-enumerate several times
     * as it settles; by the 2nd/3rd reconnect the session is already Done
     * (isActive() false) and the post-write-restart flag has been cleared, so
     * without this the auto-read prompt would wrongly fire over a device the
     * flash flow already restored. Set in onFlashSessionFinished, cleared by a
     * short single-shot timer. */
    bool m_suppressAutoReadAfterFlash = false;

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

    /* The .ui's default label + tooltip for pushButton_UpgradeFirmware,
     * captured at construction. refreshUpgradeButtonState swaps in an
     * "Install firmware" label while a board sits in the bootloader (flasher
     * mode), then restores these for an app-mode device. */
    QString m_upgradeBtnDefaultText;
    QString m_upgradeBtnDefaultToolTip;

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

    /* GUI-thread responsiveness probe, active only during a flash session.
     * A 100ms timer on the GUI thread records the longest gap between its own
     * ticks -- i.e. the longest stretch the GUI event loop did NOT run (a
     * "Not Responding" stall). Reported once at flash end. Diagnoses whether the
     * in-process HID flash actually freezes the UI or whether the grey/"Not
     * Responding" on click is just Windows ghosting a momentarily-busy window. */
    QTimer        m_flashHeartbeat;
    QElapsedTimer m_flashHeartbeatGap;
    qint64        m_flashMaxStallMs = 0;

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

    /* Last directory the user actually picked in a Load/Save Config file
     * dialog -- separate from m_cfgDirPath (the "home" the Advanced tab
     * owns) so the home stays the user's intentional setting while the
     * dialogs track where they last navigated. Persisted in QSettings as
     * [Configs]/LastUsedPath. lastUsedSaveDir() returns it if set and the
     * directory still exists, otherwise falls back to m_cfgDirPath. */
    QString lastUsedSaveDir() const;
    void setLastUsedSaveDir(const QString &filePath);

    /* Save a snapshot of the device's current config to
     * <m_cfgDirPath>/backups/. Called from configReceived's
     * m_flashSessionBackupPending branch as the BackingUp step of a
     * FlashSession. */
    QString writeAutoBackup();

    /* Build a backups/ path of the form
     * <prefix>-<deviceName>-<serial>-<YYYYMMDD-HHMMSS>.cfg. Shared by the
     * pre-flash (writeAutoBackup) and pre-write (writePreWriteDeviceBackup)
     * backups so every backup is identifiable by board + time. */
    QString makeBackupPath(const QString &prefix);

    /* Worker-thread shims. doEnterFlashMode sends the "bootloader run"
     * report; doFlashFirmwareBytes streams the .bin to the bootloader's
     * REPORT_ID_FLASH receiver. Both block on m_threadGetSendConfig
     * via QEventLoop until the underlying HID exchange returns. */
    void doEnterFlashMode();
    /* Worker-thread shim for HidDevice::enterToSystemDfu -- sends the
     * "system dfu" report so a live device reboots into ROM USB DFU for a
     * jumper-free reinstall (anpeaco/FreeJoyX#55). Mirrors doEnterFlashMode's
     * QEventLoop-on-m_threadGetSendConfig model. */
    void doEnterSystemDfu();
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

    /* Refill the config dropdown from the current save directory without
     * disturbing the selection logic (signal-blocked clear + addItems). One
     * place for the repopulate used at startup, after Save, and on dir change. */
    void repopulateConfigDropdown();

    /* Load a .cfg from disk into dev_config_t and render it: reset, read file,
     * fan out to the UI, all bracketed by m_configLoadInProgress so the dirty
     * poller doesn't clobber the staged load. Shared by the dropdown-pick and
     * the Load-from-file button. */
    void loadConfigFromFile(const QString &filePath);
    QIcon pixmapToIcon(QPixmap pixmap, const QColor &color);
    void updateColor();

    // resetDirtyBaseline: true for a DEVICE sync (read), so the "pending changes"
    // baseline is reset to match the device; false for a FILE load / reset-to-
    // default, which must stay dirty vs what's on the device.
    void UiReadFromConfig(bool resetDirtyBaseline = true);
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
    // Set the device-card Board row: tinted CPU icon in label_BoardIcon (hidden
    // for unknown) + name in label_BoardVal, both centred so they align with the
    // "Board:" key. boardId 0/unknown shows the em dash with no icon.
    void setDeviceCardBoard(int boardId);

    /* True when the live UI config differs from the last device-sync
     * snapshot (m_deviceConfigSnapshot) -- i.e. the user has unsaved edits
     * that overwriting the UI would discard. False when no snapshot has been
     * taken yet this session (a fresh session has nothing to lose). Flushes
     * the UI into dev_config_t. Shared by updatePendingChangesBadge() and the
     * auto-read-on-connect dirty gate. */
    bool uiHasUnsavedDeviceEdits();

    /* Auto-read-on-connect entry point. Called once per new device from
     * getParamsPacket() after firmware compatibility is determined. Honours
     * m_autoReadOnConnect, skips during post-write restart / active flash
     * sessions / unreadable firmware, and prompts before discarding unsaved
     * edits. On go-ahead, triggers the same Read path as the Read Config
     * button (worker reads async; configReceived splashes + resnapshots).
     *
     * hadUnsavedEdits must be sampled by the caller BEFORE getParamsPacket's
     * programmatic board switch (setConnectedBoard) mutates the in-memory
     * config -- otherwise a cross-board swap's pin migration reads as a user
     * edit and the "keep my edits?" prompt fires with no real changes. */
    void maybeAutoReadOnConnect(bool firmwareCompatible, uint16_t deviceVersion,
                                bool postWriteRestart, bool hadUnsavedEdits);

    /* Returns true if every logical button slot using Function = Logic
     * has its operator and (for binary operators) Source B set to real
     * values rather than the "-" UI sentinels. On false, this also pops
     * a warning naming the first incomplete slot. */
    bool confirmLogicConfigComplete();

    void loadAppConfig();
    void saveAppConfig();
};
#endif // MAINWINDOW_H
