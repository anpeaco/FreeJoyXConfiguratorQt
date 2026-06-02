/**
  ******************************************************************************
  * @file           : dfuinstallsession.cpp
  * @brief          : QProcess driver for the `freejoyx-flash` DfuSe
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
const char *kHelperName = "freejoyx-flash.exe";
#else
const char *kHelperName = "freejoyx-flash";
#endif

/* Protocol record tags (see header). Kept as constants so the parser and
 * any future emitter test reference the same spelling. */
const QLatin1String kRecStage("STAGE");
const QLatin1String kRecProgress("PROGRESS");
const QLatin1String kRecLog("LOG");
const QLatin1String kRecError("ERROR");
const QLatin1String kRecProbe("PROBE");

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

void DfuInstallSession::probe(bool verbose)
{
    /* Coalesce: if a probe (or an install) is already running, skip. The
     * caller polls availability via the signal, so a missed probe just
     * means the next refresh re-asks. */
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        return;
    }
    const QString helper = helperPath();
    if (helper.isEmpty()) {
        emit availability(false);
        return;
    }

    m_probing = true;
    m_probeVerbose = verbose;
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
    if (verbose) probeArgs << QStringLiteral("--verbose");
    m_proc->start(helper, probeArgs);
}

bool DfuInstallSession::start(const Params &p)
{
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        qWarning() << "DfuInstallSession::start ignored -- already active";
        return false;
    }
    const QString helper = helperPath();
    if (helper.isEmpty()) {
        m_lastErrorDetail = tr("Install helper (freejoyx-flash) is missing "
                               "from the application folder.");
        return false;
    }
    if (p.bootBinPath.isEmpty() || p.appBinPath.isEmpty()) {
        m_lastErrorDetail = tr("Bootloader and application binaries are both required.");
        return false;
    }

    m_probing = false;
    m_probeVerbose = false;
    m_sawError = false;
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

    m_proc->start(helper, { QStringLiteral("install"),
                            QStringLiteral("--board"), p.board,
                            QStringLiteral("--boot"), p.bootBinPath,
                            QStringLiteral("--app"),  p.appBinPath });
    return true;
}

void DfuInstallSession::cancel()
{
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
        const bool present = (!crashed && exitCode == 0
                              && m_lastErrorDetail == QStringLiteral("present"));
        /* A user-driven re-check gets a closing line in the log so it never
         * looks like nothing happened -- and so a non-zero exit / crash (helper
         * couldn't run) is distinguishable from a clean "no board". */
        if (m_probeVerbose) {
            if (crashed) {
                emit logLine(tr("Re-check: helper crashed before reporting."));
            } else if (exitCode != 0) {
                emit logLine(tr("Re-check: helper exited with code %1.").arg(exitCode));
            } else {
                emit logLine(present
                    ? tr("Re-check: board present in DFU mode.")
                    : tr("Re-check: no board in DFU mode."));
            }
        }
        m_lastErrorDetail.clear();
        m_probing = false;
        m_probeVerbose = false;
        m_proc->deleteLater();
        m_proc = nullptr;
        emit availability(present);
        return;
    }

    const bool success = (!crashed && exitCode == 0 && m_stage == Stage::Verifying)
                         || (!crashed && exitCode == 0 && m_stage == Stage::Done);
    m_proc->deleteLater();
    m_proc = nullptr;

    if (success) {
        setStage(Stage::Done, tr("Firmware installed."));
        emit finished(true, tr("Bootloader and application written successfully. "
                               "Unplug and replug the board to start using it."));
        return;
    }

    if (!m_sawError) {
        /* Process died without an ERROR line (crash, or non-zero exit with no
         * diagnostic). Synthesise something the user can act on. */
        m_lastErrorDetail = crashed
            ? tr("The install helper stopped unexpectedly.")
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
            emit availability(false);
            return;
        }
        m_lastErrorDetail = tr("Couldn't launch the install helper (freejoyx-flash).");
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
