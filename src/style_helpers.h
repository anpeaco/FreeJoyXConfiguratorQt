#ifndef STYLE_HELPERS_H
#define STYLE_HELPERS_H

#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QHash>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QStyle>
#include <QVariant>
#include <QWidget>

namespace freejoy_style {

// ----- Canonical accent palette -----------------------------------------
//
// Every accent colour the app uses is defined here exactly once. The QSS
// tokens (accentTokens, below) derive their hex from these accessors, and
// C++ painting / inline-stylesheet code calls the same accessors — so there
// is a single source of truth shared by both the stylesheet and the code.
//
// State accents (green/red/amber) are theme-independent; accentBlue() tracks
// QPalette::Highlight per theme.
inline QColor accentGreen()       { return QColor(0x22, 0xc5, 0x5e); } // active / success / connected
inline QColor accentGreenBorder() { return QColor(0x16, 0xa3, 0x4a); }
inline QColor accentRed()         { return QColor(0xef, 0x44, 0x44); } // error / disconnected
inline QColor accentRedBorder()   { return QColor(0xdc, 0x26, 0x26); }
inline QColor accentAmber()       { return QColor(0xf5, 0x9e, 0x0b); } // warning *fill* (white ink on top)
inline QColor accentAmberBorder() { return QColor(0xd9, 0x77, 0x06); }
inline QColor accentBlue(bool dark) { return dark ? QColor(0x26, 0x82, 0xe2) : QColor(0x21, 0x76, 0xd4); }

// Shared warning accent colour (amber). This is the *foreground / ink* amber,
// used to tint the warning icon and warning text on a window-coloured
// background, so it runs darker than accentAmber() (the fill amber painted
// behind white text). Same hue family, two roles — distinct for contrast.
inline QColor warningColor()      { return QColor(0xD0, 0x80, 0x10); }

// Distinct "conflict" red — runs darker than accentRed(); used for the
// duplicate-PID warning ink and the offending field's border in Advanced
// Settings. Kept separate so a hard conflict reads differently from a
// transient error flash.
inline QColor conflictColor()     { return QColor(0xCC, 0x33, 0x33); }

// Hyperlink blue for rich-text labels (about box, info notes).
inline QColor linkColor()         { return QColor(0x03, 0xA9, 0xF4); }

// Attention "pulse" highlight painted behind an input while it waits for an
// auto-assigned physical button. Brighter than palette(highlight) so it reads
// as transient; shared by the Axes source box and the logical Source spinbox.
inline QColor pulseHighlight()    { return QColor(0x40, 0x80, 0xff); }

// "#rrggbb" for QSS string building.
inline QString hexStr(const QColor &c) { return c.name(); }

// "rgba(r, g, b, a)" with 0-255 alpha, for translucent QSS fills.
inline QString rgbaStr(const QColor &c, int alpha)
{
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(c.red()).arg(c.green()).arg(c.blue()).arg(alpha);
}

// Reusable inline-stylesheet snippet: the auto-assign pulse fill applied to
// `widget` (e.g. "QComboBox", "QSpinBox"). One definition for every caller.
inline QString pulseFillQss(const QString &widget)
{
    return QStringLiteral("%1 { background-color: %2; }")
        .arg(widget, rgbaStr(pulseHighlight(), 110));
}

// ----- Semantic accent tokens -------------------------------------------
//
// QSS has no variables, so accent hex used to be copy-pasted across
// common.qss and the theme files — and had drifted (three different blues,
// two ambers). These tokens are the single source of truth; themeChanged()
// substitutes them into the concatenated stylesheet via applyAccentTokens()
// before applying it, so every %accent-...% reference resolves to one value.
//
// State accents (green/red/amber) are theme-independent. The blue accent
// matches QPalette::Highlight for the active theme so the translucent fills
// and the progress-bar gradient track the same hue the rest of the UI uses.
// Gradients are pre-baked stops because QSS cannot derive a lighter/darker
// shade from palette().
inline QHash<QString, QString> accentTokens(bool dark)
{
    QHash<QString, QString> t;

    // State accents — identical in both themes, derived from the canonical
    // accessors above so the QSS and C++ never drift apart.
    t.insert(QStringLiteral("%accent-green%"),        hexStr(accentGreen()));
    t.insert(QStringLiteral("%accent-green-border%"), hexStr(accentGreenBorder()));
    t.insert(QStringLiteral("%accent-red%"),          hexStr(accentRed()));
    t.insert(QStringLiteral("%accent-red-border%"),   hexStr(accentRedBorder()));
    t.insert(QStringLiteral("%accent-amber%"),        hexStr(accentAmber()));
    t.insert(QStringLiteral("%accent-amber-border%"), hexStr(accentAmberBorder()));

    // "Lit" state fills — ~18% lighter at the top, ~12% darker at the bottom
    // around the base accent. Subtle enough to stay understated, but applied
    // consistently to every state pill/button (green = active/connected,
    // red = error/disconnected, amber = warning/legacy) so they all match.
    auto fill = [](const QColor &c) {
        return QStringLiteral("qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                              "stop:0 %1, stop:1 %2)")
            .arg(hexStr(c.lighter(118)), hexStr(c.darker(112)));
    };
    t.insert(QStringLiteral("%green-fill%"), fill(accentGreen()));
    t.insert(QStringLiteral("%red-fill%"),   fill(accentRed()));
    t.insert(QStringLiteral("%amber-fill%"), fill(accentAmber()));

    // Blue accent matches QPalette::Highlight for the active theme so the
    // translucent fills and the progress-bar gradient track the same hue the
    // rest of the UI uses.
    const QColor blue = accentBlue(dark);
    t.insert(QStringLiteral("%accent-blue%"),      hexStr(blue));
    t.insert(QStringLiteral("%accent-blue-fill%"), rgbaStr(blue, 60));
    t.insert(QStringLiteral("%bar-fill%"),
             QStringLiteral("qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                            "stop:0 %1, stop:1 %2)")
                 .arg(hexStr(blue.lighter(118)), hexStr(blue)));
    return t;
}

// Substitute every accent token in `qss` with its theme-resolved value.
// Named-token replacement (not a blanket %...% regex) so literal '%' in
// comments such as "80% of colours" are left untouched.
inline QString applyAccentTokens(QString qss, bool dark)
{
    const QHash<QString, QString> tokens = accentTokens(dark);
    for (auto it = tokens.cbegin(); it != tokens.cend(); ++it) {
        qss.replace(it.key(), it.value());
    }
    return qss;
}

// The Lucide "triangle-alert" icon, rendered to a pixmap and tinted to the
// warning colour (the SVG draws with currentColor, which renders black, so
// we recolour it). Cached per size.
inline QPixmap warningPixmap(int px = 14)
{
    static QHash<int, QPixmap> cache;
    if (!cache.contains(px)) {
        QPixmap base = QIcon(QStringLiteral(":/Images/icons/lucide/triangle-alert.svg"))
                           .pixmap(px, px);
        QPixmap tinted(base.size());
        tinted.fill(Qt::transparent);
        QPainter p(&tinted);
        p.drawPixmap(0, 0, base);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(tinted.rect(), warningColor());
        p.end();
        cache.insert(px, tinted);
    }
    return cache.value(px);
}

// An inline <img> tag (base64 PNG data URI) for the warning icon, so any
// rich-text QLabel can show the icon next to text via setText(). Lets us use
// one consistent warning icon everywhere without a separate icon widget.
// Cached.
inline QString warningIconHtml(int px = 14)
{
    static QString cached;
    if (cached.isEmpty()) {
        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        warningPixmap(px).save(&buf, "PNG");
        // vertical-align:middle so the icon sits centered with the text on
        // its line rather than riding high on the baseline.
        cached = QStringLiteral("<img style=\"vertical-align: middle;\" "
                                "src=\"data:image/png;base64,%1\"/>")
                     .arg(QString::fromLatin1(ba.toBase64()));
    }
    return cached;
}

// Set a dynamic property on a widget and force a style refresh so the
// new value picks up matching QSS attribute-selector rules. Required
// because Qt only resolves [property="value"] selectors during a
// polish pass; changing the property at runtime needs an explicit
// unpolish/polish cycle.
inline void setRole(QWidget *w, const char *property, const QVariant &value)
{
    if (w == nullptr) {
        return;
    }
    w->setProperty(property, value);
    if (auto *s = w->style()) {
        s->unpolish(w);
        s->polish(w);
    }
    w->update();
}

// Clear a dynamic role so the widget reverts to its default QSS rules.
inline void clearRole(QWidget *w, const char *property)
{
    setRole(w, property, QVariant());
}

} // namespace freejoy_style

#endif // STYLE_HELPERS_H
