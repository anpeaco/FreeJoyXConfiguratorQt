// Round-trip test for dev_config_t fields that were silently DROPPED on a .cfg
// save/load before this change: the GPIO expander block, the A2B debounce timer,
// shift-config slots 6-8 (the loop stopped at 5), and rgb_leds[].is_disabled.
//
// Drives ConfigToFile save/load directly on a dev_config_t. gEnv.pDeviceConfig
// stays null so crossBoardCheck() early-returns (deviceBoard == 0) and this
// isolates the QSettings persistence of the once-dropped fields.
#include <QtTest>
#include <QTemporaryDir>
#include <cstring>
#include "configtofile.h"
#include "common_defines.h"   // FIRMWARE_VERSION, MAX_* counts
#include "global.h"

GlobalEnvironment gEnv;   // satisfy the extern in global.h

class TestConfigPersist : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;

    QString path(const char *name) { return m_dir.filePath(QString::fromLatin1(name)); }

    // A zeroed config stamped with the current firmware version, so the load-side
    // oldConfigHandler() migration is a no-op and can't perturb the blocks under test.
    static void zero(dev_config_t &c)
    {
        std::memset(&c, 0, sizeof(c));
        c.firmware_version = FIRMWARE_VERSION;
    }

private slots:
    // Every GPIO expander slot's type/address/button_cnt/flags survives save/load.
    void gpioExpanders_roundTrip()
    {
        dev_config_t a; zero(a);
        for (int i = 0; i < MAX_GPIO_EXPANDER_NUM; ++i) {
            a.gpio_expanders[i].type       = uint8_t(1 + i);
            a.gpio_expanders[i].address    = uint8_t(i & 0x07);
            a.gpio_expanders[i].button_cnt = uint8_t(16);
            a.gpio_expanders[i].flags      = uint8_t(0x01 | ((i & 0x07) << 2)); // pull-ups + CS index
        }

        const QString f = path("gpioexp_roundtrip.ini");
        ConfigToFile::saveDeviceConfigToFile(f, a);

        dev_config_t b; zero(b);
        ConfigToFile::loadDeviceConfigFromFile(nullptr, f, b);

        for (int i = 0; i < MAX_GPIO_EXPANDER_NUM; ++i) {
            QCOMPARE(b.gpio_expanders[i].type,       a.gpio_expanders[i].type);
            QCOMPARE(b.gpio_expanders[i].address,    a.gpio_expanders[i].address);
            QCOMPARE(b.gpio_expanders[i].button_cnt, a.gpio_expanders[i].button_cnt);
            QCOMPARE(b.gpio_expanders[i].flags,      a.gpio_expanders[i].flags);
        }
    }

    // The axis-to-buttons debounce timer round-trips (was never written before).
    void a2bDebounce_roundTrip()
    {
        dev_config_t a; zero(a);
        a.a2b_debounce_ms = 137;

        const QString f = path("a2b_roundtrip.ini");
        ConfigToFile::saveDeviceConfigToFile(f, a);

        dev_config_t b; zero(b);
        b.a2b_debounce_ms = 999;   // poison so the assert proves the load wrote it
        ConfigToFile::loadDeviceConfigFromFile(nullptr, f, b);

        QCOMPARE(b.a2b_debounce_ms, uint16_t(137));
    }

    // All MAX_SHIFTS_NUM shift-config slots round-trip -- slots 6..8 were dropped
    // when the loop bound was SHIFT_COUNT-1 (=5) instead of MAX_SHIFTS_NUM.
    void shiftConfig_allSlots_roundTrip()
    {
        dev_config_t a; zero(a);
        for (int i = 0; i < MAX_SHIFTS_NUM; ++i)
            a.shift_config[i].button = int8_t(10 + i);   // distinct per slot

        const QString f = path("shifts_roundtrip.ini");
        ConfigToFile::saveDeviceConfigToFile(f, a);

        dev_config_t b; zero(b);
        for (int i = 0; i < MAX_SHIFTS_NUM; ++i)
            b.shift_config[i].button = int8_t(-1);       // poison every slot
        ConfigToFile::loadDeviceConfigFromFile(nullptr, f, b);

        for (int i = 0; i < MAX_SHIFTS_NUM; ++i)
            QCOMPARE(b.shift_config[i].button, int8_t(10 + i));
    }

    // rgb_leds[].is_disabled round-trips (mute state was dropped before this change).
    void rgbIsDisabled_roundTrip()
    {
        dev_config_t a; zero(a);
        for (int i = 0; i < NUM_RGB_LEDS; ++i)
            a.rgb_leds[i].is_disabled = (i % 2);         // alternate on/off

        const QString f = path("rgb_roundtrip.ini");
        ConfigToFile::saveDeviceConfigToFile(f, a);

        dev_config_t b; zero(b);
        for (int i = 0; i < NUM_RGB_LEDS; ++i)
            b.rgb_leds[i].is_disabled = 1;               // poison
        ConfigToFile::loadDeviceConfigFromFile(nullptr, f, b);

        for (int i = 0; i < NUM_RGB_LEDS; ++i)
            QCOMPARE(uint8_t(b.rgb_leds[i].is_disabled), uint8_t(i % 2));
    }
};

QTEST_MAIN(TestConfigPersist)
#include "test_configpersist.moc"
