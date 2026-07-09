#include "shiftstimersconfig.h"
#include "ui_shiftstimersconfig.h"
#include "style_helpers.h"

#include <QSpinBox>

static const int TICKS_NS = 500;

ShiftsTimersConfig::ShiftsTimersConfig(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ShiftsTimersConfig)
{
    ui->setupUi(this);

    /* Shift modifiers live on the dedicated Shifts tab (wire gen 0x0070); the
     * old per-shift spinbox group was removed from this tab, which now hosts only
     * the global timers. (Relocating the timers to the Advanced tab is a
     * wire-independent follow-up.) */

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

    ui->spinBox_Timer1->setValue(devc->button_timer1_ms);
    ui->spinBox_Timer2->setValue(devc->button_timer2_ms);
    ui->spinBox_Timer3->setValue(devc->button_timer3_ms);

    ui->spinBox_DebounceTimer->setValue(devc->button_debounce_ms);
    ui->spinBox_A2bDebounce->setValue(devc->a2b_debounce_ms);
    ui->spinBox_LogicDebounce->setValue(devc->logic_debounce_ms);

    ui->spinBox_EncoderPressTimer->setValue(devc->encoder_press_time_ms);
    // Firmware treats encoder_gap_ms == 0 as "use the default gap" (20 ms, the
    // ENC_QUEUE_GAP_MS fallback in encoders.c). Normalise a stored 0 up to 20 in
    // BOTH the config and the widget so the spinbox (minimum 1) can't desync from
    // the config: without this, setValue(0) would clamp the display to 1 while
    // the config still held 0, and the next flushUiToConfig would silently
    // rewrite 0 -> 1 (turning the firmware's 0 -> 20 ms default into 1 ms). A 0
    // only ever reaches here from a config hand-saved by an interim gen-5 build;
    // 0 and 20 are semantically identical to the firmware, so this is a no-op
    // remap, not a behaviour change.
    if (devc->encoder_gap_ms == 0)
        devc->encoder_gap_ms = 20;
    ui->spinBox_EncoderGap->setValue(devc->encoder_gap_ms);

    ui->spinBox_ButtonsPolling->setValue(devc->button_polling_interval_ticks * TICKS_NS);
    ui->spinBox_EncoderPolling->setValue(devc->encoder_polling_interval_ticks * TICKS_NS);

    ui->spinBox_LongPressThreshold->setValue(devc->tap_cutoff_ms);
    ui->spinBox_DoubleTapWindow->setValue(devc->double_tap_window_ms);
}

void ShiftsTimersConfig::writeToConfig()
{
    dev_config_t *devc = &gEnv.pDeviceConfig->config;

    devc->button_timer1_ms = ui->spinBox_Timer1->value();
    devc->button_timer2_ms = ui->spinBox_Timer2->value();
    devc->button_timer3_ms = ui->spinBox_Timer3->value();

    devc->button_debounce_ms = ui->spinBox_DebounceTimer->value();
    devc->a2b_debounce_ms = ui->spinBox_A2bDebounce->value();
    devc->logic_debounce_ms = ui->spinBox_LogicDebounce->value();

    devc->encoder_press_time_ms = ui->spinBox_EncoderPressTimer->value();
    devc->encoder_gap_ms = ui->spinBox_EncoderGap->value();

    devc->button_polling_interval_ticks = ui->spinBox_ButtonsPolling->value() / TICKS_NS;
    devc->encoder_polling_interval_ticks = ui->spinBox_EncoderPolling->value() / TICKS_NS;

    devc->tap_cutoff_ms = ui->spinBox_LongPressThreshold->value();
    devc->double_tap_window_ms = ui->spinBox_DoubleTapWindow->value();
}

void ShiftsTimersConfig::setUiOnOff(int /*value*/)
{
    // Shifts moved to the dedicated Shifts tab; the global timers on this tab
    // are always editable, so nothing is device-gated here now.
}

void ShiftsTimersConfig::shiftStateChanged()
{
    // Live shift feedback moved to the dedicated Shifts tab (ShiftButtonConfig).
}
