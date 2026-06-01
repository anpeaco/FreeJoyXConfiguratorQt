// QtTest suite for the Pin Config model (Slice 1 deliverable).
// Drives PinConfig headlessly via its real slots/API and asserts on pin roles.
// Hard QCOMPARE/QVERIFY so a failing test == a real behaviour bug surfaced.
//
// m_buttonConfig stays null (pinconfig.h: "may stay null in isolated tests");
// ButtonConfig methods PinConfig references are satisfied by test_pinconfig_stubs.cpp.
// Tests avoid the conflict path (which opens a modal BusRemapConfirmationDialog and
// would block headless) -- the modal-dependent collision case is documented as a
// seam gap in PINCONFIG_MODEL_PLAN.md.
#include <QtTest>
#include <QSettings>
#include <QCheckBox>
#include "global.h"          // Pin enum (PA_0..), gEnv
#include "common_defines.h"  // BOARD_ID_*
#include "common_types.h"    // role enum
#include "deviceconfig.h"
#include "widgets/pins/pinconfig.h"
#include "widgets/pins/pintypehelper.h"  // PinTypeHelper::BUS_I2C / BUS_SPI

GlobalEnvironment gEnv;

class TestPinConfig : public QObject
{
    Q_OBJECT

    QSettings   *m_settings = nullptr;
    DeviceConfig *m_dc = nullptr;
    PinConfig    *m_pc = nullptr;

    void enableBus(int bus, bool enable)
    {
        QMetaObject::invokeMethod(m_pc, "onBusToggleRequested", Qt::DirectConnection,
                                  Q_ARG(int, bus), Q_ARG(bool, enable));
    }

    // Read the quick-setup bus checkbox state by object name (no production hook).
    bool busChecked(const char *objectName)
    {
        auto *cb = m_pc->findChild<QCheckBox *>(QString::fromLatin1(objectName));
        return cb && cb->isChecked();
    }

private slots:
    void initTestCase()
    {
        m_settings = new QSettings(QStringLiteral("test_pinconfig_settings.ini"),
                                   QSettings::IniFormat);
        m_settings->setValue(QStringLiteral("BoardSettings/SelectedBoard"), 0);
        gEnv.pAppSettings = m_settings;
    }

    void init()   // fresh device + widget per test
    {
        m_dc = new DeviceConfig();
        m_dc->resetConfig();
        gEnv.pDeviceConfig = m_dc;
        m_pc = new PinConfig();
        m_pc->setConnectedBoard(BOARD_ID_F103_BLUEPILL);
    }

    void cleanup()
    {
        delete m_pc; m_pc = nullptr;
        delete m_dc; m_dc = nullptr;
        gEnv.pDeviceConfig = nullptr;
    }

    // --- I2C bus enable lands on the correct per-board pins ---
    void i2cEnable_f103()
    {
        enableBus(PinTypeHelper::BUS_I2C, true);
        QCOMPARE(m_pc->pinRole(PB_10), int(I2C_SCL));  // SCL = PB10 (both boards)
        QCOMPARE(m_pc->pinRole(PB_11), int(I2C_SDA));  // F103 SDA = PB11
        QCOMPARE(m_pc->pinRole(PB_9),  int(NOT_USED)); // PB9 not touched on F103
    }

    void i2cEnable_f411()
    {
        m_pc->setConnectedBoard(BOARD_ID_F411_BLACKPILL);
        enableBus(PinTypeHelper::BUS_I2C, true);
        QCOMPARE(m_pc->pinRole(PB_10), int(I2C_SCL));  // SCL = PB10
        QCOMPARE(m_pc->pinRole(PB_9),  int(I2C_SDA));  // F411 SDA = PB9
        QCOMPARE(m_pc->pinRole(PB_11), int(NOT_USED)); // PB11 = PB2 on F411, no I2C
    }

    void i2cDisable_clearsPins_f103()
    {
        enableBus(PinTypeHelper::BUS_I2C, true);
        enableBus(PinTypeHelper::BUS_I2C, false);
        QCOMPARE(m_pc->pinRole(PB_10), int(NOT_USED));
        QCOMPARE(m_pc->pinRole(PB_11), int(NOT_USED));
    }

    void i2cDisable_clearsPins_f411()
    {
        m_pc->setConnectedBoard(BOARD_ID_F411_BLACKPILL);
        enableBus(PinTypeHelper::BUS_I2C, true);
        enableBus(PinTypeHelper::BUS_I2C, false);
        QCOMPARE(m_pc->pinRole(PB_10), int(NOT_USED));
        QCOMPARE(m_pc->pinRole(PB_9),  int(NOT_USED));
    }

    // --- Per-board I2C SDA capability ---
    void i2cSda_legalOnF103_PB11()
    {
        QVERIFY(m_pc->setPinRole(PB_11, I2C_SDA));
        QCOMPARE(m_pc->pinRole(PB_11), int(I2C_SDA));
    }

    void i2cSda_legalOnF411_PB9()
    {
        m_pc->setConnectedBoard(BOARD_ID_F411_BLACKPILL);
        QVERIFY(m_pc->setPinRole(PB_9, I2C_SDA));
        QCOMPARE(m_pc->pinRole(PB_9), int(I2C_SDA));
    }

    // --- #59 characterization: a bare board switch must NOT wipe ordinary,
    //     board-independent roles (the switch itself is innocent). ---
    void boardSwitch_preservesPlainRoles()
    {
        m_pc->setPinRole(PB_8,  SHIFT_REG_CLK);
        m_pc->setPinRole(PB_12, BUTTON_GND);
        m_pc->setConnectedBoard(BOARD_ID_F411_BLACKPILL);
        QCOMPARE(m_pc->pinRole(PB_8),  int(SHIFT_REG_CLK));
        QCOMPARE(m_pc->pinRole(PB_12), int(BUTTON_GND));
    }

    // --- I2C SDA migrates PB11 -> PB9 when switching F103 -> F411 (Slice 3 fix).
    //     PB11 = PB2 on F411 has no I2C cap, so the bus must follow to PB9. ---
    void boardSwitch_migratesI2cSda()
    {
        enableBus(PinTypeHelper::BUS_I2C, true);        // F103: SCL=PB10, SDA=PB11
        QCOMPARE(m_pc->pinRole(PB_11), int(I2C_SDA));
        m_pc->setConnectedBoard(BOARD_ID_F411_BLACKPILL);

        QCOMPARE(m_pc->pinRole(PB_10), int(I2C_SCL));   // SCL is PB10 on both boards
        QCOMPARE(m_pc->pinRole(PB_9),  int(I2C_SDA));   // SDA migrated to PB9
        QCOMPARE(m_pc->pinRole(PB_11), int(NOT_USED));  // stale SDA cleared
    }

    // Reverse direction: F411 (SDA=PB9) -> F103 migrates SDA back to PB11.
    void boardSwitch_migratesI2cSda_reverse()
    {
        m_pc->setConnectedBoard(BOARD_ID_F411_BLACKPILL);
        enableBus(PinTypeHelper::BUS_I2C, true);        // F411: SCL=PB10, SDA=PB9
        QCOMPARE(m_pc->pinRole(PB_9), int(I2C_SDA));
        m_pc->setConnectedBoard(BOARD_ID_F103_BLUEPILL);

        QCOMPARE(m_pc->pinRole(PB_10), int(I2C_SCL));   // SCL is PB10 on both boards
        QCOMPARE(m_pc->pinRole(PB_11), int(I2C_SDA));   // SDA migrated back to PB11
        QCOMPARE(m_pc->pinRole(PB_9),  int(NOT_USED));  // stale SDA cleared
    }

    // SCL (PB10) is board-independent and must survive a switch either way.
    void boardSwitch_preservesScl()
    {
        enableBus(PinTypeHelper::BUS_I2C, true);
        m_pc->setConnectedBoard(BOARD_ID_F411_BLACKPILL);
        QCOMPARE(m_pc->pinRole(PB_10), int(I2C_SCL));
        m_pc->setConnectedBoard(BOARD_ID_F103_BLUEPILL);
        QCOMPARE(m_pc->pinRole(PB_10), int(I2C_SCL));
    }

    // A board switch must not disturb pins unrelated to the per-board difference.
    void boardSwitch_doesNotDisturbUnrelatedPins()
    {
        m_pc->setPinRole(PB_12, BUTTON_GND);
        m_pc->setPinRole(PB_8,  SHIFT_REG_CLK);
        enableBus(PinTypeHelper::BUS_I2C, true);
        m_pc->setConnectedBoard(BOARD_ID_F411_BLACKPILL);
        QCOMPARE(m_pc->pinRole(PB_12), int(BUTTON_GND));
        QCOMPARE(m_pc->pinRole(PB_8),  int(SHIFT_REG_CLK));
    }

    // A plain role survives a full F103 -> F411 -> F103 round trip.
    void boardSwitch_roundTrip_preservesPlainRole()
    {
        m_pc->setPinRole(PB_8, SHIFT_REG_CLK);
        m_pc->setConnectedBoard(BOARD_ID_F411_BLACKPILL);
        m_pc->setConnectedBoard(BOARD_ID_F103_BLUEPILL);
        QCOMPARE(m_pc->pinRole(PB_8), int(SHIFT_REG_CLK));
    }

    // Enabling I2C twice is idempotent (no churn, no extra pins).
    void i2cEnable_idempotent_f103()
    {
        enableBus(PinTypeHelper::BUS_I2C, true);
        enableBus(PinTypeHelper::BUS_I2C, true);
        QCOMPARE(m_pc->pinRole(PB_10), int(I2C_SCL));
        QCOMPARE(m_pc->pinRole(PB_11), int(I2C_SDA));
    }

    // SPI bus lands on PB3/4/5 (SCK/MISO/MOSI) on both boards.
    void spiEnable_f103()
    {
        enableBus(PinTypeHelper::BUS_SPI, true);
        QCOMPARE(m_pc->pinRole(PB_3), int(SPI_SCK));
        QCOMPARE(m_pc->pinRole(PB_4), int(SPI_MISO));
        QCOMPARE(m_pc->pinRole(PB_5), int(SPI_MOSI));
    }

    // ---- #57: a sensor auto-assign that overwrites a user role must RECORD the
    //      displacement, so the user is warned instead of silently losing it. ----
    void sensorAutoAssign_recordsDisplacedRole()
    {
        m_pc->setPinRole(PB_6, SHIFT_REG_DATA);          // user role on the GEN pin (PB6)
        QCOMPARE(m_pc->pinRole(PB_6), int(SHIFT_REG_DATA));
        m_pc->setPinRole(PB_1, TLE5011_CS);              // TLE5011 -> auto-claims GEN on PB6
        bool recorded = false;
        for (const auto &d : m_pc->autoAssignDisplaced()) {
            if (d.pin == PB_6 && d.role == SHIFT_REG_DATA) recorded = true;
        }
        QVERIFY2(recorded, "#57: overwriting SR DATA on PB6 must be recorded for a warning");
    }

    // No false warning: auto-assigning onto free pins records no displacement.
    void sensorAutoAssign_noDisplacementOnFreePins()
    {
        m_pc->setPinRole(PB_1, TLE5011_CS);              // all auto-claimed pins are free
        QVERIFY(m_pc->autoAssignDisplaced().isEmpty());
    }

    // ---- #65: while a TLE owns the GEN clock on B6, the user must not be able
    //      to overwrite it -- B6's non-GEN options are locked off. ----
    void sensorActive_locksGenPin()
    {
        QVERIFY(m_pc->isPinRoleOptionEnabled(PB_6, SHIFT_REG_DATA));   // free before
        m_pc->setPinRole(PB_1, TLE5011_CS);             // TLE active -> GEN auto-claims B6
        QCOMPARE(m_pc->pinRole(PB_6), int(TLE5011_GEN));
        QVERIFY(m_pc->isPinRoleOptionEnabled(PB_6, TLE5011_GEN));      // GEN stays selectable
        QVERIFY2(!m_pc->isPinRoleOptionEnabled(PB_6, SHIFT_REG_DATA),
                 "#65: B6 non-GEN options must be disabled while a TLE is active");
        m_pc->setPinRole(PB_1, NOT_USED);               // remove sensor -> frees B6
        QVERIFY2(m_pc->isPinRoleOptionEnabled(PB_6, SHIFT_REG_DATA),
                 "B6 options restored after the sensor is removed");
    }

    // ---- #58: adding an SPI sensor auto-claims the SPI bus; removing the last
    //      one must clear SCK/MISO/MOSI AND drop the SPI bus checkbox. ----
    void spiCheckbox_clearsAfterLastSensorRemoved()
    {
        m_pc->setPinRole(PB_1, TLE5011_CS);             // SPI sensor -> auto-claims bus
        QCOMPARE(m_pc->pinRole(PB_3), int(SPI_SCK));    // SCK auto-claimed
        QVERIFY2(busChecked("checkBox_spiBus"), "SPI box should read on while a sensor owns the bus");

        m_pc->setPinRole(PB_1, NOT_USED);               // remove the only SPI sensor
        QCOMPARE(m_pc->pinRole(PB_3), int(NOT_USED));   // SCK released (user confirms this works)
        QVERIFY2(!busChecked("checkBox_spiBus"), "#58: SPI box must clear once no sensor owns the bus");
    }
};

QTEST_MAIN(TestPinConfig)
#include "test_pinconfig.moc"
