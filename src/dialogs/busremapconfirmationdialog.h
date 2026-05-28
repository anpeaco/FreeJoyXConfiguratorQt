/**
  ******************************************************************************
  * @file           : busremapconfirmationdialog.h
  * @brief          : Confirmation shown when enabling an I2C / SPI bus via the
  *                   Pin Info quick-setup toggle would overwrite pins the user
  *                   has already mapped to something else. Lists each affected
  *                   pin and its current role, and lets the user commit
  *                   (Remap & enable) or back out (Cancel).
  *
  *                   Custom QDialog rather than QMessageBox so the affected-pin
  *                   list renders the same way the flash-confirmation dialog
  *                   presents its step list, and so the destructive-action
  *                   accent matches the rest of the configurator.
  ******************************************************************************
  */

#ifndef BUSREMAPCONFIRMATIONDIALOG_H
#define BUSREMAPCONFIRMATIONDIALOG_H

#include <QDialog>
#include <QString>
#include <QVector>

namespace Ui {
class BusRemapConfirmationDialog;
}

class BusRemapConfirmationDialog : public QDialog
{
    Q_OBJECT

public:
    /* One pin that will be reassigned: its display name (e.g. "PB10") and the
     * human-readable role it currently holds (e.g. "Button (to GND)"). */
    struct Conflict {
        QString pinName;
        QString currentRole;
    };

    /* actionName is the user-facing label for the thing being enabled ("I2C
     * bus", "SPI bus", "Fast Encoder 1", ...); conflicts is the non-empty list
     * of pins that would be overwritten. brokenLogicalSlots are the 1-based
     * button slots a confirmed remap would clear (empty when none); when
     * non-empty the dialog adds a "will also clear logical buttons N" note so
     * the user sees the full consequence in one place. The caller only opens
     * this dialog when there is at least one conflict. */
    explicit BusRemapConfirmationDialog(const QString &actionName,
                                        const QVector<Conflict> &conflicts,
                                        const QVector<int> &brokenLogicalSlots,
                                        QWidget *parent = nullptr);
    ~BusRemapConfirmationDialog();

private:
    Ui::BusRemapConfirmationDialog *ui;
};

#endif // BUSREMAPCONFIRMATIONDIALOG_H
