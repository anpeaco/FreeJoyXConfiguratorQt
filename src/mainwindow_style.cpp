#include <QCheckBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "common_types.h"
#include "global.h"
#include "deviceconfig.h"
#include "style_helpers.h"
#include "boarddisplay.h"

#include <QDebug>
#include <QFile>
#include <QPalette>
#include <QToolTip>
#include <QWidget>

#include "windowthemehelper.h"
#include "pincombobox.h"

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

    // Apply the application stylesheet exactly ONCE. It is theme-INDEPENDENT now
    // (every theme colour comes from the QPalette set above; the dark/light.qss
    // files are empty and all accent tokens/icons in common.qss are
    // theme-neutral), so re-applying it on each toggle would only pay the
    // multi-second full-tree QSS repolish for an identical result. See
    // styles/dark.qss for the rule that keeps it apply-once.
    static bool s_styleSheetApplied = false;
    if (!s_styleSheetApplied) {
        const QString qss =
            loadQss(QStringLiteral(":/styles/common.qss")) +
            QStringLiteral("\n") +
            loadQss(QStringLiteral(":/styles/dark.qss"));
        qApp->setStyleSheet(freejoy_style::applyAccentTokens(qss, dark));
        s_styleSheetApplied = true;
    }

    // Refresh every widget against the new palette. This is the crux of the fast
    // theme swap: with a stylesheet active, QStyleSheetStyle resolves and caches
    // each widget's palette at apply-once time, so `qApp->setPalette` above does
    // NOT reach plain widgets (labels, checkboxes, tabs) -- they keep the old
    // theme's text/background and render mis-coloured. A per-widget
    // unpolish/polish re-runs QStyleSheetStyle's palette resolution against the
    // now-current app palette. Measured ~350 ms for the whole tree, versus
    // ~7.3 s to re-apply the stylesheet string (the only other way to force a
    // refresh -- that re-parses the QSS and re-lays-out everything). Widgets with
    // a PaletteChange handler (e.g. AxesCurvesConfig's green Ctrl button)
    // re-assert their own tint off the back of this.
    // Freeze painting on the top-level windows while we repolish so the ~350 ms
    // sweep lands as one repaint rather than a visible cascade of per-widget
    // recolours.
    const auto topLevels = QApplication::topLevelWidgets();
    for (QWidget *win : topLevels) {
        win->setUpdatesEnabled(false);
    }
    QStyle *st = qApp->style();
    const auto allWidgets = QApplication::allWidgets();
    for (QWidget *w : allWidgets) {
        st->unpolish(w);
        st->polish(w);
    }
    for (QWidget *win : topLevels) {
        win->setUpdatesEnabled(true);
    }
    // PinComboBox tints the pin-role text through its own palette
    // (ButtonText/Text); the repolish above resets it to the base palette, so
    // re-assert it afterwards. reapplyRoleColor() rebuilds the tint on top of the
    // now-correct base.
    const auto pinCombos = findChildren<PinComboBox *>();
    for (PinComboBox *pc : pinCombos) {
        pc->reapplyRoleColor();
    }

    // Re-tint every monochrome glyph icon to the new theme's ink. The palette
    // is already applied above, so iconInk() reads the correct colour. Reaches
    // all widgets (tabs + open dialogs) via qApp->allWidgets().
    freejoy_style::retintThemedIcons();

    // App-card theme toggle: show the glyph for the ACTIVE theme (moon = dark,
    // sun = light), tinted to the palette ink so it reads on either theme. The
    // tooltip names the action (what a click does). Set here so it tracks the
    // theme on startup and on every toggle. The icon path changes per theme, so
    // this can't ride retintThemedIcons() (which re-tints a fixed glyph).
    m_darkThemeActive = dark;
    ui->toolButton_ThemeToggle->setIcon(freejoy_style::tintedSvgIcon(
        dark ? QStringLiteral(":/Images/icons/lucide/moon.svg")
             : QStringLiteral(":/Images/icons/lucide/sun.svg"),
        QSize(20, 20), freejoy_style::iconInk()));
    ui->toolButton_ThemeToggle->setToolTip(
        dark ? tr("Switch to light theme") : tr("Switch to dark theme"));

    /* Re-render the Device card's board row: the F411 CPU-icon ink tracks the
     * theme, so its baked colour goes stale on a toggle. (F103 is theme-fixed
     * blue, but re-rendering is harmless.) */
    if (m_deviceCardBoardId != 0 && ui->label_BoardVal) {
        ui->label_BoardVal->setText(board_display::html(m_deviceCardBoardId));
    }

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
