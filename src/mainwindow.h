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

    void deviceFlasherController(bool isStartFlash);

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
    /* Internal hook: forward HidDevice flashStatus into the progress
     * dialog. Cheap if the dialog isn't open. */
    void onFlashStatusToDialog(int status, int bytes_sent, int bytes_total);

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
    /* True between clicking Write Config and the next deviceConnected.
     * Lets hideConnectDeviceInfo show "Restarting..." instead of
     * "Disconnected" while the chip is re-enumerating after the write,
     * which is a much friendlier signal than the red "Disconnected"
     * pill (the cable wasn't pulled). Cleared on reconnect, on
     * configSent(false), and on natural cable-pull disconnects (the
     * latter via the second hideConnectDeviceInfo call from the main
     * worker loop's empty-list path -- see comment in that slot). */
    bool m_postWriteRestarting = false;

    /* One-click firmware upgrade orchestration (issue
     * anpeaco/FreeJoyXConfiguratorQt#9 Phase B, happy-path only).
     *
     * Flow: backup-then-flash (existing) -> reconnect -> auto-Write
     * (new). Set when the user confirms the Upgrade Firmware dialog;
     * cleared when the post-flash Write completes (success or fail).
     *
     * Auto-rollback (Phase C) deferred until the reverse migrators
     * are hardware-verified and re-enabled. */
    bool    m_upgradePending = false;
    int     m_upgradeBoardId = 0;        /* paramsReport.board_id, captured at upgrade start */
    QString m_upgradeTargetVersion;      /* user-visible version string for dialogs */
    QString m_upgradeFirmwarePath;       /* absolute path to the .bin we'll flash */
    void    refreshUpgradeButtonState(); /* enables/disables based on connection + version + firmware availability */
    QString findUpgradeFirmwarePath(int boardId, QString *outVersion) const;

    /* Issue anpeaco/FreeJoyXConfiguratorQt#19: consolidated flash flow
     * state. m_flashProgress is the modal viewer; the existing
     * m_upgradePending / m_upgradeFirmwarePath fields above are
     * reused for the orchestration since the chain is identical.
     * m_consolidatedFlashActive flags this as a sidebar/Flasher-tab
     * trigger (vs the toolbar Upgrade button) so we only push stage
     * transitions to the dialog when it's actually open. */
    FlashProgressDialog *m_flashProgress = nullptr;
    bool m_consolidatedFlashActive = false;

    /* Whole-window lock applied during a flash chain (manual flasher OR
     * one-click upgrade). Disables everything except the Advanced
     * Settings tab so the user can't wander to other tabs and trigger
     * actions that'd race with the in-flight flash + reconnect.
     * Released on flash failure (onFlashTerminated(false)), post-flash
     * timeout (onPostFlashHealthTimeout), and successful chain
     * completion (getParamsPacket with compatible firmware after the
     * device returns). */
    bool    m_flashChainLocked = false;
    void    setFlashChainUiLocked(bool locked);

    /* True between the moment getParamsPacket schedules the post-upgrade
     * auto-Write and the moment configSent fires its result. Lets us
     * keep the flash-chain UI lock in effect across the auto-Write +
     * device-reset window, instead of unlocking the moment the new
     * firmware comes online. */
    bool    m_postUpgradeWriteInFlight = false;

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

    /* Auto-backup-before-flash. When the user clicks "Enter Flasher Mode"
     * with a recognised device connected, deviceFlasherController() sets
     * this flag and triggers a Read. configReceived() picks up the chain:
     * saves the migrated config to a timestamped backup file in
     * <m_cfgDirPath>/backups/, surfaces the path, then resumes the flasher
     * mode entry (via doEnterFlashMode()). On Read failure, asks the user
     * whether to proceed without a backup. */
    bool m_backupBeforeFlashPending = false;
    QString writeAutoBackup();
    void doEnterFlashMode();
    void doFlashFirmware();

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

    /* Post-flash health check. Starts a timer when the flasher emits
     * flashTerminated(true). Cleared when the device reconnects (a
     * successful firmware boot fires deviceConnected / paramsPacketReceived).
     * If the timer fires without a reconnect, surfaces a "Flash may have
     * failed" dialog pointing the user at the recovery dropdown. */
    QTimer m_postFlashHealthTimer;
    bool m_postFlashHealthPending = false;
    void onFlashTerminated(bool success);
    void onPostFlashHealthTimeout();

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

    /* Returns true if every logical button slot using Function = Logic
     * has its operator and (for binary operators) Source B set to real
     * values rather than the "-" UI sentinels. On false, this also pops
     * a warning naming the first incomplete slot. */
    bool confirmLogicConfigComplete();

    void loadAppConfig();
    void saveAppConfig();
};
#endif // MAINWINDOW_H
