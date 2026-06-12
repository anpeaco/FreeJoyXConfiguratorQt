/* Unit tests for the pure tooltip formatter in src/tip_format.h:
 * formatTip() (plain authored text -> canonical rich-text markup),
 * tipLooksLikeHtml() (the HTML-passthrough gate) and the inline **bold** marker.
 * Header-only, Qt-Core only -- no GUI / widgets. */

#include <QtTest>

#include "tip_format.h"

using namespace freejoy_style;

class TestTipFormat : public QObject
{
    Q_OBJECT

private slots:
    /* ---- passthrough: anything already HTML is returned verbatim ---- */
    void html_divPassesThrough()
    {
        // A string built by the canonical builder must survive a round-trip
        // unchanged (existing freejoy_style::tipHtml() call sites store HTML).
        const QString built = tipHtml(QStringLiteral("Heading"),
                                      QStringLiteral("Body text."));
        QCOMPARE(formatTip(built), built);
    }
    void html_inlineTagPassesThrough()
    {
        const QString s = QStringLiteral("<b>Bold</b> then plain");
        QCOMPARE(formatTip(s), s);
    }
    void html_legacyHtmlWrapperPassesThrough()
    {
        const QString s = QStringLiteral("<html><head/><body><p>Hi</p></body></html>");
        QCOMPARE(formatTip(s), s);
    }

    /* ---- plain label: no blank line -> rendered as-is (escaped) ---- */
    void plain_shortLabelUnchanged()
    {
        QCOMPARE(formatTip(QStringLiteral("Drag to reorder")),
                 QStringLiteral("Drag to reorder"));
    }
    void plain_emptyStaysEmpty()
    {
        QCOMPARE(formatTip(QString()), QString());
    }
    void plain_lessThanIsNotATag()
    {
        // "value < 5" must NOT be mistaken for HTML; it is escaped as plain text.
        QVERIFY(!tipLooksLikeHtml(QStringLiteral("value < 5")));
        QCOMPARE(formatTip(QStringLiteral("value < 5")),
                 QStringLiteral("value &lt; 5"));
    }

    /* ---- heading + body ---- */
    void headingBody_matchesBuilder()
    {
        const QString in = QStringLiteral(
            "Shift trigger\n\nLogical button that holds this shift active.");
        const QString want = tipHtml(QStringLiteral("Shift trigger"),
            QStringLiteral("Logical button that holds this shift active."));
        QCOMPARE(formatTip(in), want);
    }
    void headingBody_ampersandEscaped()
    {
        const QString out = formatTip(QStringLiteral("Reset\n\nPins & axes & buttons."));
        QVERIFY(out.contains(QStringLiteral("Pins &amp; axes &amp; buttons.")));
    }
    void headingBody_inlineBold()
    {
        const QString out = formatTip(QStringLiteral(
            "Reset\n\nClick **Write Config** to apply."));
        QVERIFY(out.contains(QStringLiteral("<b>Write Config</b>")));
    }

    /* ---- heading + bullet list ---- */
    void headingBullets_matchesBuilder()
    {
        const QString in = QStringLiteral(
            "Polling interval\n\n"
            "- A low value will not improve the operation of the buttons.\n"
            "- Increasing it may improve encoder operation.");
        const QString want = tipHtml(QStringLiteral("Polling interval"),
            QStringList{
                QStringLiteral("A low value will not improve the operation of the buttons."),
                QStringLiteral("Increasing it may improve encoder operation.") });
        QCOMPARE(formatTip(in), want);
    }
    void headingBullets_inlineBoldInPoint()
    {
        const QString in = QStringLiteral(
            "Reset\n\n- The change is in-memory only -- click **Write Config**.");
        const QString out = formatTip(in);
        QVERIFY(out.contains(QStringLiteral("<b>Write Config</b>")));
        QVERIFY(out.contains(QStringLiteral("&bull;")));   // rendered as a bullet row
    }

    /* ---- mixed body (some bullets, some not) falls back to a paragraph ---- */
    void mixedBody_isParagraphNotBullets()
    {
        const QString in = QStringLiteral("Heading\n\nIntro line.\n- looks like a bullet");
        const QString out = formatTip(in);
        QVERIFY(!out.contains(QStringLiteral("<table")));   // not treated as a list
        QVERIFY(out.contains(QStringLiteral("<div style=\"margin-bottom:11px;\"><b>Heading</b>")));
    }
};

QTEST_APPLESS_MAIN(TestTipFormat)
#include "test_tipformat.moc"
