// Headless verification of the MCP23S17 shared-CS load paths (PR self-review fixes).
// Drives GpioExpanderConfig::readFromConfig/writeToConfig over a crafted
// dev_config_t and asserts the CS index packed back into flags bits 4:2:
//   - a legacy config (2+ SPI chips all at CS0 / strap0) is migrated to the
//     positional CS order (0,1,...) instead of collapsing onto CS0;
//   - a genuine shared-CS config (chips on one CS with distinct straps) is NOT
//     migrated;
//   - an already-distinct-CS config is left unchanged.
#include <QtTest>
#include <QFrame>
#include <QLabel>
#include "global.h"
#include "deviceconfig.h"
#include "common_defines.h"
#include "common_types.h"
#include "widgets/gpio-expander/gpioexpanderconfig.h"

GlobalEnvironment gEnv;

// makeAlertBanner (used by GpioExpanderConfig's ctor) calls this; the real
// definition lives in mainwindow_style.cpp, which drags in the whole app. The
// banner's line alignment is irrelevant to this test, so stub it.
namespace freejoy_style {
void applyBannerLineAlignment(QFrame *, QLabel *, QLabel *, const QList<QWidget *> &) {}
}

// Mirror the widget's flag layout (gpio_expander.h): bit0 pull-ups, bits 4:2 CS.
static const uint8_t FLAG_PULLUPS = 0x01;
static const uint8_t CS_SHIFT = 2;
static const uint8_t CS_MASK  = 0x1C;

static int csOf(const gpio_expander_t &c) { return (c.flags & CS_MASK) >> CS_SHIFT; }

class TestExpanderCs : public QObject
{
    Q_OBJECT

    DeviceConfig *m_dc = nullptr;

    void mkSpi(int i, uint8_t strap, uint8_t csIndex)
    {
        gpio_expander_t &c = m_dc->config.gpio_expanders[i];
        c.type       = GPIO_EXP_MCP23S17;
        c.address    = strap;                                   // A2:A0 DIP strap 0..7
        c.button_cnt = 16;
        c.flags      = FLAG_PULLUPS | ((csIndex << CS_SHIFT) & CS_MASK);
    }

private slots:
    void init()
    {
        m_dc = new DeviceConfig();
        m_dc->resetConfig();
        gEnv.pDeviceConfig = m_dc;
    }
    void cleanup()
    {
        delete m_dc; m_dc = nullptr;
        gEnv.pDeviceConfig = nullptr;
    }

    // Legacy: two SPI chips both stored at CS0/strap0 (pre-shared-CS builds
    // wrote no CS index) -> positional migration restores CS 0,1.
    void legacyPositionalMigration()
    {
        mkSpi(0, 0, 0);
        mkSpi(1, 0, 0);
        GpioExpanderConfig w;
        w.readFromConfig();
        w.writeToConfig();
        QCOMPARE(csOf(m_dc->config.gpio_expanders[0]), 0);
        QCOMPARE(csOf(m_dc->config.gpio_expanders[1]), 1);
    }

    // Three legacy chips -> CS 0,1,2.
    void legacyThreeChips()
    {
        mkSpi(0, 0, 0);
        mkSpi(1, 0, 0);
        mkSpi(2, 0, 0);
        GpioExpanderConfig w;
        w.readFromConfig();
        w.writeToConfig();
        QCOMPARE(csOf(m_dc->config.gpio_expanders[0]), 0);
        QCOMPARE(csOf(m_dc->config.gpio_expanders[1]), 1);
        QCOMPARE(csOf(m_dc->config.gpio_expanders[2]), 2);
    }

    // Genuine shared-CS: two chips on CS0 with DISTINCT straps -> NOT migrated
    // (they're legitimately sharing one CS).
    void sharedCsDistinctStrapsNotMigrated()
    {
        mkSpi(0, 0, 0);   // strap 0, cs 0
        mkSpi(1, 1, 0);   // strap 1, cs 0
        GpioExpanderConfig w;
        w.readFromConfig();
        w.writeToConfig();
        QCOMPARE(csOf(m_dc->config.gpio_expanders[0]), 0);
        QCOMPARE(csOf(m_dc->config.gpio_expanders[1]), 0);   // stays shared
    }

    // Already distinct CS indices -> left unchanged.
    void distinctCsNotMigrated()
    {
        mkSpi(0, 0, 0);
        mkSpi(1, 0, 1);
        GpioExpanderConfig w;
        w.readFromConfig();
        w.writeToConfig();
        QCOMPARE(csOf(m_dc->config.gpio_expanders[0]), 0);
        QCOMPARE(csOf(m_dc->config.gpio_expanders[1]), 1);
    }

    // A single SPI chip at CS0 is not "ambiguous" (needs >= 2) -> unchanged.
    void singleChipNotMigrated()
    {
        mkSpi(0, 0, 0);
        GpioExpanderConfig w;
        w.readFromConfig();
        w.writeToConfig();
        QCOMPARE(csOf(m_dc->config.gpio_expanders[0]), 0);
    }

    // A stored CS index that no longer fits the assigned CS pins must be
    // PRESERVED (not silently clamped to 0), so it round-trips and validate()
    // can red-flag it instead of the chip snapping to CS #0.
    void outOfRangeCsPreservedNotClamped()
    {
        mkSpi(0, 0, 2);                                   // chip wired to CS index 2
        GpioExpanderConfig w;
        w.readFromConfig();
        w.onPinContextChanged(QStringList{"B6", "B7"}, false, true);  // only 2 CS pins
        w.writeToConfig();
        QCOMPARE(csOf(m_dc->config.gpio_expanders[0]), 2);   // preserved, not clamped to 0
    }
};

QTEST_MAIN(TestExpanderCs)
#include "test_expandercs.moc"
