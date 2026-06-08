#ifndef IMAGEASPECTLOCK_H
#define IMAGEASPECTLOCK_H

/* Keeps a pixmap QLabel's WIDTH tracking its HEIGHT at the pixmap's native
 * aspect ratio.
 *
 * The pin-config board image (label_ControllerImage) sits in a grid whose row
 * heights are driven by the pin dropdowns, and it uses scaledContents=true so
 * the board's pins line up with those rows. The side effect: when the window is
 * shrunk vertically the rows (and thus the image's height) shrink while the
 * column width stays put, so scaledContents stretches the board out of shape.
 *
 * This filter constrains the label's maximum width to height * aspect on every
 * resize, so the image column narrows in step with the shrinking rows -- the
 * board keeps its correct aspect ratio AND its pins stay aligned with the
 * dropdowns (it still fills the full row height). No layout loop: the image's
 * height is set by the vertical row layout, independent of its width, so
 * adjusting the width never feeds back into the height.
 */

#include <QEvent>
#include <QLabel>
#include <QObject>
#include <QPixmap>
#include <QtMath>

namespace freejoy_ui {

class ImageAspectLock : public QObject
{
    // No Q_OBJECT: we only override the virtual eventFilter (no signals/slots),
    // so this needs no moc and no .pro change.
public:
    explicit ImageAspectLock(QLabel *label)
        : QObject(label), m_label(label)
    {
        const QPixmap pm = label->pixmap(Qt::ReturnByValue);
        m_aspectW = pm.width();
        m_aspectH = pm.height();
        // Let it shrink below the pixmap's native width.
        label->setMinimumWidth(0);
        applyWidth();
    }

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        if (obj == m_label && ev->type() == QEvent::Resize) {
            applyWidth();
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    void applyWidth()
    {
        if (m_aspectH <= 0 || m_aspectW <= 0) {
            return;
        }
        const int target = qRound(m_label->height() * double(m_aspectW) / m_aspectH);
        if (target > 0 && m_label->maximumWidth() != target) {
            m_label->setMaximumWidth(target);
        }
    }

    QLabel *m_label;
    int m_aspectW = 0;
    int m_aspectH = 0;
};

// Install an aspect-ratio width lock on a pixmap label. Parented to the label,
// so it lives and dies with it.
inline void lockImageAspect(QLabel *label)
{
    if (label == nullptr) {
        return;
    }
    label->installEventFilter(new ImageAspectLock(label));
}

} // namespace freejoy_ui

#endif // IMAGEASPECTLOCK_H
