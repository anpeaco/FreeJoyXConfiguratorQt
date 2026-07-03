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

/* flags bits -- mirror firmware gpio_expander.h. NOT covered by header-sync
 * (only common_*.h are), so keep these in step with gpio_expander.h by hand.
 * Bits 4:2 hold the SPI CS index (which SPI_GPIO_CS pin a chip is wired to). */
static const uint8_t FLAG_PULLUPS  = 0x01;
static const uint8_t FLAG_INVERT   = 0x02;
static const uint8_t FLAG_CS_SHIFT = 2;
static const uint8_t FLAG_CS_MASK  = 0x1C;   // bits 4:2

static const int kAddrLo = 0x20;   // MCP2301x strap range 0x20..0x27
static const int kAddrHi = 0x27;

// Shared count-spinbox width, kept equal to the shift registers' count boxes so
// the two tables' button-count columns read identically (fits a 3-digit value).
static const int kCountWidth = 64;

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

        // SPI: which assigned SPI_GPIO_CS pin this chip is wired to. Two rows
        // picking the same CS share it (HAEN + the address DIP strap tell them
        // apart). Populated from Pin Config context in updatePinDisplays.
        row.csPin = new QComboBox(this);
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
        connect(row.csPin, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &GpioExpanderConfig::onRowChanged);

        m_rows.append(row);
    }

    applyRowEnableStates();   // grey out the disabled rows from the start
    // Address combos start empty -- nothing to address until a row's type is
    // chosen (refreshAddressItems fills the I2C/SPI values on selection).
    for (const Row &row : m_rows) refreshAddressItems(row);

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
    // Rebuild each SPI row's CS dropdown from the assigned SPI_GPIO_CS pins (in
    // pin order). row.csIndex is authoritative -- restore it (clamped) after the
    // rebuild so a late Pin Config update doesn't drop the loaded selection.
    for (Row &row : m_rows) {
        QSignalBlocker b(row.csPin);
        row.csPin->clear();
        if (row.type->currentIndex() == T_SPI) {
            row.csPin->addItems(m_csPinNames);
            if (row.csPin->count() > 0) {
                if (row.csIndex < 0 || row.csIndex >= row.csPin->count()) row.csIndex = 0;
                row.csPin->setCurrentIndex(row.csIndex);
            }
        }
    }
}

void GpioExpanderConfig::refreshAddressItems(const Row &row)
{
    // Fill the address combo for the current type: I2C shows the 0x20..0x27
    // slave address; SPI shows the A2:A0 DIP strap 0..7 (with its 3-bit binary
    // so it reads straight off the physical switches). A Disabled row has no
    // meaningful address, so the combo is emptied until a type is chosen.
    const int t = row.type->currentIndex();
    QSignalBlocker b(row.address);
    if (t != T_I2C && t != T_SPI) {
        row.address->clear();
        return;
    }
    while (row.address->count() < 8) row.address->addItem(QString());
    const bool spi = (t == T_SPI);
    for (int a = 0; a < 8; ++a) {
        row.address->setItemText(a, spi ? QString("%1  (%2)").arg(a).arg(a, 3, 2, QChar('0'))
                                        : QString("0x%1").arg(kAddrLo + a, 2, 16, QChar('0')));
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
        Row &row = m_rows[i];

        QSignalBlocker b1(row.type), b2(row.address), b3(row.count), b4(row.wiring);

        // An MCP23017 with an address outside the 0x20..0x27 strap range can
        // only arrive from a corrupt or externally-authored config (the UI
        // dropdown can't produce it). We deliberately fall through to Disabled
        // rather than guess a valid address -- a factory-reset slot is exactly
        // type=0/address=0, which is the common, correct hit of this branch.
        int type = T_DISABLED;
        int addrIdx = 0;
        if (c.type == GPIO_EXP_MCP23S17) {
            type = T_SPI;
            addrIdx = c.address & 0x07;                                // A2:A0 DIP strap
            row.csIndex = (c.flags & FLAG_CS_MASK) >> FLAG_CS_SHIFT;   // which CS pin
        } else if (c.type == GPIO_EXP_MCP23017 && c.address >= kAddrLo && c.address <= kAddrHi) {
            type = T_I2C;
            addrIdx = c.address - kAddrLo;
        }
        row.type->setCurrentIndex(type);
        refreshAddressItems(row);                    // populate items (or clear) before selecting
        if (type != T_DISABLED) row.address->setCurrentIndex(addrIdx);
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
            c.address = static_cast<uint8_t>(m_rows[i].address->currentIndex() & 0x07);  // A2:A0 DIP strap
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
        // SPI only: pack which CS pin this chip uses into flags bits 4:2.
        if (type == T_SPI)
            c.flags |= static_cast<uint8_t>((m_rows[i].csIndex << FLAG_CS_SHIFT) & FLAG_CS_MASK);
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
        row.address->setEnabled(t == T_I2C || t == T_SPI);
        row.wiring->setEnabled(active);
        row.count->setEnabled(active);
        row.csPin->setEnabled(t == T_SPI);
    }
}

void GpioExpanderConfig::onRowChanged()
{
    // Capture each SPI row's CS pick from its combo (authoritative) and relabel
    // the address combo for the current type, before repopulating the CS lists.
    for (Row &row : m_rows) {
        if (row.type->currentIndex() == T_SPI && row.csPin->currentIndex() >= 0)
            row.csIndex = row.csPin->currentIndex();
        refreshAddressItems(row);
    }
    applyRowEnableStates();
    updatePinDisplays();
    validate();
    emitCounts();
}

void GpioExpanderConfig::validate()
{
    QStringList warnings;
    QList<int> clashRows;   // address combos to red-border (I2C dup, or SPI same-CS same-strap)

    int i2cCount = 0, spiCount = 0;
    for (int i = 0; i < m_rows.size(); ++i) {
        const int t = m_rows[i].type->currentIndex();
        if (t == T_I2C) ++i2cCount; else if (t == T_SPI) ++spiCount;
    }

    // I2C: two chips can't share a 0x20..0x27 slave address.
    bool i2cDup = false;
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].type->currentIndex() != T_I2C) continue;
        for (int j = i + 1; j < m_rows.size(); ++j) {
            if (m_rows[j].type->currentIndex() == T_I2C && addressOfRow(i) == addressOfRow(j)) {
                if (!clashRows.contains(i)) clashRows.append(i);
                if (!clashRows.contains(j)) clashRows.append(j);
                i2cDup = true;
            }
        }
    }
    if (i2cDup)
        warnings << tr("Two I2C expanders share an address — give each a unique 0x20–0x27.");

    // SPI: two chips on the SAME CS pin must have distinct A2:A0 DIP straps.
    bool spiDup = false;
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].type->currentIndex() != T_SPI) continue;
        for (int j = i + 1; j < m_rows.size(); ++j) {
            if (m_rows[j].type->currentIndex() == T_SPI &&
                m_rows[i].csIndex == m_rows[j].csIndex &&
                m_rows[i].address->currentIndex() == m_rows[j].address->currentIndex()) {
                if (!clashRows.contains(i)) clashRows.append(i);
                if (!clashRows.contains(j)) clashRows.append(j);
                spiDup = true;
            }
        }
    }
    if (spiDup)
        warnings << tr("Two SPI expanders share a CS pin and address — give chips on one CS distinct DIP straps.");

    for (int i = 0; i < m_rows.size(); ++i)
        m_rows[i].address->setStyleSheet(clashRows.contains(i) ? "border: 1px solid #d9534f;" : QString());

    // Bus / CS prerequisites, checked against the LIVE pin roles pushed by
    // PinConfig (config.pins[] isn't written until writeToConfig, so reading it
    // here would be stale -- the warning wouldn't clear when you enable the bus).
    if (i2cCount > 0 && !m_i2cBusOn)
        warnings << tr("Enable the I2C bus in Pin Config (SCL/SDA) for the I2C expander(s).");
    if (spiCount > 0) {
        if (!m_spiBusOn)
            warnings << tr("Enable the SPI bus in Pin Config (SCK/MISO/MOSI) for the SPI expander(s).");
        const int cs = m_csPinNames.size();
        if (cs == 0) {
            warnings << tr("Assign at least one 'MCP23S17 expander CS' pin in Pin Config for the SPI expander(s).");
        } else {
            bool badIdx = false;
            for (int i = 0; i < m_rows.size(); ++i)
                if (m_rows[i].type->currentIndex() == T_SPI && m_rows[i].csIndex >= cs) badIdx = true;
            if (badIdx)
                warnings << tr("An SPI expander points at a CS pin that isn't assigned — pick a valid CS pin.");
        }
    }

    // Red-highlight the specific unset/invalid required fields on a typed row
    // (mirrors the address-clash borders above): an SPI row whose CS pin isn't
    // validly assigned, and any enabled row still left at 0 buttons.
    const int csAssigned = m_csPinNames.size();
    bool anyCountMissing = false;
    for (int i = 0; i < m_rows.size(); ++i) {
        const int t = m_rows[i].type->currentIndex();
        const bool csBad = (t == T_SPI) &&
                           (csAssigned == 0 || m_rows[i].csIndex >= csAssigned ||
                            m_rows[i].csPin->currentIndex() < 0);
        m_rows[i].csPin->setStyleSheet(csBad ? QStringLiteral("border: 1px solid #d9534f;")
                                             : QString());

        const bool countMissing = (t != T_DISABLED) && m_rows[i].count->value() <= 0;
        m_rows[i].count->setStyleSheet(countMissing ? QStringLiteral("border: 1px solid #d9534f;")
                                                     : QString());
        if (countMissing) anyCountMissing = true;
    }
    if (anyCountMissing)
        warnings << tr("Set a button count for the highlighted expander(s).");

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
