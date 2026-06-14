/*
 * Unit tests for freejoy::computeReportNumbers (src/widgets/buttons/hostbuttonnum.h).
 *
 * The logical-buttons tab shows a per-row "#". Historically that was the raw
 * dev_config.buttons[] slot index, which confusingly does NOT match the HID
 * button number the host (game/Windows) actually sees: the firmware COMPACTS
 * enabled slots into contiguous buttons at report time (FreeJoyX
 * application/Src/config.c buttons_cnt + buttons.c output loop).
 *
 * computeReportNumbers() reproduces that firmware counting host-side so the UI
 * can show the real HID button number. These tests pin that behaviour,
 * including the firmware's POV / disabled exclusions. (ConfiguratorQt#103)
 */
#include <QtTest>
#include "hostbuttonnum.h"

using namespace freejoy;

class TestHostButtonNum : public QObject
{
    Q_OBJECT

private:
    static ReportSlot bound(button_type_t t = BUTTON_NORMAL)  { return {true,  t}; }
    static ReportSlot unbound()                               { return {false, BUTTON_NORMAL}; }

private slots:
    // The reported screenshot config: bound slots 1, 2, 6 (0-based 0,1,5),
    // the rest unbound. Host should see them as buttons 1, 2, 3.
    void userCase()
    {
        QVector<ReportSlot> rows = {
            bound(),            // slot 1 -> host 1
            bound(LOGIC),       // slot 2 -> host 2
            unbound(),          // slot 3
            unbound(),          // slot 4
            unbound(),          // slot 5
            bound(),            // slot 6 -> host 3
            unbound(),          // slot 7 (trailing add row)
        };
        QVector<int> expected = {1, 2, 0, 0, 0, 3, 0};
        QCOMPARE(computeReportNumbers(rows), expected);
    }

    // Mute-in-place: a disabled (muted) button still occupies its position, so
    // numbering does NOT depend on the mute state -- mute is not even an input
    // to computeReportNumbers. Three bound buttons number 1,2,3 whether or not
    // the middle one is muted (the firmware reports the muted one as 0 but keeps
    // its slot; see buttons.c). This pins that mute never renumbers the others.
    void muteDoesNotRenumber()
    {
        QVector<ReportSlot> rows = { bound(), bound(), bound() };
        QVector<int> expected = {1, 2, 3};
        QCOMPARE(computeReportNumbers(rows), expected);
    }

    // Directional POV slots become a POV hat, not a button -- firmware sets
    // is_disabled for them, so they don't count.
    void directionalPovExcluded()
    {
        QVector<ReportSlot> rows = { bound(POV1_UP), bound() };
        QVector<int> expected = {0, 1};
        QCOMPARE(computeReportNumbers(rows), expected);

        // All four directions of each POV are excluded.
        QVERIFY(!isReportedButton(bound(POV1_DOWN)));
        QVERIFY(!isReportedButton(bound(POV2_LEFT)));
        QVERIFY(!isReportedButton(bound(POV4_RIGHT)));
    }

    // POV*_CENTER is NOT auto-disabled by the firmware, so it still reports as
    // a button (mirrors config.c, which only excludes UP/DOWN/LEFT/RIGHT).
    void povCenterCounts()
    {
        QVector<ReportSlot> rows = { bound(POV1_CENTER), bound() };
        QVector<int> expected = {1, 2};
        QCOMPARE(computeReportNumbers(rows), expected);
    }

    // Encoder / radio / toggle / sequential slots report as ordinary buttons.
    void nonPovSpecialTypesCount()
    {
        QVector<ReportSlot> rows = { bound(ENCODER_INPUT_A), bound(RADIO_BUTTON1), bound(BUTTON_TOGGLE) };
        QVector<int> expected = {1, 2, 3};
        QCOMPARE(computeReportNumbers(rows), expected);
    }

    void emptyConfigAllZero()
    {
        QVector<ReportSlot> rows = { unbound(), unbound(), unbound() };
        QVector<int> expected = {0, 0, 0};
        QCOMPARE(computeReportNumbers(rows), expected);
    }
};

QTEST_MAIN(TestHostButtonNum)
#include "test_hostbuttonnum.moc"
