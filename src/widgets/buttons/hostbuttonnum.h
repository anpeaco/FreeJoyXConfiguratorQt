#ifndef HOSTBUTTONNUM_H
#define HOSTBUTTONNUM_H

/*
 * Host-side replica of the firmware's joystick-button counting, so the
 * logical-buttons UI can show the HID button number the host (game / Windows)
 * actually sees -- NOT the raw dev_config.buttons[] slot index.
 *
 * The firmware compacts button slots into contiguous HID buttons at report
 * time:
 *   - A slot occupies a button position iff  physical_num >= 0 && it is not a
 *     POV-hat direction (FreeJoyX application/Src/config.c -- buttons_cnt;
 *     application/Src/buttons.c -- the output bitmap is packed with a separate
 *     counter that advances for every such slot).
 *   - POV directional slots (UP/DOWN/LEFT/RIGHT) feed a POV hat, not the button
 *     bitmap, so they do NOT count. POV*_CENTER is reported as an ordinary
 *     button, so it DOES count -- replicated here exactly.
 *   - MUTED slots (is_disabled, the eye toggle) STILL occupy their button
 *     position -- they report as always-off but keep their number, so muting
 *     one button does not renumber the others. is_disabled therefore does NOT
 *     affect the host number and is not an input here.
 *
 * Pure, header-only, no Qt-widget dependency, so it is unit-testable in
 * isolation (tests/test_hostbuttonnum.cpp). (ConfiguratorQt#103)
 */

#include <QVector>
#include "common_types.h"   // button_type_t + POV*/etc. enum values

namespace freejoy {

/* The directional POV types the firmware converts into a POV hat (and marks
 * is_disabled), so they are not reported as ordinary buttons. POV*_CENTER is
 * deliberately NOT in this set -- config.c does not exclude it. */
inline bool isDirectionalPov(button_type_t t)
{
    switch (t) {
    case POV1_UP: case POV1_DOWN: case POV1_LEFT: case POV1_RIGHT:
    case POV2_UP: case POV2_DOWN: case POV2_LEFT: case POV2_RIGHT:
    case POV3_UP: case POV3_DOWN: case POV3_LEFT: case POV3_RIGHT:
    case POV4_UP: case POV4_DOWN: case POV4_LEFT: case POV4_RIGHT:
        return true;
    default:
        return false;
    }
}

/* One logical-button slot's report-relevant state. (Muting / is_disabled is
 * deliberately absent -- a muted slot still occupies its button position.) */
struct ReportSlot {
    bool          bound;     /* physical_num >= 0 */
    button_type_t type;
};

/* Whether a slot occupies a HID joystick button position. Bound and not a
 * POV-hat direction. Muted slots still occupy a position (they just report
 * 0), so muting does not change anyone's number. */
inline bool isReportedButton(const ReportSlot &s)
{
    return s.bound && !isDirectionalPov(s.type);
}

/* For each slot, the 1-based HID button number the host will see, or 0 if the
 * slot is not reported. Counting is in slot order, exactly like the firmware
 * packs the output bitmap. */
inline QVector<int> computeReportNumbers(const QVector<ReportSlot> &rows)
{
    QVector<int> out;
    out.reserve(rows.size());
    int n = 0;
    for (const ReportSlot &s : rows) {
        out.append(isReportedButton(s) ? ++n : 0);
    }
    return out;
}

} // namespace freejoy

#endif // HOSTBUTTONNUM_H
