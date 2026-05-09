#include "fastencoder.h"
#include "ui_fastencoder.h"

FastEncoder::FastEncoder(int fastEncoderIndex, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FastEncoder)
    , m_index(fastEncoderIndex)
{
    ui->setupUi(this);
    m_notDefined = tr("Not defined");

    ui->label_EncoderIndex->setNum(m_index + 1);
    ui->label_ButtonNumberA->setText(m_notDefined);
    ui->label_ButtonNumberB->setText(m_notDefined);

    // dropdown excludes ENCODER_CONF_1x for fast encoders (2x and 4x only)
    for (int i = 1; i < ENCODER_TYPE_COUNT; ++i) {
        ui->comboBox_EncoderType->addItem(m_encoderTypeList[i].guiName);
    }

    /* The Enable checkbox is *user-facing only*; the persistent state
     * (whether this encoder is enabled) is derived from whether both
     * pins are mapped to FAST_ENCODER in Pin Config. We forward toggles
     * upstream so MainWindow can do the pin-replace prompt + assignment;
     * setInput / clearInput then drives the checkbox back to match the
     * resulting pin state. */
    connect(ui->checkBox_Enable, &QCheckBox::clicked,
            this, [this](bool checked) {
        emit enableToggleRequested(m_index, checked);
    });

    setUiOnOff();
}

FastEncoder::~FastEncoder()
{
    delete ui;
}

void FastEncoder::retranslateUi()
{
    ui->retranslateUi(this);
}

void FastEncoder::setInput(const QString &pinGuiName)
{
    if (m_inputA.isEmpty()) {
        m_inputA = pinGuiName;
        ui->label_ButtonNumberA->setText(pinGuiName);
    } else if (m_inputB.isEmpty()) {
        m_inputB = pinGuiName;
        ui->label_ButtonNumberB->setText(pinGuiName);
    }
    setUiOnOff();
}

void FastEncoder::clearInput(const QString &pinGuiName)
{
    if (m_inputA == pinGuiName) {
        m_inputA.clear();
        ui->label_ButtonNumberA->setText(m_notDefined);
    } else if (m_inputB == pinGuiName) {
        m_inputB.clear();
        ui->label_ButtonNumberB->setText(m_notDefined);
    }
    setUiOnOff();
}

bool FastEncoder::hasBothInputs() const
{
    return !m_inputA.isEmpty() && !m_inputB.isEmpty();
}

void FastEncoder::setUiOnOff()
{
    const bool on = hasBothInputs();
    /* Children except the Enable checkbox follow the on-state -- when
     * the encoder isn't fully wired the type dropdown and pin labels
     * are read-only. The checkbox stays interactive so the user can
     * always toggle the encoder on (which is the whole point of this
     * affordance). */
    for (auto&& child : findChildren<QWidget *>()) {
        if (child == ui->checkBox_Enable) continue;
        child->setEnabled(on);
    }
    syncEnableCheckbox();
}

void FastEncoder::syncEnableCheckbox()
{
    /* Reflect pin state in the checkbox without re-firing the toggle
     * signal -- otherwise pin updates from Pin Config would loop back
     * through enableToggleRequested. Tristate is set when exactly one
     * of the two pins is mapped (legacy / hand-edited config); the user
     * can click to either complete or clear the pair. */
    QSignalBlocker bl(ui->checkBox_Enable);
    const bool aSet = !m_inputA.isEmpty();
    const bool bSet = !m_inputB.isEmpty();
    if (aSet && bSet) {
        ui->checkBox_Enable->setTristate(false);
        ui->checkBox_Enable->setCheckState(Qt::Checked);
    } else if (!aSet && !bSet) {
        ui->checkBox_Enable->setTristate(false);
        ui->checkBox_Enable->setCheckState(Qt::Unchecked);
    } else {
        ui->checkBox_Enable->setTristate(true);
        ui->checkBox_Enable->setCheckState(Qt::PartiallyChecked);
    }
}

void FastEncoder::readFromConfig()
{
    // - 1 to skip ENCODER_CONF_1x which the dropdown excludes; clamp to 0 for
    // factory-defaulted (uninitialised) configs so the dropdown shows "2x".
    int idx = gEnv.pDeviceConfig->config.fast_encoders[m_index].mode - 1;
    ui->comboBox_EncoderType->setCurrentIndex(idx >= 0 ? idx : 0);
}

void FastEncoder::writeToConfig()
{
    gEnv.pDeviceConfig->config.fast_encoders[m_index].mode    = ui->comboBox_EncoderType->currentIndex() + 1;
    gEnv.pDeviceConfig->config.fast_encoders[m_index].enabled = hasBothInputs() ? 1 : 0;
}
