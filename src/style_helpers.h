#pragma once

#include <QStyle>
#include <QVariant>
#include <QWidget>

namespace freejoy_style {

// Set a dynamic property on a widget and force a style refresh so the
// new value picks up matching QSS attribute-selector rules. Required
// because Qt only resolves [property="value"] selectors during a
// polish pass; changing the property at runtime needs an explicit
// unpolish/polish cycle.
inline void setRole(QWidget *w, const char *property, const QVariant &value)
{
    if (w == nullptr) {
        return;
    }
    w->setProperty(property, value);
    if (auto *s = w->style()) {
        s->unpolish(w);
        s->polish(w);
    }
    w->update();
}

// Clear a dynamic role so the widget reverts to its default QSS rules.
inline void clearRole(QWidget *w, const char *property)
{
    setRole(w, property, QVariant());
}

} // namespace freejoy_style
