#include "centered_cbox.h"

#include <QComboBox>
#include <QFontMetricsF>
#include <QItemDelegate>
#include <QStylePainter>

CenteredCBox::CenteredCBox(QWidget *parent)
    : QComboBox(parent)
{
    m_arrowWidth = 0;
}

// setting text approximately centered
void CenteredCBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QStylePainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

    QStyleOptionComboBox option;
    initStyleOption(&option);
    painter.drawComplexControl(QStyle::CC_ComboBox, option);

    option.direction = Qt::LeftToRight;

    if (style()) {
        QRect rect = style()->subControlRect(QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxArrow, this);
        m_arrowWidth = rect.width();
    }

    QFontMetricsF font_metric(property("font").value<QFont>());
    // looks like width is measured between letter centres and ends up shorter
    // than the actual width. Multiplying by 1.1.
    qreal font_width = font_metric.horizontalAdvance(option.currentText);// * 1.1f;
    font_metric.averageCharWidth();

    // centering against the whole combobox makes it feel like there's more empty
    // space on the left because of the right-side arrow. Centering excluding the
    // arrow also looks off; arrow_width_/3 here is a compromise.
    //
    int offset = option.rect.center().x() - m_arrowWidth / 3.2f - font_width / 2;

    // when the combobox is shorter than the text, the left offset shrinks
    if (offset + font_width*1.1f > option.rect.right() - m_arrowWidth) {
        offset -= ((offset + font_width*1.1f) - (option.rect.right() - m_arrowWidth));
        if (offset < 0)
            offset = 0;
    }
    option.rect.setLeft(offset);

    painter.drawControl(QStyle::CE_ComboBoxLabel, option);
}
