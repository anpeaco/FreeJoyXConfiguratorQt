// Slice-1 test seam: stub the ButtonConfig methods PinConfig calls so the widget
// can be linked in isolation. These calls are all guarded by `if (m_buttonConfig)`
// (null in the harness) or only fire on a real conflict dialog -- so they never
// execute at runtime here; they only need to satisfy the linker.
//
// This file IS the decoupling checklist: PinConfig should depend on an interface
// for these, not the concrete ButtonConfig. (See PINCONFIG_MODEL_PLAN.md.)
#include "buttonconfig.h"

// Banner alignment lives in mainwindow_style.cpp, which drags in all of
// MainWindow. The busremapconfirmationdialog TU references it via the inline
// makeAlertBanner() helper, but no pin-logic test ever raises that banner --
// stub it so the harness links without the MainWindow cascade.
#include "style_helpers.h"
namespace freejoy_style {
void applyBannerLineAlignment(QFrame *, QLabel *, QLabel *, const QList<QWidget *> &) {}
}

void ButtonConfig::setDryRun(bool) {}
void ButtonConfig::setRemapWarningSuppressed(bool) {}
freejoy::PhysBreakdown ButtonConfig::currentBreakdown() const { return freejoy::PhysBreakdown(); }
QList<int> ButtonConfig::computeBrokenSlots(const PhysBreakdown &, const PhysBreakdown &) const { return QList<int>(); }
