/**
  ******************************************************************************
  * @file           : flashconfirmationdialog.h
  * @brief          : Pre-flash firmware picker + confirmation dialog. The user
  *                   chooses a firmware source from the dropdown (or Browse),
  *                   sees exactly what's about to happen -- which device, which
  *                   firmware, the config-survival outcome -- and commits
  *                   (Flash) or backs out (Cancel).
  *
  *                   Issue anpeaco/FreeJoyXConfiguratorQt#18. Verdict logic
  *                   uses legacy::canMigrate and the FirmwareImage parser
  *                   (#15) to classify the upcoming flash into one of:
  *                   Same generation / Upgrade / Downgrade / Recovery /
  *                   Incompatible. Board-mismatch is a hard refuse -- the
  *                   Flash button stays disabled with an explanatory hint.
  *
  *                   The dialog owns the firmware picker: the host (Flasher)
  *                   populates sources via setSources() and pushes the resolved
  *                   firmware via setResolvedTarget() as selection changes,
  *                   resolving/downloading off the sourceSelected/browseRequested
  *                   signals. The host keeps the FirmwareLibrary + download logic.
  ******************************************************************************
  */

#ifndef FLASHCONFIRMATIONDIALOG_H
#define FLASHCONFIRMATIONDIALOG_H

#include <QColor>
#include <QDialog>
#include <QPair>
#include <QString>
#include <QVariant>
#include <QVector>
#include <cstdint>

#include "firmwareimage.h"

class QPushButton;

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
        None,             /* no firmware selected yet */
    };

    struct Inputs {
        /* Identity of the device the user picked in the sidebar. */
        QString deviceName;     /* product string from HID enumeration */
        QString deviceSerial;   /* device serial (full string) */

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
         * filename out of it for display + verdict computation. */
        const FirmwareImage *image = nullptr;
    };

    /* Picker-driven dialog: constructed with the device identity only. The host
     * populates firmware sources (setSources) and pushes the resolved firmware
     * (setResolvedTarget) as the user picks. */
    FlashConfirmationDialog(const QString &deviceName, const QString &deviceSerial,
                            int deviceBoardId, uint16_t deviceFwVersion,
                            bool deviceInRecoveryMode, QWidget *parent = nullptr);
    ~FlashConfirmationDialog();

    Verdict verdict() const { return m_verdict; }

    /* Populate the source dropdown: each item is (display label, opaque host
     * data). Selecting `selectIndex` does NOT auto-resolve -- the host should
     * resolve currentSourceData() once after calling this. */
    void setSources(const QVector<QPair<QString, QVariant>> &items, int selectIndex = 0);
    QVariant currentSourceData() const;

    /* Host resolved the current selection to a loaded image: render Target +
     * verdict, enable Flash (unless Incompatible). */
    void setResolvedTarget(const FirmwareImage &image);
    /* No usable firmware for the current selection (none picked / load failed):
     * clear Target, disable Flash, show `hint` in the info banner. */
    void clearTarget(const QString &hint);
    /* A remote asset is downloading: disable Flash + picker, show `msg`. */
    void setBusy(const QString &msg);

    /* Pure verdict classifier; also used by MainWindow's consolidated-flash
     * orchestrator to decide post-flash auto-restore. */
    static Verdict computeVerdict(const Inputs &inputs);
    /* True when the verdict permits writing the (possibly-migrated) pre-flash
     * dev_config_t back after re-enumeration. */
    static bool verdictAllowsAutoRestore(Verdict v);

signals:
    /* User changed the source dropdown -- host resolves `data` (a file, or a
     * remote asset to download) and calls setBusy/setResolvedTarget/clearTarget. */
    void sourceSelected(const QVariant &data);
    /* User clicked Browse... -- host runs a file dialog, adds to sources, and
     * re-populates via setSources. */
    void browseRequested();

private:
    Ui::FlashConfirmationDialog *ui;
    Verdict m_verdict = Verdict::None;

    Inputs m_dev;                       /* device identity; image set per selection */
    QPushButton *m_flashBtn = nullptr;
    QWidget *m_infoBanner = nullptr;    /* persistent amber banner (replaced on update) */

    void renderDevice();                /* device pane -- once, from m_dev */
    void renderTargetAndVerdict(const Inputs &in);  /* target pane + verdict + banner */
    void setInfoBanner(const QColor &accent, const QString &html);

    static QString fwVersionLabel(uint16_t fw);
    static QString boardLabel(int boardId);
};

#endif // FLASHCONFIRMATIONDIALOG_H
