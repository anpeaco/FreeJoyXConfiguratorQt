// Round-trip test for the per-SR pin-select nibbles packed into
// shift_reg_config_t.reserved[2] (0 = Auto, N = the (N-1)-th role pin).
//
// Drives ConfigToFile save/load directly on a dev_config_t: gEnv.pDeviceConfig
// stays null, so crossBoardCheck() early-returns (deviceBoard == 0) and this
// isolates the QSettings persistence of the reserved bytes -- the field that was
// silently dropped on a .cfg round-trip before this change.
#include <QtTest>
#include <QSettings>
#include <QTemporaryDir>
#include <cstring>
#include "configtofile.h"
#include "common_defines.h"   // FIRMWARE_VERSION, MAX_SHIFT_REG_NUM
#include "global.h"

GlobalEnvironment gEnv;   // satisfy the extern in global.h

class TestShiftRegPins : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;

    QString path(const char *name) { return m_dir.filePath(QString::fromLatin1(name)); }

    // A zeroed config stamped with the current firmware version, so the load-side
    // oldConfigHandler() migration is a no-op and can't perturb the shift-reg block.
    static void zero(dev_config_t &c)
    {
        std::memset(&c, 0, sizeof(c));
        c.firmware_version = FIRMWARE_VERSION;
    }

private slots:
    // Explicit per-pin picks packed into the reserved nibbles survive save/load.
    void reservedNibbles_roundTrip()
    {
        dev_config_t a; zero(a);
        // reserved[0] lo = data, hi = latch; reserved[1] lo = clk (hi free).
        a.shift_registers[0].reserved[0] = int8_t(0x02 | (0x03 << 4)); // data=2, latch=3
        a.shift_registers[0].reserved[1] = int8_t(0x04);              // clk=4
        a.shift_registers[1].reserved[0] = int8_t(0x01);              // data=1, latch=Auto
        a.shift_registers[1].reserved[1] = int8_t(0x00);              // clk=Auto
        a.shift_registers[3].reserved[0] = int8_t(0x0F | (0x0F << 4)); // both nibbles maxed
        a.shift_registers[3].reserved[1] = int8_t(0x0F);

        const QString f = path("sr_roundtrip.ini");
        ConfigToFile::saveDeviceConfigToFile(f, a);

        dev_config_t b; zero(b);
        ConfigToFile::loadDeviceConfigFromFile(nullptr, f, b);

        for (int i = 0; i < MAX_SHIFT_REG_NUM; ++i) {
            QCOMPARE(b.shift_registers[i].reserved[0], a.shift_registers[i].reserved[0]);
            QCOMPARE(b.shift_registers[i].reserved[1], a.shift_registers[i].reserved[1]);
        }
    }

    // Backward-compat: an all-Auto config (reserved = 0 everywhere) round-trips as
    // all-zero. Poison the destination first so the assertion proves the load
    // actively wrote zeros back, not that b happened to already be clear.
    void allAuto_staysZero()
    {
        dev_config_t a; zero(a);   // reserved already all 0 == every SR on Auto
        const QString f = path("sr_auto.ini");
        ConfigToFile::saveDeviceConfigToFile(f, a);

        dev_config_t b; zero(b);
        for (int i = 0; i < MAX_SHIFT_REG_NUM; ++i) {
            b.shift_registers[i].reserved[0] = 0x7F;
            b.shift_registers[i].reserved[1] = 0x7F;
        }
        ConfigToFile::loadDeviceConfigFromFile(nullptr, f, b);

        for (int i = 0; i < MAX_SHIFT_REG_NUM; ++i) {
            QCOMPARE(b.shift_registers[i].reserved[0], int8_t(0));
            QCOMPARE(b.shift_registers[i].reserved[1], int8_t(0));
        }
    }

    // A file written before this feature has no Reserved0/1 keys. The loader then
    // falls back to the value already in devC -- which is 0 after resetConfig, so
    // an old file reads as all-Auto == legacy. Prove the fallback engages (missing
    // key leaves the pre-existing value) AND that the SR block otherwise loaded.
    void missingKeys_fallBackToCurrentValue()
    {
        dev_config_t a; zero(a);
        a.shift_registers[0].button_cnt = 8;   // a plausible real chain
        const QString f = path("sr_legacy.ini");
        ConfigToFile::saveDeviceConfigToFile(f, a);
        {
            QSettings s(f, QSettings::IniFormat);
            for (int i = 0; i < MAX_SHIFT_REG_NUM; ++i) {
                s.beginGroup("ShiftRegsConfig_" + QString::number(i));
                s.remove("Reserved0");
                s.remove("Reserved1");
                s.endGroup();
            }
        }

        dev_config_t b; zero(b);
        b.shift_registers[0].reserved[0] = int8_t(0x0A);   // stand-in for "current value"
        ConfigToFile::loadDeviceConfigFromFile(nullptr, f, b);

        // Missing key -> loader keeps the pre-existing value (so a reset config's 0
        // stays Auto in production).
        QCOMPARE(b.shift_registers[0].reserved[0], int8_t(0x0A));
        // ...and the surrounding SR fields DID load, so this isn't a vacuous pass.
        QCOMPARE(b.shift_registers[0].button_cnt, uint8_t(8));
    }
};

QTEST_MAIN(TestShiftRegPins)
#include "test_shiftregpins.moc"
