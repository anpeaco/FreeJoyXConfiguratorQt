#include "gpioexpanderconfig.h"

#include "deviceconfig.h"
#include "global.h"
#include "style_helpers.h"    // makeAlertBanner / accentAmber (shared alert-banner look)
#include "common_defines.h"   // MAX_GPIO_EXPANDER_NUM, USED_PINS_NUM
#include "common_types.h"     // GPIO_EXP_*, pin roles (I2C_SCL, SPI_SCK, SPI_GPIO_CS, ...)

#include <QGridLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QFrame>

/* flags bits -- mirror firmware gpio_expander.h */
static const uint8_t FLAG_PULLUPS = 0x01;
static const uint8_t FLAG_INVERT  = 0x02;

static const int kAddrLo = 0x20;   // MCP2301x strap range 0x20..0x27
static const int kAddrHi = 0x27;

// Shared count-spinbox width, kept equal to the shift registers' count boxes so
// the two tables' button-count columns read identically.
static const int kCountWidth = 60;

GpioExpanderConfig::GpioExpanderConfig(QWidget *parent)
    : QWidget(parent)
{
    auto *grid = new QGridLayout(this);
    // Same 7-column equal-stretch grid + left margin as the Shift Registers
    // table above, so the index column and the data columns line up between the
    // two tables (the expander has one fewer field, leaving col 6 empty).
    grid->setContentsMargins(9, 0, 9, 2);
    for (int c = 0; c < 7; ++c) grid->setColumnStretch(c, 1);

    int r = 0;
    grid->addWidget(new QLabel(tr("Type"),         this), r, 1, Qt::AlignCenter);
    grid->addWidget(new QLabel(tr("CS pin"),       this), r, 2, Qt::AlignCenter);
    grid->addWidget(new QLabel(tr("Address"),      this), r, 3, Qt::AlignCenter);
    grid->addWidget(new QLabel(tr("Wiring"),       this), r, 4, Qt::AlignCenter);
    // Button-count header sits in the last column (col 6) so it lines up with the
    // shift registers' "Button count" column; col 5 (their "Registers count") has
    // no expander equivalent and stays empty. Same name as the SR table.
    grid->addWidget(new QLabel(tr("Button count"), this), r, 6, Qt::AlignCenter);
    ++r;

    for (int i = 0; i < MAX_GPIO_EXPANDER_NUM; ++i, ++r) {
        Row row;
        grid->addWidget(new QLabel(QString::number(i + 1), this), r, 0, Qt::AlignCenter);

        row.type = new QComboBox(this);
        row.type->addItem(tr("Disabled"));
        row.type->addItem(tr("MCP23017 (I2C)"));
        row.type->addItem(tr("MCP23S17 (SPI)"));
        grid->addWidget(row.type, r, 1);

        // Read-only: the SPI chip's matched chip-select pin (assigned in Pin
        // Config), like the shift registers show their latch/clk/data pins.
        row.csPin = new QLabel(QStringLiteral("-"), this);
        row.csPin->setAlignment(Qt::AlignCenter);
        grid->addWidget(row.csPin, r, 2);

        row.address = new QComboBox(this);
        for (int a = kAddrLo; a <= kAddrHi; ++a)
            row.address->addItem(QString("0x%1").arg(a, 2, 16, QChar('0')));
        grid->addWidget(row.address, r, 3);

        row.wiring = new QComboBox(this);
        row.wiring->addItem(tr("Buttons to GND"));   // internal pull-up, default polarity
        row.wiring->addItem(tr("Buttons to VCC"));   // external pull-down, inverted polarity
        grid->addWidget(row.wiring, r, 4);

        row.count = new QSpinBox(this);
        row.count->setRange(0, 16);
        row.count->setAlignment(Qt::AlignCenter);
        row.count->setFixedWidth(kCountWidth);   // match the SR count-box width
        grid->addWidget(row.count, r, 6, Qt::AlignCenter);

        connect(row.type, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &GpioExpanderConfig::onRowChanged);
        connect(row.address, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &GpioExpanderConfig::onRowChanged);
        connect(row.wiring, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &GpioExpanderConfig::onRowChanged);
        connect(row.count, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &GpioExpanderConfig::onRowChanged);

        m_rows.append(row);
    }

    applyRowEnableStates();   // grey out the disabled rows from the start

    // Shared alert-banner look (triangle-alert icon + amber box), matching the
    // axes pending-pin banner and the dialog warning bars. Text is updated by
    // validate(); the message label is the word-wrapped one inside the banner.
    m_warnBanner = freejoy_style::makeAlertBanner(freejoy_style::accentAmber(), QString(), this);
    for (QLabel *l : m_warnBanner->findChildren<QLabel *>())
        if (l->wordWrap()) { m_warnText = l; break; }
    m_warnBanner->setVisible(false);
    grid->addWidget(m_warnBanner, r, 0, 1, 7);
}

void GpioExpanderConfig::onPinContextChanged(const QStringList &csPinNames, bool i2cBusOn, bool spiBusOn)
{
    m_csPinNames = csPinNames;
    m_i2cBusOn   = i2cBusOn;
    m_spiBusOn   = spiBusOn;
    updatePinDisplays();
    validate();   // bus state changed -> the "enable the bus" warning may clear
}

void GpioExpanderConfig::updatePinDisplays()
{
    // SPI expanders take the SPI_GPIO_CS pins in slot order (matching the
    // firmware). Walk the rows; each SPI row consumes the next CS pin.
    int next = 0;
    for (const Row &row : m_rows) {
        if (row.type->currentIndex() == T_SPI) {
            row.csPin->setText(next < m_csPinNames.size() ? m_csPinNames.at(next)
                                                          : tr("(none)"));
            ++next;
        } else {
            row.csPin->setText(QStringLiteral("-"));
        }
    }
}

int GpioExpanderConfig::addressOfRow(int i) const
{
    // Only meaningful for an I2C row; 0x20 + the address-combo index.
    return kAddrLo + m_rows[i].address->currentIndex();
}

void GpioExpanderConfig::readFromConfig()
{
    for (int i = 0; i < m_rows.size() && i < MAX_GPIO_EXPANDER_NUM; ++i) {
        const gpio_expander_t &c = gEnv.pDeviceConfig->config.gpio_expanders[i];
        const Row &row = m_rows[i];

        QSignalBlocker b1(row.type), b2(row.address), b3(row.count), b4(row.wiring);

        int type = T_DISABLED;
        if (c.type == GPIO_EXP_MCP23S17) {
            type = T_SPI;
        } else if (c.type == GPIO_EXP_MCP23017 && c.address >= kAddrLo && c.address <= kAddrHi) {
            type = T_I2C;
            row.address->setCurrentIndex(c.address - kAddrLo);
        }
        row.type->setCurrentIndex(type);
        row.count->setValue(c.button_cnt > 16 ? 16 : c.button_cnt);
        // Invert flag set -> active-high (VCC) wiring; otherwise GND (pull-up).
        row.wiring->setCurrentIndex((c.flags & FLAG_INVERT) ? W_VCC : W_GND);
    }
    applyRowEnableStates();
    updatePinDisplays();
    validate();
    emitCounts();
}

void GpioExpanderConfig::writeToConfig()
{
    for (int i = 0; i < m_rows.size() && i < MAX_GPIO_EXPANDER_NUM; ++i) {
        gpio_expander_t &c = gEnv.pDeviceConfig->config.gpio_expanders[i];
        const int type = m_rows[i].type->currentIndex();

        if (type == T_SPI) {
            c.type    = GPIO_EXP_MCP23S17;
            c.address = 0;                                  // hardware subaddr (CS per chip)
        } else if (type == T_I2C) {
            c.type    = GPIO_EXP_MCP23017;
            c.address = static_cast<uint8_t>(addressOfRow(i));
        } else {
            c.type    = GPIO_EXP_MCP23017;
            c.address = 0;                                  // disabled
        }
        c.button_cnt = (type == T_DISABLED) ? 0 : static_cast<uint8_t>(m_rows[i].count->value());
        // Wiring -> register flags: GND = internal pull-up + default polarity;
        // VCC = external pull-down + inverted polarity (IPOL).
        c.flags      = (m_rows[i].wiring->currentIndex() == W_VCC) ? FLAG_INVERT : FLAG_PULLUPS;
    }
}

void GpioExpanderConfig::applyRowEnableStates()
{
    // Mirror the Shift Registers table: a row whose type is Disabled greys out
    // its editable cells (the type combo stays live so it can be re-enabled).
    // Address is additionally only meaningful for an I2C chip.
    for (const Row &row : m_rows) {
        const int t = row.type->currentIndex();
        const bool active = t != T_DISABLED;
        row.address->setEnabled(t == T_I2C);
        row.wiring->setEnabled(active);
        row.count->setEnabled(active);
        row.csPin->setEnabled(active);
    }
}

void GpioExpanderConfig::onRowChanged()
{
    applyRowEnableStates();
    updatePinDisplays();
    validate();
    emitCounts();
}

void GpioExpanderConfig::validate()
{
    QStringList warnings;

    // Duplicate I2C addresses among enabled I2C rows.
    QList<int> seen, dup;
    int i2cCount = 0, spiCount = 0;
    for (int i = 0; i < m_rows.size(); ++i) {
        const int t = m_rows[i].type->currentIndex();
        if (t == T_I2C) {
            ++i2cCount;
            const int a = addressOfRow(i);
            if (seen.contains(a)) { if (!dup.contains(a)) dup.append(a); }
            else seen.append(a);
        } else if (t == T_SPI) {
            ++spiCount;
        }
    }
    for (int i = 0; i < m_rows.size(); ++i) {
        const bool clash = m_rows[i].type->currentIndex() == T_I2C && dup.contains(addressOfRow(i));
        m_rows[i].address->setStyleSheet(clash ? "border: 1px solid #d9534f;" : QString());
    }
    if (!dup.isEmpty())
        warnings << tr("Two I2C expanders share an address — give each a unique 0x20–0x27.");

    // Bus / CS prerequisites, checked against the LIVE pin roles pushed by
    // PinConfig (config.pins[] isn't written until writeToConfig, so reading it
    // here would be stale -- the warning wouldn't clear when you enable the bus).
    if (i2cCount > 0 && !m_i2cBusOn)
        warnings << tr("Enable the I2C bus in Pin Config (SCL/SDA) for the I2C expander(s).");
    if (spiCount > 0) {
        if (!m_spiBusOn)
            warnings << tr("Enable the SPI bus in Pin Config (SCK/MISO/MOSI) for the SPI expander(s).");
        const int cs = m_csPinNames.size();
        if (cs < spiCount)
            warnings << tr("Assign %1 'MCP23S17 expander CS' pin(s) in Pin Config — %2 set, %3 SPI expander(s).")
                        .arg(spiCount).arg(cs).arg(spiCount);
    }

    if (warnings.isEmpty()) {
        m_warnBanner->setVisible(false);
    } else {
        if (m_warnText) m_warnText->setText(warnings.join('\n'));
        m_warnBanner->setVisible(true);
    }
}

void GpioExpanderConfig::emitCounts()
{
    QList<int> perChip;
    int total = 0;
    for (int i = 0; i < m_rows.size(); ++i) {
        const bool active = m_rows[i].type->currentIndex() != T_DISABLED;
        const int n = active ? m_rows[i].count->value() : 0;
        perChip.append(n);
        total += n;
    }
    emit gpioExpBreakdownChanged(perChip);
    emit gpioExpButtonsCountChanged(total);
}

void GpioExpanderConfig::retranslateUi()
{
    validate();   // refreshes the (translatable) warning text
}
