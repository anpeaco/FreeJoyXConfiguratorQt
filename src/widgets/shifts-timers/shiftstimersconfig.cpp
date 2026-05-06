#include "shiftstimersconfig.h"
#include "ui_shiftstimersconfig.h"
#include "style_helpers.h"

#include <QSpinBox>

// SHIFT_COUNT lives in buttonlogical.h (MAX_SHIFTS_NUM shifts + "No shift"
// sentinel). Pulled in here for the shiftStateChanged() loop that mirrors
// the device's active-shift bitmap onto the label highlights.
#include "buttonlogical.h"

static const int TICKS_NS = 500;

ShiftsTimersConfig::ShiftsTimersConfig(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ShiftsTimersConfig)
{
    ui->setupUi(this);
    m_isShifts_act = false;
    for (int i = 0; i < MAX_SHIFTS_NUM; ++i) {
        m_shift_act[i] = false;
    }

    /* Per-slot widget tables. v1.7.8 (issue anpeaco/FreeJoyX#1) refactored
     * the prior hand-unrolled per-slot code into loops over these arrays;
     * the .ui carries spinBox_Shift1..8 / text_shiftN_logicalButton labels
     * for slots 1..MAX_SHIFTS_NUM, addressed here as 0..MAX_SHIFTS_NUM-1. */
    m_spinBoxes[0] = ui->spinBox_Shift1;
    m_spinBoxes[1] = ui->spinBox_Shift2;
    m_spinBoxes[2] = ui->spinBox_Shift3;
    m_spinBoxes[3] = ui->spinBox_Shift4;
    m_spinBoxes[4] = ui->spinBox_Shift5;
    m_spinBoxes[5] = ui->spinBox_Shift6;
    m_spinBoxes[6] = ui->spinBox_Shift7;
    m_spinBoxes[7] = ui->spinBox_Shift8;

    m_labels[0] = ui->text_shift1_logicalButton;
    m_labels[1] = ui->text_shift2_logicalButton;
    m_labels[2] = ui->text_shift3_logicalButton;
    m_labels[3] = ui->text_shift4_logicalButton;
    m_labels[4] = ui->text_shift5_logicalButton;
    m_labels[5] = ui->text_shift6_logicalButton;
    m_labels[6] = ui->text_shift7_logicalButton;
    m_labels[7] = ui->text_shift8_logicalButton;

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

    /* Forward Timer 1/2/3 edits to a single buttonTimersChanged signal
     * so MainWindow can refresh Button Config's dropdown labels in one
     * place, not three. Each handler pushes the new value into
     * gEnv.pDeviceConfig->config FIRST, then emits. The push is
     * essential because timer_label.h::timerSlotDisplayName() reads
     * from dev_config; without this, button_timerN_ms would still
     * hold the value from the last writeToConfig and the dropdowns
     * would render stale "(X ms)" suffixes until Write was clicked.
     * Fires on user edits AND on readFromConfig's setValue calls --
     * same path either way. */
    connect(ui->spinBox_Timer1, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int v) {
                gEnv.pDeviceConfig->config.button_timer1_ms = v;
                emit buttonTimersChanged();
            });
    connect(ui->spinBox_Timer2, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int v) {
                gEnv.pDeviceConfig->config.button_timer2_ms = v;
                emit buttonTimersChanged();
            });
    connect(ui->spinBox_Timer3, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int v) {
                gEnv.pDeviceConfig->config.button_timer3_ms = v;
                emit buttonTimersChanged();
            });
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

    for (int i = 0; i < MAX_SHIFTS_NUM; ++i) {
        m_spinBoxes[i]->setValue(devc->shift_config[i].button + 1);
    }

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
    for (int i = 0; i < MAX_SHIFTS_NUM; ++i) {
        devc->shift_config[i].button = m_spinBoxes[i]->value() - 1;
    }

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
    for (int i = 0; i < MAX_SHIFTS_NUM; ++i) {
        m_spinBoxes[i]->setEnabled(on);
    }
}

void ShiftsTimersConfig::shiftStateChanged()
{
    /* Mirror paramsRep->shift_button_data (one bit per shift slot) onto
     * the per-slot label "shiftActive" highlight. v1.7.8 (issue
     * anpeaco/FreeJoyX#1) collapsed the prior 5 hand-unrolled if/else
     * chains into this loop -- which incidentally fixes a bug where
     * every "shift cleared" branch checked `i == 0` regardless of the
     * actual slot, so only slot 1's clear ever fired. */
    params_report_t *paramsRep = &gEnv.pDeviceConfig->paramsReport;

    bool any_active = false;
    for (int i = 0; i < MAX_SHIFTS_NUM; ++i) {
        const bool active = (paramsRep->shift_button_data & (1 << i)) != 0;
        if (active) {
            any_active = true;
            if (!m_shift_act[i]) {
                freejoy_style::setRole(m_labels[i], "shiftActive", true);
                m_shift_act[i] = true;
            }
        } else if (m_shift_act[i]) {
            freejoy_style::clearRole(m_labels[i], "shiftActive");
            m_shift_act[i] = false;
        }
    }
    m_isShifts_act = any_active;
}
