#include "i2cgpioconfig.h"

#include "deviceconfig.h"
#include "global.h"
#include "common_defines.h"   // MAX_GPIO_EXPANDER_NUM
#include "common_types.h"     // GPIO_EXP_MCP23017, I2C_GPIO_FLAG_* live in mcp23017.h on the
                              // firmware side; the bit meanings are mirrored here.

#include <QGridLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>

/* flags bits -- mirror firmware mcp23017.h */
static const uint8_t FLAG_PULLUPS = 0x01;
static const uint8_t FLAG_INVERT  = 0x02;

static const int kAddrLo = 0x20;   // MCP23017 strap range 0x20..0x27
static const int kAddrHi = 0x27;

I2cGpioConfig::I2cGpioConfig(QWidget *parent)
    : QWidget(parent)
{
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(0, 0, 0, 0);

    int r = 0;
    grid->addWidget(new QLabel(tr("Address"),  this), r, 1);
    grid->addWidget(new QLabel(tr("Buttons"),  this), r, 2);
    grid->addWidget(new QLabel(tr("Pull-ups"), this), r, 3);
    grid->addWidget(new QLabel(tr("Invert"),   this), r, 4);
    ++r;

    for (int i = 0; i < MAX_GPIO_EXPANDER_NUM; ++i, ++r) {
        Row row;
        grid->addWidget(new QLabel(QString::number(i + 1), this), r, 0);

        row.address = new QComboBox(this);
        row.address->addItem(tr("Disabled"));
        for (int a = kAddrLo; a <= kAddrHi; ++a)
            row.address->addItem(QString("0x%1").arg(a, 2, 16, QChar('0')));
        grid->addWidget(row.address, r, 1);

        row.count = new QSpinBox(this);
        row.count->setRange(0, 16);
        grid->addWidget(row.count, r, 2);

        row.pullups = new QCheckBox(this);
        row.pullups->setChecked(true);
        grid->addWidget(row.pullups, r, 3, Qt::AlignCenter);

        row.invert = new QCheckBox(this);
        grid->addWidget(row.invert, r, 4, Qt::AlignCenter);

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
    m_warning->setVisible(false);
    grid->addWidget(m_warning, r, 0, 1, 5);
}

int I2cGpioConfig::addressOfRow(int i) const
{
    const int idx = m_rows[i].address->currentIndex();
    return (idx <= 0) ? 0 : (kAddrLo + idx - 1);
}

void I2cGpioConfig::readFromConfig()
{
    for (int i = 0; i < m_rows.size() && i < MAX_GPIO_EXPANDER_NUM; ++i) {
        const gpio_expander_t &c = gEnv.pDeviceConfig->config.gpio_expanders[i];
        const Row &row = m_rows[i];

        QSignalBlocker b1(row.address), b2(row.count), b3(row.pullups), b4(row.invert);

        int idx = 0;
        if (c.address >= kAddrLo && c.address <= kAddrHi) idx = c.address - kAddrLo + 1;
        row.address->setCurrentIndex(idx);
        row.count->setValue(c.button_cnt > 16 ? 16 : c.button_cnt);
        row.pullups->setChecked((c.flags & FLAG_PULLUPS) != 0);
        row.invert->setChecked((c.flags & FLAG_INVERT) != 0);
    }
    validate();
    emitCounts();
}

void I2cGpioConfig::writeToConfig()
{
    for (int i = 0; i < m_rows.size() && i < MAX_GPIO_EXPANDER_NUM; ++i) {
        gpio_expander_t &c = gEnv.pDeviceConfig->config.gpio_expanders[i];
        const int addr = addressOfRow(i);

        c.type       = GPIO_EXP_MCP23017;
        c.address    = static_cast<uint8_t>(addr);
        c.button_cnt = (addr == 0) ? 0 : static_cast<uint8_t>(m_rows[i].count->value());
        c.flags      = static_cast<uint8_t>(
                           (m_rows[i].pullups->isChecked() ? FLAG_PULLUPS : 0) |
                           (m_rows[i].invert->isChecked()  ? FLAG_INVERT  : 0));
    }
}

void I2cGpioConfig::onRowChanged()
{
    validate();
    emitCounts();
}

void I2cGpioConfig::validate()
{
    // Flag any I2C address used by more than one enabled expander.
    QList<int> seen;
    QList<int> dup;
    for (int i = 0; i < m_rows.size(); ++i) {
        const int a = addressOfRow(i);
        if (a == 0) continue;
        if (seen.contains(a)) { if (!dup.contains(a)) dup.append(a); }
        else seen.append(a);
    }

    for (int i = 0; i < m_rows.size(); ++i) {
        const int a = addressOfRow(i);
        const bool clash = (a != 0) && dup.contains(a);
        m_rows[i].address->setStyleSheet(clash ? "border: 1px solid #d9534f;" : QString());
    }

    if (!dup.isEmpty()) {
        m_warning->setText(tr("Two expanders share the same I2C address — give each a unique 0x20–0x27."));
        m_warning->setVisible(true);
    } else {
        m_warning->setVisible(false);
    }
}

void I2cGpioConfig::emitCounts()
{
    QList<int> perChip;
    int total = 0;
    for (int i = 0; i < m_rows.size(); ++i) {
        const int n = (addressOfRow(i) == 0) ? 0 : m_rows[i].count->value();
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
