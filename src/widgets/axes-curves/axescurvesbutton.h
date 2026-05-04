#ifndef AXESCURVESBUTTON_H
#define AXESCURVESBUTTON_H

#include <QPushButton>
#include "axescurvesplot.h"

class AxesCurvesButton : public AxesCurvesPlot
{
    Q_OBJECT

public:
    explicit AxesCurvesButton(QWidget *parent = nullptr);

    bool isChecked() const;
    bool autoExclusive() const;
    void setAutoExclusive(bool enabled);
    void setCheckable(bool chackable);
    void setDraggable(bool draggable);

    bool isAxisInUse() const { return m_axisInUse; }

public slots:
    void setChecked(bool checked);
    /* Marks this axis's curve thumbnail as not contributing to the device
     * output -- main source unset or Output checkbox off. The button stays
     * fully interactive (so the user can still preview/edit the curve)
     * but a "not in use" overlay is painted on top. */
    void setAxisInUse(bool inUse);

signals:
    void toggled(bool checked);
    void clicked(bool checked = false);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEvent *event) override;
#else
    void enterEvent(QEnterEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    void installStyleSheet();
    QList <AxesCurvesButton *> queryButtonList() const;
    AxesCurvesButton *queryCheckedButton() const;

    QPoint m_dragStartPos;
    bool m_toggled;
    bool m_autoExclusive;
    bool m_checkable;
    bool m_draggable;
    bool m_axisInUse;
};

#endif // AXESCURVESBUTTON_H
