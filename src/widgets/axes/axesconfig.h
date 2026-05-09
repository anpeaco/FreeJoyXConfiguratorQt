#ifndef AXESCONFIG_H
#define AXESCONFIG_H

#include "axes.h"
#include <QSet>
#include <QString>
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

    /* Forwarded from each Axes widget. AxesCurvesConfig listens to
     * grey-out per-axis curve thumbnails when the matching axis is
     * not contributing to device output. */
    void axisOutputActiveChanged(int axisNumber, bool active);

public slots:
    void addOrDeleteMainSource(int sourceEnum, QString sourceName, bool isAdd);

    /* PinConfig::fastEncoderSelected fires per-pin every time a pin is
     * (de-)assigned to FAST_ENCODER. Maintain a count and toggle the
     * "Encoder" entry in every axis's main-source dropdown when the
     * count crosses 0/1 -- so Encoder is only offered when at least one
     * fast-encoder pin is actually wired up. */
    void fastEncoderPinChanged(const QString &pinGuiName, bool isAdd);
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

    /* Set of pin GUI names currently assigned to FAST_ENCODER in
     * Pin Config (e.g. "A8", "A9", "B6", "B7"). Maintained by
     * fastEncoderPinChanged. completedEncoderSlots() turns it into
     * the list of valid encoder slots (0 = Enc 1 on PA8+PA9,
     * 1 = Enc 2 on PB6+PB7) -- both halves must be mapped for an
     * encoder to be considered usable. */
    QSet<QString> m_fastEncoderPins;

    /* Pure helper -- given m_fastEncoderPins, returns the slots whose
     * A and B pins are BOTH currently assigned. */
    QList<int> completedEncoderSlots() const;
};

#endif // AXESCONFIG_H
