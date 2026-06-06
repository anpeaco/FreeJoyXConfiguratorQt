/* Unit tests for the pure flash-decision logic in src/flashverdict.cpp:
 * classifyFlash() (the Confirm-flash verdict), isCrossingToFreeJoyX() (the red
 * FreeJoy->FreeJoyX banner), and firmwareNewerAvailable() (the device card's
 * Update-Firmware visibility). No GUI / FirmwareImage / legacy dependencies. */

#include <QtTest>

#include "flashverdict.h"

namespace {
/* Mirrors common_defines.h (BOARD_ID_F103_BLUEPILL = 1, F411 = 2). Inlined so
 * the test links nothing but flashverdict.cpp. */
constexpr int F103 = 1;
constexpr int F411 = 2;

/* Wire-format generations used below: FreeJoyX gen 1/2 = 0x0010/0x0020;
 * upstream FreeJoy lineage = 0x17xx. */
constexpr std::uint16_t FJX_G1 = 0x0010;
constexpr std::uint16_t FJX_G2 = 0x0020;
constexpr std::uint16_t UP_178 = 0x1780;
constexpr std::uint16_t UP_177 = 0x1770;
} // namespace

class TestFlashVerdict : public QObject
{
    Q_OBJECT

private slots:
    /* ---- classifyFlash ---- */
    void unloadedImage_isIncompatible()
    {
        QCOMPARE(classifyFlash(F411, FJX_G2, false, F411, FJX_G2, /*loaded*/ false, true),
                 FlashVerdict::Incompatible);
    }
    void unknownImageBoard_isIncompatible()
    {
        QCOMPARE(classifyFlash(F411, FJX_G2, false, /*imgBoard*/ 0, FJX_G2, true, true),
                 FlashVerdict::Incompatible);
    }
    void boardMismatch_isIncompatible()
    {
        QCOMPARE(classifyFlash(F103, FJX_G2, false, F411, FJX_G2, true, true),
                 FlashVerdict::Incompatible);
    }
    void recoveryMode_isRecovery()
    {
        QCOMPARE(classifyFlash(F411, FJX_G2, /*recovery*/ true, F411, FJX_G2, true, true),
                 FlashVerdict::Recovery);
    }
    void deviceVersionZero_isRecovery()
    {
        QCOMPARE(classifyFlash(F411, /*devFw*/ 0, false, F411, FJX_G2, true, true),
                 FlashVerdict::Recovery);
    }
    void sameGen_isSameGeneration()
    {
        QCOMPARE(classifyFlash(F411, FJX_G2, false, F411, FJX_G2, true, true),
                 FlashVerdict::SameGeneration);
    }
    void legacyImageNoVersion_isSameGeneration()
    {
        QCOMPARE(classifyFlash(F411, FJX_G2, false, F411, /*imgFw*/ 0, true, true),
                 FlashVerdict::SameGeneration);
    }
    void upgradeWithMigrator_isUpgrade()
    {
        QCOMPARE(classifyFlash(F411, FJX_G1, false, F411, FJX_G2, true, /*migrator*/ true),
                 FlashVerdict::Upgrade);
    }
    void upgradeNoMigrator_isUpgradeNoMigrator()
    {
        QCOMPARE(classifyFlash(F411, FJX_G1, false, F411, FJX_G2, true, /*migrator*/ false),
                 FlashVerdict::UpgradeNoMigrator);
    }
    void downgrade_isDowngrade()
    {
        QCOMPARE(classifyFlash(F411, FJX_G2, false, F411, FJX_G1, true, true),
                 FlashVerdict::Downgrade);
    }
    void unknownDeviceBoard_acceptedWhenImageKnown()
    {
        /* device board 0 (unknown) is not treated as a mismatch */
        QCOMPARE(classifyFlash(/*devBoard*/ 0, FJX_G2, false, F411, FJX_G2, true, true),
                 FlashVerdict::SameGeneration);
    }

    /* ---- isCrossingToFreeJoyX ---- */
    void crossing_upstreamToFreejoyx_true()
    {
        QVERIFY(isCrossingToFreeJoyX(UP_178, FJX_G2));
    }
    void crossing_upstreamToUpstream_false()
    {
        QVERIFY(!isCrossingToFreeJoyX(UP_177, UP_178));
    }
    void crossing_freejoyxToFreejoyx_false()
    {
        QVERIFY(!isCrossingToFreeJoyX(FJX_G1, FJX_G2));
    }
    void crossing_targetUnidentified_false()
    {
        QVERIFY(!isCrossingToFreeJoyX(UP_178, 0x0000));
    }

    /* ---- crossingMasksDowngrade ---- */
    void mask_downgradeOnCrossing_true()
    {
        /* The exact case the user flagged: upstream FreeJoy -> FreeJoyX flashes
         * as a "Downgrade" by raw version math, but the red crossing banner
         * covers it, so the amber box is suppressed. */
        QVERIFY(crossingMasksDowngrade(FlashVerdict::Downgrade, /*crossing*/ true));
    }
    void mask_downgradeNotCrossing_false()
    {
        /* A genuine same-project downgrade still shows its amber box. */
        QVERIFY(!crossingMasksDowngrade(FlashVerdict::Downgrade, /*crossing*/ false));
    }
    void mask_nonDowngradeOnCrossing_false()
    {
        /* Only Downgrade is masked; other verdicts render normally even when
         * crossing (e.g. an Incompatible board mismatch must stay visible). */
        QVERIFY(!crossingMasksDowngrade(FlashVerdict::Incompatible, true));
        QVERIFY(!crossingMasksDowngrade(FlashVerdict::SameGeneration, true));
    }

    /* ---- firmwareNewerAvailable ---- */
    void newer_olderSemverSameGen_true()
    {
        QVERIFY(firmwareNewerAvailable(0, 1, 5, 0, 1, 9, /*sameWireGen*/ true));
    }
    void newer_sameVersion_false()
    {
        QVERIFY(!firmwareNewerAvailable(0, 1, 9, 0, 1, 9, true));
    }
    void newer_differentWireGen_true()
    {
        QVERIFY(firmwareNewerAvailable(0, 1, 9, 0, 1, 9, /*sameWireGen*/ false));
    }
    void newer_deviceNewer_false()
    {
        QVERIFY(!firmwareNewerAvailable(0, 2, 0, 0, 1, 9, true));
    }
    void newer_allZeroDevice_true()
    {
        QVERIFY(firmwareNewerAvailable(0, 0, 0, 0, 1, 9, true));
    }
};

QTEST_APPLESS_MAIN(TestFlashVerdict)
#include "test_flashverdict.moc"
