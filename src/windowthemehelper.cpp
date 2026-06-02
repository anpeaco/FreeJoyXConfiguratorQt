#include "windowthemehelper.h"

#include <QEvent>
#include <QWidget>

#if defined Q_OS_WIN
    #include <windows.h>
    #include <dwmapi.h>
    #if defined _MSC_VER
        #pragma comment (lib, "Dwmapi.lib")
        // MinGW links Dwmapi via `LIBS += -ldwmapi` in src.pro instead.
    #endif

    namespace {
    // Documented DWM attribute ids (not always present in older MinGW headers):
    // 20 = DWMWA_USE_IMMERSIVE_DARK_MODE, 19 = the pre-20H1 spelling.
    enum : DWORD {
        kDwmwaUseImmersiveDarkMode        = 20,
        kDwmwaUseImmersiveDarkModeBefore  = 19
    };
    }
#endif

namespace {
bool g_darkActive = false;
}

namespace freejoy_style {

void applyDarkTitleBar(QWidget *w, bool dark)
{
#if defined Q_OS_WIN
    if (w == nullptr) {
        return;
    }
    const BOOL darkBorder = dark ? TRUE : FALSE;
    HWND hwnd = reinterpret_cast<HWND>(w->winId());
    if (FAILED(DwmSetWindowAttribute(hwnd, kDwmwaUseImmersiveDarkMode,
                                     &darkBorder, sizeof(darkBorder)))) {
        DwmSetWindowAttribute(hwnd, kDwmwaUseImmersiveDarkModeBefore,
                              &darkBorder, sizeof(darkBorder));
    }
#else
    Q_UNUSED(w);
    Q_UNUSED(dark);
#endif
}

void setDarkThemeActive(bool dark) { g_darkActive = dark; }
bool darkThemeActive() { return g_darkActive; }

bool TitleBarThemeFilter::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::Show) {
        if (auto *w = qobject_cast<QWidget *>(obj)) {
            // Only real top-level windows carry a title bar -- skip popups,
            // tooltips, combo dropdowns, etc.
            const Qt::WindowType t = w->windowType();
            if (w->isWindow() && (t == Qt::Window || t == Qt::Dialog)) {
                applyDarkTitleBar(w, g_darkActive);
            }
        }
    }
    return QObject::eventFilter(obj, ev);
}

} // namespace freejoy_style
