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
class QLabel;

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

    /* Capture the device's flashed axis source map (axis_config[].
     * source_main) AND its analog-pin set (pins[]==AXIS_ANALOG) as the
     * stable device-side lookup auto-detect needs. params_report_t's
     * raw_axis_data[] / detect_axis_raw[] are computed by the firmware
     * from the DEVICE's flashed config, so translating telemetry back to a
     * pin must use the device's map, not the working config the user is
     * editing. Call this whenever the working config is known to match the
     * device: right after a successful Read or Write. Until the first
     * capture, detection falls back to the live config. */
    void captureDeviceSourceMap();

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

    /* Per-axis Detect button arm/disarm primitive. Arming snapshots
     * params_report_t.raw_axis_data[] baseline, remembers axisNumber,
     * starts a 5 s auto-disarm timer, installs the click-away watch, and
     * disarms any other previously-armed axis (only one axis at a time --
     * the previous one's Detect button gets unchecked via
     * Axes::setDetectArmed(false)). Called from onDetectClicked /
     * onDetectSequence and the auto-advance path in axesValueChanged. */
    void onDetectToggled(int axisNumber, bool armed);

    /* Single click on an axis's Detect button: ends any auto-sequence
     * walk, then toggles a one-shot arm on that axis (clicking the
     * already-armed axis cancels). Wired from Axes::detectClicked. */
    void onDetectClicked(int axisNumber);

    /* Double click on an axis's Detect button: starts auto-sequence mode
     * (m_axisSeqActive) -- after each detection the next axis is armed
     * automatically, walking down the list. Wired from
     * Axes::detectSequenceRequested. */
    void onDetectSequence(int axisNumber);

    /* Tears down any active arm + auto-sequence walk and stops the
     * click-away watch. Driven by Escape, the tab-change hideEvent, the
     * click-another-control watch, and the auto-disarm timeout. */
    void seqCancel();

    /* Auto-disarm timeout. Fires 5 s after arming if no rotation
     * crossed the threshold. Restores the armed axis's Detect button
     * to idle. Without this, an unattended armed state would silently
     * soak the next stray axis jitter. */
    void onDetectTimeout();

protected:
    /* Application-wide press watch (installed on qApp only while an axis
     * is armed / mid-walk): a click on any widget that isn't an axis's
     * Detect button cancels the arm / auto-sequence. Purely observational
     * -- never consumes. */
    bool eventFilter(QObject *obj, QEvent *event) override;

    /* Switching away from the Axes Config tab (or hiding the window)
     * cancels any in-progress arm / auto-sequence walk. */
    void hideEvent(QHideEvent *event) override;

private:
    Ui::AxesConfig *ui;
    int m_a2bButtonsCount;

    /* Auto-sequence mode: after a successful detection the next axis is
     * armed automatically. Entered by double-clicking an axis's Detect
     * button; cleared by a single click, Escape, tab change, a click on
     * another control, or the auto-disarm timeout. */
    bool m_axisSeqActive = false;

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

    /* Auto-detect operates on a UNIFIED SIGNAL of kDetectSlots channels,
     * so one code path covers both detection sources:
     *   slots 0 .. MAX_AXIS_NUM-1            -> analog PINS, value =
     *       params_report.detect_axis_raw[pin], source = pin index. Lets a
     *       rotated pot be detected even if no axis sources it yet (needs
     *       firmware >= 0.1.3; see deviceSupportsPinDetect()).
     *   slots MAX_AXIS_NUM .. 2*MAX_AXIS_NUM-1 -> external/mapped logical
     *       AXES, value = raw_axis_data[logical], source via the device
     *       map. Covers TLE5011 / I2C / encoder sources once mapped.
     * m_slotValid / m_slotSource are frozen at arm time (buildDetectSlots);
     * the per-tick value is read with readDetectSig(). m_baselineRaw /
     * m_prevTickRaw are indexed by slot. m_armedAxisIdx is the axis whose
     * Detect button is pressed (-1 = none). */
    static constexpr int kDetectSlots = 2 * MAX_AXIS_NUM;
    int m_armedAxisIdx = -1;
    QVector<int> m_baselineRaw;            // per-slot baseline, kDetectSlots
    bool m_slotValid[kDetectSlots]  = {};  // which channels are active
    int  m_slotSource[kDetectSlots] = {};  // channel -> source_main to assign
    void buildDetectSlots();               // freeze validity + source map at arm
    int  readDetectSig(int slot) const;    // current value for a channel
    bool deviceSupportsPinDetect() const;  // device firmware >= 0.1.3

    /* Snapshot of every axis's source_main taken when a detect is armed
     * (single click) or a walk is started (double click), BEFORE any
     * assignment mutates dev_config_t. Detection looks up the rotated
     * axis's source pin from here, not from the live config: during a
     * sequence walk the live axis_config[] is rewritten slot-by-slot as
     * we assign, but raw_axis_data[] still reflects the device's flashed
     * mapping -- so reading the live (already-reassigned) source_main
     * would hand a later capture the wrong pin. The snapshot is the
     * stable device->pin map for the whole walk. */
    QVector<int> m_detectSourceMain;
    void snapshotDetectSources();

    /* The device's flashed source map, captured at Read/Write time (see
     * captureDeviceSourceMap). This is the authoritative
     * logical-axis -> pin lookup for detection, stable across the user's
     * edits. m_deviceSourceValid is false until the first capture, in
     * which case snapshotDetectSources falls back to the live config. */
    QVector<int> m_deviceSourceMain;
    bool m_deviceSourceValid = false;

    /* The device's flashed analog-pin set (pin indices where
     * pins[]==AXIS_ANALOG), captured alongside m_deviceSourceMain. These
     * are exactly the pins the device samples into detect_axis_raw[], so
     * detection treats them as analog candidates. Also drives the
     * "pin changes not on device yet" banner (§5.5) -- compared against the
     * live working pin config. */
    QSet<int> m_deviceAnalogPins;

    /* §5.5 affordance: a warning banner shown at the top of the Axes tab
     * when the working AXIS_ANALOG pin set differs from what's flashed (or
     * when never synced), because auto-detect only sees the device's
     * flashed pins. Rebuilt each tick + on sync events. */
    QWidget *m_pinPendingBanner = nullptr;   // banner frame (see makeAlertBanner)
    void updatePendingPinBanner();

    /* Settle gate for auto-sequence advances. When the walk advances to
     * the next axis the user is often still rotating the axis they just
     * assigned; snapshotting the baseline immediately would let that
     * continued motion bleed straight into the new slot. Instead the new
     * axis arms in a "settling" state: each tick we watch the per-tick
     * motion across all axes, and only once everything has been still
     * (max |delta| < m_kStillThresh) for m_kStillTicksNeeded consecutive
     * ticks do we lock the baseline and begin detecting a *fresh*
     * rotation -- i.e. the user must stop and start again (or move a
     * different axis). Only the sequence-advance arm settles; the initial
     * click/double-click arm detects immediately (the axis is at rest
     * when you click). */
    bool m_detectSettling = false;
    int  m_stillTicks = 0;
    QVector<int> m_prevTickRaw;

    /* Direction guard for the walk so continued rotation of the
     * just-assigned axis doesn't spill into the next slot, while a
     * deliberate reversal can still re-select it.
     *
     * After a capture we remember the detect SLOT just rotated
     * (m_directionGuardChan) and the sign of that rotation
     * (m_directionGuardSign, +1/-1). For the NEXT capture:
     *   - the settle gate waits for *that axis to stop too* (the "delay");
     *   - then in detection that axis is eligible only if it now moves in
     *     the OPPOSITE direction (a deliberate reverse past the
     *     threshold) -- same-direction motion is ignored, so overshoot /
     *     continued turning never claims the following slot.
     * Any *other* axis is detected normally. Only the immediately-
     * preceding axis is guarded; it becomes a normal axis again after the
     * next capture. -1 = no guard (walk start / one-shot arm). */
    int m_directionGuardChan = -1;
    int m_directionGuardSign = 0;

    /* Per-tick |delta| (raw ADC LSBs) below which an axis counts as "not
     * rotating". Generous enough to ignore sensor noise, well under the
     * per-tick motion of a deliberate sweep. */
    static constexpr int m_kStillThresh = 400;
    /* Consecutive still ticks required before re-baselining (~3 * 17 ms). */
    static constexpr int m_kStillTicksNeeded = 3;

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
