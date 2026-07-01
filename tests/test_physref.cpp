/*
 * Unit tests for the physical-button reference math (src/widgets/buttons/physref.h).
 *
 * Regression for C1 (review finding): the firmware enumerates physical buttons
 * Matrix -> ShiftReg -> Expander -> A2b -> Direct (buttons.c::ButtonsReadPhysical),
 * and the button grid indexes them that way -- but physref originally had no
 * Expander category, so toRef/toAbs ignored the expander block and mis-mapped
 * every A2b/Direct reference (and LOGIC src_b) once expander buttons existed.
 * These tests pin the expander category into the position math and prove that a
 * reference tracks the layout across an expander-breakdown change.
 *
 * physref.h is header-only (free functions in namespace freejoy), so this links
 * nothing else.
 */
#include <QtTest>
#include "physref.h"

using namespace freejoy;

class TestPhysRef : public QObject
{
    Q_OBJECT

    // matrix=4 | SR0=8 | Exp0=16, Exp1=4 | A2b0=2, A2b1=3 | direct=5  (total 42)
    //
    // Absolute layout:
    //   Matrix  0..3
    //   SR0     4..11
    //   Exp0    12..27
    //   Exp1    28..31
    //   A2b0    32..33
    //   A2b1    34..36
    //   Direct  37..41
    static PhysBreakdown sample()
    {
        PhysBreakdown b;
        b.matrix = 4;
        b.perSR  = { 8 };
        b.perExp = { 16, 4 };
        b.perA2b = { 2, 3 };
        b.direct = 5;
        return b;
    }

private slots:
    void total_includesExpanders()
    {
        QCOMPARE(sample().total(), 42);
        QCOMPARE(sample().totalExp(), 20);
    }

    // toRef classifies every absolute position into the right category, with the
    // expander block sitting between SR and A2b.
    void toRef_classifiesAcrossCategories()
    {
        const PhysBreakdown b = sample();

        QCOMPARE(toRef(0,  b).cat, PhysCategory::Matrix);
        QCOMPARE(toRef(4,  b).cat, PhysCategory::ShiftReg);
        QCOMPARE(toRef(4,  b).offset, 0);

        // Expander 0 (slot 0), Expander 1 (slot 1).
        QCOMPARE(toRef(12, b).cat, PhysCategory::Expander);
        QCOMPARE(toRef(12, b).subIdx, 0);
        QCOMPARE(toRef(12, b).offset, 0);
        QCOMPARE(toRef(27, b).cat, PhysCategory::Expander);
        QCOMPARE(toRef(27, b).subIdx, 0);
        QCOMPARE(toRef(27, b).offset, 15);
        QCOMPARE(toRef(28, b).subIdx, 1);
        QCOMPARE(toRef(31, b).subIdx, 1);
        QCOMPARE(toRef(31, b).offset, 3);

        // A2b sits AFTER the 20 expander buttons (regression: previously these
        // were mis-classified because the expander offset was ignored).
        QCOMPARE(toRef(32, b).cat, PhysCategory::A2b);
        QCOMPARE(toRef(32, b).subIdx, 0);
        QCOMPARE(toRef(34, b).cat, PhysCategory::A2b);
        QCOMPARE(toRef(34, b).subIdx, 1);

        QCOMPARE(toRef(37, b).cat, PhysCategory::Direct);
        QCOMPARE(toRef(37, b).offset, 0);
        QCOMPARE(toRef(41, b).cat, PhysCategory::Direct);
        QCOMPARE(toRef(41, b).offset, 4);

        QCOMPARE(toRef(42, b).cat, PhysCategory::Invalid);   // one past the end
        QCOMPARE(toRef(-1, b).cat, PhysCategory::Invalid);
    }

    // toAbs is the exact inverse of toRef for every valid position.
    void toAbs_roundTripsEveryPosition()
    {
        const PhysBreakdown b = sample();
        for (int p = 0; p < b.total(); ++p)
            QCOMPARE(toAbs(toRef(p, b), b), p);
    }

    // The C1 fix in action: removing expander 1 (4 buttons) shifts every A2b and
    // Direct reference DOWN by 4, while Matrix/SR refs (which precede expanders)
    // are untouched. Without the expander category this shift was invisible and
    // the references silently pointed at the wrong inputs.
    void expanderChange_shiftsLaterRefs_keepsEarlierRefs()
    {
        const PhysBreakdown oldB = sample();
        PhysBreakdown newB = sample();
        newB.perExp = { 16, 0 };   // expander 1 disabled -> 4 fewer buttons

        // A Direct reference moves with the layout.
        const int directAbsOld = 37;                  // Direct #0 under oldB
        const PhysRef ref = toRef(directAbsOld, oldB);
        QCOMPARE(ref.cat, PhysCategory::Direct);
        QCOMPARE(toAbs(ref, newB), 33);               // 37 - 4 removed expander buttons

        // An A2b reference likewise.
        const PhysRef a2bRef = toRef(34, oldB);       // A2b axis1 #0
        QCOMPARE(a2bRef.cat, PhysCategory::A2b);
        QCOMPARE(a2bRef.subIdx, 1);
        QCOMPARE(toAbs(a2bRef, newB), 30);            // 34 - 4

        // A Matrix / SR reference is unaffected (they precede the expander block).
        QCOMPARE(toAbs(toRef(2, oldB), newB), 2);     // Matrix #2
        QCOMPARE(toAbs(toRef(9, oldB), newB), 9);     // SR0 #5
    }

    // A reference INTO an expander that is later removed becomes invalid (-1),
    // which the auto-remap surfaces as a broken slot rather than silently
    // re-pointing it elsewhere.
    void removedExpanderRef_becomesInvalid()
    {
        const PhysBreakdown oldB = sample();
        PhysBreakdown newB = sample();
        newB.perExp = { 16, 0 };

        const PhysRef ref = toRef(28, oldB);          // Expander slot 1 #0
        QCOMPARE(ref.cat, PhysCategory::Expander);
        QCOMPARE(ref.subIdx, 1);
        QCOMPARE(toAbs(ref, newB), -1);               // slot 1 now has 0 buttons
    }

    // Without any expanders, the math is identical to the original 4-category
    // behaviour (no regression for configs that never use an expander).
    void noExpanders_behavesLikeBefore()
    {
        PhysBreakdown b;
        b.matrix = 2;
        b.perSR  = { 4 };
        b.perA2b = { 3 };
        b.direct = 2;                                 // perExp left empty
        QCOMPARE(b.totalExp(), 0);
        QCOMPARE(b.total(), 11);
        QCOMPARE(toRef(6, b).cat, PhysCategory::A2b);     // 2 matrix + 4 SR -> first a2b
        QCOMPARE(toRef(9, b).cat, PhysCategory::Direct);
        for (int p = 0; p < b.total(); ++p)
            QCOMPARE(toAbs(toRef(p, b), b), p);
    }

    void refLabel_expander()
    {
        PhysRef r;
        r.cat = PhysCategory::Expander;
        r.subIdx = 2;
        r.offset = 7;
        QCOMPARE(refLabel(r), QStringLiteral("Exp3 #8"));
    }
};

QTEST_APPLESS_MAIN(TestPhysRef)
#include "test_physref.moc"
