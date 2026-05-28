/**
  ******************************************************************************
  * @file           : busremapconfirmationdialog.cpp
  * @brief          : See busremapconfirmationdialog.h.
  ******************************************************************************
  */

#include "busremapconfirmationdialog.h"
#include "ui_busremapconfirmationdialog.h"

#include <QPushButton>
#include <QStringList>

#include "style_helpers.h"

BusRemapConfirmationDialog::BusRemapConfirmationDialog(const QString &actionName,
                                                       const QVector<Conflict> &conflicts,
                                                       const QVector<int> &brokenLogicalSlots,
                                                       QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BusRemapConfirmationDialog)
{
    ui->setupUi(this);

    ui->label_Headline->setText(tr("Enable %1?").arg(actionName));

    const QString bannerText =
        tr("Enabling %1 will reassign %n pin(s) that are already mapped. "
           "Their current functions will be cleared.", "", conflicts.size())
            .arg(actionName);
    // Light-yellow warning note with a yellow border and the Lucide
    // triangle-alert icon -- the shared warning-banner look used app-wide.
    ui->label_Banner->setText(freejoy_style::warningIconHtml()
                              + QStringLiteral("&nbsp;") + bannerText);
    ui->label_Banner->setStyleSheet(freejoy_style::warningBannerQss());

    /* Bulleted "PIN -- current role" list, rendered like the flash dialog's
     * step list so the two confirmation surfaces read consistently. */
    QStringList rows;
    for (const Conflict &c : conflicts) {
        rows << QStringLiteral("<b>%1</b> &mdash; %2")
                    .arg(c.pinName.toHtmlEscaped(), c.currentRole.toHtmlEscaped());
    }
    ui->label_Affected->setText(QStringLiteral("<ul style='margin-left:-20px'><li>")
        + rows.join(QStringLiteral("</li><li>"))
        + QStringLiteral("</li></ul>"));

    /* If the displaced pins feed logical buttons, those references get cleared
     * too -- spell out the slots so the user sees the whole consequence here
     * rather than in a second popup afterwards. Hidden when nothing breaks. */
    if (brokenLogicalSlots.isEmpty()) {
        ui->label_Logical->hide();
    } else {
        QStringList nums;
        for (int s : brokenLogicalSlots) {
            nums << QString::number(s);
        }
        ui->label_Logical->setText(
            tr("This will also clear logical button(s) %1, which reference these "
               "inputs. Reassign them before saving.", "", brokenLogicalSlots.size())
                .arg(nums.join(QStringLiteral(", "))));
    }

    /* Accept button is explicit about the destructive part of the action;
     * Cancel stays the default so an accidental Enter doesn't remap. */
    QPushButton *okBtn = ui->buttonBox->addButton(
        tr("Remap && enable"), QDialogButtonBox::AcceptRole);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);

    QPushButton *cancelBtn = ui->buttonBox->button(QDialogButtonBox::Cancel);
    if (cancelBtn) {
        cancelBtn->setDefault(true);
        cancelBtn->setFocus();
    }
}

BusRemapConfirmationDialog::~BusRemapConfirmationDialog()
{
    delete ui;
}
