#ifndef BUTTONCONFIG_H
#define BUTTONCONFIG_H

#include <QFrame>
#include <QWidget>

#include "buttonlogical.h"
#include "buttonphysical.h"
#include "physref.h"

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

    /* Walks all logical button rows and returns the slot index of the
     * first one whose LOGIC configuration is incomplete (operator left
     * at "-" or, for binary ops, Source B left at "-"). Returns -1 when
     * every row is fine. MainWindow uses this to gate Write-to-device
     * and Save-to-file. */
    int firstIncompleteLogicSlot() const;

    /* Begin / end markers for a config load. Wraps UiReadFromConfig in
     * MainWindow so the auto-remap (which fires off the same breakdown
     * signals that user pin / SR / a2b edits do) can distinguish a
     * fresh load (just snapshot the new breakdown) from a user edit
     * (remap dev_config_t against the previous snapshot). */
    void beginConfigLoad();
    void endConfigLoad();

    /* Persist the live physical-button breakdown into
     * dev_config_t.saved_breakdown. Called from MainWindow::UiWriteToConfig
     * just before serialisation so the next load can detect breakdown
     * drift and remap stale physical_num references via freejoy::toRef /
     * toAbs. Configurator-only metadata; firmware ignores the field. */
    void captureBreakdownToConfig();

    // PhysBreakdown / PhysRef / toRef / toAbs live in physref.h now.
    using PhysBreakdown = freejoy::PhysBreakdown;

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
    /* Step 4 coexistence rule: when any row binds LONG_PRESS or DOUBLE_TAP
     * to a physical input, sister rows on the same physical may host only
     * { NORMAL, LONG_PRESS, DOUBLE_TAP }; conversely, when a sister row uses
     * any other type, gesture types are hidden from rows on that physical.
     * Reruns from both functionTypeChanged() and ButtonLogical's
     * physicalNumChanged signal so type and physical-num edits both retrigger
     * the per-row dropdown filter. */
    void physicalConflictFilter();

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

    /* Toolbar "Clear All" button: confirms with the user, then resets
     * every dev_config_t.buttons[] slot to defaults (physical_num = -1,
     * type = NORMAL, src_b = -1, all flags / timers / shift / op zero)
     * and refreshes every ButtonLogical row from the cleared cfg. Pins,
     * axes, shift register and timer config are deliberately left
     * untouched. */
    void on_pushButton_ClearAllLogical_clicked();

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

    /* Snapshot of the breakdown at the moment of the last load (or
     * the previous remap). The PhysBreakdown type itself is declared
     * in the public section above. */
    PhysBreakdown m_lastBreakdown;
    bool m_breakdownInitialized = false;
    bool m_loadInProgress = false;

    PhysBreakdown currentBreakdown() const;

    /* Walk dev_config_t.buttons[] and translate every physical_num and
     * src_b from the `oldB` breakdown to the `newB` breakdown via
     * freejoy::toRef + toAbs. References whose category disappears in
     * `newB` become -1 and are surfaced via a single QMessageBox at the
     * end. Used by both the in-session auto-remap (setUiOnOff path) and
     * the load-time remap (endConfigLoad path against
     * dev_config_t.saved_breakdown). */
    void remapBreakdown(const PhysBreakdown &oldB, const PhysBreakdown &newB);

    /* Called once per setUiOnOff. If the breakdown has changed since the
     * last snapshot AND we're not mid-load, remap dev_config_t.buttons[]
     * physical_num + src_b references and refresh affected UI rows. */
    void maybeRemap();

    /* Read the configurator-only saved_breakdown metadata back out of
     * dev_config_t. Called by endConfigLoad to detect drift between
     * when the chip's config was last written (saved breakdown) and
     * the live breakdown derived from the just-loaded pin/SR/a2b
     * widgets. The public counterpart, captureBreakdownToConfig, is
     * declared above next to beginConfigLoad / endConfigLoad. */
    PhysBreakdown breakdownFromConfig() const;

    int m_logicButtonInFocus;
    bool m_autoPhysButEnabled;

    void logicaButtonsCreator();

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
    // typeLimit's count and "type is at cap" tracking. Shared with
    // physicalConflictFilter so both rules can be applied in a single pass.
    int  m_typeLimitCount[m_typeLimCount]{};
    bool m_typeAtCap[m_typeLimCount]{};
};

#endif // BUTTONCONFIG_H
