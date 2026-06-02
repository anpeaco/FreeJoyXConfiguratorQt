#include <QCheckBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "common_types.h"
#include "global.h"
#include "deviceconfig.h"
#include "style_helpers.h"

#include <QDebug>
#include <QFile>
#include <QToolTip>

#include "windowthemehelper.h"

// Read the contents of a QSS resource file (or return empty on failure).
// Caller appends or overrides as needed.
static QString loadQss(const QString &resourcePath)
{
    QFile f(resourcePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "themeChanged: failed to open" << resourcePath;
        return QString();
    }
    return QString::fromUtf8(f.readAll());
}

void MainWindow::themeChanged(bool dark)
{
    QString styleName;
    ui->tabWidget->setDocumentMode(true);

    if (dark == false)
    {
        QPalette pal(QColor(248, 248, 250));
        pal.setColor(QPalette::Window,           QColor(248, 248, 250));
        pal.setColor(QPalette::Base,             QColor(255, 255, 255));
        pal.setColor(QPalette::AlternateBase,    QColor(238, 240, 244));
        pal.setColor(QPalette::Button,           QColor(244, 244, 247));
        pal.setColor(QPalette::Mid,              QColor(204, 204, 204));
        pal.setColor(QPalette::Dark,             QColor(216, 216, 216));
        pal.setColor(QPalette::Disabled, QPalette::Button, QColor(232, 232, 235));
        pal.setColor(QPalette::Disabled, QPalette::Mid,    QColor(220, 220, 222));
        pal.setColor(QPalette::Text,             QColor(28, 30, 34));
        pal.setColor(QPalette::WindowText,       QColor(28, 30, 34));
        pal.setColor(QPalette::ButtonText,       QColor(28, 30, 34));
        pal.setColor(QPalette::Disabled, QPalette::Text,       QColor(150, 150, 150));
        pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(150, 150, 150));
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(150, 150, 150));
        pal.setColor(QPalette::ToolTipBase,      QColor(255, 255, 255));
        pal.setColor(QPalette::ToolTipText,      QColor(28, 30, 34));
        pal.setColor(QPalette::Highlight,        QColor(33, 118, 212));
        pal.setColor(QPalette::HighlightedText,  QColor(255, 255, 255));
        pal.setColor(QPalette::Disabled, QPalette::Highlight, QColor(200, 200, 200));
        pal.setColor(QPalette::Link,             QColor(33, 118, 212));
        QToolTip::setPalette(pal);
        qApp->setPalette(pal);

        styleName = "white";
    }
    else
    {
        QPalette pal;
        pal.setColor(QPalette::Window,           QColor(52, 54, 58));
        pal.setColor(QPalette::Base,             QColor(47, 48, 52));
        pal.setColor(QPalette::AlternateBase,    QColor(66, 67, 72));
        pal.setColor(QPalette::Button,           QColor(58, 60, 65));
        pal.setColor(QPalette::Mid,              QColor(80, 82, 88));
        // Midlight is the QTabBar::tab:hover background (common.qss). Without
        // setting it, Qt auto-derives a light value, washing out the near-white
        // tab text on hover. Keep it dark, a touch above the unselected tab fill.
        pal.setColor(QPalette::Midlight,         QColor(78, 80, 86));
        pal.setColor(QPalette::Dark,             QColor(40, 41, 45));
        pal.setColor(QPalette::Light,            QColor(80, 82, 88));
        pal.setColor(QPalette::Shadow,           QColor(20, 20, 20));
        pal.setColor(QPalette::Disabled, QPalette::Button, QColor(40, 41, 45));
        pal.setColor(QPalette::Disabled, QPalette::Base,   QColor(35, 36, 40));
        pal.setColor(QPalette::Disabled, QPalette::Mid,    QColor(60, 62, 66));
        pal.setColor(QPalette::Text,             QColor(230, 231, 232));
        pal.setColor(QPalette::WindowText,       QColor(230, 231, 232));
        pal.setColor(QPalette::ButtonText,       QColor(230, 231, 232));
        pal.setColor(QPalette::Disabled, QPalette::Text,       QColor(127, 127, 127));
        pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
        pal.setColor(QPalette::ToolTipBase,      QColor(60, 62, 66));
        pal.setColor(QPalette::ToolTipText,      QColor(230, 231, 232));
        pal.setColor(QPalette::BrightText,       Qt::red);
        pal.setColor(QPalette::Highlight,        QColor(38, 130, 226));
        pal.setColor(QPalette::HighlightedText,  QColor(255, 255, 255));
        pal.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
        pal.setColor(QPalette::Link,             QColor(38, 130, 226));
        QToolTip::setPalette(pal);
        qApp->setPalette(pal);

        styleName = "dark";
    }

    // Concatenate the structural QSS with the theme-specific overrides
    // and apply once at the application level. Single source of truth
    // for every styled widget; palette() refs in common.qss let palette
    // changes alone repaint most widgets without touching the stylesheet.
    const QString qss =
        loadQss(QStringLiteral(":/styles/common.qss")) +
        QStringLiteral("\n") +
        loadQss(dark ? QStringLiteral(":/styles/dark.qss")
                     : QStringLiteral(":/styles/light.qss"));
    // Resolve the semantic accent tokens (%accent-...%, gradients) to their
    // theme values before applying — single source of truth in style_helpers.h.
    qApp->setStyleSheet(freejoy_style::applyAccentTokens(qss, dark));

    // Re-tint every monochrome glyph icon to the new theme's ink. The palette
    // is already applied above, so iconInk() reads the correct colour. Reaches
    // all widgets (tabs + open dialogs) via qApp->allWidgets().
    freejoy_style::retintThemedIcons();

    // Track the active theme and re-skin every open top-level window's title
    // bar (MainWindow + any dialog already on screen). Dialogs opened later
    // pick it up via the TitleBarThemeFilter installed on qApp in main().
    freejoy_style::setDarkThemeActive(dark);
    const auto topWindows = QApplication::topLevelWidgets();
    for (QWidget *win : topWindows) {
        freejoy_style::applyDarkTitleBar(win, dark);
    }

    updateColor();

    gEnv.pAppSettings->beginGroup("StyleSettings");
    gEnv.pAppSettings->setValue("StyleSheet", styleName);
    gEnv.pAppSettings->endGroup();
}
