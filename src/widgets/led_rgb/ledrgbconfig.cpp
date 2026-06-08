#include "ledrgbconfig.h"
#include "ui_ledrgbconfig.h"
#include <QMenu>
#include <QWidgetAction>
#include "ledfunction.h"
#include "common_types.h"
#include "deviceconfig.h"
#include "global.h"


LedRGBConfig::LedRGBConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LedRGBConfig)
{
    ui->setupUi(this);

    ui->horizontalLayout_Colors->addWidget(&m_colorPicker);
    ui->spinBox_ledsCount->setMaximum(NUM_RGB_LEDS);

    connect(ui->listWidget_leds, &QListWidget::itemClicked, this, &LedRGBConfig::itemClicked);
    connect(&m_colorPicker, &ColorPicker::currentColorChanged, this, &LedRGBConfig::setColorToSelectedItems);

    ui->listWidget_leds->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidget_leds, &QListWidget::customContextMenuRequested, this, &LedRGBConfig::contextMenu);
}

LedRGBConfig::~LedRGBConfig()
{
    delete ui;
}


void LedRGBConfig::retranslateUi()
{
    ui->retranslateUi(this);
}


void LedRGBConfig::contextMenu(const QPoint &pos)
{
    QListWidgetItem *item = ui->listWidget_leds->itemAt(pos);
    if (!item) return;

    QPoint point = ui->listWidget_leds->mapToGlobal(pos);
    QMenu menu;

    LedRGB *led = nullptr;
    for (int i = 0; i < m_rgbPtrList.size(); ++i) {
        if (m_rgbPtrList[i]->item() == item) {
            led = m_rgbPtrList[i];
        }
    }

    QWidgetAction* action = new QWidgetAction(&menu);
    LedFunction* ledFunc = new LedFunction(&menu);
    action->setDefaultWidget(ledFunc);
    menu.addAction(action);

    if (led) {
        ledFunc->setInverted(led->buttonInverted());
        ledFunc->setLogButton(led->buttonNumber());
    }
    //QAction* rightClickItem = menu.exec(point);
    menu.exec(point);

    if (led) {
        led->setButtonInverted(ledFunc->Inverted());
        led->setButtonNumber(ledFunc->logButtonNum());
    }
}


void LedRGBConfig::updateLedsButtonState()
{
    for (int i = 0; i < m_rgbPtrList.size(); ++i) // can be improved
    {
        if (m_rgbPtrList[i]->buttonNumber() == gEnv.pDeviceConfig->config.rgb_leds[i].input_num + 1) {
            // logical buttons state
            int index = gEnv.pDeviceConfig->config.rgb_leds[i].input_num / 8;
            int bit = gEnv.pDeviceConfig->config.rgb_leds[i].input_num - index * 8;

            if (i >= 0 && i < m_rgbPtrList.size()) {
                if ((gEnv.pDeviceConfig->paramsReport.log_button_data[index] & (1 << (bit & 0x07)))) {
                    m_rgbPtrList[i]->setButtonState(true);
                } else if ((gEnv.pDeviceConfig->paramsReport.log_button_data[index] & (1 << (bit & 0x07))) == false) {
                    m_rgbPtrList[i]->setButtonState(false);
                }
            }
        }
    }
}


void LedRGBConfig::itemClicked(QListWidgetItem *item)
{
    for (int i = 0; i < m_rgbPtrList.size(); ++i) {
        if (m_rgbPtrList[i]->item() == item) {
            m_colorPicker.setCurrentColor(m_rgbPtrList[i]->color());
        }
    }
}

void LedRGBConfig::setColorToSelectedItems(const QColor &color)
{
    //QIcon icon(pixmapToIcon(QPixmap("://Images/buttonLed.png"), color));
    for (int i = 0; i < m_rgbPtrList.size(); ++i) {
        if (m_rgbPtrList[i]->isSelected()) {
            //m_rgbPtrList[i]->setIcon(icon, color);
            m_rgbPtrList[i]->setColor(color);
        }
    }
}


void LedRGBConfig::applyEffectMode(int maxLeds, bool colorAndList, bool delay, bool brightness)
{
    ui->spinBox_ledsCount->setMaximum(maxLeds);
    m_colorPicker.setEnabled(colorAndList);
    ui->listWidget_leds->setEnabled(colorAndList);
    ui->spinBox_delay->setEnabled(delay);
    ui->label_delay->setEnabled(delay);
    ui->spinBox_RBBrightness->setEnabled(brightness);
    ui->label_RBBrightness->setEnabled(brightness);
}

void LedRGBConfig::on_radioButton_staticColor_clicked(bool checked)
{
    //                       maxLeds        colour+list  delay  brightness
    if (checked) applyEffectMode(NUM_RGB_LEDS,    true,  false, false);
}

void LedRGBConfig::on_radioButton_rainbow_clicked(bool checked)
{
    if (checked) applyEffectMode(NUM_RGB_LEDS,    false, true,  true);
}

void LedRGBConfig::on_radioButton_simHub_clicked(bool checked)
{
    if (checked) applyEffectMode(NUM_RGB_LEDS_SH, false, false, false);
}

void LedRGBConfig::on_radioButton_flow_clicked(bool checked)
{
    if (checked) applyEffectMode(NUM_RGB_LEDS,    true,  true,  false);
}

void LedRGBConfig::on_spinBox_ledsCount_valueChanged(int value)
{
    int curCount = m_rgbPtrList.size();
    if (value == curCount) return;

    dev_config_t *devc = &gEnv.pDeviceConfig->config;

    if (value > curCount) {
        for (int i = curCount; i < value; ++i) {
            LedRGB *led = new LedRGB(this);
            led->setText(QString::number(i+1));
            int red = devc->rgb_leds[i].color.r;
            int green = devc->rgb_leds[i].color.g;
            int blue = devc->rgb_leds[i].color.b;
            led->setColor(QColor(red, green, blue));
            m_rgbPtrList.append(led);
            ui->listWidget_leds->addItem(led->item());
        }
    } else {
        for (int i = curCount; i > value; --i) {
            delete ui->listWidget_leds->takeItem(ui->listWidget_leds->row(m_rgbPtrList.last()->item()));
            m_rgbPtrList.last()->deleteLater();
            m_rgbPtrList.removeLast();
        }
    }
}


void LedRGBConfig::readFromConfig()
{
    dev_config_t *devc = &gEnv.pDeviceConfig->config;

    on_spinBox_ledsCount_valueChanged(devc->rgb_count); // change
    // REWRITE THIS MESS !!!! // forgot why I wrote this
    for (int i = 0; i < devc->rgb_count; ++i) {
        int red = devc->rgb_leds[i].color.r;
        int green = devc->rgb_leds[i].color.g;
        int blue = devc->rgb_leds[i].color.b;
        m_rgbPtrList[i]->setColor(QColor(red, green, blue));
        // button
        m_rgbPtrList[i]->setButtonNumber(devc->rgb_leds[i].input_num + 1);
        m_rgbPtrList[i]->setButtonInverted(devc->rgb_leds[i].is_inverted);
    }

    switch(devc->rgb_effect) {
        case WS2812B_STATIC:
            ui->radioButton_staticColor->setChecked(true);
            on_radioButton_staticColor_clicked(true);
            break;
        case WS2812B_RAINBOW:
            ui->radioButton_rainbow->setChecked(true);
            on_radioButton_rainbow_clicked(true);
            break;
        case WS2812B_SIMHUB:
            ui->radioButton_simHub->setChecked(true);
            on_radioButton_simHub_clicked(true);
            break;
        case WS2812B_FLOW:
            ui->radioButton_flow->setChecked(true);
            on_radioButton_flow_clicked(true);
            break;
        default:
            break;
    }

    ui->spinBox_ledsCount->setValue(devc->rgb_count);
    ui->spinBox_delay->setValue(devc->rgb_delay_ms);
    ui->spinBox_RBBrightness->setValue(devc->rgb_brightness);
}

void LedRGBConfig::writeToConfig()
{
    dev_config_t *devc = &gEnv.pDeviceConfig->config;
    for (int i = 0; i < m_rgbPtrList.size(); ++i) {
        QColor color = m_rgbPtrList[i]->color();
        devc->rgb_leds[i].color.r = color.red();
        devc->rgb_leds[i].color.g = color.green();
        devc->rgb_leds[i].color.b = color.blue();
        // button
        devc->rgb_leds[i].input_num = m_rgbPtrList[i]->buttonNumber() -1;
        devc->rgb_leds[i].is_inverted = m_rgbPtrList[i]->buttonInverted();
    }

    if (ui->radioButton_staticColor->isChecked()) {
        devc->rgb_effect = WS2812B_STATIC;
    } else if (ui->radioButton_rainbow->isChecked()) {
        devc->rgb_effect = WS2812B_RAINBOW;
    } else if (ui->radioButton_simHub->isChecked()) {
        devc->rgb_effect = WS2812B_SIMHUB;
    } else if (ui->radioButton_flow->isChecked()) {
        devc->rgb_effect = WS2812B_FLOW;
    }

    devc->rgb_count = ui->spinBox_ledsCount->value();
    devc->rgb_delay_ms = ui->spinBox_delay->value();
    devc->rgb_brightness = ui->spinBox_RBBrightness->value();
}
