#ifndef LED_H
#define LED_H

#include <QWidget>

#include "deviceconfig.h"
#include "global.h"

namespace Ui {
class LED;
}

class LED : public QWidget
{
    Q_OBJECT

public:
    explicit LED(int ledNumber, QWidget *parent = nullptr);
    ~LED();

    int currentButtonSelected() const;

    void updateButtonStyle(bool checked);

    void setLedState(bool state);

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

    bool isHostControlled() const;

    /* Re-renders the Timer dropdown items as "T<n> (<value> ms)" using
     * the current led_timer_ms[0..3] values from gEnv.pDeviceConfig.
     * LedConfig calls this on every row when one of its own timer
     * spinboxes changes, and on readFromConfig completion. */
    void refreshTimerLabels();

signals:
    void ledToggled(int ledNumber, bool state);

protected:
    void changeEvent(QEvent *event) override;

private:
    Ui::LED *ui;
    int m_ledNumber;

    bool m_ledCurrentState;

    const deviceEnum_guiName_t m_ledList[2] = // порядок обязан быть как в common_types.h!!!!!!!!!!!
    {
        {LED_NORMAL, tr("Normal")},
        {LED_INVERTED, tr("Inverted")},
    };

    const deviceEnum_guiName_t m_TimerList[5] = // порядок обязан быть как в common_types.h!!!!!!!!!!!
    {
         {-1, tr("-")},
         {0, tr("Timer 1")},
         {1, tr("Timer 2")},
         {2, tr("Timer 3")},
         {3, tr("Timer 4")},
    };
};

#endif // LED_H
