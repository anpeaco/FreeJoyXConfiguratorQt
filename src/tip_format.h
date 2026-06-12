#ifndef TIP_FORMAT_H
#define TIP_FORMAT_H

// Tooltip content formatting -- the single source of truth for how an
// instructional tooltip is rendered (bold heading + body, or a bullet list).
//
// Two layers:
//   * tipHtml(...)   -- builders that emit the canonical rich-text markup. Kept
//                       here (not in style_helpers.h) so the formatter below can
//                       share them and the whole unit stays Qt-Core-only.
//   * formatTip(raw) -- the runtime chokepoint (called from the tooltip engine):
//                       turns a plain, human-authored tooltip string -- the form
//                       now stored verbatim in the .ui files -- into that markup.
//
// Authoring convention for plain tooltips (.ui <string> or a setToolTip call):
//
//     Heading line                 (text before the first blank line = heading)
//     <blank line>
//     Body paragraph...            (a paragraph, OR)
//     - first bullet               (lines starting "- " => a bullet list)
//     - second bullet
//
//   * **word** marks inline bold.
//   * A string with NO blank line is a plain short label, rendered as-is.
//   * A string that already contains an HTML tag is passed through untouched, so
//     existing freejoy_style::tipHtml() call sites (which store HTML) keep
//     working and migration can be incremental.
//
// Because the format lives in exactly one place, restyling every tooltip is a
// change here -- not a sweep across ~17 .ui files. Pure / no widgets, so the
// formatter is unit-testable headlessly (see tests/test_tipformat).

#include <QChar>
#include <QLatin1Char>
#include <QLatin1String>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace freejoy_style {

// Instructional tooltip: a bold heading line followed by bulleted, action-first
// points (Qt renders it as rich text -- auto-detected from the markup). `heading`
// is escaped; each point may already carry inline markup (e.g. "<b>x</b>").
inline QString tipHtml(const QString &heading, const QStringList &points)
{
    // Heading on its own line with a small gap below (section breathing), then a
    // two-column table per bullet for a true hanging indent: the bullet sits in a
    // narrow first column and the text wraps within the second, so a wrapped line
    // aligns under the TEXT (not under the bullet). The bullet cell's right
    // padding is the gap to the text; the bottom padding lets the points breathe;
    // the table margin is the slight list indent.
    QString s = QStringLiteral("<div style=\"margin-bottom:11px;\"><b>%1</b></div>")
                    .arg(heading.toHtmlEscaped());
    s += QStringLiteral("<table cellspacing=\"0\" cellpadding=\"0\" style=\"margin-left:4px;\">");
    for (const QString &p : points) {
        s += QStringLiteral("<tr>"
                            "<td valign=\"top\" style=\"padding:0 8px 4px 0;\">&bull;</td>"
                            "<td style=\"padding:0 0 4px 0;\">%1</td>"
                            "</tr>").arg(p);
    }
    s += QStringLiteral("</table>");
    return s;
}

// Instructional tooltip with no discrete steps: a bold heading then a short
// descriptive line (no bullet). `heading` is escaped; `body` may carry markup.
inline QString tipHtml(const QString &heading, const QString &body)
{
    return QStringLiteral("<div style=\"margin-bottom:11px;\"><b>%1</b></div>%2")
        .arg(heading.toHtmlEscaped(), body);
}

// Escape plain text for rich-text display and apply the inline **bold** marker.
inline QString tipInlineMarkup(const QString &text)
{
    QString s = text.toHtmlEscaped();
    static const QRegularExpression bold(QStringLiteral("\\*\\*(.+?)\\*\\*"));
    s.replace(bold, QStringLiteral("<b>\\1</b>"));
    return s;
}

// True when the string already carries rich-text markup (an HTML tag), so it is
// shown verbatim rather than run through the plain-text formatter. A lone '<'
// followed by whitespace (e.g. "value < 5") is NOT treated as a tag.
inline bool tipLooksLikeHtml(const QString &s)
{
    const int i = s.indexOf(QLatin1Char('<'));
    if (i < 0 || i + 1 >= s.size()) {
        return false;
    }
    const QChar n = s.at(i + 1);
    return n.isLetter() || n == QLatin1Char('/');
}

// Turn an authored plain tooltip string into the canonical rich-text markup.
// HTML passes through untouched; see the convention documented at the top.
inline QString formatTip(const QString &raw)
{
    const QString t = raw.trimmed();
    if (t.isEmpty() || tipLooksLikeHtml(t)) {
        return raw;
    }

    const QStringList lines = t.split(QLatin1Char('\n'));
    int blank = -1;
    for (int i = 0; i < lines.size(); ++i) {
        if (lines.at(i).trimmed().isEmpty()) {
            blank = i;
            break;
        }
    }
    if (blank < 0) {                    // no heading/body split -> plain label
        return tipInlineMarkup(t);
    }

    QString heading;
    for (int i = 0; i < blank; ++i) {
        if (!heading.isEmpty()) {
            heading += QLatin1Char(' ');
        }
        heading += lines.at(i).trimmed();
    }

    QStringList body;
    for (int i = blank + 1; i < lines.size(); ++i) {
        const QString l = lines.at(i).trimmed();
        if (!l.isEmpty()) {
            body << l;
        }
    }
    if (body.isEmpty()) {               // heading only -> treat as a plain label
        return tipInlineMarkup(heading);
    }

    bool bullets = true;
    for (const QString &l : body) {
        if (!l.startsWith(QLatin1String("- "))) {
            bullets = false;
            break;
        }
    }
    if (bullets) {
        QStringList points;
        for (const QString &l : body) {
            points << tipInlineMarkup(l.mid(2).trimmed());
        }
        return tipHtml(heading, points);
    }

    QStringList parts;
    for (const QString &l : body) {
        parts << tipInlineMarkup(l);
    }
    return tipHtml(heading, parts.join(QLatin1Char(' ')));
}

} // namespace freejoy_style

#endif // TIP_FORMAT_H
