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
        QCOMPARE(static_cast<int>(FREEJOY_DEV_CONFIG_SIZE), 1654);
    }
    void i2cGpio_isAppendedAtEnd()
    {
        /* The gpio_expanders append boundary (0x0020 -> 0x0030) is unchanged by
         * the later 0x0040 slow_encoders append. */
        QCOMPARE(offsetof(dev_config_t, gpio_expanders), static_cast<size_t>(1580));
        QCOMPARE(sizeof(gpio_expander_t), static_cast<size_t>(4));
        QCOMPARE(static_cast<int>(MAX_GPIO_EXPANDER_NUM), 8);
        /* saved_per_exp sits after the 8 x 4B expander slots. */
        QCOMPARE(offsetof(dev_config_t, saved_per_exp), static_cast<size_t>(1580 + 32));
    }
    void slowEncoders_isAppendedAtEnd()
    {
        /* The 0x0030 -> 0x0040 bump appended slow_encoders[MAX_ENCODERS_NUM]
         * ({int8 btn_a, btn_b}) at the very end, so its offset == the old
         * (0x0030) config size 1620 -- the migration boundary. */
        QCOMPARE(sizeof(slow_encoder_t), static_cast<size_t>(2));
        QCOMPARE(offsetof(dev_config_t, slow_encoders), static_cast<size_t>(1620));
        /* slow_encoders (8x4? no: 16 x 2B = 32) ends at 1652, which is now the
         * offset of the 0x0050 encoder_gap_ms append, not the struct end. */
        QCOMPARE(offsetof(dev_config_t, slow_encoders) + sizeof(slow_encoder_t) * MAX_ENCODERS_NUM,
                 static_cast<size_t>(1652));
    }
    void encoderGap_isAppendedAtEnd()
    {
        /* The 0x0040 -> 0x0050 bump appended uint16_t encoder_gap_ms at the very
         * end, so its offset == the old (0x0040) config size 1652 -- the
         * migration boundary -- and it closes out the struct at 1654. */
        QCOMPARE(offsetof(dev_config_t, encoder_gap_ms), static_cast<size_t>(1652));
        QCOMPARE(offsetof(dev_config_t, encoder_gap_ms) + sizeof(uint16_t),
                 sizeof(dev_config_t));   /* == 1652 + 2 == 1654 */
    }
    void firmwareVersion_isGen5()
    {
        QCOMPARE(static_cast<int>(FIRMWARE_VERSION), 0x0050);
    }

    /* ---- legacy API ---- */
    void canMigrate_acceptsGen2()
    {
        QVERIFY(legacy::canMigrate(0x0020));
        QVERIFY(legacy::canMigrate(0x0021));   /* build-letter within the mask */
    }
    void canMigrate_acceptsGen3()
    {
        QVERIFY(legacy::canMigrate(0x0030));
        QVERIFY(legacy::canMigrate(0x0031));
        QVERIFY(legacy::canMigrate(0x0040));
        QVERIFY(legacy::canMigrate(0x0041));
        /* current gen is not "migratable" -- it's already current */
        QVERIFY(!legacy::canMigrate(0x0099));
    }
    void legacyConfigSize_gen2_isOldPrefixSize()
    {
        QCOMPARE(legacy::legacyConfigSize(0x0020),
                 offsetof(dev_config_t, gpio_expanders));
        QCOMPARE(legacy::legacyConfigSize(0x0020), static_cast<size_t>(1580));
        /* the current generation reports the full struct */
        QCOMPARE(legacy::legacyConfigSize(FIRMWARE_VERSION), sizeof(dev_config_t));
    }
    void legacyConfigSize_gen3_isPre0040PrefixSize()
    {
        QCOMPARE(legacy::legacyConfigSize(0x0030),
                 offsetof(dev_config_t, slow_encoders));
        QCOMPARE(legacy::legacyConfigSize(0x0030), static_cast<size_t>(1620));
    }
    void legacyConfigSize_gen4_isPre0050PrefixSize()
    {
        QCOMPARE(legacy::legacyConfigSize(0x0040),
                 offsetof(dev_config_t, encoder_gap_ms));
        QCOMPARE(legacy::legacyConfigSize(0x0040), static_cast<size_t>(1652));
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

    /* ---- 0x0040 -> 0x0050: pure append, encoder_gap_ms defaults ---- */
    void migrateGen4_preservesPrefix_defaultsGap_stampsVersion()
    {
        const size_t oldSize = offsetof(dev_config_t, encoder_gap_ms);

        /* A 0x0040 config already carries explicit slow_encoders[] (no synthesis
         * needed) -- truncated to the pre-encoder_gap_ms prefix. */
        dev_config_t seed = InitConfig();
        seed.firmware_version      = 0x0040;
        seed.button_debounce_ms    = 0x2468;              /* preserved-prefix probe */
        seed.encoder_press_time_ms = 50;
        seed.slow_encoders[3].btn_a = 12;                 /* preserved explicit pair */
        seed.slow_encoders[3].btn_b = 13;

        std::vector<uint8_t> raw(oldSize);
        std::memcpy(raw.data(), &seed, oldSize);

        /* Poison the destination, incl. the appended encoder_gap_ms, to prove the
         * migrator seeds it from factory defaults rather than leaving junk. */
        dev_config_t out;
        std::memset(&out, 0xFF, sizeof(out));

        legacy::MigrateResult r =
            legacy::migrateLegacyConfig(raw.data(), raw.size(), out);

        QCOMPARE(r, legacy::MigrateResult::Ok);
        QCOMPARE(out.firmware_version, static_cast<uint16_t>(FIRMWARE_VERSION));
        QCOMPARE(out.button_debounce_ms, static_cast<uint16_t>(0x2468));
        QCOMPARE(out.encoder_press_time_ms, static_cast<uint8_t>(50));
        QCOMPARE(out.slow_encoders[3].btn_a, static_cast<int8_t>(12));
        QCOMPARE(out.slow_encoders[3].btn_b, static_cast<int8_t>(13));
        /* the appended field must hold the factory default (20), not 0xFF junk */
        QCOMPARE(out.encoder_gap_ms, static_cast<uint16_t>(20));
    }

    void migrateGen4_rejectsTruncatedBuffer()
    {
        std::vector<uint8_t> tooSmall(8, 0);
        tooSmall[0] = 0x40;   /* version = 0x0040 (little-endian low byte) */
        dev_config_t out;
        QCOMPARE(legacy::migrateLegacyConfig(tooSmall.data(), tooSmall.size(), out),
                 legacy::MigrateResult::BufferTooSmall);
    }

    /* ---- 0x0030 -> 0x0040: positional ENCODER_INPUT_A/_B -> explicit pairs ---- */
    void migrateGen3_synthesizesSlowEncoderPairsFromPositional()
    {
        const size_t oldSize = offsetof(dev_config_t, slow_encoders);

        /* Build a 0x0030 raw config with two encoders wired the old way: button
         * slots typed ENCODER_INPUT_A / _B, paired positionally (Nth A with Nth
         * B in slot-index order). No slow_encoders[] bytes existed on the wire. */
        dev_config_t seed = InitConfig();
        seed.firmware_version   = 0x0030;
        seed.button_debounce_ms = 0x1234;                 /* preserved-prefix probe */
        seed.buttons[5].type  = ENCODER_INPUT_A;          /* encoder 1: A=5, B=6 */
        seed.buttons[6].type  = ENCODER_INPUT_B;
        seed.buttons[10].type = ENCODER_INPUT_A;          /* encoder 2: A=10, B=11 */
        seed.buttons[11].type = ENCODER_INPUT_B;

        std::vector<uint8_t> raw(oldSize);
        std::memcpy(raw.data(), &seed, oldSize);

        dev_config_t out;
        std::memset(&out, 0xFF, sizeof(out));             /* poison, incl. slow_encoders */

        legacy::MigrateResult r =
            legacy::migrateLegacyConfig(raw.data(), raw.size(), out);

        QCOMPARE(r, legacy::MigrateResult::Ok);
        QCOMPARE(out.firmware_version, static_cast<uint16_t>(FIRMWARE_VERSION));
        QCOMPARE(out.button_debounce_ms, static_cast<uint16_t>(0x1234));

        /* Button types survive the prefix copy AND are normalised: every
         * encoder line becomes the single ENCODER_INPUT_A (219) marker; the
         * legacy ENCODER_INPUT_B (220) is rewritten to 219 (A/B role now lives
         * in slow_encoders[]). */
        QCOMPARE(static_cast<int>(out.buttons[5].type),  static_cast<int>(ENCODER_INPUT_A));
        QCOMPARE(static_cast<int>(out.buttons[6].type),  static_cast<int>(ENCODER_INPUT_A));
        for (int i = 0; i < MAX_BUTTONS_NUM; ++i)
            QVERIFY(out.buttons[i].type != ENCODER_INPUT_B);

        /* Fast slots stay unwired; slow slots start at MAX_FAST_ENCODER_NUM. */
        for (int i = 0; i < MAX_FAST_ENCODER_NUM; ++i) {
            QCOMPARE(out.slow_encoders[i].btn_a, static_cast<int8_t>(-1));
            QCOMPARE(out.slow_encoders[i].btn_b, static_cast<int8_t>(-1));
        }
        QCOMPARE(out.slow_encoders[MAX_FAST_ENCODER_NUM + 0].btn_a, static_cast<int8_t>(5));
        QCOMPARE(out.slow_encoders[MAX_FAST_ENCODER_NUM + 0].btn_b, static_cast<int8_t>(6));
        QCOMPARE(out.slow_encoders[MAX_FAST_ENCODER_NUM + 1].btn_a, static_cast<int8_t>(10));
        QCOMPARE(out.slow_encoders[MAX_FAST_ENCODER_NUM + 1].btn_b, static_cast<int8_t>(11));
        /* Remaining slots are unwired (poison 0xFF was overwritten with -1). */
        for (int i = MAX_FAST_ENCODER_NUM + 2; i < MAX_ENCODERS_NUM; ++i) {
            QCOMPARE(out.slow_encoders[i].btn_a, static_cast<int8_t>(-1));
            QCOMPARE(out.slow_encoders[i].btn_b, static_cast<int8_t>(-1));
        }
    }

    /* A fresh (current-version) factory config has no encoder buttons, so every
     * slow_encoders[] slot must be unwired -- guards the InitConfig default
     * against the designated-init zero-fill ({0,0} = phantom encoders). */
    void freshConfig_hasNoPhantomEncoders()
    {
        dev_config_t c = InitConfig();
        for (int i = 0; i < MAX_ENCODERS_NUM; ++i) {
            QCOMPARE(c.slow_encoders[i].btn_a, static_cast<int8_t>(-1));
            QCOMPARE(c.slow_encoders[i].btn_b, static_cast<int8_t>(-1));
        }
    }
};

QTEST_APPLESS_MAIN(TestLegacyMigrate)
#include "test_legacymigrate.moc"
