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

    /* Pull the FastEncoder's Enable checkbox back into sync with the
     * actual pin state. Used by MainWindow's toggle handler after the
     * user dismisses the "pin already in use" confirmation, so the
     * checkbox doesn't get stuck on "checked" when no pin assignment
     * happened. */
    void refreshFastEncoderUi(int slotIndex);

signals:
    /* Forwarded from each FastEncoder when the user toggles its
     * Enable checkbox. MainWindow handles the cross-tab work
     * (checking PinConfig for current usage, prompting on conflict,
     * applying / clearing the FAST_ENCODER assignment on both pins). */
    void fastEncoderEnableToggleRequested(int slotIndex, bool desiredEnabled);

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
