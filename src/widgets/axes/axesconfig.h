#ifndef AXESCONFIG_H
#define AXESCONFIG_H

#include "axes.h"
#include <QSet>
#include <QString>
#include <QTimer>
#include <QWidget>

#include "deviceconfig.h"
#include "global.h"

namespace Ui {
class AxesConfig;
}

class QCheckBox;

class AxesConfig : public QWidget
{
    Q_OBJECT

public:
    explicit AxesConfig(QWidget *parent = nullptr);
    ~AxesConfig();

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

    void axesValueChanged();

signals:
    void axisRawValueChanged(int);
    void axisOutValueChanged(int);
    void a2bCountChanged(int count);
    /* Per-axis a2b button counts. Index = axis number (0..MAX_AXIS_NUM-1).
     * Emitted alongside a2bCountChanged so subscribers building grouped
     * UI can sub-divide the "Axis-to-buttons" section by axis. */
    void a2bBreakdownChanged(const QList<int> &perAxis);

    /* Forwarded from each Axes widget. AxesCurvesConfig listens to
     * grey-out per-axis curve thumbnails when the matching axis is
     * not contributing to device output. */
    void axisOutputActiveChanged(int axisNumber, bool active);

public slots:
    void addOrDeleteMainSource(int sourceEnum, QString sourceName, bool isAdd);

    /* PinConfig::fastEncoderSelected fires per-pin every time a pin is
     * (de-)assigned to FAST_ENCODER. Maintain a count and toggle the
     * "Encoder" entry in every axis's main-source dropdown when the
     * count crosses 0/1 -- so Encoder is only offered when at least one
     * fast-encoder pin is actually wired up. */
    void fastEncoderPinChanged(const QString &pinGuiName, bool isAdd);

    /* Fan-out for Axes::setConnectedBoard. Driven from MainWindow when
     * params_report_t.board_id arrives so per-board pin-name overrides
     * (pinboardnames.h, e.g. "B11" -> "B2" on F411) reach every axis's
     * main-source dropdown. */
    void setConnectedBoard(int boardId);
private slots:
    void a2bCountCalc(int count, int previousCount);
    void hideAxis(int index, bool hide);
    //    void axesValueChanged(int value);

    /* Recompute, for every axis, which OTHER axes have already
     * selected each candidate source, and push that map down so each
     * axis can grey-out + suffix the duplicate items in its dropdown.
     * Called from each axis's mainSourceChanged signal, plus the tail
     * of readFromConfig and addOrDeleteMainSource. */
    void refreshSourceUsage();

    /* Per-axis Detect button toggled (armed=true) or cancelled
     * (armed=false). Arming snapshots params_report_t.raw_axis_data[]
     * baseline, remembers axisNumber, starts a 5 s auto-disarm timer,
     * and disarms any other previously-armed axis (only one axis at a
     * time -- the previous one's Detect button gets unchecked via
     * Axes::setDetectArmed(false)). Wired from each
     * Axes::detectSourceRequested. */
    void onDetectToggled(int axisNumber, bool armed);

    /* Auto-disarm timeout. Fires 5 s after arming if no rotation
     * crossed the threshold. Restores the armed axis's Detect button
     * to idle. Without this, an unattended armed state would silently
     * soak the next stray axis jitter. */
    void onDetectTimeout();

private:
    Ui::AxesConfig *ui;
    int m_a2bButtonsCount;

    QList<Axes *> m_axesPtrList;
    QList<QCheckBox *> m_hideChBoxes;

    /* Set of pin GUI names currently assigned to FAST_ENCODER in
     * Pin Config (e.g. "A8", "A9", "B6", "B7"). Maintained by
     * fastEncoderPinChanged. completedEncoderSlots() turns it into
     * the list of valid encoder slots (0 = Enc 1 on PA8+PA9,
     * 1 = Enc 2 on PB6+PB7) -- both halves must be mapped for an
     * encoder to be considered usable. */
    QSet<QString> m_fastEncoderPins;

    /* Pure helper -- given m_fastEncoderPins, returns the slots whose
     * A and B pins are BOTH currently assigned. */
    QList<int> completedEncoderSlots() const;

    /* Auto-detect state. m_armedAxisIdx is the axis whose Detect
     * button is currently pressed (-1 = no axis armed). m_baselineRaw
     * is the params_report_t.raw_axis_data[] snapshot captured at arm
     * time. axesValueChanged() (called every ~17 ms by MainWindow's
     * tick when the Axes Config tab is visible) compares current raw
     * values against the baseline, and when one axis's |delta| exceeds
     * m_kDetectThresh, looks up that axis's source in dev_config_t
     * and pushes it onto m_armedAxisIdx via Axes::setSourceByEnum. */
    int m_armedAxisIdx = -1;
    QVector<int> m_baselineRaw;

    /* Auto-disarm if no rotation crossed the threshold within this
     * window (ms). Generous so a deliberate user has time to put down
     * the mouse and rotate the right axis. */
    QTimer m_detectTimeout;

    /* Threshold in raw ADC LSBs (analog_data_t is int16_t, so the
     * scale is roughly -32768..32767). Set generously to avoid noise
     * triggers but low enough that a normal pot sweep crosses it
     * within a single 17 ms tick. ~6% of full scale. */
    static constexpr int m_kDetectThresh = 4000;

    /* Timeout in ms before an armed-but-untouched watcher disarms. */
    static constexpr int m_kDetectTimeoutMs = 5000;
};

#endif // AXESCONFIG_H
