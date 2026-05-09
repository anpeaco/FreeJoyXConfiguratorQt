#ifndef AXES_H
#define AXES_H

#include <QHash>
#include <QMap>
#include <QStringList>
#include <QWidget>
#include "axesextended.h"
#include "axestobuttonsslider.h"

#include "deviceconfig.h"
#include "global.h"

const QVector <deviceEnum_guiName_t> &axesList();

namespace Ui {
class Axes;
}

class Axes : public QWidget
{
    Q_OBJECT

public:
    explicit Axes(int axisNumber, QWidget *parent = nullptr);
    ~Axes();

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

    void updateAxisRaw();
    void updateAxisOut();

    void addOrDeleteMainSource(int sourceEnum, QString sourceName, bool isAdd);

    /* Update the "Encoder N" pseudo-source rows in the main-source
     * dropdown. Driven by AxesConfig in response to FAST_ENCODER pin
     * assignments in Pin Config: encoderSlots is the set of encoder
     * slot indices (0 = Enc 1 on PA8+PA9, 1 = Enc 2 on PB6+PB7) whose
     * A and B pins are BOTH currently assigned -- a half-mapped
     * encoder isn't usable. The wire-format channel value written to
     * axis_config_t.channel matches the slot index, so encoder N's
     * dropdown selection round-trips through firmware unchanged. */
    void setEncoderSlotsAvailable(const QList<int> &encoderSlots);

    /* Live a2b button count for this axis, mirroring m_a2bButtonsCount.
     * Returns 0 when this axis isn't contributing buttons (e.g. function
     * disabled). Used by AxesConfig to compose its per-axis breakdown. */
    int a2bButtonCount() const { return m_a2bButtonsCount; }

    /* True iff a real input source (not the "None" placeholder) is
     * selected for this axis. Used by applyOutputGuard() to decide
     * whether the Output checkbox should be enabled. */
    bool hasMainSource() const;

    /* Returns the deviceEnum of the currently-selected main source,
     * or the internal "None" sentinel when no source is selected.
     * AxesConfig uses this together with isSharedSource() to compose
     * the per-source "used by which axes" map. */
    int currentSource() const;

    /* True for sources that are conceptually shared and should never
     * be flagged as "used by another axis" -- the "None" placeholder
     * (axis disabled) and the "Encoder" pseudo-source (every axis can
     * pick the same encoder source). Encapsulates the private
     * Axes-internal enum so AxesConfig doesn't have to reach in. */
    static bool isSharedSource(int sourceEnum);

    /* Annotate combobox items whose source is already selected by other
     * axes: greyed foreground + " - used by X, Y" suffix + tooltip. The
     * items remain selectable; this is purely a usage hint. usedByOthers
     * maps source-enum -> list of axis display names using that source.
     * Pass an empty map to clear all annotations. */
    void markSourcesInUse(const QMap<int, QStringList> &usedByOthers);

    /* True iff the axis is currently contributing to the device output
     * -- main source is set to a real pin AND the Output checkbox is
     * checked. The Curves tab uses this to grey-out thumbnails for
     * axes whose curves don't currently affect anything. */
    bool isOutputActive() const;

signals:
    void a2bCountChanged(int count, int previousCount);

    /* Fired whenever the main-source combobox selection changes (user
     * edit or readFromConfig-driven). AxesConfig listens to recompute
     * the global per-source usage map and re-annotate every axis's
     * dropdown. */
    void mainSourceChanged();

    /* Fires whenever isOutputActive() may have changed -- Output
     * checkbox toggled, or main source switched between real / "None".
     * AxesConfig forwards to AxesCurves so the per-axis curve thumbnails
     * can paint a "not in use" overlay when the curve doesn't currently
     * affect device output. */
    void outputActiveChanged(int axisNumber, bool active);

private:
    /* Enforce "no source -> do not output". When the main-source
     * combobox is at "None", forces the Output checkbox to unchecked
     * + disabled with an explanatory tooltip. Re-enables the checkbox
     * (without changing its checked state) once a real source is
     * selected. Called from the ctor, mainSourceIndexChanged, and the
     * tail of readFromConfig (to catch loaded configs whose source
     * already matches the combobox default and don't fire a change
     * signal). */
    void applyOutputGuard();

    /* Original (un-annotated) display text for each combobox item,
     * keyed by deviceEnum. markSourcesInUse uses this to restore the
     * base text when an annotation is removed and to compose new
     * "(used by X)" suffixes from a clean baseline rather than
     * compounding suffixes across re-annotations. */
    QHash<int, QString> m_baseDisplayTextByEnum;

private slots:
    void calibMinMaxValueChanged(int value);
    void calibrationStarted(int rawValue);
    void mainSourceIndexChanged(int index);

    void a2bSpinBoxChanged(int count);

    void outputValueChanged(bool isChecked);

    void on_pushButton_StartCalib_clicked(bool checked);
    void on_pushButton_SetCenter_clicked();
    void on_checkBox_Center_stateChanged(int state);

    void on_pushButton_ResetCalib_clicked();

    void on_checkBox_ShowExtend_stateChanged(int state);

private:
    Ui::Axes *ui;
    const int m_kMinA2bButtons = 1;//2
    bool m_calibrationStarted;
    bool m_outputEnabled;
    int m_a2bButtonsCount;
    int m_lastA2bCount;
    int m_axisNumber;
    const QString m_kStartCalStr = tr("Calibrate");
    const QString m_kStopCalStr = tr("Stop && Save");
    AxesExtended *m_axesExtend;

    QVector<int> m_mainSource_enumIndex;

    /* Parallel to m_mainSource_enumIndex. For rows whose enum is
     * Encoder, holds the encoder slot (0 = Enc 1 on PA8+PA9,
     * 1 = Enc 2 on PB6+PB7) -- this is what gets written to
     * axis_config_t.channel on save and read back on load. For
     * non-Encoder rows the value is 0 and unused. */
    QVector<int> m_mainSource_channelIndex;

    enum
    {
        Encoder = -3,
        I2C = -2,
        None = -1,
        A0 = 0,
        A1,
        A2,
        A3,
        A4,
        A5,
        A6,
        A7,
        A8,
        A9,
        A10,
        A15,
        B0,
        B1,
        B3,
        B4,
        B5,
        B6,
        B7,
        B8,
        B9,
        B10,
        B11,
        B12,
        B13,
        B14,
        B15,
        C13,
        C14,
        C15,
    };


    const QVector <deviceEnum_guiName_t> m_axesPinList =      // any order, but the first 2 are added in the constructor
    {{
        {None,     tr("None")},
        {Encoder,  tr("Encoder")},
        {I2C,      "I2C"},
        {A0,       "A0"},
        {A1,       "A1"},
        {A2,       "A2"},
        {A3,       "A3"},
        {A4,       "A4"},
        {A5,       "A5"},
        {A6,       "A6"},
        {A7,       "A7"},
        {A8,       "A8"},
        {A9,       "A9"},
        {A10,      "A10"},
        {A15,      "A15"},
        {B0,       "B0"},
        {B1,       "B1"},
        {B3,       "B3"},
        {B4,       "B4"},
        {B5,       "B5"},
        {B6,       "B6"},
        {B7,       "B7"},
        {B8,       "B8"},
        {B9,       "B9"},
        {B10,      "B10"},
        {B11,      "B11"},
        {B12,      "B12"},
        {B13,      "B13"},
        {B14,      "B14"},
        {B15,      "B15"},
        {C13,      "C13"},
        {C14,      "C14"},
        {C15,      "C15"},
    }};

    const QVector <deviceEnum_guiName_t> m_axisSourceMain =      // order MUST match common_types.h!
    {{
        {None,          tr("None")},
        {Encoder,       tr("Encoder")},
    }};
};

#endif // AXES_H
