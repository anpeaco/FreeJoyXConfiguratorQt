#ifndef AXESCURVESCONFIG_H
#define AXESCURVESCONFIG_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE
class AxesCurvesProfiles;

namespace Ui {
class AxesCurvesConfig;
}

class AxesCurvesConfig : public QWidget
{
    Q_OBJECT

public:
    explicit AxesCurvesConfig(QWidget *parent = nullptr);
    ~AxesCurvesConfig();

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

    void setExclusive(bool exclusive);

    void updateAxesCurves();
    void deviceStatus(bool isConnect);

public slots:
    /* Relays AxesConfig::axisOutputActiveChanged into the inner
     * AxesCurves widget so per-axis curve thumbnails show a
     * "not in use" overlay when the axis isn't contributing to
     * device output. */
    void setAxisInUse(int axisNumber, bool inUse);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void setPoints(const QVector<int> &values);
    void on_toolButton_Ctrl_toggled(bool checked);

private:
    Ui::AxesCurvesConfig *ui;
    QList<AxesCurvesProfiles *> m_profiles;

    void updateColor();
    QPixmap coloringPixmap(QPixmap pixmap, const QColor &color);
    QString m_toolTip;
};

#endif // AXESCURVESCONFIG_H
