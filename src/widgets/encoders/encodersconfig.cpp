#include "encodersconfig.h"
#include "ui_encodersconfig.h"

#include <QLabel>

EncodersConfig::EncodersConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EncodersConfig)
{
    ui->setupUi(this);
    m_encodersInput_A_count = 0;
    m_encodersInput_B_count = 0;
    m_notDefined = tr("Not defined");

    m_fastEncoderInput_A = 0;
    m_fastEncoderInput_B = 0;
    m_fastEncoder2Input_A = 0;
    m_fastEncoder2Input_B = 0;

    for (int i = 1; i < ENCODER_TYPE_COUNT; ++i) {      // i = 1 - fast encoder without ENCODER_CONF_1x
        ui->comboBox_EncoderType->addItem(m_fastEncoderTypeList[i].guiName);
        ui->comboBox_EncoderType2->addItem(m_fastEncoderTypeList[i].guiName);
    }
    ui->label_ButtonNumberA->setText(m_notDefined);
    ui->label_ButtonNumberB->setText(m_notDefined);
    ui->label_ButtonNumberA2->setText(m_notDefined);
    ui->label_ButtonNumberB2->setText(m_notDefined);

    ui->layoutV_Encoders->setAlignment(Qt::AlignTop);
    // encoders spawn
    for (int i = 0; i < MAX_ENCODERS_NUM - MAX_FAST_ENCODER_NUM; i++)
    {
        Encoders * encoder = new Encoders(i, this);
        ui->layoutV_Encoders->addWidget(encoder);
        m_encodersPtrList.append(encoder);
        //encoder->hide();
    }
}

EncodersConfig::~EncodersConfig()
{
    delete ui;
}

void EncodersConfig::retranslateUi()
{
    ui->retranslateUi(this);
    for (int i = 0; i < m_encodersPtrList.size(); ++i) {
        m_encodersPtrList[i]->retranslateUi();
    }
}

void EncodersConfig::fastEncoderSelected(const QString &pinGuiName, bool isSelected)
{
    // FAST_ENCODER role is allowed on PA8/PA9 (Encoder 1, TIM1) and PB6/PB7
    // (Encoder 2, TIM4). Dispatch by pin name to the right group of UI labels
    // and counters.
    const bool isEncoder2 = pinGuiName.endsWith("B6") || pinGuiName.endsWith("B7");

    QLabel *labelA = isEncoder2 ? ui->label_ButtonNumberA2 : ui->label_ButtonNumberA;
    QLabel *labelB = isEncoder2 ? ui->label_ButtonNumberB2 : ui->label_ButtonNumberB;
    int &inputA = isEncoder2 ? m_fastEncoder2Input_A : m_fastEncoderInput_A;
    int &inputB = isEncoder2 ? m_fastEncoder2Input_B : m_fastEncoderInput_B;

    if (isSelected == true) {
        if (labelA->text() == m_notDefined) {
            labelA->setText(pinGuiName);
            inputA++;
        } else {
            labelB->setText(pinGuiName);
            inputB++;
        }
    } else {
        if (labelA->text() == pinGuiName) {
            labelA->setText(m_notDefined);
            inputA--;
        } else {
            labelB->setText(m_notDefined);
            inputB--;
        }
    }
    setUiOnOff();
}

void EncodersConfig::setUiOnOff()
{
    const bool enc1On = (m_fastEncoderInput_A > 0 && m_fastEncoderInput_B > 0);
    for (auto&& child : ui->groupBox_FastEncoder->findChildren<QWidget *>()) {
        child->setEnabled(enc1On);
    }

    const bool enc2On = (m_fastEncoder2Input_A > 0 && m_fastEncoder2Input_B > 0);
    for (auto&& child : ui->groupBox_FastEncoder2->findChildren<QWidget *>()) {
        child->setEnabled(enc2On);
    }
}


void EncodersConfig::encoderInputChanged(int encoder_A, int encoder_B)      // говнокод
{
    int tmp_add = 0;

    // add encoder A input
    if (encoder_A > 0)
    {
        m_encodersInput_A_count++;
        for (int i = 0; i < m_encodersInput_A_count; ++i)
        {
            if (encoder_A < m_encodersPtrList[i]->inputA() || m_encodersPtrList[i]->inputA() == 0)    // encoder_A != 0 && ( legacy
            {
                if (m_encodersPtrList[i]->inputA() != 0)
                {
                    tmp_add = m_encodersPtrList[i]->inputA();
                    m_encodersPtrList[i]->setInputA(encoder_A);
                    encoder_A = tmp_add;
                }
                else if (m_encodersPtrList[i]->inputA() == 0)
                {
                    m_encodersPtrList[i]->setInputA(encoder_A);
                }
            }
        }
    }
    // delete encoder A input
    else if (encoder_A < 0)
    {
        encoder_A = -encoder_A;
        for (int i = 0; i < m_encodersInput_A_count; ++i)
        {
            if (m_encodersPtrList[i]->inputA() == encoder_A)   //encoder_A != 0 && (
            {
                for (int j = i; j < m_encodersInput_A_count; ++j)
                {
                    if (j+1 < m_encodersPtrList.size()) {
                        m_encodersPtrList[j]->setInputA(m_encodersPtrList[j+1]->inputA());
                    } else {
                        m_encodersPtrList[j]->setInputA(0);
                    }
                }
                break;
            }
        }
        m_encodersInput_A_count--;
    }

    // add encoder B input
    if (encoder_B > 0)              // else?
    {
        m_encodersInput_B_count++;

        for (int i = 0; i < m_encodersInput_B_count; ++i)
        {
            if (encoder_B < m_encodersPtrList[i]->inputB() || m_encodersPtrList[i]->inputB() == 0)        //encoder_B != 0 && (
            {
                if (m_encodersPtrList[i]->inputB() != 0)
                {
                    tmp_add = m_encodersPtrList[i]->inputB();
                    m_encodersPtrList[i]->setInputB(encoder_B);
                    encoder_B = tmp_add;
                }
                else if (m_encodersPtrList[i]->inputB() == 0)
                {
                    m_encodersPtrList[i]->setInputB(encoder_B);
                }
            }
        }
    }
    // delete encoder B input
    else if (encoder_B < 0)
    {
        encoder_B = -encoder_B;
        for (int i = 0; i < m_encodersInput_B_count; ++i)
        {
            if (m_encodersPtrList[i]->inputB() == encoder_B)       //encoder_B != 0 &&
            {
                for (int j = i; j < m_encodersInput_B_count; ++j)
                {
                    if (j+1 < m_encodersPtrList.size()) {
                        m_encodersPtrList[j]->setInputB(m_encodersPtrList[j+1]->inputB());
                    } else {
                        m_encodersPtrList[j]->setInputB(0);
                    }
                }
                break;
            }
        }
        m_encodersInput_B_count--;
    }
}

void EncodersConfig::readFromConfig()
{
    // - 1 to skip ENCODER_CONF_1x which the dropdown excludes; clamp to 0 for
    // factory-defaulted (uninitialised) configs so the dropdown shows "2x"
    // rather than no selection.
    int idx0 = gEnv.pDeviceConfig->config.fast_encoders[0].mode - 1;
    ui->comboBox_EncoderType->setCurrentIndex(idx0 >= 0 ? idx0 : 0);

    int idx1 = gEnv.pDeviceConfig->config.fast_encoders[1].mode - 1;
    ui->comboBox_EncoderType2->setCurrentIndex(idx1 >= 0 ? idx1 : 0);

    for (int i = 0; i < MAX_ENCODERS_NUM - MAX_FAST_ENCODER_NUM; i++)
    {
        m_encodersPtrList[i]->readFromConfig();
    }
}

void EncodersConfig::writeToConfig()
{
    // + 1 to map dropdown index back to ENCODER_CONF_2x/_4x enum values.
    // .enabled tracks whether the user has selected FAST_ENCODER on both
    // pins of the encoder (counters maintained by fastEncoderSelected).
    gEnv.pDeviceConfig->config.fast_encoders[0].mode = ui->comboBox_EncoderType->currentIndex() + 1;
    gEnv.pDeviceConfig->config.fast_encoders[0].enabled =
        (m_fastEncoderInput_A > 0 && m_fastEncoderInput_B > 0) ? 1 : 0;

    gEnv.pDeviceConfig->config.fast_encoders[1].mode = ui->comboBox_EncoderType2->currentIndex() + 1;
    gEnv.pDeviceConfig->config.fast_encoders[1].enabled =
        (m_fastEncoder2Input_A > 0 && m_fastEncoder2Input_B > 0) ? 1 : 0;

    for (int i = 0; i < MAX_ENCODERS_NUM - MAX_FAST_ENCODER_NUM; i++)
    {
        m_encodersPtrList[i]->writeToConfig();
    }
}
