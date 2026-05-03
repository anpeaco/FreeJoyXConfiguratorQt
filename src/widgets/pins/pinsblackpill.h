#ifndef PINBLACKPILL_H
#define PINBLACKPILL_H

/* WeAct STM32F411CEU6 BlackPill V3.x pin host. Mirrors PinsBluePill for
 * Phase 7 -- same 30-slot pin set per the locked F411 pin map (CLAUDE.md
 * + F411_PORT_PLAN.md). The .ui currently reuses BluePill's silkscreen
 * arrangement; a board-faithful BlackPill image + sentinel-marking pass
 * (PB11 not bonded on UFQFPN48; PC14/PC15 LSE-tied) is a follow-up
 * polish task once a real board is in hand. */

#include <QWidget>
#include "pincombobox.h"

QT_BEGIN_NAMESPACE
class QGridLayout;
QT_END_NAMESPACE

namespace Ui {
class PinsBlackPill;
}

class PinsBlackPill : public QWidget
{
    Q_OBJECT

public:
    explicit PinsBlackPill(QWidget *parent = nullptr);
    ~PinsBlackPill();

    void addPinComboBox (QList<PinComboBox *> pinList);

private:
    Ui::PinsBlackPill *ui;
};

#endif // PINBLACKPILL_H
