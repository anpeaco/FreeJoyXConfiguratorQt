#ifndef ENCODERSCONFIG_H
#define ENCODERSCONFIG_H

#include <QWidget>

#include "deviceconfig.h"
#include "encoders.h"
#include "fastencoder.h"
#include "global.h"

namespace Ui {
class EncodersConfig;
}

class EncodersConfig : public QWidget
{
    Q_OBJECT

public:
    explicit EncodersConfig(QWidget *parent = nullptr);
    ~EncodersConfig();

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

public slots:
    void encoderInputChanged(int encoder_A, int encoder_B);
    void fastEncoderSelected(const QString &pinGuiName, bool isSelected);

private:
    Ui::EncodersConfig *ui;

    QList<Encoders *> m_encodersPtrList;
    QList<FastEncoder *> m_fastEncodersPtrList;
    int m_encodersInput_A_count;
    int m_encodersInput_B_count;
};

#endif // ENCODERSCONFIG_H
