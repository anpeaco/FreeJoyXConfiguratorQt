#include "axesconfig.h"
#include "ui_axesconfig.h"
#include <algorithm>
#include <cstdlib>
#include <QApplication>
#include <QCheckBox>
#include <QHideEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QSettings>
#include <QShortcut>
#include "devicesync.h"
#include "global.h"
#include "style_helpers.h"

AxesConfig::AxesConfig(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AxesConfig)
{
    ui->setupUi(this);
    m_a2bButtonsCount = 0;

    ui->layoutV_Axes->setAlignment(Qt::AlignTop);
    // axes spawn
    for (int i = 0; i < MAX_AXIS_NUM; i++) {
        Axes *axis = new Axes(i, this);
        ui->layoutV_Axes->addWidget(axis);
        m_axesPtrList.append(axis);
        connect(axis, SIGNAL(a2bCountChanged(int, int)),
                this, SLOT(a2bCountCalc(int, int)));
        // Re-mark source dropdowns whenever any axis's selection
        // changes -- ensures "(used by X)" suffixes track the live
        // global usage map without per-axis state.
        connect(axis, &Axes::mainSourceChanged,
                this, &AxesConfig::refreshSourceUsage);
        // Forward per-axis output-active changes so the Curves tab
        // can grey-out thumbnails for axes whose curves don't
        // currently affect device output.
        connect(axis, &Axes::outputActiveChanged,
                this, &AxesConfig::axisOutputActiveChanged);
        // Per-axis Detect button. Single click arms the rotation watcher
        // for exactly this axis (one-shot); double click starts an
        // auto-sequence walk down the axes. axesValueChanged() (driven by
        // the MainWindow tick) consumes the armed state.
        connect(axis, &Axes::detectClicked,
                this, &AxesConfig::onDetectClicked);
        connect(axis, &Axes::detectSequenceRequested,
                this, &AxesConfig::onDetectSequence);
        // added hidden axes checkboxes
        QCheckBox *chb = new QCheckBox(axesList()[i].guiName, this);
        ui->layoutH_HiddenAxes->addWidget(chb);
        chb->setProperty("index", i);
        m_hideChBoxes.append(chb);
        connect(chb, &QCheckBox::toggled, this, [&](bool hide) {
            hideAxis(sender()->property("index").toInt(), hide);
        });
    }

    gEnv.pAppSettings->beginGroup("OtherSettings");
    for (int i = 0; i < m_hideChBoxes.size(); ++i) {
        m_hideChBoxes[i]->setChecked(gEnv.pAppSettings->value(axesList()[i].guiName, "false").toBool());
    }
    gEnv.pAppSettings->endGroup();

    /* Auto-detect baseline buffer sized once -- raw_axis_data[] is
     * MAX_AXIS_NUM entries on every params report. m_prevTickRaw backs
     * the sequence-advance settle gate (see header). */
    // Baseline / prev-tick are indexed by unified detect SLOT (analog pins
    // + external logical axes); the source maps stay logical-axis sized.
    m_baselineRaw.resize(kDetectSlots);
    m_prevTickRaw.resize(kDetectSlots);
    m_detectSourceMain.resize(MAX_AXIS_NUM);
    m_deviceSourceMain.resize(MAX_AXIS_NUM);

    /* §5.5 "pin changes not on device yet" banner. Inserted above the axis
     * list; hidden until the working analog-pin set diverges from the
     * device's flashed set (updatePendingPinBanner). */
    m_pinPendingBanner = new QLabel(this);
    m_pinPendingBanner->setWordWrap(true);
    m_pinPendingBanner->setText(
        freejoy_style::warningIconHtml() + QStringLiteral("&nbsp;") + tr(
        "Pin changes aren't on the device yet — auto-detect uses the "
        "device's current pins. Write Config to detect newly-assigned "
        "analog pins."));
    freejoy_style::setRole(m_pinPendingBanner, "role", "status-warning");
    m_pinPendingBanner->hide();
    ui->verticalLayout->insertWidget(0, m_pinPendingBanner);

    /* Single-shot timer disarms the watcher if no rotation lands
     * within m_kDetectTimeoutMs. Otherwise an armed-but-untouched
     * watcher would soak the next stray axis jitter long after the
     * user has wandered off. */
    m_detectTimeout.setSingleShot(true);
    m_detectTimeout.setInterval(m_kDetectTimeoutMs);
    connect(&m_detectTimeout, &QTimer::timeout,
            this, &AxesConfig::onDetectTimeout);

    // Escape -- cancel any active arm / auto-sequence walk.
    auto *scEsc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    scEsc->setContext(Qt::WidgetWithChildrenShortcut);
    connect(scEsc, &QShortcut::activated, this, &AxesConfig::seqCancel);

    // App-wide device-sync hub: re-read the device's axis source map when
    // the working config syncs with the device (read/write), and drop it
    // when sync is lost (disconnect / device swap) so auto-detect never
    // translates against a stale or wrong-board map.
    if (gEnv.pDeviceSync) {
        connect(gEnv.pDeviceSync, &DeviceSync::synced, this,
                [this](DeviceSync::Reason) {
            captureDeviceSourceMap();
            updatePendingPinBanner();   // working pins now match the device
        });
        connect(gEnv.pDeviceSync, &DeviceSync::desynced, this,
                [this](DeviceSync::Reason) {
            m_deviceSourceValid = false;
            m_deviceAnalogPins.clear();
            seqCancel();   // any in-progress walk is meaningless without the map
            updatePendingPinBanner();
        });
    }
}

AxesConfig::~AxesConfig()
{
    gEnv.pAppSettings->beginGroup("OtherSettings");
    for (int i = 0; i < m_hideChBoxes.size(); ++i) {
        gEnv.pAppSettings->setValue(axesList()[i].guiName, m_hideChBoxes[i]->isChecked());
    }
    gEnv.pAppSettings->endGroup();
    delete ui;
}

void AxesConfig::retranslateUi()
{
    ui->retranslateUi(this);
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        m_axesPtrList[i]->retranslateUi();
    }
}

void AxesConfig::a2bCountCalc(int count, int previousCount)
{
    m_a2bButtonsCount += count - previousCount;

    // Per-axis breakdown alongside the total -- emit FIRST so anyone
    // grouping UI off the breakdown has fresh data when the total triggers
    // a rebuild.
    QList<int> perAxis;
    perAxis.reserve(m_axesPtrList.size());
    for (auto *a : m_axesPtrList) {
        perAxis.append(a->a2bButtonCount());
    }
    emit a2bBreakdownChanged(perAxis);

    emit a2bCountChanged(m_a2bButtonsCount);
}

void AxesConfig::addOrDeleteMainSource(int sourceEnum, QString sourceName, bool isAdd)
{
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        m_axesPtrList[i]->addOrDeleteMainSource(sourceEnum, sourceName, isAdd);
    }
    // The set of candidate sources changed; refresh the "used by X"
    // annotations so newly-added items pick up suffixes immediately
    // and removed items don't leave dangling references in other
    // axes' dropdowns.
    refreshSourceUsage();
}

void AxesConfig::setConnectedBoard(int boardId)
{
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        m_axesPtrList[i]->setConnectedBoard(boardId);
    }
    // Each Axes::setConnectedBoard emits mainSourceChanged after
    // relabelling, which feeds refreshSourceUsage via the existing
    // signal wiring -- no explicit refresh needed here.
}

void AxesConfig::fastEncoderPinChanged(const QString &pinGuiName, bool isAdd)
{
    const QList<int> prevSlots = completedEncoderSlots();
    if (isAdd) {
        m_fastEncoderPins.insert(pinGuiName);
    } else {
        m_fastEncoderPins.remove(pinGuiName);
    }
    const QList<int> nowSlots = completedEncoderSlots();

    // Suppress per-pin churn -- only push when the set of *complete*
    // encoder pairs actually changes (e.g. assigning the second half
    // of a pair, or removing one half of a pair, both shift the slot
    // list, but adding the first half of an unpaired encoder doesn't).
    if (prevSlots == nowSlots) return;

    for (int i = 0; i < m_axesPtrList.size(); ++i) {
        m_axesPtrList[i]->setEncoderSlotsAvailable(nowSlots);
    }
    refreshSourceUsage();
}

QList<int> AxesConfig::completedEncoderSlots() const
{
    /* Encoder pin pairs are board-independent on F103 and F411:
     *   slot 0 = Enc 1 = PA8 + PA9 (TIM1)
     *   slot 1 = Enc 2 = PB6 + PB7 (TIM4)
     * pinList[].guiName arrives as a tr() string like "Pin A8", so
     * match by suffix (the same pattern encodersconfig.cpp::
     * fastEncoderSelected uses for its slot dispatch) -- this stays
     * correct under translation and any future "Pin "/"P"/"" prefix
     * shifts in the pin combobox. */
    auto hasPin = [&](const QString &suffix) {
        for (const QString &name : m_fastEncoderPins) {
            if (name.endsWith(suffix)) return true;
        }
        return false;
    };
    QList<int> completedSlots;
    if (hasPin(QStringLiteral("A8")) && hasPin(QStringLiteral("A9"))) {
        completedSlots.append(0);
    }
    if (hasPin(QStringLiteral("B6")) && hasPin(QStringLiteral("B7"))) {
        completedSlots.append(1);
    }
    return completedSlots;
}

void AxesConfig::refreshSourceUsage()
{
    // Build the global "source enum -> list of axis names" usage map.
    QMap<int, QStringList> globalUsed;
    for (int i = 0; i < m_axesPtrList.size(); ++i) {
        const int e = m_axesPtrList[i]->currentSource();
        if (Axes::isSharedSource(e)) continue;   // None / Encoder -- never count
        globalUsed[e].append(axesList()[i].guiName);
    }

    // Push to each axis a per-axis "used by OTHERS" map (own usage stripped).
    for (int i = 0; i < m_axesPtrList.size(); ++i) {
        const int ownEnum = m_axesPtrList[i]->currentSource();
        const QString ownName = axesList()[i].guiName;
        QMap<int, QStringList> usedByOthers = globalUsed;
        if (!Axes::isSharedSource(ownEnum) && usedByOthers.contains(ownEnum)) {
            usedByOthers[ownEnum].removeAll(ownName);
            if (usedByOthers[ownEnum].isEmpty()) {
                usedByOthers.remove(ownEnum);
            }
        }
        m_axesPtrList[i]->markSourcesInUse(usedByOthers);
    }
}

void AxesConfig::axesValueChanged()
{
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        m_axesPtrList[i]->updateAxisOut();
        m_axesPtrList[i]->updateAxisRaw();
    }

    /* Auto-detect: if some axis's Detect button is armed, compare
     * current raw_axis_data[] against the baseline captured at arm
     * time. The axis with the largest delta over m_kDetectThresh is
     * the axis whose source pin the user is currently rotating. Look
     * up that axis's source from dev_config_t and push it onto the
     * armed axis's dropdown via setSourceByEnum. One-shot: disarm
     * afterwards so wiggling to verify doesn't re-trigger; the user
     * clicks Detect again to re-arm. */
    // §5.5: keep the "pin changes not on device" banner current while the
    // tab is live (cheap set compare).
    updatePendingPinBanner();

    if (m_armedAxisIdx < 0) return;

    /* Detection runs over the unified signal slots (analog pins +
     * external/mapped logical axes); see header. readDetectSig() yields a
     * channel's current value; m_slotValid/m_slotSource were frozen at arm
     * by buildDetectSlots(). */

    /* Settle gate (sequence advances): hold off until the user has stopped
     * rotating, so leftover motion from the channel just assigned doesn't
     * spill into this slot. Re-baseline only after a few still ticks. */
    if (m_detectSettling) {
        int maxTickDelta = 0;
        for (int s = 0; s < kDetectSlots; ++s) {
            if (!m_slotValid[s]) continue;
            const int v = readDetectSig(s);
            maxTickDelta = std::max(maxTickDelta, std::abs(v - m_prevTickRaw[s]));
            m_prevTickRaw[s] = v;
        }
        if (maxTickDelta < m_kStillThresh) {
            if (++m_stillTicks >= m_kStillTicksNeeded) {
                for (int s = 0; s < kDetectSlots; ++s)
                    m_baselineRaw[s] = m_slotValid[s] ? readDetectSig(s) : 0;
                m_detectSettling = false;
                m_stillTicks = 0;
                m_detectTimeout.start();
            }
        } else {
            m_stillTicks = 0;
        }
        return;   // never detect while still settling
    }

    int bestSlot = -1;
    int bestDelta = 0;
    int bestSign = 0;
    for (int s = 0; s < kDetectSlots; ++s) {
        if (!m_slotValid[s]) continue;
        const int signedDelta = readDetectSig(s) - m_baselineRaw[s];
        const int sign = (signedDelta > 0) - (signedDelta < 0);

        // Direction guard: the channel assigned to the previous slot may
        // only re-trigger on a deliberate *reversal*. Same-direction
        // motion (overshoot / continued turning) is ignored.
        if (s == m_directionGuardChan && sign == m_directionGuardSign) {
            continue;
        }

        const int mag = std::abs(signedDelta);
        if (mag > bestDelta) {
            bestDelta = mag;
            bestSlot = s;
            bestSign = sign;
        }
    }
    if (bestDelta < m_kDetectThresh) return;

    // Source to assign: for an analog-pin slot this is the pin index
    // (read straight from the device's sampled set); for an external slot
    // it's the device-map source. Frozen at arm, so the walk's own
    // reassignments can't poison it.
    const int detectedSource = m_slotSource[bestSlot];
    if (detectedSource < 0) {            // safety -- valid slots carry >= 0
        m_baselineRaw[bestSlot] = readDetectSig(bestSlot);
        return;
    }
    const int armed = m_armedAxisIdx;
    m_armedAxisIdx = -1;
    m_detectTimeout.stop();
    m_axesPtrList[armed]->setSourceByEnum(detectedSource);
    m_axesPtrList[armed]->setDetectArmed(false);

    // Arm the direction guard on the slot just rotated.
    m_directionGuardChan = bestSlot;
    m_directionGuardSign = bestSign;

    // Auto-sequence: walk forward to the next visible axis and re-arm so
    // a series of rotations fills the axis sources in order. Hidden axes
    // are skipped (their Detect button isn't on screen). Ends when no
    // further visible axis remains. onDetectToggled snapshots a fresh
    // baseline + restarts the timeout for the new axis.
    if (m_axisSeqActive) {
        int next = -1;
        for (int i = armed + 1; i < MAX_AXIS_NUM; ++i) {
            if (m_axesPtrList[i]->isVisible()) { next = i; break; }
        }
        if (next >= 0) {
            m_axesPtrList[next]->setDetectArmed(true, true);
            onDetectToggled(next, true);
            // Arm in the settling state: wait for the user to stop the
            // rotation in progress before locking this axis's baseline.
            m_detectSettling = true;
            m_stillTicks = 0;
            for (int s = 0; s < kDetectSlots; ++s)
                m_prevTickRaw[s] = m_slotValid[s] ? readDetectSig(s) : 0;
        } else {
            seqCancel();   // reached the bottom -- end the walk
        }
    }
}

void AxesConfig::onDetectToggled(int axisNumber, bool armed)
{
    if (armed) {
        /* If a different axis was already armed, disarm it visually
         * first -- only one watcher at a time, and the user just
         * decided which axis they actually want. */
        if (m_armedAxisIdx >= 0 && m_armedAxisIdx != axisNumber) {
            m_axesPtrList[m_armedAxisIdx]->setDetectArmed(false);
        }
        /* Freeze which channels are active + their source mapping for this
         * arm, then snapshot each channel's current value as the baseline
         * so the next tick detects a *real* rotation rather than stale
         * accumulated deltas. */
        buildDetectSlots();
        for (int s = 0; s < kDetectSlots; ++s) {
            m_baselineRaw[s] = m_slotValid[s] ? readDetectSig(s) : 0;
        }
        m_armedAxisIdx = axisNumber;
        // Keep the armed axis in view: during an auto-sequence walk the next
        // axis is auto-armed and may be off-screen. No-op if already visible.
        if (axisNumber >= 0 && axisNumber < m_axesPtrList.size()) {
            ui->scrollArea->ensureWidgetVisible(m_axesPtrList[axisNumber], 0, 10);
        }
        // Initial / one-shot arms detect immediately (the axis is at rest
        // when the user clicks). Only the sequence-advance path turns
        // m_detectSettling back on, right after this call.
        m_detectSettling = false;
        m_stillTicks = 0;
        m_detectTimeout.start();
        // Watch for a click on any other control (or tab switch) while
        // armed -- it cancels the arm / walk. Installed on qApp; removed
        // on full disarm. Re-installing is idempotent.
        qApp->installEventFilter(this);
    } else {
        /* User cancelled the same axis that was armed. */
        if (m_armedAxisIdx == axisNumber) {
            m_armedAxisIdx = -1;
            m_detectTimeout.stop();
            if (!m_axisSeqActive) {
                qApp->removeEventFilter(this);
            }
        }
    }
}

void AxesConfig::onDetectTimeout()
{
    if (m_armedAxisIdx < 0) return;
    // 5 s with no rotation ends the whole interaction, including any
    // auto-sequence walk.
    seqCancel();
}

void AxesConfig::onDetectClicked(int axisNumber)
{
    // A single click ends any auto-sequence walk, then toggles a one-shot
    // arm on the clicked axis: clicking the already-armed axis cancels;
    // clicking any other axis arms it (disarming the previous inside
    // onDetectToggled).
    m_axisSeqActive = false;
    if (m_armedAxisIdx == axisNumber) {
        m_axesPtrList[axisNumber]->setDetectArmed(false);
        onDetectToggled(axisNumber, false);
    } else {
        snapshotDetectSources();   // capture device->pin map before any assign
        m_directionGuardChan = -1; // one-shot: no previous axis to guard
        m_axesPtrList[axisNumber]->setDetectArmed(true, false);
        onDetectToggled(axisNumber, true);
    }
}

void AxesConfig::onDetectSequence(int axisNumber)
{
    m_axisSeqActive = true;
    snapshotDetectSources();       // capture device->pin map before the walk assigns
    m_directionGuardChan = -1;     // first capture of the walk has no predecessor
    m_axesPtrList[axisNumber]->setDetectArmed(true, true);
    onDetectToggled(axisNumber, true);
}

void AxesConfig::snapshotDetectSources()
{
    // Prefer the device's flashed source map (the map raw_axis_data is
    // actually computed from). Fall back to the live config only if we
    // never synced with a device this session (e.g. detect used purely
    // against a loaded file), in which case the user is responsible for
    // the working config matching what's on the device.
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        m_detectSourceMain[i] = m_deviceSourceValid
            ? m_deviceSourceMain[i]
            : gEnv.pDeviceConfig->config.axis_config[i].source_main;
    }
}

void AxesConfig::captureDeviceSourceMap()
{
    const dev_config_t &cfg = gEnv.pDeviceConfig->config;
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        m_deviceSourceMain[i] = cfg.axis_config[i].source_main;
    }
    // The device samples detect_axis_raw[] for exactly these pins (PA0..PA7
    // tagged AXIS_ANALOG in the flashed config), so they are the analog
    // detect candidates and the reference for the §5.5 banner.
    m_deviceAnalogPins.clear();
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        if (cfg.pins[i] == AXIS_ANALOG) m_deviceAnalogPins.insert(i);
    }
    m_deviceSourceValid = true;
}

bool AxesConfig::deviceSupportsPinDetect() const
{
    // Per-pin analog detect (detect_axis_raw) ships in firmware >= 0.1.3.
    const params_report_t &pr = gEnv.pDeviceConfig->paramsReport;
    const int v = pr.freejoyx_version_major * 10000
                + pr.freejoyx_version_minor * 100
                + pr.freejoyx_version_patch;
    return v >= (0 * 10000 + 1 * 100 + 3);
}

int AxesConfig::readDetectSig(int slot) const
{
    const params_report_t &pr = gEnv.pDeviceConfig->paramsReport;
    if (slot < MAX_AXIS_NUM) {
        return pr.detect_axis_raw[slot];            // analog pin (pin index)
    }
    return pr.raw_axis_data[slot - MAX_AXIS_NUM];   // external/mapped logical axis
}

void AxesConfig::buildDetectSlots()
{
    for (int s = 0; s < kDetectSlots; ++s) { m_slotValid[s] = false; m_slotSource[s] = -1; }

    const bool pinDetect = deviceSupportsPinDetect();

    // Analog candidate pins: the device's flashed analog set when we've
    // synced, else the working config's (extra non-sampled pins are inert --
    // detect_axis_raw reads AXIS_MIN for them, so they never trigger).
    QSet<int> analogPins;
    if (m_deviceSourceValid) {
        analogPins = m_deviceAnalogPins;
    } else {
        const dev_config_t &cfg = gEnv.pDeviceConfig->config;
        for (int i = 0; i < MAX_AXIS_NUM; ++i)
            if (cfg.pins[i] == AXIS_ANALOG) analogPins.insert(i);
    }

    // Slots 0..MAX_AXIS_NUM-1: analog pins (detect_axis_raw). Needs firmware
    // that fills the field; otherwise these stay inactive and only the
    // external path below runs (pre-0.1.3 behaviour).
    if (pinDetect) {
        for (int pin = 0; pin < MAX_AXIS_NUM; ++pin) {
            if (!analogPins.contains(pin)) continue;
            m_slotValid[pin]  = true;
            m_slotSource[pin] = pin;        // analog source_main == pin index
        }
    }

    // Slots MAX_AXIS_NUM..2N-1: external / already-mapped logical axes
    // (raw_axis_data, source via the per-arm device map). When pin-detect is
    // live, an analog axis is already covered by its pin slot, so skip it
    // here to avoid double-handling.
    for (int log = 0; log < MAX_AXIS_NUM; ++log) {
        const int src = m_detectSourceMain[log];
        if (src < 0) continue;                       // encoder/None -- unnameable
        if (pinDetect && analogPins.contains(src)) continue;
        m_slotValid[MAX_AXIS_NUM + log]  = true;
        m_slotSource[MAX_AXIS_NUM + log] = src;
    }
}

void AxesConfig::updatePendingPinBanner()
{
    if (!m_pinPendingBanner) return;
    // Working analog-pin set (pins 0..MAX_AXIS_NUM-1 == PA0..PA7).
    QSet<int> liveAnalog;
    const dev_config_t &cfg = gEnv.pDeviceConfig->config;
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        if (cfg.pins[i] == AXIS_ANALOG) liveAnalog.insert(i);
    }
    // Show only when we have a device baseline AND the working analog pins
    // differ from what's flashed -- detect can't see pins not yet written.
    // Offline / never-synced: hidden (detect falls back to the live config
    // and there's nothing to write to).
    const bool diverged = m_deviceSourceValid && (liveAnalog != m_deviceAnalogPins);
    m_pinPendingBanner->setVisible(diverged);
}

void AxesConfig::seqCancel()
{
    m_axisSeqActive = false;
    m_detectSettling = false;
    m_stillTicks = 0;
    m_directionGuardChan = -1;
    if (m_armedAxisIdx >= 0) {
        m_axesPtrList[m_armedAxisIdx]->setDetectArmed(false);
        m_armedAxisIdx = -1;
        m_detectTimeout.stop();
    }
    qApp->removeEventFilter(this);
}

bool AxesConfig::eventFilter(QObject *obj, QEvent *event)
{
    // Click-away watch (installed on qApp only while armed / mid-walk):
    // a press on anything other than an axis's Detect button cancels the
    // arm / auto-sequence. Purely observational -- never consume.
    if (event->type() == QEvent::MouseButtonPress
        && (m_armedAxisIdx >= 0 || m_axisSeqActive)) {
        if (QWidget *w = qobject_cast<QWidget *>(obj)) {
            if (w->objectName() != QLatin1String("pushButton_DetectSource")) {
                seqCancel();
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void AxesConfig::hideEvent(QHideEvent *event)
{
    seqCancel();
    QWidget::hideEvent(event);
}

void AxesConfig::hideAxis(int index, bool hide)
{
    if (index < 0) index = 0;
    else if (index >= m_axesPtrList.size()) index = m_axesPtrList.size() - 1;

    if (hide) {
        m_axesPtrList[index]->hide();
    } else {
        m_axesPtrList[index]->show();
    }
}

void AxesConfig::readFromConfig()
{
    for (int i = 0; i < MAX_AXIS_NUM; i++) {
        m_axesPtrList[i]->readFromConfig();
    }
    // Annotate dropdowns now that the loaded source selections are
    // settled. axes whose source matched the combobox's current value
    // didn't fire mainSourceChanged during load, so do a single sweep.
    refreshSourceUsage();
}

void AxesConfig::writeToConfig()
{
    for (int i = 0; i < MAX_AXIS_NUM; i++) {
        m_axesPtrList[i]->writeToConfig();
    }
}
