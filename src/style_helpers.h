#ifndef STYLE_HELPERS_H
#define STYLE_HELPERS_H

#include <QAbstractButton>
#include <QApplication>
#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QHash>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QSize>
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

// The Lucide "triangle-alert" icon rendered to a pixmap and tinted to `color`
// (the SVG draws with currentColor, which renders black, so we recolour it).
// Cached per (colour, size) so the amber warning and red danger variants can
// coexist.
inline QPixmap tintedTrianglePixmap(const QColor &color, int px = 14)
{
    static QHash<QString, QPixmap> cache;
    const QString key = color.name() + QLatin1Char('@') + QString::number(px);
    if (!cache.contains(key)) {
        QPixmap base = QIcon(QStringLiteral(":/Images/icons/lucide/triangle-alert.svg"))
                           .pixmap(px, px);
        QPixmap tinted(base.size());
        tinted.fill(Qt::transparent);
        QPainter p(&tinted);
        p.drawPixmap(0, 0, base);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(tinted.rect(), color);
        p.end();
        cache.insert(key, tinted);
    }
    return cache.value(key);
}

// An inline <img> tag (base64 PNG data URI) for the tinted triangle, so any
// rich-text QLabel can show the icon next to text via setText(). Cached per
// (colour, size).
inline QString triangleIconHtml(const QColor &color, int px = 14)
{
    static QHash<QString, QString> cache;
    const QString key = color.name() + QLatin1Char('@') + QString::number(px);
    if (!cache.contains(key)) {
        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        tintedTrianglePixmap(color, px).save(&buf, "PNG");
        // vertical-align:middle so the icon sits centered with the text on
        // its line rather than riding high on the baseline.
        cache.insert(key, QStringLiteral("<img style=\"vertical-align: middle;\" "
                                          "src=\"data:image/png;base64,%1\"/>")
                              .arg(QString::fromLatin1(ba.toBase64())));
    }
    return cache.value(key);
}

// Amber warning variants (the long-standing default).
inline QPixmap warningPixmap(int px = 14)    { return tintedTrianglePixmap(warningColor(), px); }
inline QString warningIconHtml(int px = 14)  { return triangleIconHtml(warningColor(), px); }

// Red "danger" triangle, for destructive-action warnings (erase / overwrite).
inline QString dangerIconHtml(int px = 14)   { return triangleIconHtml(accentRed(), px); }

// Shared "warning note" banner stylesheet: a light-yellow fill with a yellow
// (amber) border and amber ink, for confirmation/warning surfaces that need to
// catch the eye without the heavy solid-amber treatment. Translucent fill so it
// reads as pale yellow on the light theme and a soft amber tint on dark. Apply
// to a QLabel and prefix its text with warningIconHtml() for the leading icon,
// so every warning banner across the app looks identical.
// Shared alert-banner styling for full-width message bars (warning / danger /
// info). The legibility key: text uses palette(text) -- a high-contrast
// neutral ink that tracks the theme (light grey on dark, near-black on light)
// -- instead of same-hue coloured text on a coloured fill, which washed out.
// Severity is signalled by the accent-tinted fill, the solid accent border,
// and the matching coloured leading icon (pair with *IconHtml()). One look for
// every bar; only the hue differs.
inline QString alertBannerQss(const QColor &accent)
{
    return QStringLiteral("padding:8px 10px; border-radius:4px; "
                          "background-color:%1; border:1px solid %2; "
                          "color:palette(text);")
        .arg(rgbaStr(accent, 46), hexStr(accent));
}

// Amber caution bar.   Pair with warningIconHtml().
inline QString warningBannerQss() { return alertBannerQss(accentAmber()); }
// Red destructive bar. Pair with dangerIconHtml().
inline QString dangerBannerQss()  { return alertBannerQss(accentRed()); }
// Blue informational bar (theme-aware blue). Pair with a tinted info icon.
inline QString infoBannerQss()    { return alertBannerQss(accentBlue(true)); }

// Build a complete alert banner as a widget: a coloured, outlined box holding a
// tinted triangle icon and the message, laid out side-by-side and vertically
// centred. This is the alignment-safe replacement for the old "icon-as-inline-
// <img> inside one QLabel" pattern, where Qt's rich-text baseline handling left
// the icon riding above the text. Text uses the default (palette) label colour,
// so it stays legible and theme-tracking. Pass accentAmber()/accentRed()/
// accentBlue() for warning / danger / info.
inline QFrame *makeAlertBanner(const QColor &accent, const QString &text,
                               QWidget *parent = nullptr)
{
    auto *frame = new QFrame(parent);
    // Box chrome only (fill + border + radius); inner padding comes from the
    // layout margins so it can't double up with QSS padding. Scope the rule to
    // this frame via an objectName selector -- a bare `QFrame {}` selector also
    // matches QLabel (a QFrame subclass), which drew a second border around the
    // child text label.
    frame->setObjectName(QStringLiteral("fjAlertBanner"));
    frame->setStyleSheet(QStringLiteral(
        "QFrame#fjAlertBanner { border-radius:4px; background-color:%1; border:1px solid %2; }")
        .arg(rgbaStr(accent, 46), hexStr(accent)));

    auto *row = new QHBoxLayout(frame);
    row->setContentsMargins(10, 7, 10, 7);
    row->setSpacing(8);

    auto *icon = new QLabel(frame);
    icon->setPixmap(tintedTrianglePixmap(accent, 16));
    icon->setFixedSize(16, 16);
    icon->setScaledContents(true);

    auto *msg = new QLabel(text, frame);
    msg->setWordWrap(true);

    row->addWidget(icon, 0, Qt::AlignVCenter);
    row->addWidget(msg, 1);
    return frame;
}

// ----- Themed monochrome glyph icons ------------------------------------
//
// The Lucide SVGs draw with `currentColor`, which Qt's SVG renderer paints
// black regardless of palette -- fine on the light theme, invisible-ish on
// the dark one. The long-standing fix (see warningPixmap above and the
// per-widget pixmapToIcon()/coloringPixmap() copies in AxesCurves*) is to
// render the glyph and recolour every opaque pixel via CompositionMode_SourceIn.
//
// These helpers generalise that into one place plus a tiny opt-in registry:
// setThemedIcon() tags a widget with its source SVG, and retintThemedIcons()
// re-renders every tagged widget when the theme flips. Tint colour tracks
// QPalette::Text so it flips automatically -- near-black on light, light grey
// on dark -- matching the rest of the UI's text ink.

// Foreground ink for monochrome glyph icons (tracks the active palette).
inline QColor iconInk() { return QApplication::palette().color(QPalette::Text); }

// Render a monochrome (currentColor) SVG and tint every opaque pixel to
// `color`, preserving the glyph's alpha shape.
inline QPixmap tintedSvgPixmap(const QString &svgPath, const QSize &size, const QColor &color)
{
    QPixmap pm = QIcon(svgPath).pixmap(size);
    if (pm.isNull()) {
        return pm;
    }
    QPainter p(&pm);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(pm.rect(), color);
    p.end();
    return pm;
}

inline QIcon tintedSvgIcon(const QString &svgPath, const QSize &size, const QColor &color)
{
    return QIcon(tintedSvgPixmap(svgPath, size, color));
}

// Dynamic-property keys recording a widget's themed-icon source + render size,
// so retintThemedIcons() can re-render it after a theme change.
constexpr const char *kThemedIconSvgProp  = "fjThemedIconSvg";
constexpr const char *kThemedIconSizeProp = "fjThemedIconSize";

// Give a button a theme-tracking icon: tinted to iconInk() now, re-tinted by
// retintThemedIcons() on every theme flip. Render size follows the button's
// configured iconSize() (set in the .ui), falling back to 16x16.
inline void setThemedIcon(QAbstractButton *btn, const QString &svgPath)
{
    if (btn == nullptr) {
        return;
    }
    QSize sz = btn->iconSize();
    if (!sz.isValid() || sz.isEmpty()) {
        sz = QSize(16, 16);
    }
    btn->setProperty(kThemedIconSvgProp, svgPath);
    btn->setProperty(kThemedIconSizeProp, sz);
    btn->setIcon(tintedSvgIcon(svgPath, sz, iconInk()));
}

// QLabel variant for icon-as-pixmap headers / info badges.
inline void setThemedIcon(QLabel *label, const QString &svgPath, const QSize &size)
{
    if (label == nullptr) {
        return;
    }
    label->setProperty(kThemedIconSvgProp, svgPath);
    label->setProperty(kThemedIconSizeProp, size);
    label->setPixmap(tintedSvgPixmap(svgPath, size, iconInk()));
}

// Drop a widget's themed-icon tag (e.g. when an indicator icon is cleared) so
// retintThemedIcons() won't resurrect it.
inline void clearThemedIcon(QWidget *w)
{
    if (w == nullptr) {
        return;
    }
    w->setProperty(kThemedIconSvgProp, QVariant());
    w->setProperty(kThemedIconSizeProp, QVariant());
}

// Re-tint every themed icon in the application to the current palette ink.
// Uses qApp->allWidgets() so it reaches child tabs and free-standing dialogs
// alike -- no parent walking, no per-widget event filters.
inline void retintThemedIcons()
{
    const QColor ink = iconInk();
    const auto widgets = QApplication::allWidgets();
    for (QWidget *w : widgets) {
        const QVariant pathVar = w->property(kThemedIconSvgProp);
        if (!pathVar.isValid()) {
            continue;
        }
        const QString path = pathVar.toString();
        const QSize sz = w->property(kThemedIconSizeProp).toSize();
        if (auto *btn = qobject_cast<QAbstractButton *>(w)) {
            btn->setIcon(tintedSvgIcon(path, sz, ink));
        } else if (auto *lbl = qobject_cast<QLabel *>(w)) {
            lbl->setPixmap(tintedSvgPixmap(path, sz, ink));
        }
    }
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
