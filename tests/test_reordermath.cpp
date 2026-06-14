/*
 * Unit tests for the logical-button drag-reorder hit-testing
 * (src/widgets/buttons/reordermath.h).
 *
 * Regression for the "can only drop into one position / drop line in the wrong
 * place" bug: the old targetSlotForY / indicatorYForCursor walked ALL 128 rows,
 * including ones hidden by the "Show bound only" filter, which retain stale
 * geometry. The midline sequence then wasn't monotonic and the drop collapsed
 * onto a hidden slot. These helpers take only the VISIBLE rows (caller filters)
 * so the math stays correct with gaps. (ConfiguratorQt: reorder-with-filter fix)
 */
#include <QtTest>
#include "reordermath.h"

using namespace freejoy;

class TestReorderMath : public QObject
{
    Q_OBJECT

private:
    // All 30px-high rows stacked from y=0; `slotIdx` is the config slot.
    static RowGeom row(int slotIdx, int top) { return {slotIdx, top, 30}; }

private slots:
    // Baseline: every row visible and contiguous (slots 0..3 at y 0/30/60/90).
    void contiguousRows()
    {
        QVector<RowGeom> rows = { row(0,0), row(1,30), row(2,60), row(3,90) };
        QCOMPARE(dropTargetSlot(rows, 5),   0);   // above row 0 midline
        QCOMPARE(dropTargetSlot(rows, 40),  1);   // into row 1
        QCOMPARE(dropTargetSlot(rows, 70),  2);   // into row 2
        QCOMPARE(dropTargetSlot(rows, 200), 3);   // below all -> last slot
    }

    // The reported config: bound rows at slots 0,1,5,6 are COMPACTED to the top
    // (slots 2,3,4 hidden, not in the list). A drop over the slot-5 row must
    // resolve to slot 5 -- NOT a hidden slot, and not collapse to one position.
    void gappySlotsResolveToVisibleRow()
    {
        QVector<RowGeom> rows = { row(0,0), row(1,30), row(5,60), row(6,90) };
        QCOMPARE(dropTargetSlot(rows, 70),  5);   // over the slot-5 row
        QCOMPARE(dropTargetSlot(rows, 100), 6);   // over the slot-6 row
        QCOMPARE(dropTargetSlot(rows, 40),  1);   // over the slot-1 row
        QCOMPARE(dropTargetSlot(rows, 5),   0);   // above the first visible row
        QCOMPARE(dropTargetSlot(rows, 500), 6);   // below all visible -> slot 6
    }

    // Indicator line lands at the top of the target gap, using only visible rows.
    void indicatorWithGaps()
    {
        QVector<RowGeom> rows = { row(0,0), row(1,30), row(5,60), row(6,90) };
        QCOMPARE(dropIndicatorY(rows, 5),   0);    // above first -> first.top
        QCOMPARE(dropIndicatorY(rows, 70),  60);   // gap above the slot-5 row
        QCOMPARE(dropIndicatorY(rows, 500), 120);  // below last -> last bottom (90+30)
    }

    // The "below the last visible row" tail drop (drives the moveButton
    // index-shift skip in the Drop handler).
    void belowLastRow()
    {
        QVector<RowGeom> rows = { row(0,0), row(1,30), row(5,60), row(6,90) };
        QVERIFY(!isBelowLastRow(rows, 100));   // over last row, above its midline
        QVERIFY( isBelowLastRow(rows, 120));   // past the last row's midline
    }

    void emptyList()
    {
        QVector<RowGeom> rows;
        QCOMPARE(dropTargetSlot(rows, 50), -1);
        QVERIFY(!isBelowLastRow(rows, 50));
    }

    // A single visible row (e.g. only one bound button): every drop targets it.
    void singleRow()
    {
        QVector<RowGeom> rows = { row(3, 0) };
        QCOMPARE(dropTargetSlot(rows, 5),   3);
        QCOMPARE(dropTargetSlot(rows, 25),  3);
    }
};

QTEST_MAIN(TestReorderMath)
#include "test_reordermath.moc"
