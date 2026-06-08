#include "pinsbluepill.h"
#include "ui_pinsbluepill.h"
#include <QComboBox>

#include "imageaspectlock.h"

PinsBluePill::PinsBluePill(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PinsBluePill)
{
    ui->setupUi(this);

    /* Keep the board render's aspect ratio when the window is shrunk vertically
     * (its height is driven by the pin rows). Without this, scaledContents
     * stretches the board out of shape. */
    freejoy_ui::lockImageAspect(ui->label_ControllerImage);
}

PinsBluePill::~PinsBluePill()
{
    delete ui;
}

void PinsBluePill::addPinComboBox(QList<PinComboBox *> pinList)
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
            // add widget to column 0
            for (int k = 0; k < pinList.size(); ++k) {
                if (pinList[k]->objectName() == label->text()) {// clone pinList and remove item if addWidget?
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
            // add widget to column 1
            for (int k = 0; k < pinList.size(); ++k) {
                if (pinList[k]->objectName() == label->text()) {// clone pinList and remove item if addWidget?
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

    /* Uniform height for every pin line -- the fixed-pin rows (5V / GND / etc.)
     * carry only a label and would otherwise sit shorter than the dropdown rows.
     * Pin every label-bearing row to the dropdown height and drop row stretch. */
    const int rowH = pinList.isEmpty() ? 0 : pinList.first()->minimumHeight();
    if (rowH > 0) {
        const struct { QGridLayout *grid; int labelCol; } grids[] = {
            { ui->layoutG_pinsLeft, 1 }, { ui->layoutG_pinsRight, 0 } };
        for (const auto &g : grids) {
            for (int r = 0; r < g.grid->rowCount(); ++r) {
                if (g.grid->itemAtPosition(r, g.labelCol)) {
                    g.grid->setRowMinimumHeight(r, rowH);
                    g.grid->setRowStretch(r, 0);
                }
            }
        }
    }
}
