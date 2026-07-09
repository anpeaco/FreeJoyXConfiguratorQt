#include "shiftbuttonconfig.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>

#include "buttonlogical.h"
#include "global.h"
#include "deviceconfig.h"

ShiftButtonConfig::ShiftButtonConfig(QWidget *parent)
    : QWidget(parent)
{
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(4);

    auto *hint = new QLabel(
        tr("A shift layer is held while its button is active. Shift buttons are "
           "configured like normal buttons but never appear as joystick buttons "
           "— use Toggle for a latching shift-lock, or Logic to combine inputs."),
        this);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color: palette(mid); font-style: italic;"));
    lay->addWidget(hint);

    for (int i = 0; i < MAX_SHIFTS_NUM; ++i) {
        auto *row = new ButtonLogical(i, this);
        lay->addWidget(row);
        m_rows.append(row);
    }
    // Pin the rows to the top; the stretch collapses if they ever overflow.
    lay->addStretch(1);

    // Build each row's dropdowns, THEN switch it to shift mode (setTarget's
    // type filter needs the Function dropdown populated first). Deferred to the
    // event loop -- like ButtonConfig::logicaButtonsCreator -- because doing the
    // grouped-combobox setup during MainWindow construction (before the event
    // loop, before the widget is shown) hangs the UI.
    QTimer::singleShot(0, this, [this] {
        for (ButtonLogical *row : m_rows) {
            row->initialization();
            row->setTarget(ButtonLogical::ShiftButtons);
        }
    });
}

void ShiftButtonConfig::readFromConfig()
{
    for (ButtonLogical *row : m_rows) {
        row->readFromConfig();
    }
}

void ShiftButtonConfig::writeToConfig()
{
    for (ButtonLogical *row : m_rows) {
        row->writeToConfig();
    }
}

void ShiftButtonConfig::setUiOnOff(int value)
{
    // value == total physical buttons (0 when disconnected). Set the per-row
    // physical-button spinbox max and enabled state, same as ButtonConfig.
    for (ButtonLogical *row : m_rows) {
        row->setMaxPhysButtons(value);
        row->setSpinBoxOnOff(value);
    }
}

void ShiftButtonConfig::shiftStateChanged()
{
    if (gEnv.pDeviceConfig == nullptr) {
        return;
    }
    const params_report_t *pr = &gEnv.pDeviceConfig->paramsReport;
    for (int i = 0; i < m_rows.size() && i < MAX_SHIFTS_NUM; ++i) {
        m_rows[i]->setButtonState((pr->shift_button_data & (1 << i)) != 0);
    }
}
