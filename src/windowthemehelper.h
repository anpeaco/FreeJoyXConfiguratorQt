#ifndef WINDOWTHEMEHELPER_H
#define WINDOWTHEMEHELPER_H

#include <QObject>

class QWidget;
class QEvent;

// Applies the OS dark/light title-bar treatment (Windows DWM immersive dark
// mode) to top-level windows. MainWindow used to own this for itself only;
// these helpers extend it to every dialog so headers track the theme too.
//
// On non-Windows platforms the apply call is a no-op (the window manager owns
// the frame).
namespace freejoy_style {

// Apply the dark/light title bar to a single top-level window now.
void applyDarkTitleBar(QWidget *w, bool dark);

// Remember the active theme so windows shown later can self-apply via the
// event filter below.
void setDarkThemeActive(bool dark);
bool darkThemeActive();

// Install one instance on qApp: it applies the active theme's title bar to
// every Window / Dialog as it is first shown. Catches modal dialogs that are
// created on demand (DfuInstall, Flash confirmation, folder picker, ...).
class TitleBarThemeFilter : public QObject
{
public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
};

} // namespace freejoy_style

#endif // WINDOWTHEMEHELPER_H
