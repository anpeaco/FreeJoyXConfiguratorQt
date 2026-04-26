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
    for (auto&& child : findChildren<QWidget *>()) {
        child->setEnabled(on);
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
