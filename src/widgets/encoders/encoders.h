#ifndef ENCODERS_H
#define ENCODERS_H

#include <QWidget>

#include "deviceconfig.h"
#include "global.h"

#define ENCODER_TYPE_COUNT 3
namespace Ui {
class Encoders;
}

class Encoders : public QWidget
{
    Q_OBJECT

public:
    explicit Encoders(int encodersNumber, QWidget *parent = nullptr);
    ~Encoders();

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

    int inputA() const;
    int inputB() const;

    void setInputA(int);
    void setInputB(int);

private:
    Ui::Encoders *ui;
    void setUiOnOff();

    int m_encodersNumber;	// 1-based display index shown in the UI label
    int m_configIndex;		// index into dev_config encoders[] / encoders_state[]
    int m_input_A;
    int m_input_B;
    QString m_notDefined;

    const deviceEnum_guiName_t m_encoderTypeList[ENCODER_TYPE_COUNT] = // order MUST match common_types.h!
    {
        {ENCODER_CONF_1x, tr("Encoder 1x")},
        {ENCODER_CONF_2x, tr("Encoder 2x")},
        {ENCODER_CONF_4x, tr("Encoder 4x")},
    };
};

#endif // ENCODERS_H
