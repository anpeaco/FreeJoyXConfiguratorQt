/* Unit test for board_display::cardBoardId() -- the device board_id byte ->
 * shown-board mapping used by BOTH getParamsPacket branches (recognised and
 * unrecognised firmware) for the Device card.
 *
 * Guards the fix for the "F411 shown as F103 after a firmware bump" regression:
 * an F411 (board_id 2) must resolve to F411 regardless of whether the firmware
 * generation is recognised, because board_id sits at a wire-stable early offset
 * in params_report_t. Anything else -> Blue Pill (the safe default). */

#include <QtTest>

#include "boarddisplay.h"   // board_display::cardBoardId + text()

/* Mirror common_defines.h so intent reads clearly (F103 = 1, F411 = 2). */
static constexpr int F103 = BOARD_ID_F103_BLUEPILL;
static constexpr int F411 = BOARD_ID_F411_BLACKPILL;

class TestBoardDisplay : public QObject
{
    Q_OBJECT

private slots:
    /* The regression that prompted this: an F411 must map to Black Pill even
     * when the firmware generation is unrecognised (the old fallback forced
     * F103 here). */
    void f411Byte_showsBlackPill()   { QCOMPARE(board_display::cardBoardId(2), F411); }

    void f103Byte_showsBluePill()    { QCOMPARE(board_display::cardBoardId(1), F103); }

    /* board_id 0 = an old/upstream build with no board tag -> Blue Pill default. */
    void zeroByte_showsBluePill()    { QCOMPARE(board_display::cardBoardId(0), F103); }

    /* Any unexpected value collapses to the safe Blue Pill default, never F411. */
    void unknownByte_showsBluePill() { QCOMPARE(board_display::cardBoardId(99), F103); }

    /* The mapping feeds the shared card text, so verify the end-to-end label. */
    void mappingDrivesCardText()
    {
        QCOMPARE(board_display::text(board_display::cardBoardId(2)),
                 QStringLiteral("F411 (Black Pill)"));
        QCOMPARE(board_display::text(board_display::cardBoardId(1)),
                 QStringLiteral("F103 (Blue Pill)"));
    }
};

QTEST_APPLESS_MAIN(TestBoardDisplay)
#include "test_boarddisplay.moc"
