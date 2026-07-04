#ifndef ENCODERS_H
#define ENCODERS_H

#include <QWidget>
#include <QVector>
#include <QPair>
#include <QSet>

#include "deviceconfig.h"
#include "global.h"

#define ENCODER_TYPE_COUNT 3
namespace Ui {
class Encoders;
}
class CenteredCBox;

// One slow (software) encoder row on the Encoders tab. Holds an explicit
// Pin A / Pin B selection (chosen from the buttons tagged "Encoder"), a detent
// mode (1x/2x/4x) and a direction-swap toggle. Pairs are stored in
// dev_config.slow_encoders[configIndex]; mode + swap in dev_config.encoders[].
class Encoders : public QWidget
{
    Q_OBJECT

public:
    explicit Encoders(int encodersNumber, QWidget *parent = nullptr);
    ~Encoders();

    void retranslateUi();

    int configIndex() const { return m_configIndex; }

    // (buttonSlotIndex, displayName) for every pin tagged "Encoder". Repopulates
    // the Pin A / Pin B combos, preserving the current selection where possible.
    void setEncoderButtons(const QVector<QPair<int, QString>> &buttons);

    int inputA() const;   // selected button-slot index, -1 = none
    int inputB() const;

    // Exchange the Pin A / Pin B selections (reverses direction). Bound to the
    // Swap button; persists and re-validates like a user edit.
    void swapInputs();

    // Grey out (disable) every button in `usedButtons` in both Pin combos,
    // except each combo's own current pick -- so a pin already assigned to an
    // encoder can't be picked twice.
    void applyUsageMask(const QSet<int> &usedButtons);

    // Red-border the Pin A / Pin B combos when they clash (A==B, or the pin is
    // also used by another encoder).
    void setInputClash(bool aClash, bool bClash);

    void readFromConfig();
    void writeToConfig();

signals:
    // User changed A / B / swap / type -> the container re-validates (clash
    // highlight) and persists.
    void pairingEdited();

private slots:
    void onUserEdited();

private:
    void updateEnabledState();
    void fillCombo(CenteredCBox *combo, const QVector<QPair<int, QString>> &buttons, int keepSel);
    static void selectData(CenteredCBox *combo, int slotIndex);

    Ui::Encoders *ui;

    int m_encodersNumber;   // 1-based display index
    int m_configIndex;      // index into dev_config encoders[] / slow_encoders[]
    bool m_populating = false;   // guard: suppress signals during programmatic fill

    const deviceEnum_guiName_t m_encoderTypeList[ENCODER_TYPE_COUNT] = // order MUST match common_types.h!
    {
        {ENCODER_CONF_1x, tr("Encoder 1x")},
        {ENCODER_CONF_2x, tr("Encoder 2x")},
        {ENCODER_CONF_4x, tr("Encoder 4x")},
    };
};

#endif // ENCODERS_H
