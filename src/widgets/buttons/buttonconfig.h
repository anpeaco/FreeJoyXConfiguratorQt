#ifndef BUTTONCONFIG_H
#define BUTTONCONFIG_H

#include <QFrame>
#include <QWidget>

#include "buttonlogical.h"
#include "buttonphysical.h"

#include "common_defines.h"
#include "common_types.h"

#include "deviceconfig.h"
#include "global.h"

//#define DYNAMIC_CREATION
#ifdef DYNAMIC_CREATION
    //#define DYNAMIC_CREATION_ALL
#endif
namespace Ui {
class ButtonConfig;
}

class ButtonConfig : public QWidget
{
    Q_OBJECT

public:
    explicit ButtonConfig(QWidget *parent = nullptr);
    ~ButtonConfig();

    void readFromConfig();
    void writeToConfig();
    void buttonStateChanged();

    void retranslateUi();

    /* Move the button at slot `from` into slot `to`, shifting the rows
     * in between. Pure data shuffle on dev_config_t.buttons[] -- no
     * remap of physical_num / src_b references in other rows. Refreshes
     * affected ButtonLogical widgets. Called from the drop handler. */
    void moveButton(int from, int to);

signals:
    void encoderInputChanged(int ecoder_A, int ecoder_B);
    void logicalButtonsCreated();

public slots:
    void setUiOnOff(int value);
    /* Receives the per-source totals that back the section headers in
     * the physical button display. Called by mainwindow's wiring just
     * before setUiOnOff (which triggers the rebuild). */
    void onPhysicalButtonBreakdownChanged(int matrix, int shiftRegs, int axes, int direct);
    /* Per-register / per-axis breakdowns. Index = register or axis
     * number, value = button count. When a non-empty breakdown is
     * available we sub-divide the corresponding section in the button
     * grid (e.g. "Shift register 1" + "Shift register 2" rather than a
     * combined "Shift registers"). */
    void onShiftRegBreakdownChanged(const QList<int> &perRegister);
    void onA2bBreakdownChanged(const QList<int> &perAxis);

private slots:
    void functionTypeChanged(button_type_t current, button_type_t previous, int buttonIndex);
    void setPhysicButton(int buttonIndex);
    void typeLimit(button_type_t current, button_type_t previous);

#ifdef DYNAMIC_CREATION
private slots:
    void logScrollValueChanged(int value);
    void createLogButtons(int count);
protected:
    void resizeEvent(QResizeEvent *event) override;
#endif

protected:
    /* Receives drag/drop events on scrollAreaWidgetContents (which we
     * setAcceptDrops on in the ctor). Source MIME identifies the row
     * being dragged; the cursor Y position picks the target slot. */
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    /* Cursor Y -> target slot index. Drop above row N's vertical
     * midline means "land at slot N" (row N and below shift down by 1). */
    int targetSlotForY(int yPos) const;

    /* Cursor Y -> Y coord (in scrollAreaWidgetContents space) where the
     * drop indicator line should be drawn -- right at the gap the row
     * will land in. Different from targetSlotForY only at the very
     * bottom (drops below the last row land at slot n-1 but the
     * indicator goes below the row, not at its top). */
    int indicatorYForCursor(int yPos) const;

    /* Reposition + show the drop-indicator line at the given Y coord,
     * sized to the contents widget's full width. */
    void showDropIndicatorAtY(int y);

    QFrame *m_dropIndicator;

private slots:
    void on_checkBox_AutoPhysBut_toggled(bool checked);

private:
    Ui::ButtonConfig *ui;
    void physButtonsCreator(int count);

    /* Compute the source-category breakdown of the current dev_config. Returns
     * an ordered list of {label, count} mirroring the firmware order
     * (matrix -> shift register -> axis-to-buttons -> direct). Used by
     * physButtonsCreator to insert section headers between groups. */
    struct ButtonGroup {
        QString label;
        int     count;
    };
    QList<ButtonGroup> computeButtonGroups();

    QString m_defaultShiftStyle;

    /* Latest per-source breakdown of the physical button count, pushed in
     * from CurrentConfig via PinConfig. Used by computeButtonGroups()
     * instead of reading dev_config -- some sources (e.g. axis-to-buttons)
     * update their UI count before writing to dev_config, so dev_config
     * lags behind the live UI state. */
    int m_groupMatrix    = 0;
    int m_groupShiftRegs = 0;
    int m_groupAxes      = 0;
    int m_groupDirect    = 0;
    QList<int> m_shiftRegBreakdown;	// per-register button counts (empty until first emit)
    QList<int> m_axisBreakdown;		// per-axis a2b button counts (empty until first emit)

    int m_logicButtonInFocus;
    bool m_autoPhysButEnabled;

    bool m_isShifts_act;
    bool m_shift1_act;
    bool m_shift2_act;
    bool m_shift3_act;
    bool m_shift4_act;
    bool m_shift5_act;

    void logicaButtonsCreator();

    void spinBoxStep(int value);

    QList<ButtonLogical *> m_logicButtonPtrList;
    QList<ButtonPhysical *> m_PhysButtonPtrList;

    struct pinTypeLimit_t
    {
        button_type_t type;
        int maxCount;
    };

    static const int m_typeLimCount = 2;
    const pinTypeLimit_t m_ButtonsTypeLimit[m_typeLimCount] =
    {
        {ENCODER_INPUT_A,        15},
        {ENCODER_INPUT_B,        15},
    };
};

#endif // BUTTONCONFIG_H
