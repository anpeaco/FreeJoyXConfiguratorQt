/**
  ******************************************************************************
  * @file           : dfuinstallsession.cpp
  * @brief          : QProcess driver for the `freejoyx-dfu` DfuSe
  *                   install/reinstall helper. See dfuinstallsession.h for the
  *                   CLI + stdout contract this file implements against.
  ******************************************************************************
  */

#include "dfuinstallsession.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

namespace {

/* Helper binary basename, platform-suffixed. Bundled next to the
 * configurator executable by the build/release step. */
#ifdef Q_OS_WIN
const char *kHelperName = "freejoyx-dfu.exe";
#else
const char *kHelperName = "freejoyx-dfu";
#endif

/* Protocol record tags (see header). Kept as constants so the parser and
 * any future emitter test reference the same spelling. */
const QLatin1String kRecStage("STAGE");
const QLatin1String kRecProgress("PROGRESS");
const QLatin1String kRecLog("LOG");
const QLatin1String kRecVerify("VERIFY");
const QLatin1String kRecError("ERROR");
const QLatin1String kRecProbe("PROBE");

/* GATED: the current Rust helper doesn't emit the per-region `VERIFY boot|app
 * ok|fail` records yet (anpeaco/FreeJoyXConfigurator#35). Until it does,
 * success stays "reached the verify stage + exit 0". Flip this to true in
 * lockstep with the helper release that adds the read-back verify; success then
 * also requires BOTH regions to have reported `ok`. */
constexpr bool kHelperEmitsVerifyResults = false;

} // namespace


DfuInstallSession::DfuInstallSession(QObject *parent)
    : QObject(parent)
{
}

DfuInstallSession::~DfuInstallSession()
{
    /* QProcess is parented to this; if a process is still running on
     * teardown, kill it synchronously so we don't leak a child holding the
     * USB device open. */
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        qWarning() << "~DfuInstallSession -- session destroyed while helper still "
                      "running; killing it (this aborts an in-flight install)";
        m_proc->kill();
        m_proc->waitForFinished(2000);
    }
}

QString DfuInstallSession::helperPath()
{
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QString path = appDir.absoluteFilePath(QString::fromLatin1(kHelperName));
    return QFileInfo::exists(path) ? path : QString();
}

void DfuInstallSession::probe(bool verbose, bool checkDriver)
{
    /* Coalesce: if a probe (or an install) is already running, skip. The
     * caller polls availability via the signal, so a missed probe just
     * means the next refresh re-asks. */
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        return;
    }
    const QString helper = helperPath();
    if (helper.isEmpty()) {
        emit availability(Availability::Absent);
        return;
    }

    m_probing = true;
    m_probeVerbose = verbose;
    m_binding = false;
    m_sawError = false;
    m_stdoutBuf.clear();
    m_stderrBuf.clear();

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput,
            this, &DfuInstallSession::onReadyReadStdout);
    connect(m_proc, &QProcess::readyReadStandardError,
            this, &DfuInstallSession::onReadyReadStderr);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DfuInstallSession::onProcessFinished);
    connect(m_proc, &QProcess::errorOccurred,
            this, &DfuInstallSession::onProcessErrorOccurred);

    QStringList probeArgs{ QStringLiteral("probe"),
                           QStringLiteral("--board"), QStringLiteral("f411") };
    /* --verbose already implies the driver-layer check (and narrates); only add
     * --check-driver for the quiet background escalation case. */
    if (verbose) probeArgs << QStringLiteral("--verbose");
    else if (checkDriver) probeArgs << QStringLiteral("--check-driver");
    /* Pin the child's working directory to the helper's own folder. Otherwise it
     * inherits the app's CWD (e.g. Qt Creator's run dir), and Windows' legacy DLL
     * search includes the CWD -- a stray DLL there can be loaded into the helper
     * and crash it. Every reliable standalone run used the helper's folder as CWD. */
    m_proc->setWorkingDirectory(QFileInfo(helper).absolutePath());
    m_proc->start(helper, probeArgs);
}

void DfuInstallSession::installDriver()
{
    /* Coalesce against any in-flight probe/install/bind. */
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        return;
    }
    const QString helper = helperPath();
    if (helper.isEmpty()) {
        emit driverInstallFinished(false,
            tr("Install helper (freejoyx-dfu) is missing from the "
               "application folder."));
        return;
    }

    m_probing = false;
    m_probeVerbose = false;
    m_binding = true;
    m_sawError = false;
    m_lastErrorDetail.clear();
    m_stdoutBuf.clear();
    m_stderrBuf.clear();

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput,
            this, &DfuInstallSession::onReadyReadStdout);
    connect(m_proc, &QProcess::readyReadStandardError,
            this, &DfuInstallSession::onReadyReadStderr);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DfuInstallSession::onProcessFinished);
    connect(m_proc, &QProcess::errorOccurred,
            this, &DfuInstallSession::onProcessErrorOccurred);

    m_proc->setWorkingDirectory(QFileInfo(helper).absolutePath());
    m_proc->start(helper, { QStringLiteral("bind"),
                            QStringLiteral("--board"), QStringLiteral("f411") });
}

void DfuInstallSession::leaveDfu()
{
    /* Coalesce against any in-flight probe/install/bind/leave. */
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        return;
    }
    const QString helper = helperPath();
    if (helper.isEmpty()) {
        emit leaveFinished(false,
            tr("Install helper (freejoyx-dfu) is missing from the "
               "application folder."));
        return;
    }

    m_probing = false;
    m_probeVerbose = false;
    m_binding = false;
    m_leaving = true;
    m_sawError = false;
    m_lastErrorDetail.clear();
    m_stdoutBuf.clear();
    m_stderrBuf.clear();

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput,
            this, &DfuInstallSession::onReadyReadStdout);
    connect(m_proc, &QProcess::readyReadStandardError,
            this, &DfuInstallSession::onReadyReadStderr);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DfuInstallSession::onProcessFinished);
    connect(m_proc, &QProcess::errorOccurred,
            this, &DfuInstallSession::onProcessErrorOccurred);

    m_proc->setWorkingDirectory(QFileInfo(helper).absolutePath());
    m_proc->start(helper, { QStringLiteral("leave"),
                            QStringLiteral("--board"), QStringLiteral("f411") });
}

void DfuInstallSession::eraseChip()
{
    /* Coalesce against any in-flight probe/install/bind/leave/erase. */
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        return;
    }
    const QString helper = helperPath();
    if (helper.isEmpty()) {
        emit eraseFinished(false,
            tr("Install helper (freejoyx-dfu) is missing from the "
               "application folder."));
        return;
    }

    m_probing = false;
    m_probeVerbose = false;
    m_binding = false;
    m_leaving = false;
    m_erasing = true;
    m_sawError = false;
    m_lastErrorDetail.clear();
    m_stdoutBuf.clear();
    m_stderrBuf.clear();
    setStage(Stage::Erasing, tr("Erasing chip..."));

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput,
            this, &DfuInstallSession::onReadyReadStdout);
    connect(m_proc, &QProcess::readyReadStandardError,
            this, &DfuInstallSession::onReadyReadStderr);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DfuInstallSession::onProcessFinished);
    connect(m_proc, &QProcess::errorOccurred,
            this, &DfuInstallSession::onProcessErrorOccurred);

    m_proc->setWorkingDirectory(QFileInfo(helper).absolutePath());
    m_proc->start(helper, { QStringLiteral("erase"),
                            QStringLiteral("--board"), QStringLiteral("f411") });
}

bool DfuInstallSession::start(const Params &p)
{
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        qWarning() << "DfuInstallSession::start ignored -- already active";
        return false;
    }
    const QString helper = helperPath();
    if (helper.isEmpty()) {
        m_lastErrorDetail = tr("Install helper (freejoyx-dfu) is missing "
                               "from the application folder.");
        return false;
    }
    if (p.bootBinPath.isEmpty() && p.appBinPath.isEmpty()) {
        m_lastErrorDetail = tr("Select at least one of the bootloader / application to install.");
        return false;
    }

    m_probing = false;
    m_probeVerbose = false;
    m_binding = false;
    m_leaving = false;
    m_erasing = false;
    m_sawError = false;
    m_verifiedBoot = false;
    m_verifiedApp = false;
    m_lastErrorDetail.clear();
    m_stdoutBuf.clear();
    m_stderrBuf.clear();
    setStage(Stage::BindingDriver, tr("Preparing USB driver..."));

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput,
            this, &DfuInstallSession::onReadyReadStdout);
    connect(m_proc, &QProcess::readyReadStandardError,
            this, &DfuInstallSession::onReadyReadStderr);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DfuInstallSession::onProcessFinished);
    connect(m_proc, &QProcess::errorOccurred,
            this, &DfuInstallSession::onProcessErrorOccurred);

    /* --boot / --app are independently optional (the dialog's Boot/App/Both
     * selection). Send only the region(s) chosen; the helper writes just those
     * and, for a boot-only install, leaves the running app + its config intact. */
    QStringList args{ QStringLiteral("install"),
                      QStringLiteral("--board"), p.board };
    if (!p.bootBinPath.isEmpty())
        args << QStringLiteral("--boot") << p.bootBinPath;
    if (!p.appBinPath.isEmpty())
        args << QStringLiteral("--app") << p.appBinPath;

    /* DfuSe timing flags from the Advanced section. GATED: the current Rust
     * helper doesn't know these flags and would reject the whole command, so
     * they're only appended once the helper (anpeaco/FreeJoyXConfigurator#35)
     * accepts them. Flip kHelperSupportsTiming to true in lockstep with that
     * helper release. Until then the values are collected by the UI but not
     * sent (the helper uses its built-in == Normal defaults). */
    constexpr bool kHelperSupportsTiming = true;
    const Timing &t = p.timing;
    if (kHelperSupportsTiming) {
        args << QStringLiteral("--dnload-delay-ms")     << QString::number(t.dnloadDelayMs)
             << QStringLiteral("--poll-timeout-ms")     << QString::number(t.pollTimeoutMs)
             << QStringLiteral("--transfer-timeout-ms") << QString::number(t.transferTimeoutMs)
             << QStringLiteral("--retries")             << QString::number(t.retries)
             << QStringLiteral("--settle-ms")           << QString::number(t.settleMs)
             << QStringLiteral("--idle-confirmations")  << QString::number(t.idleConfirmations)
             << QStringLiteral("--min-block-ms")        << QString::number(t.minBlockMs);
    }

    /* Surface the chosen advanced timing in the log/debug output, and be honest
     * about whether it actually takes effect this build. */
    const QString timingMsg =
        QStringLiteral("DfuSe timing: dnload-delay=%1ms, poll-timeout=%2ms, "
                       "transfer-timeout=%3ms, retries=%4, settle=%5ms %6")
            .arg(t.dnloadDelayMs).arg(t.pollTimeoutMs).arg(t.transferTimeoutMs)
            .arg(t.retries).arg(t.settleMs)
            .arg(kHelperSupportsTiming
                     ? QStringLiteral("(applied)")
                     : QStringLiteral("(selected; helper uses built-in defaults this build)"));
    qInfo().noquote() << "[dfu-install]" << timingMsg;
    emit logLine(timingMsg);

    m_proc->setWorkingDirectory(QFileInfo(helper).absolutePath());
    m_proc->start(helper, args);
    return true;
}

void DfuInstallSession::cancel()
{
    qWarning() << "DfuInstallSession::cancel() -- killing helper (running:"
               << (m_proc && m_proc->state() != QProcess::NotRunning) << ")";
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        m_proc->kill();
    }
}

void DfuInstallSession::onReadyReadStdout()
{
    if (!m_proc) return;
    /* The helper may flush partial lines; buffer until we see a newline so a
     * record split across two reads parses as one. */
    m_stdoutBuf += QString::fromUtf8(m_proc->readAllStandardOutput());
    int nl;
    while ((nl = m_stdoutBuf.indexOf(QLatin1Char('\n'))) >= 0) {
        QString line = m_stdoutBuf.left(nl);
        m_stdoutBuf.remove(0, nl + 1);
        line.remove(QLatin1Char('\r'));     /* tolerate CRLF from the Windows build */
        if (!line.isEmpty()) {
            handleLine(line);
        }
    }
}

void DfuInstallSession::onReadyReadStderr()
{
    if (!m_proc) return;
    /* The helper writes its protocol to stdout; stderr only carries
     * out-of-band noise -- a Rust panic, a libwdi diagnostic, a dynamic-linker
     * "missing DLL" message. Historically this channel was dropped on the
     * floor, which is exactly why a helper that failed to start or panicked
     * looked like a silent "no board detected". Surface every line in the log
     * so those failures are visible. */
    m_stderrBuf += QString::fromUtf8(m_proc->readAllStandardError());
    int nl;
    while ((nl = m_stderrBuf.indexOf(QLatin1Char('\n'))) >= 0) {
        QString line = m_stderrBuf.left(nl);
        m_stderrBuf.remove(0, nl + 1);
        line.remove(QLatin1Char('\r'));
        if (!line.isEmpty()) {
            emit logLine(QStringLiteral("[helper] ") + line);
        }
    }
}

void DfuInstallSession::handleLine(const QString &line)
{
    const int sp = line.indexOf(QLatin1Char(' '));
    const QString tag = (sp < 0) ? line : line.left(sp);
    const QString rest = (sp < 0) ? QString() : line.mid(sp + 1);

    if (tag == kRecStage) {
        setStage(stageFromToken(rest.trimmed()));
    } else if (tag == kRecProgress) {
        const QStringList parts = rest.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            emit progress(parts[0].toLongLong(), parts[1].toLongLong());
        }
    } else if (tag == kRecLog) {
        emit logLine(rest);
    } else if (tag == kRecVerify) {
        /* `VERIFY <boot|app> <ok|fail>` -- per-region read-back result. Record
         * the pass flags (consumed by the success check on exit) and surface a
         * human line in the log either way. */
        const QStringList parts = rest.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        const QString region = parts.value(0).toLower();
        const bool ok = (parts.value(1).toLower() == QStringLiteral("ok"));
        if (region == QStringLiteral("boot")) m_verifiedBoot = ok;
        else if (region == QStringLiteral("app")) m_verifiedApp = ok;
        if (ok) {
            emit logLine(tr("Verify: %1 region read-back matches.").arg(region));
        } else {
            emit logLine(tr("Verify: %1 region read-back MISMATCH.").arg(region));
        }
    } else if (tag == kRecError) {
        /* `ERROR <code> <message>` -- keep the whole remainder as the detail
         * so the dialog shows the helper's own wording. */
        m_sawError = true;
        m_lastErrorDetail = rest.trimmed();
    } else if (tag == kRecProbe) {
        /* Recorded here; the present/absent verdict is emitted on process
         * exit so availability() fires once with a settled result. */
        m_lastErrorDetail = rest.trimmed();   /* reuse field to stash present/absent */
    } else {
        /* Unknown record -- surface it in the log rather than dropping it,
         * so a contract drift between helper and app is visible. */
        emit logLine(line);
    }
}

void DfuInstallSession::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    const bool crashed = (status == QProcess::CrashExit);

    if (m_probing) {
        /* Map the stashed PROBE token to the tri-state verdict. A crash or
         * non-zero exit is treated as Absent (the helper couldn't report). */
        Availability avail = Availability::Absent;
        if (!crashed && exitCode == 0) {
            if (m_lastErrorDetail == QStringLiteral("present")) {
                avail = Availability::Ready;
            } else if (m_lastErrorDetail == QStringLiteral("needs-driver")) {
                avail = Availability::NeedsDriver;
            }
        }
        /* A user-driven re-check gets a closing line in the log so it never
         * looks like nothing happened -- and so a non-zero exit / crash (helper
         * couldn't run) is distinguishable from a clean "no board". */
        if (m_probeVerbose) {
            if (crashed) {
                emit logLine(tr("Re-check: helper crashed before reporting."));
            } else if (exitCode != 0) {
                emit logLine(tr("Re-check: helper exited with code %1.").arg(exitCode));
            } else {
                switch (avail) {
                case Availability::Ready:
                    emit logLine(tr("Re-check: board present in DFU mode.")); break;
                case Availability::NeedsDriver:
                    emit logLine(tr("Re-check: board found, but its USB driver "
                                    "needs installing.")); break;
                case Availability::Absent:
                    emit logLine(tr("Re-check: no board in DFU mode.")); break;
                }
            }
        }
        m_lastErrorDetail.clear();
        m_probing = false;
        m_probeVerbose = false;
        m_proc->deleteLater();
        m_proc = nullptr;
        emit availability(avail);
        return;
    }

    if (m_binding) {
        const bool ok = (!crashed && exitCode == 0);
        if (!ok && !m_sawError) {
            m_lastErrorDetail = crashed
                ? tr("The driver install stopped unexpectedly.")
                : tr("The driver install exited with code %1.").arg(exitCode);
        }
        const QString detail = ok ? tr("WinUSB driver step completed.")
                                  : m_lastErrorDetail;
        m_binding = false;
        m_proc->deleteLater();
        m_proc = nullptr;
        emit driverInstallFinished(ok, detail);
        return;
    }

    if (m_leaving) {
        const bool ok = (!crashed && exitCode == 0);
        if (!ok && !m_sawError) {
            m_lastErrorDetail = crashed
                ? tr("The DFU-leave helper stopped unexpectedly.")
                : tr("The DFU-leave helper exited with code %1.").arg(exitCode);
        }
        const QString detail = ok
            ? tr("Sent DFU-leave; the board should reboot into its firmware.")
            : m_lastErrorDetail;
        m_leaving = false;
        m_proc->deleteLater();
        m_proc = nullptr;
        emit leaveFinished(ok, detail);
        return;
    }

    if (m_erasing) {
        const bool ok = (!crashed && exitCode == 0);
        if (!ok && !m_sawError) {
            m_lastErrorDetail = crashed
                ? tr("The erase helper stopped unexpectedly.")
                : tr("The erase helper exited with code %1.").arg(exitCode);
        }
        const QString detail = ok
            ? tr("Chip erased. The board is blank and still in DFU -- install "
                 "firmware to make it usable.")
            : m_lastErrorDetail;
        m_erasing = false;
        m_proc->deleteLater();
        m_proc = nullptr;
        emit eraseFinished(ok, detail);
        return;
    }

    bool success = (!crashed && exitCode == 0 && m_stage == Stage::Verifying)
                   || (!crashed && exitCode == 0 && m_stage == Stage::Done);

    /* Once the helper emits per-region read-back results, a clean exit isn't
     * enough -- BOTH the bootloader and the app region must have verified ok.
     * Gated until the helper supports it (see kHelperEmitsVerifyResults). */
    if (success && kHelperEmitsVerifyResults && !(m_verifiedBoot && m_verifiedApp)) {
        success = false;
        if (!m_sawError) {
            QStringList failed;
            if (!m_verifiedBoot) failed << tr("bootloader");
            if (!m_verifiedApp)  failed << tr("application");
            m_lastErrorDetail = tr("Write completed but read-back verification "
                                   "failed for the %1 region. The flash may be "
                                   "corrupt -- re-run the install.")
                                    .arg(failed.join(tr(" and ")));
        }
    }

    m_proc->deleteLater();
    m_proc = nullptr;

    if (success) {
        setStage(Stage::Done, tr("Firmware installed."));
        emit finished(true, tr("Bootloader and application written successfully. "
                               "Unplug and replug the board to start using it."));
        return;
    }

    if (!m_sawError) {
        /* Process died without an ERROR line. Include the raw code so we can tell
         * a real native crash (e.g. 0xC0000005 access violation) from the app
         * having terminated it (Qt's QProcess::kill uses 0xF291) -- the latter
         * means the dialog/session was destroyed mid-write, not a helper bug. */
        const QString hexCode =
            QStringLiteral("0x") + QString::number(static_cast<quint32>(exitCode), 16).toUpper();
        m_lastErrorDetail = crashed
            ? tr("The install helper stopped unexpectedly (code %1).").arg(hexCode)
            : tr("The install helper exited with code %1.").arg(exitCode);
    }
    setStage(Stage::Failed, m_lastErrorDetail);
    emit finished(false, m_lastErrorDetail);
}

void DfuInstallSession::onProcessErrorOccurred(QProcess::ProcessError error)
{
    /* FailedToStart is the common one (helper not executable / blocked). The
     * finished handler won't fire for FailedToStart, so emit the terminal
     * result here for that case; other errors still funnel through finished. */
    if (error == QProcess::FailedToStart) {
        if (m_probing) {
            /* The probe normally fails silently (the periodic poll would spam
             * otherwise), but FailedToStart means the helper binary itself
             * couldn't launch -- blocked by SmartScreen/AV, missing runtime
             * DLL, not executable. That's never "no board"; always surface it
             * so the user/log shows the real reason rather than a misleading
             * "not detected". */
            emit logLine(tr("Couldn't launch the install helper (%1): %2")
                             .arg(helperPath(),
                                  m_proc ? m_proc->errorString() : tr("unknown error")));
            m_probing = false;
            m_probeVerbose = false;
            if (m_proc) { m_proc->deleteLater(); m_proc = nullptr; }
            emit availability(Availability::Absent);
            return;
        }
        if (m_binding) {
            emit logLine(tr("Couldn't launch the install helper (%1): %2")
                             .arg(helperPath(),
                                  m_proc ? m_proc->errorString() : tr("unknown error")));
            m_binding = false;
            if (m_proc) { m_proc->deleteLater(); m_proc = nullptr; }
            emit driverInstallFinished(false,
                tr("Couldn't launch the install helper (freejoyx-dfu)."));
            return;
        }
        if (m_leaving) {
            m_leaving = false;
            if (m_proc) { m_proc->deleteLater(); m_proc = nullptr; }
            emit leaveFinished(false,
                tr("Couldn't launch the install helper (freejoyx-dfu)."));
            return;
        }
        if (m_erasing) {
            m_erasing = false;
            if (m_proc) { m_proc->deleteLater(); m_proc = nullptr; }
            emit eraseFinished(false,
                tr("Couldn't launch the install helper (freejoyx-dfu)."));
            return;
        }
        m_lastErrorDetail = tr("Couldn't launch the install helper (freejoyx-dfu).");
        m_sawError = true;
        if (m_proc) { m_proc->deleteLater(); m_proc = nullptr; }
        setStage(Stage::Failed, m_lastErrorDetail);
        emit finished(false, m_lastErrorDetail);
    }
}

DfuInstallSession::Stage DfuInstallSession::stageFromToken(const QString &token)
{
    if (token == QStringLiteral("bind-driver")) return Stage::BindingDriver;
    if (token == QStringLiteral("erase"))       return Stage::Erasing;
    if (token == QStringLiteral("write-boot"))  return Stage::WritingBootloader;
    if (token == QStringLiteral("write-app"))   return Stage::WritingApp;
    if (token == QStringLiteral("verify"))      return Stage::Verifying;
    if (token == QStringLiteral("done"))        return Stage::Done;
    return Stage::Idle;     /* unknown -> no transition of substance */
}

void DfuInstallSession::setStage(Stage s, const QString &detail)
{
    m_stage = s;
    emit stageChanged(s, detail);
}
