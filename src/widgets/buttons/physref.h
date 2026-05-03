#ifndef PHYSREF_H
#define PHYSREF_H

/* freejoy::physref -- canonical identity of a physical button input.
 *
 * The firmware enumerates physical buttons positionally:
 *
 *     [0 .. matrix-1]
 *         matrix buttons (row x column intersections)
 *     [matrix .. matrix + sumPerSR - 1]
 *         shift register inputs, by SR index then offset within SR
 *     [matrix + sumPerSR .. matrix + sumPerSR + sumPerA2b - 1]
 *         axis-to-buttons inputs, by axis index then offset within axis
 *     [matrix + sumPerSR + sumPerA2b .. total-1]
 *         direct GPIOs (BUTTON_GND / BUTTON_VCC), in pin index order
 *
 * The absolute number stored in dev_config_t.buttons[i].physical_num is
 * just "position N in this enumeration", which means the same number
 * shifts meaning whenever any earlier category is added/removed/resized.
 * PhysRef carries the stable identity -- (category, sub-index, offset)
 * -- and the toRef/toAbs free functions translate to/from the absolute
 * position under any given PhysBreakdown. The auto-remap snapshots the
 * breakdown on save (in dev_config_t.saved_breakdown) and uses these
 * helpers to translate references across breakdown changes.
 */

#include <QList>
#include <QString>
#include <numeric>

namespace freejoy {

enum class PhysCategory : int {
    Invalid = -1,
    Matrix = 0,
    ShiftReg,
    A2b,
    Direct,
};

struct PhysRef {
    PhysCategory cat = PhysCategory::Invalid;
    int subIdx = 0;     // SR index (0..MAX_SHIFT_REG_NUM-1) or axis index for A2b; 0 for Matrix/Direct
    int offset = 0;     // offset within the (sub-)group

    bool isValid() const { return cat != PhysCategory::Invalid; }
};

struct PhysBreakdown {
    int matrix = 0;
    QList<int> perSR;       // length = configured number of SR slots; trailing zeros allowed
    QList<int> perA2b;      // length = configured number of axes; trailing zeros allowed
    int direct = 0;

    bool operator==(const PhysBreakdown &other) const {
        return matrix == other.matrix && direct == other.direct
            && perSR == other.perSR && perA2b == other.perA2b;
    }
    bool operator!=(const PhysBreakdown &other) const { return !(*this == other); }

    bool isZero() const {
        if (matrix != 0 || direct != 0) return false;
        for (int n : perSR)  if (n != 0) return false;
        for (int n : perA2b) if (n != 0) return false;
        return true;
    }

    int totalSR()  const { return std::accumulate(perSR.begin(),  perSR.end(),  0); }
    int totalA2b() const { return std::accumulate(perA2b.begin(), perA2b.end(), 0); }
    int total()    const { return matrix + totalSR() + totalA2b() + direct; }
};

/* Classify an absolute physical_num under a breakdown into its stable
 * (category, sub-index, offset) identity. Returns PhysCategory::Invalid
 * when the number is out of range or negative. */
inline PhysRef toRef(int absolutePos, const PhysBreakdown &b)
{
    PhysRef r;
    if (absolutePos < 0) return r;

    int p = absolutePos;
    if (p < b.matrix) {
        r.cat = PhysCategory::Matrix;
        r.offset = p;
        return r;
    }
    p -= b.matrix;

    const int srSum = b.totalSR();
    if (p < srSum) {
        for (int i = 0; i < b.perSR.size(); ++i) {
            if (p < b.perSR[i]) {
                r.cat = PhysCategory::ShiftReg;
                r.subIdx = i;
                r.offset = p;
                return r;
            }
            p -= b.perSR[i];
        }
    }
    p = absolutePos - b.matrix - srSum;

    const int a2bSum = b.totalA2b();
    if (p < a2bSum) {
        for (int i = 0; i < b.perA2b.size(); ++i) {
            if (p < b.perA2b[i]) {
                r.cat = PhysCategory::A2b;
                r.subIdx = i;
                r.offset = p;
                return r;
            }
            p -= b.perA2b[i];
        }
    }
    p = absolutePos - b.matrix - srSum - a2bSum;

    if (p >= 0 && p < b.direct) {
        r.cat = PhysCategory::Direct;
        r.offset = p;
        return r;
    }
    return r;   // Invalid
}

/* Re-derive the absolute physical_num for a stable reference under a
 * (possibly different) breakdown. Returns -1 when the reference's
 * category / sub-index / offset no longer exists in the new breakdown
 * (e.g. SR removed entirely, SR shrunk past offset, direct count
 * reduced). The auto-remap surfaces these to the user as broken slots. */
inline int toAbs(const PhysRef &r, const PhysBreakdown &b)
{
    switch (r.cat) {
    case PhysCategory::Matrix:
        return r.offset < b.matrix ? r.offset : -1;
    case PhysCategory::ShiftReg: {
        if (r.subIdx < 0 || r.subIdx >= b.perSR.size()) return -1;
        if (r.offset < 0 || r.offset >= b.perSR[r.subIdx]) return -1;
        int base = b.matrix;
        for (int i = 0; i < r.subIdx; ++i) base += b.perSR[i];
        return base + r.offset;
    }
    case PhysCategory::A2b: {
        if (r.subIdx < 0 || r.subIdx >= b.perA2b.size()) return -1;
        if (r.offset < 0 || r.offset >= b.perA2b[r.subIdx]) return -1;
        int base = b.matrix + b.totalSR();
        for (int i = 0; i < r.subIdx; ++i) base += b.perA2b[i];
        return base + r.offset;
    }
    case PhysCategory::Direct:
        return (r.offset >= 0 && r.offset < b.direct)
               ? b.matrix + b.totalSR() + b.totalA2b() + r.offset
               : -1;
    case PhysCategory::Invalid:
        return -1;
    }
    return -1;
}

/* Human-readable label for a PhysRef -- "SR2 #8", "Direct #3", "Matrix #4",
 * "Axis 1 #5". Used for tooltips and category-qualified UI labels. */
inline QString refLabel(const PhysRef &r)
{
    switch (r.cat) {
    case PhysCategory::Matrix:
        return QStringLiteral("Matrix #%1").arg(r.offset + 1);
    case PhysCategory::ShiftReg:
        return QStringLiteral("SR%1 #%2").arg(r.subIdx + 1).arg(r.offset + 1);
    case PhysCategory::A2b:
        return QStringLiteral("Axis %1 #%2").arg(r.subIdx + 1).arg(r.offset + 1);
    case PhysCategory::Direct:
        return QStringLiteral("Direct #%1").arg(r.offset + 1);
    case PhysCategory::Invalid:
        return QStringLiteral("(invalid)");
    }
    return QStringLiteral("(invalid)");
}

} // namespace freejoy

#endif // PHYSREF_H
