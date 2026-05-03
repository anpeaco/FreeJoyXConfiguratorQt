#include "pinsblackpill.h"
#include "ui_pinsblackpill.h"
#include <QComboBox>

PinsBlackPill::PinsBlackPill(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PinsBlackPill)
{
    ui->setupUi(this);

    ui->label_ControllerImage->contentsMargins();
}

PinsBlackPill::~PinsBlackPill()
{
    delete ui;
}

/* Phase 7 option B: F411 BlackPill silkscreen labels pin slot 22 as
 * "B2" (because PB11 isn't bonded on the UFQFPN48 package; PB2 is bonded
 * at the same physical position). The wire-format slot 22 still carries
 * the cross-board PinComboBox whose objectName is "B11" -- one shared
 * combobox list serves all board widgets. So when we encounter a label
 * named "B2" in the BlackPill .ui, alias it to "B11" before matching. */
static QString pinAliasForBlackPill(const QString &labelText)
{
    if (labelText == QLatin1String("B2")) {
        return QStringLiteral("B11");
    }
    return labelText;
}

void PinsBlackPill::addPinComboBox(QList<PinComboBox *> pinList)
{
    QElapsedTimer timer;
    timer.start();
    int tmp = 0;
    int labelColumn;
    int pinsColumn;
    // left layout
    labelColumn = 1;
    pinsColumn = 0;
    QComboBox *prevCBox = nullptr;
    for (int i = 0; i < ui->layoutG_pinsLeft->rowCount(); ++i) {
        // continue if no label or pin row not empty
        if (!ui->layoutG_pinsLeft->itemAtPosition(i, labelColumn) || ui->layoutG_pinsLeft->itemAtPosition(i, pinsColumn)) {
            continue;
        }
        // search label in column 1
        QLabel *label = qobject_cast<QLabel *>(ui->layoutG_pinsLeft->itemAtPosition(i, labelColumn)->widget());
        if (label) {
            const QString matchName = pinAliasForBlackPill(label->text());
            // add widget to column 0
            for (int k = 0; k < pinList.size(); ++k) {
                if (pinList[k]->objectName() == matchName) {
                    ui->layoutG_pinsLeft->addWidget(pinList[k], i, pinsColumn);
                    tmp++;
                    // tab order
                    QComboBox *child = pinList[k]->findChild<QComboBox*>();
                    if (!prevCBox) {
                        child->setTabOrder(this, child);
                        prevCBox = child;
                    } else {
                        child->setTabOrder(prevCBox, child);
                        prevCBox = child;
                    }
                    break;
                }
            }
        }
    }
    // right layout
    labelColumn = 0;
    pinsColumn = 1;
    for (int i = 0; i < ui->layoutG_pinsRight->rowCount(); ++i) {
        // continue if no label or pin row not empty
        if (!ui->layoutG_pinsRight->itemAtPosition(i, labelColumn) || ui->layoutG_pinsRight->itemAtPosition(i, pinsColumn)) {
            continue;
        }
        // search label in column 0
        QLabel *label = qobject_cast<QLabel *>(ui->layoutG_pinsRight->itemAtPosition(i, labelColumn)->widget());
        if (label) {
            const QString matchName = pinAliasForBlackPill(label->text());
            // add widget to column 1
            for (int k = 0; k < pinList.size(); ++k) {
                if (pinList[k]->objectName() == matchName) {
                    ui->layoutG_pinsRight->addWidget(pinList[k], i, pinsColumn);
                    tmp++;
                    // tab order
                    QComboBox *child = pinList[k]->findChild<QComboBox*>();
                    if (!prevCBox) {
                        child->setTabOrder(this, child);
                        prevCBox = child;
                    } else {
                        child->setTabOrder(prevCBox, child);
                        prevCBox = child;
                    }
                    break;
                }
            }
        }
    }
    Q_ASSERT(tmp == PINS_COUNT);
}
