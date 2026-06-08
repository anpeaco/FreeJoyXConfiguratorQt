#ifndef LOADINGOVERLAY_H
#define LOADINGOVERLAY_H

/* A modal loading mask: a dimmed overlay with a centred spinner + message that
 * covers its parent and swallows mouse/key input, so the UI is locked while a
 * long operation runs (selecting a device + auto-loading config, reading,
 * writing, ...).
 *
 * Parent it to the window you want to cover; it tracks the parent's size and
 * paints over everything. start(message) shows + spins; stop() hides. A safety
 * timer force-hides it after a ceiling so a missed completion signal can never
 * leave the UI permanently locked.
 *
 * No Q_OBJECT: it only overrides virtuals + uses functor connections (no moc).
 */

#include <QColor>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QResizeEvent>
#include <QString>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>

namespace freejoy_ui {

class LoadingOverlay : public QWidget
{
public:
    explicit LoadingOverlay(QWidget *parent)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_NoSystemBackground);
        setFocusPolicy(Qt::StrongFocus);   // hold focus so keystrokes don't reach the UI

        m_spin.setInterval(33);            // ~30 fps
        QObject::connect(&m_spin, &QTimer::timeout, this, [this]() {
            m_angle = (m_angle + 10) % 360;
            update();
        });
        m_safety.setSingleShot(true);
        QObject::connect(&m_safety, &QTimer::timeout, this, [this]() { stop(); });

        if (parent) {
            parent->installEventFilter(this);
        }
        hide();
    }

    // Show the mask over the parent with `message`. The mask self-hides after
    // safetyMs (0 = no ceiling) as a backstop against a missed completion.
    void start(const QString &message, int safetyMs = 20000)
    {
        m_message = message;
        coverParent();
        raise();
        show();
        setFocus();
        m_spin.start();
        if (safetyMs > 0) {
            m_safety.start(safetyMs);
        }
    }

    void stop()
    {
        m_spin.stop();
        m_safety.stop();
        hide();
    }

protected:
    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (o == parent() && e->type() == QEvent::Resize) {
            coverParent();
        }
        return QWidget::eventFilter(o, e);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Dim wash over everything beneath.
        p.fillRect(rect(), QColor(0, 0, 0, 110));

        const qreal r = 20;     // spinner radius
        const int gap = 16;     // spinner -> text gap
        const int padX = 30;    // card padding
        const int padY = 26;

        QFont f = font();
        f.setPointSizeF(f.pointSizeF() + 1.0);
        const QFontMetrics fm(f);
        const bool hasText = !m_message.isEmpty();
        const int textW = hasText ? fm.horizontalAdvance(m_message) : 0;
        const int textH = hasText ? fm.height() : 0;

        // Opaque card sized to the spinner + text + padding, centred.
        const int contentW = qMax(int(2 * r), textW);
        const int contentH = int(2 * r) + (hasText ? gap + textH : 0);
        const qreal cardW = contentW + 2 * padX;
        const qreal cardH = contentH + 2 * padY;
        const QRectF card((width() - cardW) / 2.0, (height() - cardH) / 2.0, cardW, cardH);
        p.setPen(QPen(palette().color(QPalette::Mid), 1));
        p.setBrush(palette().color(QPalette::Window));   // fully opaque panel
        p.drawRoundedRect(card, 10, 10);

        // Spinner: faint full ring + a brighter rotating accent arc, near the top
        // of the card's content area.
        const QPointF sc(card.center().x(), card.top() + padY + r);
        const QRectF ring(sc.x() - r, sc.y() - r, 2 * r, 2 * r);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(palette().color(QPalette::Mid), 4));
        p.drawEllipse(ring);
        p.setPen(QPen(palette().color(QPalette::Highlight), 4, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(ring, -m_angle * 16, 100 * 16);

        // Message on the opaque card.
        if (hasText) {
            p.setPen(palette().color(QPalette::WindowText));
            p.setFont(f);
            p.drawText(QRectF(card.left(), sc.y() + r + gap, cardW, textH),
                       Qt::AlignHCenter | Qt::AlignTop, m_message);
        }
    }

    // Swallow input so the locked UI underneath can't be touched.
    void mousePressEvent(QMouseEvent *e) override { e->accept(); }
    void mouseReleaseEvent(QMouseEvent *e) override { e->accept(); }
    void mouseDoubleClickEvent(QMouseEvent *e) override { e->accept(); }
    void keyPressEvent(QKeyEvent *e) override { e->accept(); }
    void wheelEvent(QWheelEvent *e) override { e->accept(); }

private:
    void coverParent()
    {
        if (QWidget *pw = parentWidget()) {
            setGeometry(pw->rect());
        }
    }

    QTimer  m_spin;
    QTimer  m_safety;
    int     m_angle = 0;
    QString m_message;
};

} // namespace freejoy_ui

#endif // LOADINGOVERLAY_H
