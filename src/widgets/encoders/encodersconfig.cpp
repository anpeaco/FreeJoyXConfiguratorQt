#include "encodersconfig.h"
#include "ui_encodersconfig.h"

EncodersConfig::EncodersConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EncodersConfig)
{
    ui->setupUi(this);
    m_encodersInput_A_count = 0;
    m_encodersInput_B_count = 0;

    // spawn fast-encoder rows -- one per fast encoder slot. The widgets
    // own their own pin-name labels, mode dropdown, and counter state.
    ui->layoutV_FastEncoders->setAlignment(Qt::AlignTop);
    for (int i = 0; i < MAX_FAST_ENCODER_NUM; ++i) {
        FastEncoder *fe = new FastEncoder(i, this);
        ui->layoutV_FastEncoders->addWidget(fe);
        m_fastEncodersPtrList.append(fe);
    }

    // spawn slow-encoder rows below
    ui->layoutV_Encoders->setAlignment(Qt::AlignTop);
    for (int i = 0; i < MAX_ENCODERS_NUM - MAX_FAST_ENCODER_NUM; i++) {
        Encoders * encoder = new Encoders(i, this);
        ui->layoutV_Encoders->addWidget(encoder);
        m_encodersPtrList.append(encoder);
    }
}

EncodersConfig::~EncodersConfig()
{
    delete ui;
}

void EncodersConfig::retranslateUi()
{
    ui->retranslateUi(this);
    for (int i = 0; i < m_fastEncodersPtrList.size(); ++i) {
        m_fastEncodersPtrList[i]->retranslateUi();
    }
    for (int i = 0; i < m_encodersPtrList.size(); ++i) {
        m_encodersPtrList[i]->retranslateUi();
    }
}

void EncodersConfig::fastEncoderSelected(const QString &pinGuiName, bool isSelected)
{
    // FAST_ENCODER role is allowed on PA8/PA9 (Encoder 1, TIM1, fast slot 0)
    // and PB6/PB7 (Encoder 2, TIM4, fast slot 1). Dispatch by pin name suffix.
    int slot = (pinGuiName.endsWith("B6") || pinGuiName.endsWith("B7")) ? 1 : 0;

    if (slot >= m_fastEncodersPtrList.size()) {
        return;
    }
    FastEncoder *fe = m_fastEncodersPtrList[slot];
    if (isSelected) {
        fe->setInput(pinGuiName);
    } else {
        fe->clearInput(pinGuiName);
    }
}


void EncodersConfig::encoderInputChanged(int encoder_A, int encoder_B)      // messy -- worth a rewrite
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
    for (int i = 0; i < m_fastEncodersPtrList.size(); ++i) {
        m_fastEncodersPtrList[i]->readFromConfig();
    }
    for (int i = 0; i < m_encodersPtrList.size(); ++i) {
        m_encodersPtrList[i]->readFromConfig();
    }
}

void EncodersConfig::writeToConfig()
{
    for (int i = 0; i < m_fastEncodersPtrList.size(); ++i) {
        m_fastEncodersPtrList[i]->writeToConfig();
    }
    for (int i = 0; i < m_encodersPtrList.size(); ++i) {
        m_encodersPtrList[i]->writeToConfig();
    }
}
