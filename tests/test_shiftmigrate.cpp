/* Regression guard for the 0x0050 -> 0x0060 shift migration (legacy_migrator.cpp).
 *
 * Wire gen 0x0060 moved shift modifiers out of the mid-struct shift_config[]
 * (logical-button indices) into a dedicated shift_buttons[] array appended at the
 * end. The migrator must lift each configured shift_config[i].button into
 * shift_buttons[i] = buttons[idx] (a deep copy of that button's definition) so
 * TOGGLE/etc. shift behaviour survives, and must leave the original buttons[]
 * slot untouched (clearing it would silently renumber the user's HID buttons). */

#include <QtTest>
#include <cstddef>   // offsetof

#include "legacy_migrator.h"   // legacy::migrateLegacyConfig, legacy::MigrateResult
extern "C" {
#include "stm_main.h"          // InitConfig()  (C symbol, matches the migrator's own bridge)
}
#include "common_types.h"
#include "common_defines.h"

class TestShiftMigrate : public QObject
{
    Q_OBJECT

private slots:
    void migrate_v0050_lifts_shift_config_into_shift_buttons()
    {
        // Synthesise a 0x0050 device config: it is the current struct minus the
        // appended shift_buttons[] -- i.e. the byte-exact prefix up to shift_buttons.
        dev_config_t src = InitConfig();
        src.firmware_version = 0x0050;

        // Two ordinary buttons used as shift triggers on the old board.
        src.buttons[10].physical_num = 5;
        src.buttons[10].type = BUTTON_TOGGLE;   // -> a latching shift-lock after migration
        src.buttons[20].physical_num = 7;
        src.buttons[20].type = BUTTON_NORMAL;
        // A real 0x0050 device inits every shift_config slot to -1 (unwired).
        for (int i = 0; i < MAX_SHIFTS_NUM; ++i)
            src.shift_config[i].button = -1;
        src.shift_config[0].button = 10;        // Shift 1 <- button 10
        src.shift_config[1].button = 20;        // Shift 2 <- button 20

        // The device sends exactly the pre-0x0060 prefix.
        const size_t blobSize = offsetof(dev_config_t, shift_buttons);
        QVERIFY(blobSize < sizeof(dev_config_t));   // shift_buttons really is a tail append

        dev_config_t out;
        const legacy::MigrateResult r = legacy::migrateLegacyConfig(
            reinterpret_cast<const uint8_t *>(&src), blobSize, out);

        QVERIFY2(r == legacy::MigrateResult::Ok, "0x0050 must be migratable to current");
        QCOMPARE(int(out.firmware_version), int(FIRMWARE_VERSION));   // re-stamped to 0x0060

        // The two configured shifts are lifted into shift_buttons[] as deep copies.
        QCOMPARE(int(out.shift_buttons[0].physical_num), 5);
        QCOMPARE(int(out.shift_buttons[0].type), int(BUTTON_TOGGLE));
        QCOMPARE(int(out.shift_buttons[1].physical_num), 7);
        QCOMPARE(int(out.shift_buttons[1].type), int(BUTTON_NORMAL));

        // Unwired shift slots stay unwired.
        QCOMPARE(int(out.shift_buttons[2].physical_num), -1);
        QCOMPARE(int(out.shift_buttons[7].physical_num), -1);

        // The original buttons[] slots are preserved (no HID renumber).
        QCOMPARE(int(out.buttons[10].physical_num), 5);
        QCOMPARE(int(out.buttons[20].physical_num), 7);
    }

    void already_current_is_not_migrated()
    {
        // A config already at the current version must be a no-op (NotNeeded),
        // NOT re-transformed (its shift_buttons are already authoritative).
        dev_config_t src = InitConfig();               // firmware_version == FIRMWARE_VERSION
        dev_config_t out;
        const legacy::MigrateResult r = legacy::migrateLegacyConfig(
            reinterpret_cast<const uint8_t *>(&src), sizeof(src), out);
        QVERIFY(r == legacy::MigrateResult::NotNeeded);
    }
};

QTEST_APPLESS_MAIN(TestShiftMigrate)
#include "test_shiftmigrate.moc"
