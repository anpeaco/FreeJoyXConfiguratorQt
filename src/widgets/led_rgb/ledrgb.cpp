#include "ledrgb.h"
#include <QPainter>
#include <QIcon>

const QString LedRGB::m_buttonOffPixPath = QString("://Images/rgbButtonOff.png");
const QString LedRGB::m_buttonOnPixPath = QString("://Images/rgbButtonOn.png");

namespace {
    //! QPixmap gray-scale image (an alpha map) to colored QIcon
    QIcon pixmapToIcon(QPixmap pixmap, const QColor &color, int num = 0)
    {
        // initialize painter to draw on a pixmap and set composition mode
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);//CompositionMode_SourceIn
        // set color
        painter.setBrush(color);
        painter.setPen(color);
        // paint rect
        painter.drawRect(pixmap.rect());

        if (num > 0) {
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            // invert color
            painter.setPen(color.rgba() ^ 0xffffff);
            // if (color.valueF() > 0.6f || color.alphaF() < 0.6f) {
            // } else {
            // }

            QFont font = painter.font();
            font.setPixelSize(16);
            font.setBold(true);
            painter.setFont(font);
            painter.drawText(pixmap.rect(), Qt::AlignCenter, QString::number(num));
        }

        return QIcon(pixmap);
    }
}


LedRGB::LedRGB(QObject *parent)
    : QObject{parent}
    , m_number(0)
    , m_buttonInverted(false)
    , m_buttonState(false)
    , m_item(new QListWidgetItem)
{
    m_item->setIcon(baseIcon());
}

LedRGB::~LedRGB()
{
    //delete m_item;
}


QIcon LedRGB::baseIcon()
{
    static QIcon icon("://Images/rgbButtonOff.png");
    return icon;
}


bool LedRGB::isSelected() const
{
    return m_item->isSelected();
}

QListWidgetItem *LedRGB::item() const
{
    return m_item;
}


void LedRGB::updateLedColor()
{
    bool state = m_buttonState;
    if (m_buttonInverted) state = !state;

    if (state) {
        m_item->setIcon(pixmapToIcon(QPixmap(m_buttonOnPixPath), m_color, m_number));
    } else {
        m_item->setIcon(pixmapToIcon(QPixmap(m_buttonOffPixPath), m_color, m_number));
    }
}


QColor LedRGB::color() const
{
    return m_color;
}

void LedRGB::setColor(const QColor &color)
{
    if (color == m_color) return;
    m_color = color;
    updateLedColor();
}


void LedRGB::setButtonState(bool state)
{
    if (state == m_buttonState) return;
    m_buttonState = state;
    updateLedColor();
}

// why?
QIcon LedRGB::icon() const
{
    return baseIcon();
}
// why?
void LedRGB::setIcon(const QIcon &icon, const QColor &color)
{
    m_color = color;
    m_item->setIcon(icon);
}


QString LedRGB::text() const
{
    return m_item->text();
}

void LedRGB::setText(const QString &text)
{
    m_item->setText(text);
}


void LedRGB::setButtonNumber(int num)
{
    if (num == m_number) return;
    m_number = num;
    updateLedColor();
}

int LedRGB::buttonNumber() const
{
    return m_number;
}


void LedRGB::setButtonInverted(bool inverted)
{
    if (m_buttonInverted == inverted) return;
    m_buttonInverted = inverted;
    updateLedColor();
}

int LedRGB::buttonInverted() const
{
    return m_buttonInverted;
}
