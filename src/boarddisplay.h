#ifndef BOARDDISPLAY_H
#define BOARDDISPLAY_H

/* Shared board-identity presentation: a CPU glyph tinted to the board's colour
 * (blue = F103 Blue Pill, theme-ink = F411 Black Pill, grey = unknown) followed
 * by "F103 (Blue Pill)" / "F411 (Black Pill)" text. Used everywhere a board is
 * shown -- the sidebar Device card, the Upgrade Firmware dialog's Device and
 * Firmware panes -- so they read identically.
 *
 * text()/color() are the plain pieces; html() is the rich-text icon+text for a
 * QLabel (set the label's textFormat to RichText). Note html() bakes the
 * current theme ink for the F411 colour, so a persistent surface (the device
 * card) must re-render on a theme change; a modal dialog needn't bother. */

#include <QColor>
#include <QString>

#include "common_defines.h"
#include "style_helpers.h"

namespace board_display {

inline QString text(int boardId)
{
    switch (boardId) {
        case BOARD_ID_F103_BLUEPILL:  return QStringLiteral("F103 (Blue Pill)");
        case BOARD_ID_F411_BLACKPILL: return QStringLiteral("F411 (Black Pill)");
        default:                      return QString::fromUtf8("\xE2\x80\x94"); // em dash
    }
}

inline QColor color(int boardId)
{
    switch (boardId) {
        // Blue Pill uses the canonical accent blue (single source of truth in
        // freejoy_style) rather than a copy of its hex.
        case BOARD_ID_F103_BLUEPILL:  return freejoy_style::accentBlue(true);
        case BOARD_ID_F411_BLACKPILL: return freejoy_style::iconInk();
        default:                      return QColor(0x90, 0x90, 0x90);
    }
}

// Rich-text "CPU icon + board name" for a QLabel. The icon is omitted for an
// unknown board (just the em dash).
inline QString html(int boardId, int px = 14)
{
    if (boardId != BOARD_ID_F103_BLUEPILL && boardId != BOARD_ID_F411_BLACKPILL) {
        return text(boardId);
    }
    /* text-bottom centres the CPU glyph on the text's cap region; the default
     * "middle" rides slightly high for this icon at small sizes. */
    return freejoy_style::svgIconHtml(QStringLiteral(":/Images/icons/lucide/cpu.svg"),
                                      color(boardId), px, QStringLiteral("middle"))
         + QStringLiteral("&nbsp;") + text(boardId);
}

} // namespace board_display

#endif // BOARDDISPLAY_H
