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

#if defined Q_OS_WIN && _MSC_VER
    #include <dwmapi.h>
    #pragma comment (lib,"Dwmapi.lib") // fixes error LNK2019: unresolved external symbol __imp__DwmExtendFrameIntoClientArea

    enum : WORD {
        DwmwaUseImmersiveDarkMode = 20,
        DwmwaUseImmersiveDarkModeBefore20h1 = 19
    };

    bool setDarkBorderToWindow(HWND hwnd, bool dark)
    {
        const BOOL darkBorder = dark ? TRUE : FALSE;
        const bool ok =
                SUCCEEDED(DwmSetWindowAttribute(hwnd, DwmwaUseImmersiveDarkMode, &darkBorder, sizeof(darkBorder)))
                || SUCCEEDED(DwmSetWindowAttribute(hwnd, DwmwaUseImmersiveDarkModeBefore20h1, &darkBorder, sizeof(darkBorder)));
        if (!ok) {
            qDebug()<<QString("%1: Unable to set %2 window border.").arg(__FUNCTION__, dark ? "dark" : "light");
        }
        return ok;
    }
#endif

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
#if defined Q_OS_WIN && _MSC_VER
        setDarkBorderToWindow((HWND)window()->winId(), false);
#endif
    }
    else
    {
        QPalette pal;
        pal.setColor(QPalette::Window,           QColor(52, 54, 58));
        pal.setColor(QPalette::Base,             QColor(47, 48, 52));
        pal.setColor(QPalette::AlternateBase,    QColor(66, 67, 72));
        pal.setColor(QPalette::Button,           QColor(58, 60, 65));
        pal.setColor(QPalette::Mid,              QColor(80, 82, 88));
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
#if defined Q_OS_WIN && _MSC_VER
        setDarkBorderToWindow((HWND)window()->winId(), true);
#endif
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

    ui->pushButton_Wiki->setIcon(QIcon(":/Images/icons/lucide/book-open.svg"));

    updateColor();

    gEnv.pAppSettings->beginGroup("StyleSettings");
    gEnv.pAppSettings->setValue("StyleSheet", styleName);
    gEnv.pAppSettings->endGroup();
}
