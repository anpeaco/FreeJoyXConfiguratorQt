#ifndef TOOLTIPPOSITIONER_H
#define TOOLTIPPOSITIONER_H

/* Application-wide tooltip with control-anchored placement.
 *
 * Qt's QToolTip anchors to the mouse cursor and, worse, re-clamps the position
 * against the rendered label width -- so near a screen edge it can fling the tip
 * far from its control. To get exact, control-relative placement we drive our
 * OWN frameless tooltip widget (FloatingTip) and position it ourselves:
 *
 *   - vertically: just BELOW the control (flip to ABOVE only if below overflows
 *     the window and above fits);
 *   - horizontally: LEFT edge aligned with the control, but if that overflows
 *     the window's right edge the tip is RIGHT-aligned to the control's right
 *     edge so it extends leftwards (a top-right checkbox shows its tip below it,
 *     growing left).
 *
 * Because we move() the real widget after adjustSize(), the size used for the
 * right-align is exact (no estimate) and nothing re-clamps it.
 *
 * Hidden on: leaving the control, any mouse/key/wheel input, window deactivate,
 * or a 10 s timeout -- mirroring QToolTip's dismissal. Installed once on qApp;
 * acts only on widgets whose toolTip() is non-empty (dynamic per-cell tips fall
 * through untouched).
 */

#include <QCheckBox>
#include <QColor>
#include <QCursor>
#include <QEvent>
#include <QLabel>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPoint>
#include <QPointer>
#include <QRadioButton>
#include <QRect>
#include <QSize>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWidget>

#include "tip_format.h"   // freejoy_style::formatTip -- plain-text -> canonical markup

namespace freejoy_ui {

// Frameless, non-focus-stealing tooltip surface. The window is translucent (for a
// soft drop shadow in the surrounding margin); the opaque themed box AND the
// shadow are painted directly in paintEvent -- not via QGraphicsDropShadowEffect,
// which on a translucent top-level fails to composite a child's themed background
// (the box came out see-through, with the shadow cast by the text glyphs). The
// child QLabel paints ONLY the text (transparent), on top of the box we draw. The
// box (excluding the shadow margin) is what anchors to a control.
class FloatingTip : public QWidget
{
public:
    static constexpr int kMargin = 34;        // shadow room around the box
    static constexpr int kPadH = 8;           // box interior padding (text inset)
    static constexpr int kPadV = 5;
    static constexpr int kMaxCardWidth = 300; // wrap only beyond this

    FloatingTip()
        : QWidget(nullptr, Qt::ToolTip | Qt::FramelessWindowHint)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        auto *lay = new QVBoxLayout(this);
        lay->setContentsMargins(kMargin + kPadH, kMargin + kPadV,
                                kMargin + kPadH, kMargin + kPadV);
        m_card = new QLabel(this);
        m_card->setTextFormat(Qt::RichText);
        m_card->setFont(QToolTip::font());
        // Text only -- the box fill/border is painted in paintEvent.
        m_card->setStyleSheet(QStringLiteral(
            "QLabel { color: palette(tool-tip-text); background: transparent; }"));
        lay->addWidget(m_card);
    }

    // Set the tip text and size the card smartly: render at the content's natural
    // (unwrapped) width when that fits within kMaxCardWidth -- so a short tip never
    // spills a single word onto a second line -- and only wrap once the content
    // genuinely exceeds the max width.
    void setTipText(const QString &t)
    {
        m_card->setWordWrap(false);
        m_card->setMinimumWidth(0);
        m_card->setMaximumWidth(QWIDGETSIZE_MAX);
        m_card->setText(t);
        m_card->adjustSize();
        if (m_card->width() > kMaxCardWidth) {
            m_card->setWordWrap(true);
            m_card->setFixedWidth(kMaxCardWidth);
            m_card->adjustSize();
        }
        adjustSize();   // widget = box + shadow margins
    }

    // The visible box (excludes the shadow margin) -- what anchors to the control.
    QSize cardSize() const { return QSize(width() - 2 * kMargin, height() - 2 * kMargin); }
    int   margin() const { return kMargin; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const QRectF box = QRectF(rect()).adjusted(kMargin, kMargin, -kMargin, -kMargin);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Soft drop shadow: layered translucent rounded rects, nudged down and
        // expanding outward so the overlap density fades to nothing at the edge.
        // Tuned (deeper + larger spread) to read like the native Windows dialog
        // shadow rather than a faint hint -- bigger blur, stronger near-edge
        // density, and a larger downward nudge. kMargin must stay >= blur + dy
        // so the bottom band isn't clipped.
        const int blur = 26;
        const qreal dy = 4.0;
        p.setPen(Qt::NoPen);
        for (int i = blur; i >= 1; --i) {
            QColor c(0, 0, 0);
            c.setAlphaF(0.32 / blur);
            p.setBrush(c);
            p.drawRoundedRect(box.adjusted(-i, -i + dy, i, i + dy), 5 + i, 5 + i);
        }

        // Opaque themed box.
        p.setPen(QPen(palette().color(QPalette::Mid), 1));
        p.setBrush(palette().color(QPalette::ToolTipBase));
        p.drawRoundedRect(box.adjusted(0.5, 0.5, -0.5, -0.5), 5, 5);
    }

private:
    QLabel *m_card = nullptr;
};

class TooltipPositioner : public QObject
{
    // No Q_OBJECT: overrides only the virtual eventFilter; the QTimer is wired
    // with a functor connection, which needs no moc.
public:
    using QObject::QObject;

    // The cutoff window: once a tooltip has shown, moving to another control
    // within this many ms shows that control's tip immediately; lapse it (the
    // pointer sits off all controls for longer) and the next tip waits the full
    // wake-up delay again.
    static constexpr int kGraceMs = 1500;

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        switch (ev->type()) {
        case QEvent::ToolTip: {
            // Full-delay path: Qt sends QEvent::ToolTip after its (consistent,
            // app-wide) wake-up delay. Show our tip for the control under it.
            QWidget *owner = findTipWidget(qobject_cast<QWidget *>(obj));
            if (owner) {
                showTipFor(owner);
                return true;   // consume so Qt's native tip doesn't also show
            }
            return false;
        }
        case QEvent::Enter: {
            // Grace path: while the engine is "awake" (a tip is up, or one was up
            // within the cutoff window), entering another control shows its tip
            // immediately -- no second wake-up delay.
            if (m_graceActive) {
                QWidget *owner = findTipWidget(qobject_cast<QWidget *>(obj));
                if (owner && owner != m_active.data()) {
                    showTipFor(owner);
                }
            }
            break;
        }
        case QEvent::MouseButtonPress:
        case QEvent::KeyPress:
        case QEvent::Wheel:
            dismiss(/*keepGrace=*/false);
            break;
        default:
            break;
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    // The widget that owns the tooltip under `w`: `w` itself, or its nearest
    // ancestor with a non-empty toolTip() (so a tip set on a container still
    // resolves). Stops at the window boundary.
    static QWidget *findTipWidget(QWidget *w)
    {
        for (QWidget *p = w; p != nullptr; p = p->parentWidget()) {
            if (!p->toolTip().isEmpty()) {
                return p;
            }
            if (p->isWindow()) {
                break;
            }
        }
        return nullptr;
    }

    void showTipFor(QWidget *w)
    {
        const QString tip = w->toolTip();
        QWidget *top = w->window();
        if (tip.isEmpty() || top == nullptr) {
            return;
        }
        if (m_tip == nullptr) {
            m_tip = new FloatingTip();
        }
        // Author tooltips as plain delimited text (see tip_format.h); formatTip
        // turns them into the canonical heading/body/bullet markup at display
        // time. Strings that are already HTML pass through untouched.
        m_tip->setTipText(freejoy_style::formatTip(tip));   // sizes the card (smart wrap)
        const QSize cs = m_tip->cardSize();  // the box, excluding shadow margin
        const int m = m_tip->margin();

        // Anchor rect = the control. For check boxes / radio buttons, anchor to
        // the indicator (the box itself), not the whole widget (which also spans
        // the label text) -- so the tip lines up with the box, not the caption.
        QRect wr(w->mapToGlobal(QPoint(0, 0)), w->size());
        if (qobject_cast<QCheckBox *>(w) || qobject_cast<QRadioButton *>(w)) {
            QStyleOptionButton opt;
            opt.initFrom(w);
            const QStyle::SubElement se = qobject_cast<QRadioButton *>(w)
                ? QStyle::SE_RadioButtonIndicator
                : QStyle::SE_CheckBoxIndicator;
            const QRect ind = w->style()->subElementRect(se, &opt, w);
            if (ind.isValid()) {
                wr = QRect(w->mapToGlobal(ind.topLeft()), ind.size());
            }
        }
        const QRect area = top->frameGeometry();                 // app window, global
        const int gap = 3;

        // Compute the CARD's top-left (the visible box). Horizontal: left edge with
        // the control; right-align (extend left) if a left-aligned card would
        // overflow the window's right edge.
        int x = wr.left();
        if (x + cs.width() > area.right()) {
            x = wr.right() - cs.width();
        }
        if (x < area.left()) {
            x = area.left();
        }
        // Vertical: below the control; flip above only if below overflows and
        // above fits.
        int y = wr.bottom() + gap;
        if (y + cs.height() > area.bottom() && (wr.top() - gap - cs.height()) >= area.top()) {
            y = wr.top() - gap - cs.height();
        }

        // The widget is larger than the card by `m` on every side (shadow room),
        // so offset the move so the CARD itself lands at (x, y).
        m_tip->move(x - m, y - m);
        m_tip->show();
        m_active = w;
        // Engine is awake while a tip is up; pause the grace countdown.
        m_graceActive = true;
        if (m_graceTimer) {
            m_graceTimer->stop();
        }

        if (m_hideTimer == nullptr) {
            m_hideTimer = new QTimer(this);
            m_hideTimer->setSingleShot(true);
            QObject::connect(m_hideTimer, &QTimer::timeout, this,
                             [this]() { dismiss(/*keepGrace=*/false); });
        }
        m_hideTimer->start(10000);

        // Poll the cursor so the tip reliably hides the moment the pointer moves
        // off the control -- more dependable than relying on a Leave event, and it
        // keeps the tip up while the cursor is anywhere over the control (incl.
        // child widgets of a compound control).
        if (m_watchTimer == nullptr) {
            m_watchTimer = new QTimer(this);
            m_watchTimer->setInterval(100);
            QObject::connect(m_watchTimer, &QTimer::timeout, this, [this]() {
                if (m_active.isNull() || !m_active->isVisible()
                    || !m_active->rect().contains(m_active->mapFromGlobal(QCursor::pos()))) {
                    dismiss(/*keepGrace=*/true);
                }
            });
        }
        m_watchTimer->start();
    }

    // Hide the tip. keepGrace=true keeps the engine "awake" for kGraceMs so the
    // next control's tip shows immediately; keepGrace=false (input / timeout)
    // resets, so the next tip waits the full wake-up delay.
    void dismiss(bool keepGrace)
    {
        if (m_tip) {
            m_tip->hide();
        }
        if (m_hideTimer) {
            m_hideTimer->stop();
        }
        if (m_watchTimer) {
            m_watchTimer->stop();
        }
        m_active = nullptr;

        if (keepGrace) {
            m_graceActive = true;
            if (m_graceTimer == nullptr) {
                m_graceTimer = new QTimer(this);
                m_graceTimer->setSingleShot(true);
                QObject::connect(m_graceTimer, &QTimer::timeout, this,
                                 [this]() { m_graceActive = false; });
            }
            m_graceTimer->start(kGraceMs);
        } else {
            m_graceActive = false;
            if (m_graceTimer) {
                m_graceTimer->stop();
            }
        }
    }

    FloatingTip       *m_tip = nullptr;
    QTimer            *m_hideTimer = nullptr;
    QTimer            *m_graceTimer = nullptr;   // counts down the grace window after a hide
    QTimer            *m_watchTimer = nullptr;   // polls cursor-over-control to hide on move-off
    bool               m_graceActive = false;    // engine awake -> next tip is immediate
    QPointer<QWidget>  m_active;
};

} // namespace freejoy_ui

#endif // TOOLTIPPOSITIONER_H
