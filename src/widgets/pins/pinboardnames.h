#ifndef PINBOARDNAMES_H
#define PINBOARDNAMES_H

#include <QHash>
#include <QString>

#include "common_defines.h"

/* Per-board override layer for the user-facing pin display name.
 *
 * Internally the configurator addresses every pin by a stable
 * cross-board identifier (e.g. "B11" for slot 22 in m_axesPinList,
 * pincombobox::m_pinList, and the INI persistence key in configtofile).
 * That identifier corresponds to the F103 BluePill silkscreen, which
 * happens to be where the cross-board scheme started. On other boards
 * the same physical position has a different silkscreen label -- the
 * F411 BlackPill V3.x marks the slot as "B2" because PB11 isn't bonded
 * on the UFQFPN48 package (PB2 is bonded at the same physical
 * position), so showing "B11" to a BlackPill user is incorrect.
 *
 * This helper lets every display surface (axis source dropdowns,
 * tooltips, label text) translate the internal "B11" identifier to
 * the right per-board label. Identity surfaces (objectName, INI keys,
 * dev_config_t pin enums) keep using the raw internal name. */
inline QString pinDisplayName(int boardId, const QString &internalName)
{
    static const QHash<int, QHash<QString, QString>> kPerBoardOverrides = {
        { BOARD_ID_F411_BLACKPILL,
          { { QStringLiteral("B11"), QStringLiteral("B2") } } },
    };

    const auto boardIt = kPerBoardOverrides.constFind(boardId);
    if (boardIt == kPerBoardOverrides.constEnd()) {
        return internalName;
    }
    const auto pinIt = boardIt.value().constFind(internalName);
    if (pinIt == boardIt.value().constEnd()) {
        return internalName;
    }
    return pinIt.value();
}

#endif // PINBOARDNAMES_H
