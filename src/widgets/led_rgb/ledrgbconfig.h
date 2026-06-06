#ifndef LEDRGBCONFIG_H
#define LEDRGBCONFIG_H

#include <QWidget>
#include "ledrgb.h"
#include "colorpicker.h"

namespace Ui {
class LedRGBConfig;
}

class LedRGBConfig : public QWidget
{
    Q_OBJECT

public:
    explicit LedRGBConfig(QWidget *parent = nullptr);
    ~LedRGBConfig();

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

public slots:
    void itemClicked(QListWidgetItem *item);
    void setColorToSelectedItems(const QColor &color);
    void updateLedsButtonState();

private slots:
    void on_radioButton_staticColor_clicked(bool checked);
    void on_radioButton_rainbow_clicked(bool checked);
    void on_radioButton_simHub_clicked(bool checked);
    void on_radioButton_flow_clicked(bool checked);

    void on_spinBox_ledsCount_valueChanged(int value);

    void contextMenu(const QPoint &pos);

private:
    /* Single source of truth for which controls each RGB effect enables, so the
     * four radio handlers don't each hand-maintain the same enable matrix. The
     * LED list always tracks the colour picker, so they share one flag. */
    void applyEffectMode(int maxLeds, bool colorAndList, bool delay, bool brightness);

    Ui::LedRGBConfig *ui;

    QList<LedRGB *> m_rgbPtrList;
    ColorPicker m_colorPicker;
};

#endif // LEDRGBCONFIG_H
