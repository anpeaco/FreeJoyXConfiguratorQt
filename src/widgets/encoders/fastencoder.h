#ifndef FASTENCODER_H
#define FASTENCODER_H

#include <QWidget>

#include "deviceconfig.h"
#include "encoders.h"		// for ENCODER_TYPE_COUNT and deviceEnum_guiName_t
#include "global.h"

namespace Ui {
class FastEncoder;
}

class FastEncoder : public QWidget
{
    Q_OBJECT

public:
    explicit FastEncoder(int fastEncoderIndex, QWidget *parent = nullptr);
    ~FastEncoder();

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

    // Pin selection routes here from EncodersConfig::fastEncoderSelected.
    // setInput populates whichever of the two slots (A/B) is empty;
    // clearInput clears the slot that matches pinGuiName.
    void setInput(const QString &pinGuiName);
    void clearInput(const QString &pinGuiName);

    bool hasBothInputs() const;

private:
    Ui::FastEncoder *ui;
    void setUiOnOff();

    int m_index;					// 0..MAX_FAST_ENCODER_NUM-1, indexes fast_encoders[]
    QString m_inputA;				// pin gui name in slot A (e.g. "Pin A8"), empty if unset
    QString m_inputB;
    QString m_notDefined;

    const deviceEnum_guiName_t m_encoderTypeList[ENCODER_TYPE_COUNT] =
    {
        {ENCODER_CONF_1x, tr("Encoder 1x")},
        {ENCODER_CONF_2x, tr("Encoder 2x")},
        {ENCODER_CONF_4x, tr("Encoder 4x")},
    };
};

#endif // FASTENCODER_H
