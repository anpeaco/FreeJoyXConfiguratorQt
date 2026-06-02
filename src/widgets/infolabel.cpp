#include "infolabel.h"
#include "style_helpers.h"

InfoLabel::InfoLabel(QWidget *parent)
    : QLabel(parent)
{
    setMaximumSize(12, 12);
    setScaledContents(false);
    // Tint the monochrome glyph to the palette ink (light grey on dark) and
    // tag it so the central retint pass tracks theme changes.
    const QSize sz(12, 12);
    setProperty(freejoy_style::kThemedIconSvgProp,  QStringLiteral(":/Images/icons/lucide/info.svg"));
    setProperty(freejoy_style::kThemedIconSizeProp, sz);
    setPixmap(freejoy_style::tintedSvgPixmap(QStringLiteral(":/Images/icons/lucide/info.svg"), sz, freejoy_style::iconInk()));
}

void InfoLabel::setPixmap(const QPixmap &p)
{
    pix = p;
    QLabel::setPixmap(scaledPixmap());
}

int InfoLabel::heightForWidth(int width) const
{
    return pix.isNull() ? height() : ((qreal)pix.height()*width)/pix.width();
}

QSize InfoLabel::sizeHint() const
{
    int w = width();
    return QSize(w, heightForWidth(w));
}

QPixmap InfoLabel::scaledPixmap() const
{
    return pix.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void InfoLabel::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    if(!pix.isNull()) {
        QLabel::setPixmap(scaledPixmap());
    }
}
