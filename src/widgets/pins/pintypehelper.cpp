#include "pintypehelper.h"
#include "ui_pintypehelper.h"
#include <QHoverEvent>
#include <QDebug>
#include <QCheckBox>
#include <QLabel>
#include <QSignalBlocker>
#include <QToolTip>
#include <QCursor>
#include <QEvent>
#include <QRegularExpression>
#include "style_helpers.h"

PinTypeHelper::PinTypeHelper(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PinTypeHelper)
{
    ui->setupUi(this);

    /* Short delay before a row's wiring-detail tooltip pops, so it doesn't
     * flash up the instant the cursor crosses the info icon (QToolTip::showText
     * is otherwise immediate, unlike Qt's built-in tooltip delay). */
    m_tipTimer.setSingleShot(true);
    connect(&m_tipTimer, &QTimer::timeout, this, [this] {
        if (m_pendingTipLabel)
            QToolTip::showText(QCursor::pos(),
                               m_rowHelp.value(m_pendingTipLabel), m_pendingTipLabel);
    });

    // not sure which is better -- this approach, or a new HoverLabel class inheriting from QLabel
    for (auto &&c : ui->groupBox->children()) {
        QLabel *label = qobject_cast<QLabel*>(c);
        if (!label) continue;
        // Whole-row hover still drives the pin highlight (helpHovered).
        label->installEventFilter(this);
        label->setMouseTracking(true);
        label->setAttribute(Qt::WA_Hover, true);

        /* But the wiring-detail tooltip should only pop over the info icon,
         * not anywhere on the row. Pull the row's help HTML out of the
         * widget tooltip, make the icon's <a href> link drive it via
         * linkHovered, and show/hide it manually as a rich-text tooltip. */
        const QString help = label->toolTip();
        if (help.isEmpty()) continue;
        m_rowHelp.insert(label, help);
        label->setToolTip(QString());
        label->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        connect(label, &QLabel::linkHovered, this, [this, label](const QString &href) {
            if (href.isEmpty()) {
                m_tipTimer.stop();
                m_pendingTipLabel = nullptr;
                QToolTip::hideText();
            } else {
                m_pendingTipLabel = label;
                m_tipTimer.start(600);   // ms before the tooltip appears
            }
        });
    }

    ui->label_analogAxis->setProperty("pinType", AXIS_ANALOG);
    ui->label_buttons->setProperty("pinType", BUTTON_GND);
    ui->label_shiftReg->setProperty("pinType", SHIFT_REG_LATCH);
    ui->label_spi->setProperty("pinType", MCP3201_CS);
    ui->label_i2c->setProperty("pinType", I2C_SDA);
    ui->label_rgbLed->setProperty("pinType", LED_RGB_WS2812B);
    ui->label_pwmLed->setProperty("pinType", LED_PWM);
    ui->label_monoLed->setProperty("pinType", LED_SINGLE);
    ui->label_fastEncoder->setProperty("pinType", FAST_ENCODER);
    ui->label_uart->setProperty("pinType", UART_TX);

    connect(ui->checkBox_i2cBus, &QCheckBox::toggled, this, [this](bool on) {
        emit busToggleRequested(BUS_I2C, on);
    });
    connect(ui->checkBox_spiBus, &QCheckBox::toggled, this, [this](bool on) {
        emit busToggleRequested(BUS_SPI, on);
    });

    applyThemedInfoIcons();
}

PinTypeHelper::~PinTypeHelper()
{
    delete ui;
}

void PinTypeHelper::setBusState(int bus, bool checked, bool enabled)
{
    QCheckBox *cb = (bus == BUS_I2C) ? ui->checkBox_i2cBus : ui->checkBox_spiBus;
    // block the toggled() signal so syncing UI to pin state doesn't loop back
    // into PinConfig as if the user had clicked.
    const QSignalBlocker blocker(cb);
    cb->setChecked(checked);
    cb->setEnabled(enabled);
}


void PinTypeHelper::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    /* The legend's info icons are tinted to the text colour; re-tint them when
     * the theme flips so they stay legible (white on dark, grey on light). */
    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::StyleChange) {
        applyThemedInfoIcons();
    }
}

void PinTypeHelper::applyThemedInfoIcons()
{
    /* The .ui ships each row as `<p>Label <a href="info"><img info.svg></a></p>`
     * with a fixed-colour SVG that vanishes on dark. Rebuild each row with a
     * palette-tinted icon, keeping the prefix text and the "info" link (so the
     * wiring-detail tooltip still fires on icon hover). Re-reads the prefix from
     * the current text each call, so it survives both theme and language flips. */
    QString icon = freejoy_style::svgIconHtml(
        QStringLiteral(":/Images/icons/lucide/info.svg"), freejoy_style::iconInk(), 12);
    /* svgIconHtml emits `<img style=...>`; add right-alignment so it sits at the
     * row's right edge as the original markup did. */
    const int imgPos = icon.indexOf(QStringLiteral("<img"));
    if (imgPos >= 0) icon.insert(imgPos + 4, QStringLiteral(" align=\"right\""));
    const QString link = QStringLiteral("<a href=\"info\">") + icon + QStringLiteral("</a>");

    static const QRegularExpression rx(
        QStringLiteral("<p>(.*?)\\s*<a href=\"info\""),
        QRegularExpression::DotMatchesEverythingOption);

    for (QObject *c : ui->groupBox->children()) {
        QLabel *label = qobject_cast<QLabel *>(c);
        if (!label) continue;
        const QRegularExpressionMatch m = rx.match(label->text());
        if (!m.hasMatch()) continue;            // not an info row (e.g. the hint)
        const QString prefix = m.captured(1).trimmed();
        label->setText(QStringLiteral("<html><head/><body><p>%1 %2</p></body></html>")
                           .arg(prefix, link));
    }
}

bool PinTypeHelper::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::HoverEnter) {
        bool ok;
        pin_t pinType = obj->property("pinType").toInt(&ok);
        if (ok) {
            emit helpHovered(pinType, true);
        }
    } else if (event->type() == QEvent::HoverLeave) {
        bool ok;
        pin_t pinType = obj->property("pinType").toInt(&ok);
        if (ok) {
            emit helpHovered(pinType, false);
        }
    }
    return false;
}
