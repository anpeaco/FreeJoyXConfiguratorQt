/* Regression guard for the factory-default seed (src/stm_main.c::InitConfig).
 *
 * tap_cutoff_ms / double_tap_window_ms were absent from the designated
 * initializer, so they defaulted to 0. With tap_cutoff_ms == 0 the gesture
 * ARMED state aborts immediately, so TAP and DOUBLE_TAP never fire -- on
 * every config that starts from InitConfig() (new/blank, old INI missing the
 * keys, legacy migration base). These asserts fail if the fields are dropped
 * again or drift away from the firmware default (FreeJoyX/application/Inc/main.h). */

#include <QtTest>

extern "C" {
#include "stm_main.h"   // InitConfig() -- C symbol, matches the app's bridge in deviceconfig.cpp
}

class TestInitConfig : public QObject
{
    Q_OBJECT

private slots:
    /* The regression itself: a fresh factory config must carry a usable tap
     * window, not 0. This is the assertion that would have caught the bug. */
    void gestureTimers_areNonZero()
    {
        const dev_config_t c = InitConfig();
        QVERIFY2(c.tap_cutoff_ms != 0,
                 "tap_cutoff_ms seeded to 0 -- TAP/DOUBLE_TAP will never fire");
        QVERIFY2(c.double_tap_window_ms != 0,
                 "double_tap_window_ms seeded to 0 -- DOUBLE_TAP will never fire");
    }

    /* Pin the exact defaults so they stay in lockstep with firmware main.h. */
    void tapCutoff_defaultIs200()
    {
        QCOMPARE(int(InitConfig().tap_cutoff_ms), 200);
    }

    void doubleTapWindow_defaultIs200()
    {
        QCOMPARE(int(InitConfig().double_tap_window_ms), 200);
    }

    /* Sanity-anchor an adjacent timing field that was already correct, so a
     * bad struct layout / wrong file wouldn't pass by coincidence. */
    void buttonDebounce_defaultIs20()
    {
        QCOMPARE(int(InitConfig().button_debounce_ms), 20);
    }
};

QTEST_APPLESS_MAIN(TestInitConfig)
#include "test_initconfig.moc"
