#include "axes.h"
#include "ui_axes.h"
#include <QEvent>
#include <QTimer>
#include <QTranslator>
#include "converter.h"
#include "widgets/pins/pinboardnames.h"

const QVector <deviceEnum_guiName_t> &axesList()    // order MUST match common_types.h!
{
    static const QVector <deviceEnum_guiName_t> aL =
    {{
        {0,      ("X")},
        {1,      ("Y")},
        {2,      ("Z")},
        {3,      ("Rx")},
        {4,      ("Ry")},
        {5,      ("Rz")},
        {6,      QCoreApplication::translate("Axes", "Slider 1")},
        {7,      QCoreApplication::translate("Axes", "Slider 2")},
    }};

    return aL;
}

Axes::Axes(int axisNumber, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Axes)
{
    ui->setupUi(this);

    m_a2bButtonsCount = 0;
    m_lastA2bCount = 1;
    m_calibrationStarted = false;
    m_outputEnabled = ui->checkBox_Output->isChecked();

    m_axisNumber = axisNumber;
    ui->groupBox_AxixName->setTitle(axesList()[m_axisNumber].guiName);

    // Add the "None" placeholder unconditionally. The "Encoder"
    // pseudo-source (m_axesPinList[1]) is added on demand by
    // setEncoderSourceVisible() once Pin Config reports at least one
    // FAST_ENCODER pin assignment -- selecting Encoder when nothing is
    // wired up would point an axis at a peripheral that doesn't exist.
    // Cache the original display text under each enum so
    // markSourcesInUse() can append/restore "(used by X)" suffixes
    // without compounding.
    ui->comboBox_AxisSource1->addItem(m_axesPinList[0].guiName);
    m_mainSource_enumIndex.push_back(m_axesPinList[0].deviceEnumIndex);
    m_mainSource_channelIndex.push_back(0);
    m_mainSource_baseSourceName.push_back(m_axesPinList[0].guiName);
    m_baseDisplayTextByEnum.insert(m_axesPinList[0].deviceEnumIndex, m_axesPinList[0].guiName);

    // set a2b  // duplicated work vs. readFromConfig()?
    ui->spinBox_A2bCount->setMaximum(MAX_A2B_BUTTONS);
    if (ui->spinBox_A2bCount->value() < m_kMinA2bButtons) {
        ui->widget_A2bSlider->setEnabled(false);
        ui->widget_A2bSlider->setPointsCount(m_kMinA2bButtons);//+1
    } else {
        ui->widget_A2bSlider->setEnabled(true);
        ui->widget_A2bSlider->setPointsCount(ui->spinBox_A2bCount->value() + 1);
    }

    // add axes extended settings
    m_axesExtend = new AxesExtended(m_axisNumber, this);
    m_axesExtend->setVisible(false);
    ui->layoutH_AxesExtend->addWidget(m_axesExtend);
    //ui->layoutV_Axes->addWidget(axes_extend);

    // output checked
    connect(ui->checkBox_Output, &QCheckBox::toggled, this, &Axes::outputValueChanged);
    // calibration value changed
    connect(ui->spinBox_CalibMax, qOverload<int>(&QSpinBox::valueChanged), this, &Axes::calibMinMaxValueChanged);
    connect(ui->spinBox_CalibMin, qOverload<int>(&QSpinBox::valueChanged), this, &Axes::calibMinMaxValueChanged);
    // main source changed
    connect(ui->comboBox_AxisSource1, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &Axes::mainSourceIndexChanged);
    // a2b count changed
    connect(ui->spinBox_A2bCount, qOverload<int>(&QSpinBox::valueChanged), this, &Axes::a2bSpinBoxChanged);

    Q_ASSERT(ui->groupBox_AxixName->objectName() == QStringLiteral("groupBox_AxixName"));

    /* Watch comboBox_AxisSource1 for FocusIn so AxesConfig can arm
     * its auto-detect baseline-and-watch state on click. The combobox
     * itself has no built-in "focused" signal -- event filter is the
     * standard Qt route. We install on the combobox AND its line-edit
     * child if any (combobox is non-editable here so just the box). */
    ui->comboBox_AxisSource1->installEventFilter(this);

    // Default state at construction: combobox at "None" -> Output
    // checkbox starts disabled + unchecked. addOrDeleteMainSource will
    // re-enable as analog pins get assigned in Pin Config.
    applyOutputGuard();
}

Axes::~Axes()
{
    m_mainSource_enumIndex.clear();
    m_mainSource_enumIndex.shrink_to_fit();
    delete ui;
}

void Axes::retranslateUi()
{
    ui->retranslateUi(this);
    m_axesExtend->retranslateUi();
}

void Axes::addOrDeleteMainSource(int sourceEnum, QString sourceName, bool isAdd)
{
    if (isAdd == true) {
        const int pinIdx = Converter::EnumToIndex(sourceEnum, m_axesPinList);
        /* sourceEnum can be a pin slot index that's not in m_axesPinList
         * -- e.g. a board variant exposes a slot the universal list
         * doesn't enumerate. Skip silently rather than indexing
         * m_axesPinList[-1], which used to ASSERT-crash the configurator
         * after a successful Read Config. */
        if (pinIdx < 0) {
            return;
        }
        const QString baseText = composePinLabel(pinIdx, sourceName);
        ui->comboBox_AxisSource1->addItem(baseText);
        m_mainSource_enumIndex.push_back(m_axesPinList[pinIdx].deviceEnumIndex);
        m_mainSource_channelIndex.push_back(0);
        m_mainSource_baseSourceName.push_back(sourceName);
        m_baseDisplayTextByEnum.insert(sourceEnum, baseText);
    } else {
        for (int i = 0; i < m_mainSource_enumIndex.size(); ++i) {
            if (m_mainSource_enumIndex[i] == sourceEnum) {
                if (ui->comboBox_AxisSource1->currentIndex() == i) {
                    ui->comboBox_AxisSource1->setCurrentIndex(0);
                }
                ui->comboBox_AxisSource1->removeItem(i);
                m_mainSource_enumIndex.erase(m_mainSource_enumIndex.begin() + i);
                m_mainSource_channelIndex.erase(m_mainSource_channelIndex.begin() + i);
                m_mainSource_baseSourceName.removeAt(i);
                m_baseDisplayTextByEnum.remove(sourceEnum);
                break;
            }
        }
    }
}

QString Axes::composePinLabel(int pinIdx, const QString &sourceName) const
{
    return pinDisplayName(m_boardId, m_axesPinList[pinIdx].guiName)
        + QStringLiteral(" - ") + sourceName;
}

void Axes::setConnectedBoard(int boardId)
{
    if (boardId == m_boardId) return;
    m_boardId = boardId;
    relabelMainSourceItems();
}

void Axes::relabelMainSourceItems()
{
    /* Walk every row in the main-source dropdown. Pin rows: re-derive
     * the display text from the stored sourceName + the pin's
     * board-specific label. Non-pin rows (None/Encoder/I2C): leave
     * untouched -- their labels don't include a pin name. The
     * "used by X" annotations layered on top by markSourcesInUse
     * become stale; re-emit mainSourceChanged so AxesConfig
     * recomputes the per-source usage map and re-annotates. */
    for (int i = 0; i < m_mainSource_enumIndex.size(); ++i) {
        const int devEnum = m_mainSource_enumIndex[i];
        if (devEnum == Encoder || devEnum == I2C || devEnum == None) {
            continue;
        }
        const int pinIdx = Converter::EnumToIndex(devEnum, m_axesPinList);
        if (pinIdx < 0) continue;
        const QString newLabel = composePinLabel(pinIdx, m_mainSource_baseSourceName[i]);
        ui->comboBox_AxisSource1->setItemText(i, newLabel);
        m_baseDisplayTextByEnum[devEnum] = newLabel;
    }
    emit mainSourceChanged();
}

void Axes::setSourceByEnum(int sourceEnum)
{
    /* Find the row whose deviceEnum matches and switch the dropdown
     * to it. setCurrentIndex emits currentIndexChanged on the
     * QComboBox, which triggers mainSourceIndexChanged here -- so
     * applyOutputGuard, mainSourceChanged emission, and the Output
     * checkbox propagation all run as if the user had selected the
     * row themselves. No-op if the enum isn't in the current dropdown
     * (e.g. caller asked for a pin that hasn't been assigned as
     * ANALOG_IN in Pin Config), or if it's already selected. */
    for (int i = 0; i < m_mainSource_enumIndex.size(); ++i) {
        if (m_mainSource_enumIndex[i] != sourceEnum) continue;
        if (ui->comboBox_AxisSource1->currentIndex() == i) return;
        ui->comboBox_AxisSource1->setCurrentIndex(i);
        return;
    }
}

bool Axes::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->comboBox_AxisSource1 && event->type() == QEvent::FocusIn) {
        emit sourceComboFocused(m_axisNumber);
    }
    return QWidget::eventFilter(obj, event);
}

void Axes::setEncoderSlotsAvailable(const QList<int> &encoderSlots)
{
    /* Snapshot the user's current selection so we can restore it after
     * rebuilding the encoder rows. We track both the enum and (for
     * Encoder rows) the channel slot, since two Encoder rows share
     * the same enum and only differ by channel. */
    const int prevIdx = ui->comboBox_AxisSource1->currentIndex();
    int prevEnum = None;
    int prevChannel = 0;
    if (prevIdx >= 0 && prevIdx < m_mainSource_enumIndex.size()) {
        prevEnum = m_mainSource_enumIndex[prevIdx];
        prevChannel = m_mainSource_channelIndex[prevIdx];
    }

    /* Strip every existing Encoder row (potentially 0, 1, or 2 of
     * them). Iterate from the end so removals don't shift the indices
     * of rows still to be examined. */
    for (int i = m_mainSource_enumIndex.size() - 1; i >= 0; --i) {
        if (m_mainSource_enumIndex[i] == Encoder) {
            ui->comboBox_AxisSource1->removeItem(i);
            m_mainSource_enumIndex.erase(m_mainSource_enumIndex.begin() + i);
            m_mainSource_channelIndex.erase(m_mainSource_channelIndex.begin() + i);
            m_mainSource_baseSourceName.removeAt(i);
        }
    }

    /* Insert one row per available encoder slot, immediately after
     * "None" (index 0). Insertion order matches the slot order in
     * encoderSlots so the visual ordering is stable -- Enc 1 above
     * Enc 2 when both are present. */
    int insertAt = 1;
    for (int slot : encoderSlots) {
        const QString label = tr("Encoder %1").arg(slot + 1);
        ui->comboBox_AxisSource1->insertItem(insertAt, label);
        m_mainSource_enumIndex.insert(insertAt, Encoder);
        m_mainSource_channelIndex.insert(insertAt, slot);
        m_mainSource_baseSourceName.insert(insertAt, label);
        ++insertAt;
    }

    /* Restore the prior selection where possible. For non-Encoder
     * sources the enum is unique, so a plain enum match works. For
     * Encoder, also match the channel -- if the user's previously
     * selected encoder slot is no longer available the axis falls
     * back to None (index 0), and the existing mainSourceIndexChanged
     * machinery (Output checkbox, refreshSourceUsage) takes care of
     * the rest via the setCurrentIndex side effect. */
    int restoreIdx = -1;
    for (int i = 0; i < m_mainSource_enumIndex.size(); ++i) {
        if (m_mainSource_enumIndex[i] != prevEnum) continue;
        if (prevEnum == Encoder && m_mainSource_channelIndex[i] != prevChannel) continue;
        restoreIdx = i;
        break;
    }
    ui->comboBox_AxisSource1->setCurrentIndex(restoreIdx >= 0 ? restoreIdx : 0);
}

void Axes::mainSourceIndexChanged(int index)
{
    /* setCurrentIndex(-1) (deselection) fires this slot with index=-1.
     * Indexing m_mainSource_enumIndex[-1] would QList-ASSERT-crash. */
    if (index < 0 || index >= m_mainSource_enumIndex.size()) {
        m_axesExtend->setI2CEnabled(false);
        applyOutputGuard();
        emit mainSourceChanged();
        emit outputActiveChanged(m_axisNumber, isOutputActive());
        return;
    }
    if (m_mainSource_enumIndex[index] == I2C) {
        m_axesExtend->setI2CEnabled(true);
    } else {
        m_axesExtend->setI2CEnabled(false);
    }
    applyOutputGuard();
    emit mainSourceChanged();
    // applyOutputGuard may have flipped the Output checkbox to false
    // when source went to None; that already fires outputActiveChanged
    // via outputValueChanged. We still emit here so the source-changed
    // path is covered when the checkbox state didn't change but the
    // *active* state did (e.g. switching between two real sources --
    // active stays true; covered via outputValueChanged not firing,
    // but the check below short-circuits redundant re-emits).
    emit outputActiveChanged(m_axisNumber, isOutputActive());
}

bool Axes::isOutputActive() const
{
    return hasMainSource() && ui->checkBox_Output->isChecked();
}

bool Axes::hasMainSource() const
{
    return currentSource() != None;
}

int Axes::currentSource() const
{
    if (m_mainSource_enumIndex.isEmpty()) return None;
    const int idx = ui->comboBox_AxisSource1->currentIndex();
    if (idx < 0 || idx >= m_mainSource_enumIndex.size()) return None;
    return m_mainSource_enumIndex[idx];
}

bool Axes::isSharedSource(int sourceEnum)
{
    return sourceEnum == None || sourceEnum == Encoder;
}

void Axes::markSourcesInUse(const QMap<int, QStringList> &usedByOthers)
{
    for (int i = 0; i < m_mainSource_enumIndex.size(); ++i) {
        const int e = m_mainSource_enumIndex[i];
        // "None" and "Encoder" are conceptually shared -- never mark.
        if (e == None || e == Encoder) continue;

        const QString base = m_baseDisplayTextByEnum.value(
            e, ui->comboBox_AxisSource1->itemText(i));

        if (usedByOthers.contains(e)) {
            const QStringList by = usedByOthers.value(e);
            ui->comboBox_AxisSource1->setItemText(
                i, base + tr(" — used by %1").arg(by.join(QStringLiteral(", "))));
            ui->comboBox_AxisSource1->setItemData(
                i, QBrush(QColor(140, 140, 140)), Qt::ForegroundRole);
            ui->comboBox_AxisSource1->setItemData(
                i,
                tr("Already selected as the main source of: %1").arg(by.join(QStringLiteral(", "))),
                Qt::ToolTipRole);
        } else {
            ui->comboBox_AxisSource1->setItemText(i, base);
            ui->comboBox_AxisSource1->setItemData(i, QVariant(), Qt::ForegroundRole);
            ui->comboBox_AxisSource1->setItemData(i, QVariant(), Qt::ToolTipRole);
        }
    }
}

void Axes::applyOutputGuard()
{
    if (hasMainSource()) {
        ui->checkBox_Output->setEnabled(true);
        ui->checkBox_Output->setToolTip(QString());
    } else {
        // Force unchecked so the visual state reads unambiguously as
        // "do not output" when there is no input to produce output
        // from. setChecked(false) emits toggled() -> outputValueChanged
        // -> m_outputEnabled = false, keeping the local cache honest.
        ui->checkBox_Output->setChecked(false);
        ui->checkBox_Output->setEnabled(false);
        ui->checkBox_Output->setToolTip(tr(
            "No input source assigned. Select a Main Source above to "
            "enable HID output for this axis."));
    }
}

void Axes::calibMinMaxValueChanged(int value)
{
    Q_UNUSED(value)
    if (ui->checkBox_Center->isChecked() == false) {
        ui->spinBox_CalibCenter->setValue((ui->spinBox_CalibMax->value() + ui->spinBox_CalibMin->value()) / 2);
    }
}

void Axes::calibrationStarted(int rawValue)
{
    if (ui->spinBox_CalibMax->value() < rawValue) {
        ui->spinBox_CalibMax->setValue(rawValue);
    } else if (ui->spinBox_CalibMin->value() > rawValue) {
        ui->spinBox_CalibMin->setValue(rawValue);
    }
}

void Axes::on_pushButton_StartCalib_clicked(bool checked)
{
    m_calibrationStarted = checked;
    if (checked == true) {
        ui->pushButton_StartCalib->setText(m_kStopCalStr);
        ui->spinBox_CalibMax->setValue(AXIS_MIN_VALUE); // not swapped (intentional)
        ui->spinBox_CalibMin->setValue(AXIS_MAX_VALUE);

        connect(ui->progressBar_Raw, SIGNAL(valueChanged(int)), this, SLOT(calibrationStarted(int)));

    } else {
        disconnect(ui->progressBar_Raw, SIGNAL(valueChanged(int)), nullptr, nullptr);
        ui->pushButton_StartCalib->setText(m_kStartCalStr);
    }
}

void Axes::updateAxisRaw()
{
    ui->progressBar_Raw->setValue(gEnv.pDeviceConfig->paramsReport.raw_axis_data[m_axisNumber]);
}

void Axes::updateAxisOut()
{
    ui->progressBar_Out->setValue(gEnv.pDeviceConfig->paramsReport.axis_data[m_axisNumber]);

    // a2b  axis_number_
    ui->widget_A2bSlider->setAxisOutputValue(gEnv.pDeviceConfig->paramsReport.axis_data[m_axisNumber], m_outputEnabled);
}

void Axes::outputValueChanged(bool isChecked)
{
    m_outputEnabled = isChecked;
    updateAxisOut();
    if (isChecked == true) {
        ui->progressBar_Out->setEnabled(true);
        ui->progressBar_Raw->setEnabled(true);
    } else {
        ui->progressBar_Out->setEnabled(false);
        ui->progressBar_Raw->setEnabled(false);
    }
    emit outputActiveChanged(m_axisNumber, isOutputActive());
}

void Axes::on_pushButton_SetCenter_clicked()
{
    ui->checkBox_Center->setChecked(true);
    params_report_t *paramsRep = &gEnv.pDeviceConfig->paramsReport;

    if (paramsRep->raw_axis_data[m_axisNumber] > ui->spinBox_CalibMax->value()) {
        ui->spinBox_CalibCenter->setValue(ui->spinBox_CalibMax->value());
    } else if (paramsRep->raw_axis_data[m_axisNumber] < ui->spinBox_CalibMin->value()) {
        ui->spinBox_CalibCenter->setValue(ui->spinBox_CalibMin->value());
    } else {
        ui->spinBox_CalibCenter->setValue(paramsRep->raw_axis_data[m_axisNumber]);
    }
}

void Axes::on_checkBox_Center_stateChanged(int state)
{
    if (state == 2) // 2 = true
    {
        ui->spinBox_CalibCenter->setEnabled(true);
    } else {
        ui->spinBox_CalibCenter->setEnabled(false);
        calibMinMaxValueChanged(0);
    }
}

void Axes::on_pushButton_ResetCalib_clicked()
{
    ui->spinBox_CalibMax->setValue(AXIS_MAX_VALUE);
    ui->spinBox_CalibMin->setValue(AXIS_MIN_VALUE);
}

void Axes::a2bSpinBoxChanged(int count)
{
    if (count < m_kMinA2bButtons) {
        ui->widget_A2bSlider->setEnabled(false);
        ui->widget_A2bSlider->setPointsCount(0);
        //count = kMinA2bButtons;
    } else {
        ui->widget_A2bSlider->setEnabled(true);
        ui->widget_A2bSlider->setPointsCount(count + 1);
    }

    if (ui->widget_A2bSlider->isEnabled() == true) {
        emit a2bCountChanged(count, m_a2bButtonsCount);
        m_a2bButtonsCount = count;
    } else { // optional?
        emit a2bCountChanged(0, m_a2bButtonsCount);
        m_a2bButtonsCount = 0;
    }
}

void Axes::on_checkBox_ShowExtend_stateChanged(int state)
{
    // QTimer::singleShot(10 - antiblink
    if (state == 2) { // 2 = true
        m_axesExtend->setMinimumHeight(115);
        QTimer::singleShot(10, this, [this] {
            m_axesExtend->setVisible(true);
        });
    } else {
        m_axesExtend->setVisible(false);
        QTimer::singleShot(10, this, [this] {
            m_axesExtend->setMinimumHeight(0);
            ui->frame->setMinimumHeight(0);
        });
    }
}

void Axes::readFromConfig()
{
    axis_config_t *axCfg = &gEnv.pDeviceConfig->config.axis_config[m_axisNumber];
    axis_to_buttons_t *a2bCfg = &gEnv.pDeviceConfig->config.axes_to_buttons[m_axisNumber];
    // output, inverted
    ui->checkBox_Output->setChecked(axCfg->out_enabled);
    ui->checkBox_Inverted->setChecked(axCfg->inverted);
    /* Source row lookup. For Encoder we have to also match the channel
     * (= encoder slot), since rows for Encoder 1 / Encoder 2 share the
     * same source_main enum value and only differ by axCfg->channel.
     * For everything else there's at most one matching row, so the
     * channel comparison is a no-op (channel is 0 for non-Encoder rows). */
    int index = -1;
    if (axCfg->source_main == Encoder) {
        for (int i = 0; i < m_mainSource_enumIndex.size(); ++i) {
            if (m_mainSource_enumIndex[i] == Encoder &&
                m_mainSource_channelIndex[i] == axCfg->channel) {
                index = i;
                break;
            }
        }
    } else {
        index = Converter::EnumToIndex(axCfg->source_main, m_mainSource_enumIndex);
    }
    if (index == -1) index = 0;
    ui->comboBox_AxisSource1->setCurrentIndex(index);
    // calibration
    ui->spinBox_CalibMin->setValue(axCfg->calib_min);
    ui->spinBox_CalibCenter->setValue(axCfg->calib_center);
    ui->checkBox_Center->setChecked(axCfg->is_centered);
    ui->spinBox_CalibMax->setValue(axCfg->calib_max);
    // axes to buttons
    /* Clamp buttons_cnt against the storage cap so a garbage value
     * (e.g. an uninitialised field on a fresh-flashed board reading
     * 192 / 0xC0) doesn't drive the loop past axis_to_buttons_t::points'
     * 13-byte array OR past the slider's allocated point widgets. */
    const int kMaxA2bPoints = sizeof(a2bCfg->points);
    int a2bCount = a2bCfg->buttons_cnt;
    if (a2bCount < 0) a2bCount = 0;
    if (a2bCount > kMaxA2bPoints - 1) a2bCount = kMaxA2bPoints - 1;
    ui->spinBox_A2bCount->setValue(a2bCount);
    if (a2bCount > 0) {
        for (int i = 0; i < a2bCount + 1 && i < kMaxA2bPoints; ++i) {
            ui->widget_A2bSlider->setPointValue(a2bCfg->points[i], i);
        }
    }
    // axes extended settings
    m_axesExtend->readFromConfig();
    // Reapply the no-source-no-output rule explicitly: when the loaded
    // source matches the combobox's current value, setCurrentIndex
    // doesn't fire mainSourceIndexChanged, so the guard wouldn't run
    // through that path. This catches axes that load with source = None
    // and out_enabled = true (force-uncheck them).
    applyOutputGuard();
    // Push current active state out so the Curves tab thumbnails get
    // the right "not in use" overlay state on load. Same rationale as
    // applyOutputGuard above: the toggled / index-changed signals may
    // not have fired if the loaded values match defaults.
    emit outputActiveChanged(m_axisNumber, isOutputActive());
}

void Axes::writeToConfig()
{
    axis_config_t *axCfg = &gEnv.pDeviceConfig->config.axis_config[m_axisNumber];
    axis_to_buttons_t *a2bCfg = &gEnv.pDeviceConfig->config.axes_to_buttons[m_axisNumber];
    // output, inverted
    axCfg->out_enabled = ui->checkBox_Output->isChecked();
    axCfg->inverted = ui->checkBox_Inverted->isChecked();
    // I2C, sources, function. Combobox may be at -1 if Read populated
    // it with an unmappable enum (garbage in flash on a fresh-flashed
    // board); preserve the existing field rather than indexing
    // m_mainSource_enumIndex[-1] and crashing.
    {
        const int idx = ui->comboBox_AxisSource1->currentIndex();
        if (idx >= 0 && idx < m_mainSource_enumIndex.size()) {
            axCfg->source_main = m_mainSource_enumIndex[idx];
            /* axis_config_t.channel is the encoder slot when source is
             * Encoder (read by analog.c::SOURCE_ENCODER branch as the
             * encoders_state[] index). Write it for Encoder rows so
             * Enc 1 vs Enc 2 round-trips cleanly; leave it at 0
             * otherwise so we don't clobber the field for other
             * sources that don't use it. */
            if (axCfg->source_main == Encoder) {
                axCfg->channel = m_mainSource_channelIndex[idx];
            } else {
                axCfg->channel = 0;
            }
        }
    }
    // calibration
    axCfg->calib_min = ui->spinBox_CalibMin->value();
    axCfg->calib_center = ui->spinBox_CalibCenter->value();
    axCfg->is_centered = ui->checkBox_Center->isChecked();
    axCfg->calib_max = ui->spinBox_CalibMax->value();
    // axes to buttons. Same buttons_cnt clamp as readFromConfig --
    // the spinbox's value is bounded by its setRange(), but defending
    // explicitly costs nothing and matches the read path.
    {
        const int kMaxA2bPoints = sizeof(a2bCfg->points);
        int a2bCount = ui->spinBox_A2bCount->value();
        if (a2bCount < 0) a2bCount = 0;
        if (a2bCount > kMaxA2bPoints - 1) a2bCount = kMaxA2bPoints - 1;
        a2bCfg->buttons_cnt = a2bCount;
        for (int i = 0; i < a2bCount + 1 && i < kMaxA2bPoints; ++i) {
            a2bCfg->points[i] = ui->widget_A2bSlider->pointValue(i);
        }
    }
    // axes extended settings
    m_axesExtend->writeToConfig();
}
