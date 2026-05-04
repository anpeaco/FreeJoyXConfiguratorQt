#ifndef AXESCURVES_H
#define AXESCURVES_H

#include <QWidget>
#include "axescurvesplot.h"

#include "deviceconfig.h"
#include "global.h"

namespace Ui {
class AxesCurves;
}
class AxesCurvesButton;

class AxesCurves : public QWidget
{
    Q_OBJECT

public:
    explicit AxesCurves(QWidget *parent = nullptr);
    ~AxesCurves();

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

    void setDarkIcon(bool isDark);

    int pointValue(int pointNumber) const;
    QVector<int> pointValues() const;

    void setPointValue(int pointNumber, int value);
    void setPointValues(const QVector<int> &values);

    void setExclusive(bool exclusive);

    void updateAxis();
    void deviceStatus(bool isConnect);

public slots:
    /* Toggle the "not in use" overlay for the curve button at
     * axisNumber. Called from AxesConfig (relayed through
     * AxesCurvesConfig) whenever an axis's main-source or Output
     * checkbox changes. */
    void setAxisInUse(int axisNumber, bool inUse);

signals:
    void deviceStatusChanged(bool isConnect);

    void axisCurveIndexChanged(int axisNumber, int index);
    void curvePointValueChanged(int axisNumber, int pointNumber, int value);
    //void resetCurvesProfiles();

private slots:
    void pointValueChanged(int pointNumber, int value);

private:
    Ui::AxesCurves *ui;
    QList <AxesCurvesButton *> m_curvesList;
    bool m_ctrlActivated;
    int m_lastSelectedIndex;
    int m_curvePointsValue[CURVE_PLOT_POINTS_COUNT];
};

#endif // AXESCURVES_H
