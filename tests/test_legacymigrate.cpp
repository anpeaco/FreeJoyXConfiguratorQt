/* Unit tests for the 0x0020 -> 0x0030 wire-format bump that appended
 * i2c_gpio[MAX_GPIO_EXPANDER_NUM] (MCP23017 button expanders) to the END of
 * dev_config_t, plus the legacy migrator that reads pre-0x0030 configs via the
 * shared prefix path. See MCP23017_PLAN.md. Links legacy_migrator.cpp +
 * stm_main.c (InitConfig); no GUI / FirmwareImage. */

#include <QtTest>
#include <cstring>
#include <cstddef>
#include <vector>

#include "common_types.h"
#include "common_defines.h"
#include "legacy_migrator.h"

extern "C" {
#include "stm_main.h"   /* InitConfig() -- factory defaults */
}

class TestLegacyMigrate : public QObject
{
    Q_OBJECT

private slots:
    /* ---- wire-format size pins ---- */
    void devConfigSize_matchesConstant()
    {
        QCOMPARE(sizeof(dev_config_t), static_cast<size_t>(FREEJOY_DEV_CONFIG_SIZE));
        QCOMPARE(static_cast<int>(FREEJOY_DEV_CONFIG_SIZE), 1620);
    }
    void i2cGpio_isAppendedAtEnd()
    {
        /* The old (0x0020) shape is exactly the prefix up to gpio_expanders: a
         * pure append adds the member size with no padding (alignment-1 member
         * onto an even-sized, alignment-2 struct). saved_per_exp[8] was appended
         * AFTER gpio_expanders (1612 -> 1620) so offsetof(gpio_expanders) -- the
         * 0x0020 migration boundary -- is unchanged. */
        QCOMPARE(offsetof(dev_config_t, gpio_expanders), static_cast<size_t>(1580));
        QCOMPARE(sizeof(gpio_expander_t), static_cast<size_t>(4));
        QCOMPARE(static_cast<int>(MAX_GPIO_EXPANDER_NUM), 8);
        /* saved_per_exp sits after the 8 x 4B expander slots. */
        QCOMPARE(offsetof(dev_config_t, saved_per_exp), static_cast<size_t>(1580 + 32));
    }
    void firmwareVersion_isGen3()
    {
        QCOMPARE(static_cast<int>(FIRMWARE_VERSION), 0x0030);
    }

    /* ---- legacy API ---- */
    void canMigrate_acceptsGen2()
    {
        QVERIFY(legacy::canMigrate(0x0020));
        QVERIFY(legacy::canMigrate(0x0021));   /* build-letter within the mask */
    }
    void legacyConfigSize_gen2_isOldPrefixSize()
    {
        QCOMPARE(legacy::legacyConfigSize(0x0020),
                 offsetof(dev_config_t, gpio_expanders));
        QCOMPARE(legacy::legacyConfigSize(0x0020), static_cast<size_t>(1580));
        /* the current generation reports the full struct */
        QCOMPARE(legacy::legacyConfigSize(FIRMWARE_VERSION), sizeof(dev_config_t));
    }

    /* ---- 0x0020 -> 0x0030 round-trip ---- */
    void migrateGen2_preservesPrefix_zeroesExpanders_stampsVersion()
    {
        const size_t oldSize = offsetof(dev_config_t, gpio_expanders);

        /* Build a 0x0020 raw config: current defaults, stamped gen 2, with a
         * recognisable preserved field, truncated to the old prefix size (no
         * i2c_gpio bytes ever travelled on the 0x0020 wire). */
        dev_config_t seed = InitConfig();
        seed.firmware_version   = 0x0020;
        seed.button_debounce_ms = 0xABCD;
        std::strncpy(seed.device_name, "Gen2Cfg", sizeof(seed.device_name) - 1);

        std::vector<uint8_t> raw(oldSize);
        std::memcpy(raw.data(), &seed, oldSize);

        /* Pre-fill the destination with 0xFF to prove the migrator clears the
         * appended expander slots regardless of prior contents. */
        dev_config_t out;
        std::memset(&out, 0xFF, sizeof(out));

        legacy::MigrateResult r =
            legacy::migrateLegacyConfig(raw.data(), raw.size(), out);

        QCOMPARE(r, legacy::MigrateResult::Ok);
        QCOMPARE(out.firmware_version, static_cast<uint16_t>(FIRMWARE_VERSION));
        QCOMPARE(out.button_debounce_ms, static_cast<uint16_t>(0xABCD));
        QCOMPARE(QByteArray(out.device_name), QByteArray("Gen2Cfg"));

        for (int i = 0; i < MAX_GPIO_EXPANDER_NUM; ++i) {
            QCOMPARE(out.gpio_expanders[i].type,       static_cast<uint8_t>(0));
            QCOMPARE(out.gpio_expanders[i].address,    static_cast<uint8_t>(0));
            QCOMPARE(out.gpio_expanders[i].button_cnt, static_cast<uint8_t>(0));
            QCOMPARE(out.gpio_expanders[i].flags,      static_cast<uint8_t>(0));
            /* The appended remap snapshot must also clear (it was 0xFF). */
            QCOMPARE(out.saved_per_exp[i],             static_cast<uint8_t>(0));
        }
    }

    void migrateGen2_rejectsTruncatedBuffer()
    {
        std::vector<uint8_t> tooSmall(8, 0);
        tooSmall[0] = 0x20;   /* version = 0x0020 (little-endian low byte) */
        dev_config_t out;
        QCOMPARE(legacy::migrateLegacyConfig(tooSmall.data(), tooSmall.size(), out),
                 legacy::MigrateResult::BufferTooSmall);
    }
};

QTEST_APPLESS_MAIN(TestLegacyMigrate)
#include "test_legacymigrate.moc"
