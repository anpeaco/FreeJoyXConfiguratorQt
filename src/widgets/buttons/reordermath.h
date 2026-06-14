#ifndef REORDERMATH_H
#define REORDERMATH_H

/*
 * Pure hit-testing for the logical-button drag-reorder, factored out of
 * ButtonConfig so it is unit-testable without a live widget tree.
 *
 * The caller passes ONLY the visible rows (in top-to-bottom order). This is the
 * fix for the "drop collapses to one position / indicator in the wrong gap"
 * bug: the previous in-widget code walked all 128 rows including ones hidden by
 * the "Show bound only" filter, which keep stale geometry, so the midline
 * sequence wasn't monotonic. Skipping hidden rows at the source keeps the math
 * correct with arbitrary gaps.
 *
 * `slot` is the config slot index (== ButtonConfig::m_logicButtonPtrList index);
 * moveButton() operates on those, so dropTargetSlot returns a slot directly.
 */

#include <QVector>

namespace freejoy {

struct RowGeom {
    int slot;    /* config slot index of this row */
    int top;     /* y of the row's top edge, in the contents-widget coords */
    int height;
};

/* The config slot to drop onto: the first visible row whose vertical midline is
 * below the cursor ("drop above row i's midline -> target slot i"). Cursor
 * below every row lands on the last visible slot. Empty -> -1. */
inline int dropTargetSlot(const QVector<RowGeom> &rows, int cursorY)
{
    if (rows.isEmpty()) return -1;
    for (const RowGeom &r : rows) {
        if (cursorY < r.top + r.height / 2) return r.slot;
    }
    return rows.last().slot;
}

/* Y (contents-widget coords) at which to draw the drop-indicator line -- the
 * top of the gap the row will land in. Above the first row's midline -> first
 * row's top; below the last row's midline -> last row's bottom; otherwise the
 * top of the row whose midline is just below the cursor. Empty -> 0. */
inline int dropIndicatorY(const QVector<RowGeom> &rows, int cursorY)
{
    if (rows.isEmpty()) return 0;
    const RowGeom &first = rows.first();
    const RowGeom &last  = rows.last();
    if (cursorY < first.top + first.height / 2) return first.top;
    if (cursorY > last.top + last.height / 2)   return last.top + last.height;
    for (int i = 1; i < rows.size(); ++i) {
        if (cursorY < rows[i].top + rows[i].height / 2) return rows[i].top;
    }
    return last.top + last.height;
}

/* True when the cursor is past the last visible row's midline -- the "tail"
 * drop that should land at the final slot. ButtonConfig uses this to skip the
 * moveButton removal-shift adjustment for downward drops at the very bottom. */
inline bool isBelowLastRow(const QVector<RowGeom> &rows, int cursorY)
{
    if (rows.isEmpty()) return false;
    const RowGeom &last = rows.last();
    return cursorY > last.top + last.height / 2;
}

} // namespace freejoy

#endif // REORDERMATH_H
