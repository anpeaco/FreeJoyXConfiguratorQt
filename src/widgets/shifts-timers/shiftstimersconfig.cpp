#include "shiftstimersconfig.h"
#include "ui_shiftstimersconfig.h"
#include "style_helpers.h"

#include <QSpinBox>

// SHIFT_COUNT lives in buttonlogical.h (5 shifts + "No shift" sentinel).
// Pulled in here for the shiftStateChanged() loop that mirrors the device's
// active-shift bitmap onto the label highlights.
#include "buttonlogical.h"

static const int TICKS_NS = 500;

ShiftsTimersConfig::ShiftsTimersConfig(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ShiftsTimersConfig)
{
    ui->setupUi(this);
    m_isShifts_act = false;
    m_shift1_act = false;
    m_shift2_act = false;
    m_shift3_act = false;
    m_shift4_act = false;
    m_shift5_act = false;

    // Polling spinboxes round to TICKS_NS multiples on user edit. Same
    // helper that ButtonConfig used to own.
    auto stepSnap = [](QSpinBox *sb, int value) {
        int remainder = value % TICKS_NS;
        if (remainder == 0) return;
        value = (value / TICKS_NS) * TICKS_NS;
        if (remainder > TICKS_NS / 2) {
            sb->setValue(value + TICKS_NS);
        } else {
            sb->setValue(value);
        }
    };
    connect(ui->spinBox_ButtonsPolling, qOverload<int>(&QSpinBox::valueChanged),
            this, [this, stepSnap](int v) { stepSnap(ui->spinBox_ButtonsPolling, v); });
    connect(ui->spinBox_EncoderPolling, qOverload<int>(&QSpinBox::valueChanged),
            this, [this, stepSnap](int v) { stepSnap(ui->spinBox_EncoderPolling, v); });
}

ShiftsTimersConfig::~ShiftsTimersConfig()
{
    delete ui;
}

void ShiftsTimersConfig::retranslateUi()
{
    ui->retranslateUi(this);
}

void ShiftsTimersConfig::readFromConfig()
{
    dev_config_t *devc = &gEnv.pDeviceConfig->config;

    ui->spinBox_Shift1->setValue(devc->shift_config[0].button + 1);
    ui->spinBox_Shift2->setValue(devc->shift_config[1].button + 1);
    ui->spinBox_Shift3->setValue(devc->shift_config[2].button + 1);
    ui->spinBox_Shift4->setValue(devc->shift_config[3].button + 1);
    ui->spinBox_Shift5->setValue(devc->shift_config[4].button + 1);

    ui->spinBox_Timer1->setValue(devc->button_timer1_ms);
    ui->spinBox_Timer2->setValue(devc->button_timer2_ms);
    ui->spinBox_Timer3->setValue(devc->button_timer3_ms);

    ui->spinBox_DebounceTimer->setValue(devc->button_debounce_ms);
    ui->spinBox_A2bDebounce->setValue(devc->a2b_debounce_ms);

    ui->spinBox_EncoderPressTimer->setValue(devc->encoder_press_time_ms);

    ui->spinBox_ButtonsPolling->setValue(devc->button_polling_interval_ticks * TICKS_NS);
    ui->spinBox_EncoderPolling->setValue(devc->encoder_polling_interval_ticks * TICKS_NS);

    ui->spinBox_LongPressThreshold->setValue(devc->long_press_threshold_ms);
    ui->spinBox_DoubleTapWindow->setValue(devc->double_tap_window_ms);
}

void ShiftsTimersConfig::writeToConfig()
{
    dev_config_t *devc = &gEnv.pDeviceConfig->config;
    devc->shift_config[0].button = ui->spinBox_Shift1->value() - 1;
    devc->shift_config[1].button = ui->spinBox_Shift2->value() - 1;
    devc->shift_config[2].button = ui->spinBox_Shift3->value() - 1;
    devc->shift_config[3].button = ui->spinBox_Shift4->value() - 1;
    devc->shift_config[4].button = ui->spinBox_Shift5->value() - 1;

    devc->button_timer1_ms = ui->spinBox_Timer1->value();
    devc->button_timer2_ms = ui->spinBox_Timer2->value();
    devc->button_timer3_ms = ui->spinBox_Timer3->value();

    devc->button_debounce_ms = ui->spinBox_DebounceTimer->value();
    devc->a2b_debounce_ms = ui->spinBox_A2bDebounce->value();

    devc->encoder_press_time_ms = ui->spinBox_EncoderPressTimer->value();

    devc->button_polling_interval_ticks = ui->spinBox_ButtonsPolling->value() / TICKS_NS;
    devc->encoder_polling_interval_ticks = ui->spinBox_EncoderPolling->value() / TICKS_NS;

    devc->long_press_threshold_ms = ui->spinBox_LongPressThreshold->value();
    devc->double_tap_window_ms = ui->spinBox_DoubleTapWindow->value();
}

void ShiftsTimersConfig::setUiOnOff(int value)
{
    const bool on = (value > 0);
    ui->spinBox_Shift1->setEnabled(on);
    ui->spinBox_Shift2->setEnabled(on);
    ui->spinBox_Shift3->setEnabled(on);
    ui->spinBox_Shift4->setEnabled(on);
    ui->spinBox_Shift5->setEnabled(on);
}

void ShiftsTimersConfig::shiftStateChanged()
{
    params_report_t *paramsRep = &gEnv.pDeviceConfig->paramsReport;

    for (int i = 0; i < SHIFT_COUNT; ++i)
    {
        if (paramsRep->shift_button_data & (1 << (i & 0x07))) {
            m_isShifts_act = true;

            if (i == 0 && m_shift1_act == false) {
                freejoy_style::setRole(ui->text_shift1_logicalButton, "shiftActive", true);
                m_shift1_act = true;
            } else if (i == 1 && m_shift2_act == false) {
                freejoy_style::setRole(ui->text_shift2_logicalButton, "shiftActive", true);
                m_shift2_act = true;
            } else if (i == 2 && m_shift3_act == false) {
                freejoy_style::setRole(ui->text_shift3_logicalButton, "shiftActive", true);
                m_shift3_act = true;
            } else if (i == 3 && m_shift4_act == false) {
                freejoy_style::setRole(ui->text_shift4_logicalButton, "shiftActive", true);
                m_shift4_act = true;
            } else if (i == 4 && m_shift5_act == false) {
                freejoy_style::setRole(ui->text_shift5_logicalButton, "shiftActive", true);
                m_shift5_act = true;
            }

        } else if (m_isShifts_act == true) {
            if (i == 0 && m_shift1_act == true) {
                freejoy_style::clearRole(ui->text_shift1_logicalButton, "shiftActive");
                m_shift1_act = false;
            } else if (i == 0 && m_shift2_act == true) {
                freejoy_style::clearRole(ui->text_shift2_logicalButton, "shiftActive");
                m_shift2_act = false;
            } else if (i == 0 && m_shift3_act == true) {
                freejoy_style::clearRole(ui->text_shift3_logicalButton, "shiftActive");
                m_shift3_act = false;
            } else if (i == 0 && m_shift4_act == true) {
                freejoy_style::clearRole(ui->text_shift4_logicalButton, "shiftActive");
                m_shift4_act = false;
            } else if (i == 0 && m_shift5_act == true) {
                freejoy_style::clearRole(ui->text_shift5_logicalButton, "shiftActive");
                m_shift5_act = false;
            }

            if (m_shift1_act == false && m_shift2_act == false && m_shift3_act == false && m_shift4_act == false
                && m_shift5_act == false) {
                m_isShifts_act = false;
            }
        }
    }
}
