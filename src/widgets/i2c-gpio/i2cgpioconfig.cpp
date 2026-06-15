#include "i2cgpioconfig.h"

#include "deviceconfig.h"
#include "global.h"
#include "common_defines.h"   // MAX_GPIO_EXPANDER_NUM, USED_PINS_NUM
#include "common_types.h"     // GPIO_EXP_*, pin roles (I2C_SCL, SPI_SCK, SPI_GPIO_CS, ...)

#include <QGridLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>

/* flags bits -- mirror firmware gpio_expander.h */
static const uint8_t FLAG_PULLUPS = 0x01;
static const uint8_t FLAG_INVERT  = 0x02;

static const int kAddrLo = 0x20;   // MCP2301x strap range 0x20..0x27
static const int kAddrHi = 0x27;

I2cGpioConfig::I2cGpioConfig(QWidget *parent)
    : QWidget(parent)
{
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(0, 0, 0, 0);

    int r = 0;
    grid->addWidget(new QLabel(tr("Type"),     this), r, 1);
    grid->addWidget(new QLabel(tr("Address"),  this), r, 2);
    grid->addWidget(new QLabel(tr("Buttons"),  this), r, 3);
    grid->addWidget(new QLabel(tr("Pull-ups"), this), r, 4);
    grid->addWidget(new QLabel(tr("Invert"),   this), r, 5);
    ++r;

    for (int i = 0; i < MAX_GPIO_EXPANDER_NUM; ++i, ++r) {
        Row row;
        grid->addWidget(new QLabel(QString::number(i + 1), this), r, 0);

        row.type = new QComboBox(this);
        row.type->addItem(tr("Disabled"));
        row.type->addItem(tr("MCP23017 (I2C)"));
        row.type->addItem(tr("MCP23S17 (SPI)"));
        grid->addWidget(row.type, r, 1);

        row.address = new QComboBox(this);
        for (int a = kAddrLo; a <= kAddrHi; ++a)
            row.address->addItem(QString("0x%1").arg(a, 2, 16, QChar('0')));
        grid->addWidget(row.address, r, 2);

        row.count = new QSpinBox(this);
        row.count->setRange(0, 16);
        grid->addWidget(row.count, r, 3);

        row.pullups = new QCheckBox(this);
        row.pullups->setChecked(true);
        grid->addWidget(row.pullups, r, 4, Qt::AlignCenter);

        row.invert = new QCheckBox(this);
        grid->addWidget(row.invert, r, 5, Qt::AlignCenter);

        connect(row.type, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &I2cGpioConfig::onRowChanged);
        connect(row.address, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &I2cGpioConfig::onRowChanged);
        connect(row.count, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &I2cGpioConfig::onRowChanged);
        connect(row.pullups, &QCheckBox::toggled, this, &I2cGpioConfig::onRowChanged);
        connect(row.invert,  &QCheckBox::toggled, this, &I2cGpioConfig::onRowChanged);

        m_rows.append(row);
    }

    m_warning = new QLabel(this);
    m_warning->setStyleSheet("color: #d9534f;");   // alert red
    m_warning->setWordWrap(true);
    m_warning->setVisible(false);
    grid->addWidget(m_warning, r, 0, 1, 6);
}

int I2cGpioConfig::addressOfRow(int i) const
{
    // Only meaningful for an I2C row; 0x20 + the address-combo index.
    return kAddrLo + m_rows[i].address->currentIndex();
}

void I2cGpioConfig::readFromConfig()
{
    for (int i = 0; i < m_rows.size() && i < MAX_GPIO_EXPANDER_NUM; ++i) {
        const gpio_expander_t &c = gEnv.pDeviceConfig->config.gpio_expanders[i];
        const Row &row = m_rows[i];

        QSignalBlocker b1(row.type), b2(row.address), b3(row.count),
                       b4(row.pullups), b5(row.invert);

        int type = T_DISABLED;
        if (c.type == GPIO_EXP_MCP23S17) {
            type = T_SPI;
        } else if (c.type == GPIO_EXP_MCP23017 && c.address >= kAddrLo && c.address <= kAddrHi) {
            type = T_I2C;
            row.address->setCurrentIndex(c.address - kAddrLo);
        }
        row.type->setCurrentIndex(type);
        row.count->setValue(c.button_cnt > 16 ? 16 : c.button_cnt);
        row.pullups->setChecked((c.flags & FLAG_PULLUPS) != 0);
        row.invert->setChecked((c.flags & FLAG_INVERT) != 0);
        row.address->setEnabled(type == T_I2C);
    }
    validate();
    emitCounts();
}

void I2cGpioConfig::writeToConfig()
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
        c.flags      = static_cast<uint8_t>(
                           (m_rows[i].pullups->isChecked() ? FLAG_PULLUPS : 0) |
                           (m_rows[i].invert->isChecked()  ? FLAG_INVERT  : 0));
    }
}

void I2cGpioConfig::onRowChanged()
{
    for (const Row &row : m_rows)
        row.address->setEnabled(row.type->currentIndex() == T_I2C);
    validate();
    emitCounts();
}

static int countPinRole(int role)
{
    int n = 0;
    for (int p = 0; p < USED_PINS_NUM; ++p)
        if (gEnv.pDeviceConfig->config.pins[p] == role) ++n;
    return n;
}

void I2cGpioConfig::validate()
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

    // Bus / CS prerequisites (the firmware brings up a bus from its pin roles).
    if (i2cCount > 0 && countPinRole(I2C_SCL) == 0)
        warnings << tr("Enable the I2C bus in Pin Config (SCL/SDA) for the I2C expander(s).");
    if (spiCount > 0) {
        if (countPinRole(SPI_SCK) == 0 && countPinRole(SPI_MOSI) == 0)
            warnings << tr("Enable the SPI bus in Pin Config (SCK/MISO/MOSI) for the SPI expander(s).");
        const int cs = countPinRole(SPI_GPIO_CS);
        if (cs < spiCount)
            warnings << tr("Assign %1 'MCP23S17 expander CS' pin(s) in Pin Config — %2 set, %3 SPI expander(s).")
                        .arg(spiCount).arg(cs).arg(spiCount);
    }

    if (warnings.isEmpty()) {
        m_warning->setVisible(false);
    } else {
        m_warning->setText(warnings.join('\n'));
        m_warning->setVisible(true);
    }
}

void I2cGpioConfig::emitCounts()
{
    QList<int> perChip;
    int total = 0;
    for (int i = 0; i < m_rows.size(); ++i) {
        const bool active = m_rows[i].type->currentIndex() != T_DISABLED;
        const int n = active ? m_rows[i].count->value() : 0;
        perChip.append(n);
        total += n;
    }
    emit i2cGpioBreakdownChanged(perChip);
    emit i2cGpioButtonsCountChanged(total);
}

void I2cGpioConfig::retranslateUi()
{
    validate();   // refreshes the (translatable) warning text
}
