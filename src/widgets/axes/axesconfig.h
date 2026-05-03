#ifndef AXESCONFIG_H
#define AXESCONFIG_H

#include "axes.h"
#include <QWidget>

#include "deviceconfig.h"
#include "global.h"

namespace Ui {
class AxesConfig;
}

class QCheckBox;

class AxesConfig : public QWidget
{
    Q_OBJECT

public:
    explicit AxesConfig(QWidget *parent = nullptr);
    ~AxesConfig();

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

    void axesValueChanged();

signals:
    void axisRawValueChanged(int);
    void axisOutValueChanged(int);
    void a2bCountChanged(int count);
    /* Per-axis a2b button counts. Index = axis number (0..MAX_AXIS_NUM-1).
     * Emitted alongside a2bCountChanged so subscribers building grouped
     * UI can sub-divide the "Axis-to-buttons" section by axis. */
    void a2bBreakdownChanged(const QList<int> &perAxis);

public slots:
    void addOrDeleteMainSource(int sourceEnum, QString sourceName, bool isAdd);
private slots:
    void a2bCountCalc(int count, int previousCount);
    void hideAxis(int index, bool hide);
    //    void axesValueChanged(int value);

    /* Recompute, for every axis, which OTHER axes have already
     * selected each candidate source, and push that map down so each
     * axis can grey-out + suffix the duplicate items in its dropdown.
     * Called from each axis's mainSourceChanged signal, plus the tail
     * of readFromConfig and addOrDeleteMainSource. */
    void refreshSourceUsage();

private:
    Ui::AxesConfig *ui;
    int m_a2bButtonsCount;

    QList<Axes *> m_axesPtrList;
    QList<QCheckBox *> m_hideChBoxes;
};

#endif // AXESCONFIG_H
