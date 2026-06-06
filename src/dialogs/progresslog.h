#ifndef PROGRESSLOG_H
#define PROGRESSLOG_H

/* Shared status-log sink for the flash / DFU-install progress surfaces.
 *
 * Both FlashProgressDialog and the DfuInstallDialog progress window show a
 * scrolling status log, and both want each line (a) timestamped [HH:mm:ss] in
 * the on-screen view and (b) mirrored to the on-disk log when file logging is
 * enabled. That logic was duplicated byte-for-byte; this is the single copy.
 *
 * (The two dialogs' widget trees are still separate -- unifying those into one
 * dialog class is a larger refactor; this just removes the duplicated log code.)
 */

#include <QDateTime>
#include <QPlainTextEdit>
#include <QString>

#include "global.h"
#include "widgets/debugwindow.h"

namespace progresslog {

// Append `line` to `view` with an [HH:mm:ss] prefix, and mirror the same line to
// the on-disk log via DebugWindow when file logging is enabled. No-op on an
// empty line or null view.
inline void append(QPlainTextEdit *view, const QString &line)
{
    if (view == nullptr || line.isEmpty()) {
        return;
    }
    const QString stamped = QStringLiteral("[%1] %2")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")), line);
    view->appendPlainText(stamped);
    if (gEnv.pDebugWindow) {
        gEnv.pDebugWindow->appendProgressLine(stamped);
    }
}

} // namespace progresslog

#endif // PROGRESSLOG_H
