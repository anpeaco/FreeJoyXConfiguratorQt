#ifndef STYLE_HELPERS_H
#define STYLE_HELPERS_H

#include <QAbstractButton>
#include <QApplication>
#include <QBoxLayout>
#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QHash>
#include <QIcon>
#include <QLabel>
#include <QList>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPixmap>
#include <QRectF>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QStyle>
#include <QToolButton>
#include <QVariant>
#include <QWidget>
#include <algorithm>

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
// Subtle "active row" wash. accentGreen at ~18% alpha — composites over the
// row's window colour as a faint mint (light) / green-grey (dark) tint. This is
// the same translucency the app's alert banners use (alertBannerQss), so the
// active row reads at the app's standard accent weight. Painted on
// QPalette::Window with autoFillBackground so it shows only in the gutters
// between the row's child widgets: a peripheral "this row" cue that pairs with
// the crisp buttonState pip on the row-number label. Far quieter than the
// full-saturation flood it replaces.
inline QColor accentGreenWash()   { return QColor(0x22, 0xc5, 0x5e, 46); }
inline QColor accentRed()         { return QColor(0xef, 0x44, 0x44); } // error / disconnected
inline QColor accentRedBorder()   { return QColor(0xdc, 0x26, 0x26); }
inline QColor accentAmber()       { return QColor(0xf5, 0x9e, 0x0b); } // warning *fill* (white ink on top)
inline QColor accentAmberBorder() { return QColor(0xd9, 0x77, 0x06); }
inline QColor accentBlue(bool dark) { return dark ? QColor(0x26, 0x82, 0xe2) : QColor(0x21, 0x76, 0xd4); }

// ----- Status-box colour style guide ------------------------------------
//
// Every coloured message box / pill / banner in the app picks one of three
// state accents. The colour answers a single question: "what does the user
// need to feel about this?" -- not how severe the wording is. Apply
// consistently so a glance at the hue conveys outcome before the text is read.
//
//   GREEN  (accentGreen)  -- Safe / success / no data loss.
//                            The action completes cleanly and nothing the
//                            user configured is lost. Examples: device
//                            connected; "Same generation -- configuration
//                            preserved"; "Upgrade -- configuration migrated";
//                            a Read/Write that succeeded.
//
//   AMBER  (accentAmber)  -- Caution / proceed-with-consequence.
//                            The action is allowed and will work, but the
//                            user loses or must re-do something, or must take
//                            care during it. Reversible or recoverable.
//                            Examples: factory-reset on flash (config saved to
//                            disk first); downgrade / no-migrator; recovery
//                            flash; "do not unplug" process notes.
//
//   RED    (accentRed)    -- Blocked / error / destructive-and-irreversible.
//                            The action is refused, has failed, or will
//                            irrecoverably destroy state. Examples:
//                            incompatible board (flash refused); a failed
//                            Read/Write; erase-and-reinstall warnings; the
//                            project-crossing (FreeJoy->FreeJoyX) banner.
//
// BLUE (accentBlue) is reserved for neutral, non-state UI accents (selection,
// drop targets, progress) -- never a success/caution/danger signal.
//
// All three render with the SAME translucent treatment (alertBannerQss):
// accent fill at ~18% alpha, a solid 1px accent border, neutral palette(text)
// ink, and a leading accent-tinted icon (check for green, triangle for
// amber/red). Only the hue differs -- see makeAlertBanner / alertBannerQss.

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

    // Blue accent for the QSS tokens. Deliberately theme-INDEPENDENT: the
    // applied stylesheet must be identical for both themes so it can be applied
    // once (a theme swap is then just a cheap palette change, not a multi-second
    // full-tree QSS repolish -- see MainWindow::themeChanged). The two themes'
    // highlight blues are nearly identical, so one fixed blue reads correctly on
    // both; per-theme blue still flows through the palette for palette()-driven
    // rules. `dark` is intentionally unused here.
    (void)dark;
    const QColor blue = accentBlue(true);
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
// tinted triangle icon and the message, laid out side-by-side. The icon is
// top-aligned so on a multi-line banner (heading + list + note) it rides next
// to the heading rather than floating in the vertical middle of the block; on a
// single-line banner top and centre coincide. This is the alignment-safe
// replacement for the old "icon-as-inline-
// <img> inside one QLabel" pattern, where Qt's rich-text baseline handling left
// the icon riding above the text. Text uses the default (palette) label colour,
// so it stays legible and theme-tracking. Pass accentAmber()/accentRed()/
// accentBlue() for warning / danger / info.
inline int alertSeverityRank(const QColor &accent);   // defined just below

inline QFrame *makeAlertBanner(const QColor &accent, const QString &text,
                               QWidget *parent = nullptr,
                               const QPixmap &leadingIcon = QPixmap())
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
    // Hug content vertically -- don't let a parent layout stretch the box taller
    // than its text (which would strand the top-pinned icon above the text).
    frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    auto *row = new QHBoxLayout(frame);
    row->setContentsMargins(10, 7, 10, 7);
    row->setSpacing(8);

    auto *icon = new QLabel(frame);
    icon->setPixmap(leadingIcon.isNull() ? tintedTrianglePixmap(accent, 18) : leadingIcon);
    // Native pixmap (no scaledContents/fixedSize squash) + a 2px top margin so
    // the glyph drops onto the text's cap line rather than riding above it.
    icon->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    icon->setContentsMargins(0, 2, 0, 0);

    auto *msg = new QLabel(text, frame);
    msg->setWordWrap(true);

    // Both icon and text top-aligned: the icon rides at the top of the box and
    // the text starts on the same line beside it. Without AlignTop on the text
    // it would vertically-centre and float below the top-pinned icon whenever
    // the box is taller than one line.
    row->addWidget(icon, 0, Qt::AlignTop);
    row->addWidget(msg, 1, Qt::AlignTop);
    // Tag the banner's severity so a stack of banners can be ordered red ->
    // amber -> green (see restackBanners).
    frame->setProperty("fjAlertRank", alertSeverityRank(accent));
    return frame;
}

// Severity rank for ordering a stack of banners at the top of a dialog:
// red (0) is most urgent, then amber (1), then green (2); anything else
// (e.g. info blue) sorts last. Used by restackBanners().
inline int alertSeverityRank(const QColor &accent)
{
    if (accent == accentRed())   return 0;
    if (accent == accentAmber()) return 1;
    if (accent == accentGreen()) return 2;
    return 3;
}

// Re-stack the managed `banners` inside `area` (a vertical layout pinned to the
// top of a dialog) ordered by severity: red, then amber, then green. Null and
// hidden (setVisible(false)) banners are dropped from the layout; order is
// stable within a rank. Call after any banner is created / shown / hidden /
// recoloured so the dialog's top message area stays correctly ordered.
inline void restackBanners(QBoxLayout *area, QList<QWidget *> banners)
{
    if (!area) return;
    // Pull every managed banner out first so re-adding lands them in order.
    for (QWidget *b : banners) {
        if (b) area->removeWidget(b);
    }
    std::stable_sort(banners.begin(), banners.end(), [](QWidget *a, QWidget *b) {
        const int ra = a ? a->property("fjAlertRank").toInt() : 99;
        const int rb = b ? b->property("fjAlertRank").toInt() : 99;
        return ra < rb;
    });
    for (QWidget *b : banners) {
        if (b && !b->isHidden()) {
            area->addWidget(b);
            // Adding a child to an ALREADY-VISIBLE parent does not auto-show it
            // (Qt only shows layout children on the parent's own show()), so a
            // banner created while the dialog is open would stay invisible.
            // Show it explicitly; harmless before the dialog is first shown.
            b->show();
        }
    }
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

// A small rounded-square colour chip for list/legend indicators (e.g. the
// board marker in the firmware dropdown). Filled with `fill` and outlined with
// `border` (1px) so it reads on either theme; pass a transparent fill for a
// hollow "unknown" chip.
inline QPixmap colorChipPixmap(const QColor &fill, const QColor &border, int px = 12)
{
    QPixmap pm(px, px);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setBrush(fill);
    QPen pen(border);
    pen.setWidthF(1.0);
    p.setPen(pen);
    p.drawRoundedRect(QRectF(0.5, 0.5, px - 1.0, px - 1.0), 2.5, 2.5);
    p.end();
    return pm;
}

// Inline <img> (base64 PNG) for any bundled monochrome lucide glyph, tinted to
// `color`. Generalises triangleIconHtml() so a status box can pick a
// severity-appropriate leading icon (a check for a green/success box, the
// triangle for amber caution / red danger). Cached per (path, colour, size).
inline QString svgIconHtml(const QString &svgPath, const QColor &color, int px = 14,
                           const QString &vAlign = QStringLiteral("middle"))
{
    static QHash<QString, QString> cache;
    const QString key = svgPath + QLatin1Char('|') + color.name()
                        + QLatin1Char('@') + QString::number(px)
                        + QLatin1Char('/') + vAlign;
    if (!cache.contains(key)) {
        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        tintedSvgPixmap(svgPath, QSize(px, px), color).save(&buf, "PNG");
        cache.insert(key, QStringLiteral("<img style=\"vertical-align: %1;\" "
                                          "src=\"data:image/png;base64,%2\"/>")
                              .arg(vAlign, QString::fromLatin1(ba.toBase64())));
    }
    return cache.value(key);
}

// Green "success" check, tinted to `color`. Pair with a green status box.
inline QString checkIconHtml(const QColor &color, int px = 14)
{
    return svgIconHtml(QStringLiteral(":/Images/icons/lucide/check-dark.svg"), color, px);
}

// Pick the leading icon for a status box from its accent: a check for the
// green/success hue, the alert triangle for amber caution and red danger.
// Keeps the green/amber/red style guide (above) in one decision.
inline QString statusIconHtml(const QColor &accent, int px = 14)
{
    return (accent == accentGreen()) ? checkIconHtml(accent, px)
                                     : triangleIconHtml(accent, px);
}

// Pixmap form of the status icon (check for green, triangle otherwise), tinted
// to `accent`. For makeAlertBanner's leadingIcon so a status banner picks the
// right glyph while sharing the banner's icon/text alignment.
inline QPixmap statusPixmap(const QColor &accent, int px = 18)
{
    return (accent == accentGreen())
        ? tintedSvgPixmap(QStringLiteral(":/Images/icons/lucide/check-dark.svg"),
                          QSize(px, px), accent)
        : tintedTrianglePixmap(accent, px);
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

// Forward declaration: the QLabel overload of setThemedIcon is defined below,
// but configureSectionToggle (which tints its chevron QLabel) needs it visible.
inline void setThemedIcon(QLabel *label, const QString &svgPath, const QSize &size);

// Configure a QToolButton as a collapsible-section toggle with the app-standard
// look: a themed gear icon, the section label, and a trailing lucide chevron
// that flips chevron-right (collapsed) -> chevron-down (expanded). Used so every
// "Extended Settings"-style disclosure looks identical (gear, label, chevron)
// and matches the combo drop-arrows. The caller wires the actual show/hide to
// the button's toggled(bool); this only keeps the icon in sync.
//
// QToolButton has a single icon slot (the gear), so the trailing chevron is a
// small right-aligned QLabel overlay added via a layout on the button. The
// label text carries trailing spaces to reserve room beneath the chevron.
inline void configureSectionToggle(QToolButton *btn, const QString &label)
{
    if (btn == nullptr) {
        return;
    }
    btn->setCheckable(true);
    btn->setAutoRaise(true);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btn->setIconSize(QSize(16, 16));
    setThemedIcon(btn, QStringLiteral(":/Images/icons/lucide/settings.svg"));   // leading gear
    btn->setText(label + QStringLiteral("      "));   // reserve room for the chevron

    QLabel *chevron = btn->findChild<QLabel *>(QStringLiteral("fjSectionChevron"));
    if (chevron == nullptr) {
        auto *lay = new QHBoxLayout(btn);
        lay->setContentsMargins(0, 0, 8, 0);
        lay->addStretch(1);
        chevron = new QLabel(btn);
        chevron->setObjectName(QStringLiteral("fjSectionChevron"));
        chevron->setAttribute(Qt::WA_TransparentForMouseEvents);
        lay->addWidget(chevron, 0, Qt::AlignVCenter);
    }
    auto applyChevron = [chevron](bool on) {
        setThemedIcon(chevron,
                      on ? QStringLiteral(":/Images/icons/lucide/chevron-down-neutral.svg")
                         : QStringLiteral(":/Images/icons/lucide/chevron-right-neutral.svg"),
                      QSize(16, 16));
    };
    applyChevron(btn->isChecked());
    QObject::connect(btn, &QToolButton::toggled, btn, [applyChevron](bool on) {
        applyChevron(on);
    });
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
