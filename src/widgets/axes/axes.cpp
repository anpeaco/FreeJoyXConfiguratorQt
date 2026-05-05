#include "axes.h"
#include "ui_axes.h"
#include <QTimer>
#include <QTranslator>
#include "converter.h"

const QVector <deviceEnum_guiName_t> &axesList()    // порядок обязан быть как в common_types.h!!!!!!!!!!!
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

    // add main source. Cache the original display text under each
    // enum so markSourcesInUse() can append/restore "(used by X)"
    // suffixes without compounding.
    for (int i = 0; i < 2; ++i) {
        ui->comboBox_AxisSource1->addItem(m_axesPinList[i].guiName);
        m_mainSource_enumIndex.push_back(m_axesPinList[i].deviceEnumIndex);
        m_baseDisplayTextByEnum.insert(m_axesPinList[i].deviceEnumIndex, m_axesPinList[i].guiName);
    }

    // set a2b  // двойная работа? readFromConfig()
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
        const QString baseText = m_axesPinList[pinIdx].guiName + " - " + sourceName;
        ui->comboBox_AxisSource1->addItem(baseText);
        m_mainSource_enumIndex.push_back(m_axesPinList[pinIdx].deviceEnumIndex);
        m_baseDisplayTextByEnum.insert(sourceEnum, baseText);
    } else {
        for (int i = 0; i < m_mainSource_enumIndex.size(); ++i) {
            if (m_mainSource_enumIndex[i] == sourceEnum) {
                if (ui->comboBox_AxisSource1->currentIndex() == i) {
                    ui->comboBox_AxisSource1->setCurrentIndex(0);
                }
                ui->comboBox_AxisSource1->removeItem(i);
                m_mainSource_enumIndex.erase(m_mainSource_enumIndex.begin() + i);
                m_baseDisplayTextByEnum.remove(sourceEnum);
                break;
            }
        }
    }
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
        ui->spinBox_CalibMax->setValue(AXIS_MIN_VALUE); // не перепутано
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
    } else { // необязательно?
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
    int index = Converter::EnumToIndex(axCfg->source_main, m_mainSource_enumIndex);
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
    // I2C, sources, function
    axCfg->source_main = m_mainSource_enumIndex[ui->comboBox_AxisSource1->currentIndex()];
    // calibration
    axCfg->calib_min = ui->spinBox_CalibMin->value();
    axCfg->calib_center = ui->spinBox_CalibCenter->value();
    axCfg->is_centered = ui->checkBox_Center->isChecked();
    axCfg->calib_max = ui->spinBox_CalibMax->value();
    // axes to buttons
    a2bCfg->buttons_cnt = ui->spinBox_A2bCount->value();
    for (int i = 0; i < ui->spinBox_A2bCount->value() + 1; ++i) {
        a2bCfg->points[i] = ui->widget_A2bSlider->pointValue(i);
    }
    // axes extended settings
    m_axesExtend->writeToConfig();
}
