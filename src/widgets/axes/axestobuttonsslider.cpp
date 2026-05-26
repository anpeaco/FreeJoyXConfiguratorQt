#include "axestobuttonsslider.h"
#include "ui_axestobuttonsslider.h"
#include <cmath>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPolygon>
#include <QToolTip>

//#include <QDebug>
AxesToButtonsSlider::AxesToButtonsSlider(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AxesToButtonsSlider)
{
    ui->setupUi(this);

    setMouseTracking(true);

    this->setMinimumHeight(m_kMinHeight);

    // call SetPointsCount?
    m_pointsCount = 0;
    m_axisOutputValue = 0;
    m_axisOutputWidth = 0;
    m_axisRectColor = m_kAxisRectColor_dis;
    m_isOutEnabled = true;
}

AxesToButtonsSlider::~AxesToButtonsSlider()
{
    for (uint i = 0; i < m_pointsCount; ++i) {
        A2B_point *point = m_pointPtrList.takeLast();
        delete point;
    }
    delete ui;
}

void AxesToButtonsSlider::setAxisOutputValue(int outValue, bool isEnable)
{
    m_isOutEnabled = isEnable;
    m_axisOutputValue = outValue; // + AXIS_MAX_VALUE) / (float)AXIS_FULLSCALE;        //abs((value  - min)/ (float)(max - min));
    if (isEnable && this->isEnabled() == true) {
        m_axisRectColor = m_kAxisRectColor;
    } else {
        m_axisRectColor = m_kAxisRectColor_dis;
    }
    m_activeButton = activeButtonForValue();
    update();
}

/* Which axis-to-buttons zone the current output lands in, evaluated with the
 * exact integer thresholding the firmware uses in GetPressedFromAxis()
 * (axis_to_buttons.c): tmp = (scaled - AXIS_MIN_VALUE) * 255 is compared
 * against points[k] * AXIS_FULLSCALE. Button 0's lower edge is inclusive;
 * every later zone's lower edge is exclusive, so the zones are mutually
 * exclusive and at most one is active -- matching the firmware's break-on-
 * first-match. Returns the zone index [0 .. buttons_cnt-1], or -1 if none.
 * m_pointsCount == buttons_cnt + 1, so there are m_pointsCount-1 zones. */
int AxesToButtonsSlider::activeButtonForValue() const
{
    if (m_pointsCount < 2)
        return -1;

    const qint64 tmp = qint64(m_axisOutputValue - AXIS_MIN_VALUE) * 255;
    for (uint j = 0; j + 1 < m_pointsCount; ++j) {
        const qint64 lower = qint64(m_pointPtrList[j]->current_value) * AXIS_FULLSCALE;
        const qint64 upper = qint64(m_pointPtrList[j + 1]->current_value) * AXIS_FULLSCALE;
        const bool inZone = (j == 0) ? (tmp >= lower && tmp <= upper)
                                     : (tmp > lower && tmp <= upper);
        if (inZone)
            return int(j);
    }
    return -1;
}

void AxesToButtonsSlider::paintEvent(QPaintEvent *event) // optimise width_ - offset_*2
{
    Q_UNUSED(event)
    QPainter painter;
    const int rect_height = 5;
    // Centre the track vertically so the whole control fits the 30px row.
    const int rect_y = (this->height() - rect_height) / 2;

    painter.begin(this);

    painter.setPen(Qt::lightGray);
    painter.drawRect(QRect(m_kOffset, rect_y, this->width() - m_kOffset * 2, rect_height));

    // calc and paint raw data on a2b
    m_axisOutputWidth = (m_axisOutputValue + AXIS_MAX_VALUE) / (float) AXIS_FULLSCALE * (this->width() - m_kOffset * 2);
    QRect rect(m_kOffset, rect_y, m_axisOutputWidth, rect_height);
    painter.drawRect(rect);
    // Subtle top-lighter gradient on the filled track, matching the Out/Raw
    // bars and the app-wide progress-bar fill. Derived from m_axisRectColor so
    // the enabled/disabled variants both carry through.
    QLinearGradient axisGrad(rect.topLeft(), rect.bottomLeft());
    axisGrad.setColorAt(0.0, m_axisRectColor.lighter(118));
    axisGrad.setColorAt(1.0, m_axisRectColor);
    painter.fillRect(rect, axisGrad);

    // paint separators -- short ticks above and below the track, leaving the
    // track itself clear.
    painter.setPen(Qt::lightGray);
    for (uint i = 0; i < 25; ++i) {
        const int x = int(i * m_lineSpacing) + m_kOffset;
        painter.drawLine(x, rect_y - 4, x, rect_y - 1);                            // above
        painter.drawLine(x, rect_y + rect_height + 1, x, rect_y + rect_height + 4); // below
    }

    painter.setRenderHint(QPainter::Antialiasing, true);
    const int cy = this->height() / 2;
    const bool live = m_isOutEnabled && this->isEnabled();

    // Per-button state dots: one grey-outlined circle centred in each zone
    // (the gap between two adjacent boundary points). The dot for the zone the
    // axis currently sits in fills green, mirroring the physical-button
    // indicator on the Buttons tab. Drawn first so the boundary handles sit on
    // top where they overlap a narrow zone.
    for (uint j = 0; j + 1 < m_pointsCount; ++j) {
        const int midX = (int(m_pointPtrList[j]->posX) + int(m_pointPtrList[j + 1]->posX)) / 2;
        const bool active = live && int(j) == m_activeButton;
        painter.setPen(QPen(active ? m_kButtonActiveColor : m_kOutlineColor, 1.5));
        painter.setBrush(active ? m_kButtonActiveColor : QBrush(Qt::white));
        painter.drawEllipse(QPoint(midX, cy), m_kButtonDotRadius, m_kButtonDotRadius);
    }

    // Boundary dividers: a filled diamond at each internal threshold, and a
    // half-diamond (inward-pointing triangle) at each outer end. Filled in the
    // point's own colour (blue by default, black on hover, grey while dragging/
    // disabled). Hit-test polygon (radius m_kPointerRadius, centred on posX) is
    // unchanged.
    painter.setPen(Qt::NoPen);
    const int rx = m_kDiamondRadius;
    const int ry = m_kDiamondHalfHeight;
    for (uint i = 0; i < m_pointsCount; ++i) {
        painter.setBrush(m_pointPtrList[i]->color);

        const int x = int(m_pointPtrList[i]->posX);
        QPolygon shape;
        if (i == 0) {
            // left end: right half of a diamond -> triangle pointing inward (right)
            shape << QPoint(x, cy - ry) << QPoint(x + rx, cy) << QPoint(x, cy + ry);
        } else if (i + 1 == m_pointsCount) {
            // right end: left half of a diamond -> triangle pointing inward (left)
            shape << QPoint(x, cy - ry) << QPoint(x - rx, cy) << QPoint(x, cy + ry);
        } else {
            // internal threshold: full diamond
            shape << QPoint(x, cy - ry) << QPoint(x + rx, cy)
                  << QPoint(x, cy + ry) << QPoint(x - rx, cy);
        }
        painter.drawPolygon(shape);
    }
    painter.end();
}

void AxesToButtonsSlider::drawPoint(const QPoint &pointPos, uint pointNumber)
{
    if (pointPos.x() < m_kOffset || pointPos.x() > this->width() - m_kOffset) {
        return;
    } else if (m_pointPtrList[pointNumber]->is_drag && pointPos.x() < this->width() + m_kOffset) {
        if (m_pointsCount > 1) {
            if (pointNumber > 0 && pointNumber < m_pointsCount - 1) {
                if (uint(pointPos.x()) < m_pointPtrList[pointNumber - 1]->posX + m_kRangeBetween // bug on config load?
                    || uint(pointPos.x()) > m_pointPtrList[pointNumber + 1]->posX - m_kRangeBetween) // when points are flush and rounding kicks in
                {
                    return;
                }
            } else if (pointNumber == 0) {
                if (uint(pointPos.x()) > m_pointPtrList[pointNumber + 1]->posX - m_kRangeBetween) {
                    return;
                }
            } else {
                if (uint(pointPos.x()) < m_pointPtrList[pointNumber - 1]->posX + m_kRangeBetween) {
                    return;
                }
            }
        }
        movePointer(pointPos.x(), pointNumber);
        m_pointPtrList[pointNumber]->posX = pointPos.x();
        m_pointPtrList[pointNumber]->current_value = calcPointValue(m_pointPtrList[pointNumber]->posX);
        return;
    }
}

//void AxesToButtonsSlider::setLableValue(int pointPos, uint pointNumber)
//{
//    m_labelPtrList[pointNumber]->setNum(int(calcPointValue(pointPos)));
//}

void AxesToButtonsSlider::setPointsCount(uint count)
{
    if (count < 2)
        count = 2;
    // create points
    if (count > m_pointsCount) {
        for (uint i = 0; i < count - m_pointsCount; ++i) {
            A2B_point *point = new A2B_point;
            m_pointPtrList.append(point);
            // Diamond: 4 points, positioned in movePointer().
            point->polygon << QPoint() << QPoint() << QPoint() << QPoint();
        }
    }
    // delete points
    else if (count < m_pointsCount) {
        for (uint i = 0; i < m_pointsCount - count; ++i) {
            A2B_point *point = m_pointPtrList.takeLast();
            delete point;
        }
    }
    m_pointsCount = count;
    pointsPositionReset();
}

uint AxesToButtonsSlider::pointsCount() const
{
    return m_pointsCount;
}

void AxesToButtonsSlider::setPointValue(uint value, uint pointNumber)
{
    /* Both lists are sized to MAX points at construction; m_pointsCount
     * tracks the *active* count. The original guard used > and clamped
     * to m_pointsCount itself, but m_pointPtrList[m_pointsCount] is one
     * past the end of valid indices [0..m_pointsCount-1]. With a
     * garbage buttons_cnt from the device (e.g. uninitialised
     * a2bCfg->buttons_cnt = 192), Axes::readFromConfig would loop with
     * pointNumber up to 192, fall through this guard, and ASSERT-crash
     * on m_pointPtrList[192]. Bail on out-of-range. */
    if (pointNumber >= uint(m_pointPtrList.size())) {
        return;
    }
    uint pos = uint((value * (this->width() - m_kOffset * 2.0f) / m_kMaxPointValue)); // tweak
    // ?
    pos += m_kOffset;
    if (pos > uint(this->width() - m_kOffset)) {
        pos = this->width() - m_kOffset;
    } else if (pos < uint(m_kOffset)) {
        pos = m_kOffset;
    }
    movePointer(pos, pointNumber);
    m_pointPtrList[pointNumber]->current_value = value;
    update();
}

uint AxesToButtonsSlider::pointValue(uint pointNumber) const
{
    return m_pointPtrList[pointNumber]->current_value;
}

uint AxesToButtonsSlider::calcPointValue(int currentPos) const
{
    float tmp_value = (this->width() - m_kOffset * 2.0f) / m_kMaxPointValue;
    return uint(round((currentPos - m_kOffset) / tmp_value));
}

void AxesToButtonsSlider::pointsPositionReset()
{
    float tmp_distance = (this->width() - m_kOffset * 2.0f) / (m_pointsCount - 1); // tweak
    // apply color, position
    for (int i = 0; i < m_pointPtrList.size(); ++i) {
        if (this->isEnabled() == true) {
            m_pointPtrList[i]->color = m_kPointerColor;
            //axis_rect_color_ = kAxisRectColor;
        } else {
            m_pointPtrList[i]->color = Qt::lightGray;
            //axis_rect_color_ = kAxisRectColor_dis;
        }
        //PointAdrList[i]->color = pointer_color_;
        m_pointPtrList[i]->is_drag = false;
        m_pointPtrList[i]->posX = round(i * tmp_distance + m_kOffset);
    }
    // last to last X position
    m_pointPtrList[m_pointsCount - 1]->posX = this->width() - m_kOffset;
    // move points
    for (uint i = 0; i < m_pointsCount; ++i) {
        movePointer(m_pointPtrList[i]->posX, i);
        m_pointPtrList[i]->current_value = calcPointValue(m_pointPtrList[i]->posX);
    }
    update();
}

void AxesToButtonsSlider::movePointer(uint posX, uint pointNumber)
{
    // Square bounding box centred on the track; the marker is drawn as a
    // circle inside it (matching the filter/deadband slider handle). The
    // square doubles as the hit-test region.
    QPolygon *polygon = &m_pointPtrList[pointNumber]->polygon;
    const int cy = this->height() / 2;
    const int r  = m_kPointerRadius;
    m_pointPtrList[pointNumber]->posX = posX;
    polygon->setPoint(0, int(posX) - r, cy - r);
    polygon->setPoint(1, int(posX) + r, cy - r);
    polygon->setPoint(2, int(posX) + r, cy + r);
    polygon->setPoint(3, int(posX) - r, cy + r);
}

void AxesToButtonsSlider::mouseMoveEvent(QMouseEvent *event)
{
    for (uint i = 0; i < m_pointsCount; ++i) {
        if (m_pointPtrList[i]->is_drag == false) {
            if (m_pointPtrList[i]->polygon.containsPoint(event->pos(), Qt::WindingFill)) {
                m_pointPtrList[i]->color = Qt::black;
            } else {
                m_pointPtrList[i]->color = m_kPointerColor;
            }
        } else if (m_pointPtrList[i]->is_drag == true) { // lots of redundancy here -- revisit later
            if (event->buttons() & Qt::LeftButton) {
                drawPoint(event->pos(), i);
            }
            if (m_pointPtrList[i]->is_drag) {
                m_pointPtrList[i]->color = Qt::lightGray;
            }
            break; // not tested
        }
    }
    update();
}

void AxesToButtonsSlider::mousePressEvent(QMouseEvent *event)
{
    for (uint i = 0; i < m_pointsCount; ++i) {
        if (m_pointPtrList[i]->polygon.containsPoint(event->pos(), Qt::WindingFill)) {
            m_pointPtrList[i]->is_drag = true;
            break;
        }
    }
}

void AxesToButtonsSlider::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    for (uint i = 0; i < m_pointsCount; ++i) {
        if (m_pointPtrList[i]->is_drag == true) {
            m_pointPtrList[i]->is_drag = false;
            m_pointPtrList[i]->color = m_kPointerColor; // ????
            break;
        }
    }
    update();
}

void AxesToButtonsSlider::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    float tmp_value = (this->width() - m_kOffset * 2.0f) / (float) m_kMaxPointValue;
    for (int i = 0; i < m_pointPtrList.size(); ++i) {
        m_pointPtrList[i]->posX = (m_pointPtrList[i]->current_value * tmp_value) + m_kOffset;
        movePointer(m_pointPtrList[i]->posX, i);
    }
    m_lineSpacing = (this->width() - m_kOffset * 2.0f) / 24.0f;
}

bool AxesToButtonsSlider::event(QEvent *event)
{
    // Per-marker value moved off the track into a hover tooltip (keeps the
    // control inside 30px). Show the value of the diamond under the cursor.
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *he = static_cast<QHelpEvent *>(event);
        for (uint i = 0; i < m_pointsCount; ++i) {
            if (m_pointPtrList[i]->polygon.containsPoint(he->pos(), Qt::WindingFill)) {
                QToolTip::showText(he->globalPos(),
                                   QString::number(m_pointPtrList[i]->current_value), this);
                return true;
            }
        }
        QToolTip::hideText();
        event->ignore();
        return true;
    }
    if (event->type() == QEvent::EnabledChange) {
        if (this->isEnabled() == true) {
            for (int i = 0; i < m_pointPtrList.size(); ++i) {
                m_pointPtrList[i]->color = m_kPointerColor;
            }
            if (m_isOutEnabled == true) {
                m_axisRectColor = m_kAxisRectColor;
            }
        } else {
            for (int i = 0; i < m_pointPtrList.size(); ++i) {
                m_pointPtrList[i]->color = Qt::lightGray;
            }
            m_axisRectColor = m_kAxisRectColor_dis;
        }
        update();
        return true;
    }
    return QWidget::event(event);
}
