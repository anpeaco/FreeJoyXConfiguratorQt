#ifndef AXESTOBUTTONSSLIDER_H
#define AXESTOBUTTONSSLIDER_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE
//#include <QLabel>

#include "deviceconfig.h"
#include "global.h"

#define MAX_A2B_BUTTONS 12

namespace Ui {
class AxesToButtonsSlider;
}

class AxesToButtonsSlider : public QWidget
{
    Q_OBJECT

public:
    explicit AxesToButtonsSlider(QWidget *parent = nullptr);
    ~AxesToButtonsSlider();

    void setPointsCount(uint count);
    uint pointsCount() const;

    void setPointValue(uint value, uint pointNumber);
    uint pointValue(uint pointNumber) const;

    void setAxisOutputValue(int outValue, bool isEnable);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *) override;
    bool event(QEvent *event) override;

private:
    Ui::AxesToButtonsSlider *ui;
    void drawPoint(const QPoint &pointPos, uint pointNumber); // QPoint &point_pos
    void movePointer(uint posX, uint pointNumber);
    uint calcPointValue(int currentPos) const;
    void pointsPositionReset();
    //void setLableValue(int pointPos, uint pointNumber);

    float m_lineSpacing;

    float m_axisOutputValue;
    int m_axisOutputWidth;
    bool m_isOutEnabled;

    // Index of the button (zone between two adjacent boundary points) the
    // axis output currently lands in, computed with the same threshold rule
    // as the firmware's GetPressedFromAxis(). -1 = no zone active. Drives the
    // green fill of the per-button state dots.
    int m_activeButton = -1;
    int activeButtonForValue() const;

    const QColor m_kAxisRectColor = QColor(160, 0, 0);
    const QColor m_kAxisRectColor_dis = QColor(160, 0, 0, 80);
    const QColor m_kPointRawActivColor = QColor(0, 170, 0);
    QColor m_axisRectColor;

    // Boundary-point hit-test region radius. The grab area stays at 7px (the
    // square polygon built in movePointer doubles as the hit test), but the
    // boundary handle is now drawn smaller and more subtly than the
    // per-button state dot so the two don't compete -- see m_kHandleRadius.
    const int m_kPointerRadius = 7;
    // Visible radius of a boundary handle (the draggable threshold marker).
    const int m_kHandleRadius = 4;
    // Visible radius of a per-button state dot (centred in each zone).
    const int m_kButtonDotRadius = 6;
    // Boundary dividers: a filled diamond at each internal threshold, and a
    // half-diamond (inward-pointing triangle) at each outer end. Vertical
    // extent is 2px taller than the horizontal half-width.
    const int m_kDiamondRadius = 6;     // horizontal half-width
    const int m_kDiamondHalfHeight = 7; // vertical half-height
    // Grey outline shared by idle button dots and boundary handles.
    const QColor m_kOutlineColor = QColor(0x9c, 0xa3, 0xaf);
    // Green fill of an active button dot -- matches the Buttons tab physical
    // indicator (#22c55e in common.qss).
    const QColor m_kButtonActiveColor = QColor(0x22, 0xc5, 0x5e);

    const QColor m_kPointerColor = QColor(1, 119, 215);
    const uint m_kMaxPointValue = 255;
    // Track inset = bar inset (9, the Out/Raw bars' layout margin) + the
    // diamond half-width, so a diamond at either end has its outer edge flush
    // with the Out/Raw bars above. The track itself is therefore slightly
    // narrower than those bars.
    const int m_kOffset = 15;
    const int m_kRangeBetween = 4; // minimum distance between adjacent markers
    const int m_kMinHeight = 30;

    uint m_pointsCount;

    struct A2B_point // uint caused a lot of grief
    {
        QPolygon polygon;
        QColor color;
        QLabel *text_label;
        uint posX;
        uint current_value;
        bool is_drag;
    };
    QList<A2B_point *> m_pointPtrList;
};

#endif // AXESTOBUTTONSSLIDER_H
