/**
  ******************************************************************************
  * @file           : flashconfirmationdialog.h
  * @brief          : Pre-flash confirmation dialog. Shows the user exactly
  *                   what's about to happen -- which device, which firmware,
  *                   what the config-survival outcome will be -- and lets
  *                   them commit (Flash) or back out (Cancel).
  *
  *                   Issue anpeaco/FreeJoyXConfiguratorQt#18. Verdict logic
  *                   uses legacy::canMigrate and the FirmwareImage parser
  *                   (#15) to classify the upcoming flash into one of:
  *                   Same generation / Upgrade / Downgrade / Recovery /
  *                   Incompatible. Board-mismatch is a hard refuse -- the
  *                   Flash button stays disabled with an explanatory hint.
  ******************************************************************************
  */

#ifndef FLASHCONFIRMATIONDIALOG_H
#define FLASHCONFIRMATIONDIALOG_H

#include <QDialog>
#include <QString>
#include <cstdint>

#include "firmwareimage.h"

namespace Ui {
class FlashConfirmationDialog;
}

class FlashConfirmationDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Verdict {
        SameGeneration,   /* wire-format identical; config will be preserved */
        Upgrade,          /* target newer; migrator exists; config will be migrated */
        UpgradeNoMigrator,/* target newer; no migrator; config will be lost */
        Downgrade,        /* target older; refuse auto-restore; factory-reset */
        Recovery,         /* device already in bootloader; no backup possible */
        Incompatible,     /* board mismatch; Flash disabled */
    };

    struct Inputs {
        /* Identity of the device the user picked in the sidebar. */
        QString deviceName;     /* product string from HID enumeration */
        QString deviceSerial;   /* device serial (full string; dialog renders last 6) */

        /* paramsReport.firmware_version captured before any flasher
         * transitions. Zero means "not known" -- a stuck-in-bootloader
         * device, or a slot loaded from disk without device contact. */
        uint16_t deviceFwVersion = 0;

        /* BOARD_ID_F103_BLUEPILL / BOARD_ID_F411_BLACKPILL, or 0 when
         * not known. Drives the board-match check against image. */
        int deviceBoardId = 0;

        /* True when the configurator sees this device only in flasher
         * mode -- backup is impossible, restore is skipped. Drives the
         * Recovery verdict and the abbreviated step list. */
        bool deviceInRecoveryMode = false;

        /* Parsed target firmware. The dialog reads board / fwVersion /
         * filename out of it for display + verdict computation; the
         * caller keeps ownership of the underlying bytes. */
        const FirmwareImage *image = nullptr;
    };

    explicit FlashConfirmationDialog(const Inputs &inputs, QWidget *parent = nullptr);
    ~FlashConfirmationDialog();

    Verdict verdict() const { return m_verdict; }
    bool flashEnabled() const { return m_verdict != Verdict::Incompatible; }

    /* Compute the verdict from the inputs. Pure function -- depends only
     * on the captured inputs, not on any external state, so it's
     * straightforward to unit-test once test scaffolding lands.
     * Exposed for MainWindow's consolidated-flash orchestrator
     * (issue anpeaco/FreeJoyXConfiguratorQt#20), which needs to decide
     * whether to run the post-flash auto-restore based on whether the
     * verdict permits writing the backup back unchanged. */
    static Verdict computeVerdict(const Inputs &inputs);

    /* True when the verdict permits writing the (possibly-migrated)
     * pre-flash dev_config_t back to the device after re-enumeration.
     * False for Downgrade / UpgradeNoMigrator / Recovery /
     * Incompatible -- those cases factory-reset the device. */
    static bool verdictAllowsAutoRestore(Verdict v);

private:
    Ui::FlashConfirmationDialog *ui;
    Verdict m_verdict = Verdict::Incompatible;

    /* Render the verdict line + step list + warning into the form. The
     * verdict drives both visible content and the Flash button enable
     * state. */
    void renderForVerdict(const Inputs &inputs);

    static QString fwVersionLabel(uint16_t fw);
    static QString shortSerial(const QString &full);
    static QString boardLabel(int boardId);
};

#endif // FLASHCONFIRMATIONDIALOG_H
