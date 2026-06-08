#include "advancedsettings.h"
#include "ui_advancedsettings.h"

//#include <QFile>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTextStream>
#include <QTimer>

#include <QDesktopServices>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>

#include "version.h"
#include "deviceconfig.h"
#include "global.h"
#include "style_helpers.h"
#include "selectfolder.h"

#include <QDebug>

AdvancedSettings::AdvancedSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AdvancedSettings)
{
    ui->setupUi(this);

    // info_VID / info_PID are InfoLabels carrying an OS-compatibility tooltip
    // (some PID values break on some OSes) -- they are NOT conflict indicators,
    // so a permanent amber warning triangle read as a false alarm. Render them
    // as a subtle grey info "i" glyph instead: an unobtrusive "hover for help"
    // affordance that stays put without implying a problem. A real VID:PID
    // collision is surfaced separately by the conflict banner below. Grey
    // (#909090) reads on both themes, so this set-once is enough.
    {
        const QPixmap infoGlyph = freejoy_style::tintedSvgPixmap(
            QStringLiteral(":/Images/icons/lucide/info.svg"), QSize(16, 16),
            QColor(0x90, 0x90, 0x90));
        ui->info_VID->setPixmap(infoGlyph);
        ui->info_PID->setPixmap(infoGlyph);
    }

    m_flasher = new Flasher(this);
    ui->layoutH_Flasher->addWidget(m_flasher);

    gEnv.pAppSettings->beginGroup("FontSettings");
    ui->spinBox_FontSize->setValue(gEnv.pAppSettings->value("FontSize", "8").toInt());
    gEnv.pAppSettings->endGroup();

    /* The theme toggle moved to the App card (MainWindow's
     * toolButton_ThemeToggle) -- a single sun/moon icon button. The startup
     * theme is still seeded from the saved StyleSheet setting by MainWindow. */

#ifndef Q_OS_WIN
    ui->text_removeName->setHidden(true);
    ui->pushButton_removeName->setHidden(true);
    ui->info_removeName->hide();
#endif

    ui->layoutG_Lang->setAlignment(Qt::AlignCenter);

    /* PID-conflict surface. Hosted on the OUTER "USB settings" group
     * box layout (gridLayout_3) so the pill spans the full width of
     * the group rather than being squeezed into the narrow VID/PID
     * column. The pill stays empty when no conflict; surfaces a
     * triangle-alert lucide icon + warning text when collisions exist. */
    /* Styled exactly like the app's other alert banners (makeAlertBanner):
     * translucent red fill + solid red border + a red triangle icon
     * top-aligned beside neutral palette text. The fill/border/icon carry the
     * severity, so the text is the normal ink, not red-bold. */
    auto *conflictFrame = new QFrame(this);
    conflictFrame->setObjectName(QStringLiteral("fjConflictBanner"));
    conflictFrame->setStyleSheet(QStringLiteral(
        "QFrame#fjConflictBanner { border-radius:4px; background-color:%1; "
        "border:1px solid %2; }")
        .arg(freejoy_style::rgbaStr(freejoy_style::accentRed(), 46),
             freejoy_style::hexStr(freejoy_style::accentRed())));
    m_pidConflictRow = conflictFrame;
    auto *rowLayout = new QHBoxLayout(m_pidConflictRow);
    rowLayout->setContentsMargins(10, 7, 10, 7);
    rowLayout->setSpacing(8);

    m_pidConflictIcon = new QLabel(m_pidConflictRow);
    m_pidConflictIcon->setFixedSize(18, 18);
    m_pidConflictIcon->setScaledContents(true);
    m_pidConflictIcon->setPixmap(
        freejoy_style::tintedTrianglePixmap(freejoy_style::accentRed(), 18));

    m_pidConflictLabel = new QLabel(m_pidConflictRow);
    m_pidConflictLabel->setText(QString());
    m_pidConflictLabel->setWordWrap(true);
    m_pidConflictLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    rowLayout->addWidget(m_pidConflictIcon, 0, Qt::AlignTop);
    rowLayout->addWidget(m_pidConflictLabel, 1);
    m_pidConflictRow->setVisible(false);

    m_showAllDevicesButton = new QPushButton(tr("Show all connected devices"), this);
    m_showAllDevicesButton->setToolTip(freejoy_style::tipHtml(
        tr("List connected USB identities"),
        { tr("Dumps every detected FreeJoy device's USB identity (VID:PID, name, serial)."),
          tr("Useful for diagnosing phantom PID conflicts.") }));
    connect(m_showAllDevicesButton, &QPushButton::clicked,
            this, &AdvancedSettings::showAllConnectedDevicesRequested);

    /* Attach to the OUTER "USB settings" VBox (layoutV_USBSettings). The
     * conflict banner goes LAST -- below the "Show all connected devices"
     * button -- so when it appears/disappears it only grows the bottom of the
     * group rather than shoving the VID/PID fields and buttons the user is
     * interacting with (the "controls jump about" complaint). */
    QVBoxLayout *outerVBox = findChild<QVBoxLayout *>(QStringLiteral("layoutV_USBSettings"));
    if (outerVBox) {
        outerVBox->addWidget(m_showAllDevicesButton, 0, Qt::AlignLeft);
        outerVBox->addWidget(m_pidConflictRow);
    }

    /* Live conflict check on every PID edit. */
    connect(ui->lineEdit_PID, &QLineEdit::textChanged,
            this, &AdvancedSettings::onPidTextChanged);

    /* Initial state -- pill hidden, button shown. */
    refreshPidConflictPill();

    /* Auto-read-on-connect toggle. Added programmatically (same approach as
     * m_showAllDevicesButton) into the "Other settings" group so the .ui grid
     * doesn't need restructuring. Persists to OtherSettings/AutoReadOnConnect
     * and signals MainWindow, which owns the connect-time behaviour. */
    m_autoReadCheck = new QCheckBox(tr("Auto-read config from device on connect"), this);
    m_autoReadCheck->setToolTip(freejoy_style::tipHtml(
        tr("Auto-read on connect"),
        { tr("When a compatible device connects, automatically reads its stored configuration into the configurator."),
          tr("If you have unsaved changes you'll be asked first."),
          tr("Turn off to manage reads manually.") }));
    gEnv.pAppSettings->beginGroup("OtherSettings");
    m_autoReadCheck->setChecked(
        gEnv.pAppSettings->value("AutoReadOnConnect", true).toBool());
    gEnv.pAppSettings->endGroup();
    if (auto *otherGrid = findChild<QGridLayout *>(QStringLiteral("gridLayout_7"))) {
        // Row after the theme switch / font size / About rows, spanning both
        // columns so the longer label isn't squeezed into one cell.
        otherGrid->addWidget(m_autoReadCheck, 3, 0, 1, 2, Qt::AlignHCenter);
    }
    connect(m_autoReadCheck, &QCheckBox::toggled, this, [this](bool on) {
        gEnv.pAppSettings->beginGroup("OtherSettings");
        gEnv.pAppSettings->setValue("AutoReadOnConnect", on);
        gEnv.pAppSettings->endGroup();
        emit autoReadOnConnectChanged(on);
    });

    /* "Write log to file" toggle -- moved out of the debug pane so it's
     * reachable without opening Show Debug. Same pattern as auto-read:
     * persists to OtherSettings/LogEnabled and signals MainWindow, which
     * forwards it to the DebugWindow logger. */
    m_writeLogCheck = new QCheckBox(tr("Write log to file"), this);
    m_writeLogCheck->setToolTip(freejoy_style::tipHtml(
        tr("Write debug log to file"),
        { tr("Appends the debug log to a dated file under Documents/FreeJoy/log/."),
          tr("Useful for capturing a bench session to review later.") }));
    gEnv.pAppSettings->beginGroup("OtherSettings");
    m_writeLogCheck->setChecked(gEnv.pAppSettings->value("LogEnabled", false).toBool());
    gEnv.pAppSettings->endGroup();
    if (auto *otherGrid = findChild<QGridLayout *>(QStringLiteral("gridLayout_7"))) {
        otherGrid->addWidget(m_writeLogCheck, 4, 0, 1, 2, Qt::AlignHCenter);
    }
    connect(m_writeLogCheck, &QCheckBox::toggled, this, [this](bool on) {
        gEnv.pAppSettings->beginGroup("OtherSettings");
        gEnv.pAppSettings->setValue("LogEnabled", on);
        gEnv.pAppSettings->endGroup();
        emit writeLogChanged(on);
    });
}

AdvancedSettings::~AdvancedSettings()
{
    delete ui;
}

void AdvancedSettings::retranslateUi()
{
    ui->retranslateUi(this);
    m_flasher->retranslateUi();
    // Programmatically-created widget: ui->retranslateUi doesn't touch it, so
    // refresh its strings here for live language switches.
    if (m_autoReadCheck) {
        m_autoReadCheck->setText(tr("Auto-read config from device on connect"));
        m_autoReadCheck->setToolTip(freejoy_style::tipHtml(
            tr("Auto-read on connect"),
            { tr("When a compatible device connects, automatically reads its stored configuration into the configurator."),
              tr("If you have unsaved changes you'll be asked first."),
              tr("Turn off to manage reads manually.") }));
    }
    if (m_writeLogCheck) {
        m_writeLogCheck->setText(tr("Write log to file"));
        m_writeLogCheck->setToolTip(freejoy_style::tipHtml(
            tr("Write debug log to file"),
            { tr("Appends the debug log to a dated file under Documents/FreeJoy/log/."),
              tr("Useful for capturing a bench session to review later.") }));
    }
}

Flasher *AdvancedSettings::flasher() const
{
    return m_flasher;
}

void AdvancedSettings::on_pushButton_LangEnglish_clicked()
{
    gEnv.pAppSettings->beginGroup("LanguageSettings");
    gEnv.pAppSettings->setValue("Language", "english");
    gEnv.pAppSettings->endGroup();

    emit languageChanged("english");
}

void AdvancedSettings::on_pushButton_LangRussian_clicked()
{
    gEnv.pAppSettings->beginGroup("LanguageSettings");
    gEnv.pAppSettings->setValue("Language", "russian");
    gEnv.pAppSettings->endGroup();

    emit languageChanged("russian");
}

void AdvancedSettings::on_pushButton_LangSChinese_clicked()
{
    gEnv.pAppSettings->beginGroup("LanguageSettings");
    gEnv.pAppSettings->setValue("Language", "schinese");
    gEnv.pAppSettings->endGroup();

    emit languageChanged("schinese");
}

void AdvancedSettings::on_pushButton_LangDeutsch_clicked()
{
    gEnv.pAppSettings->beginGroup("LanguageSettings");
    gEnv.pAppSettings->setValue("Language", "deutsch");
    gEnv.pAppSettings->endGroup();

    emit languageChanged("deutsch");
}

void AdvancedSettings::on_spinBox_FontSize_valueChanged(int fontSize)
{
    QFont defaultFont = QApplication::font();
    defaultFont.setPointSize(fontSize);
    qApp->setFont(defaultFont);

    emit fontChanged();
}


// about
void AdvancedSettings::on_pushButton_About_clicked()
{
    const QString version = QString("<p align=\"center\">%1 Configurator %2"
                                    "<br><span style='font-size:small;color:#999999'>"
                                    "An independent fork of FreeJoy by anpeaco, "
                                    "with a separate version line and feature roadmap."
                                    "</span></p>")
                                .arg(FORK_NAME, QStringLiteral(FREEJOYX_VERSION));
    const QString source = tr("<br>Built with Qt %1 (%2)<br>"
                              R"(Fork source on <a style="color: #03A9F4; text-decoration:none;"
                                href="https://github.com/anpeaco/FreeJoyXConfiguratorQt">GitHub</a>;
                                upstream <a style="color: #03A9F4; text-decoration:none;"
                                href="https://github.com/FreeJoy-Team/FreeJoyConfiguratorQt">FreeJoyConfiguratorQt</a>.<br>
                                Released under GPLv3.<br>)")
                               .arg(QT_VERSION_STR, QSysInfo::buildCpuArchitecture());

    const QString wiki(tr(R"(<br>See the upstream <a style="color: #03A9F4; text-decoration:none;"
                            href="https://github.com/FreeJoy-Team/FreeJoyWiki">FreeJoy wiki</a>
                            for detailed wiring and sensor instructions.)"));
    QMessageBox::about(this, tr("About %1 Configurator").arg(FORK_NAME), version + source + wiki);
}

// remove name from registry
void AdvancedSettings::on_pushButton_removeName_clicked()
{
#ifdef Q_OS_WIN
        qDebug()<<"Remove device OEMName from registry";
        QString path("HKEY_CURRENT_USER\\System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_%1&PID_%2");
        QString path2("HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet001\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_%1&PID_%2");
        QSettings(path.arg(QString::number(gEnv.pDeviceConfig->config.vid, 16), QString::number(gEnv.pDeviceConfig->config.pid, 16)),
                  QSettings::NativeFormat).remove("OEMName");
        QSettings(path2.arg(QString::number(gEnv.pDeviceConfig->config.vid, 16), QString::number(gEnv.pDeviceConfig->config.pid, 16)),
                  QSettings::NativeFormat).remove("OEMName");
#endif
}


void AdvancedSettings::readFromConfig()
{
    /* Block the live PID-conflict check while the load bulk-sets the fields:
     * the per-keystroke textChanged otherwise fires mid-load against a device
     * list that hasn't settled (the just-connected device isn't excluded from
     * the conflict set yet), flashing a false "VID:PID already used" banner that
     * clears once loading finishes. The pill is re-evaluated authoritatively by
     * MainWindow::UiReadFromConfig (via refreshOtherConnectedDevices ->
     * setOtherConnectedDevices) once the load is complete and the selected
     * device -- i.e. the one this config came from -- is correctly excluded. */
    const QSignalBlocker blockVid(ui->lineEdit_VID);
    const QSignalBlocker blockPid(ui->lineEdit_PID);

    // VID
    ui->lineEdit_VID->setText(QString::number(gEnv.pDeviceConfig->config.vid, 16).toUpper().rightJustified(4, '0'));
    // PID
    //ui->lineEdit_PID->setInputMask("HHHH");
    ui->lineEdit_PID->setText(QString::number(gEnv.pDeviceConfig->config.pid, 16).toUpper().rightJustified(4, '0'));
    // device name
    ui->lineEdit_DeviceUSBName->setText(gEnv.pDeviceConfig->config.device_name);
    // usb exchange period
    ui->spinBox_USBExchangePeriod->setValue(gEnv.pDeviceConfig->config.exchange_period_ms);
}

void AdvancedSettings::writeToConfig()
{
    // VID
    gEnv.pDeviceConfig->config.vid = uint16_t(ui->lineEdit_VID->text().toInt(nullptr, 16));
    // PID
    gEnv.pDeviceConfig->config.pid = uint16_t(ui->lineEdit_PID->text().toInt(nullptr, 16));
    // device name
    std::string tmp_string = ui->lineEdit_DeviceUSBName->text().toStdString();
    for (uint i = 0; i < sizeof(gEnv.pDeviceConfig->config.device_name); i++) {
        if (i < tmp_string.size()) {
            gEnv.pDeviceConfig->config.device_name[i] = tmp_string[i];
        } else {
            gEnv.pDeviceConfig->config.device_name[i] = '\0';
        }
    }
    // usb exchange period
    gEnv.pDeviceConfig->config.exchange_period_ms = uint8_t(ui->spinBox_USBExchangePeriod->value());
}

void AdvancedSettings::setOtherConnectedDevices(
    const QList<OtherDevice> &devices)
{
    m_otherConnectedDevices = devices;
    refreshPidConflictPill();
}

void AdvancedSettings::onPidTextChanged(const QString &)
{
    /* A user edit means the shown config is now intentionally "this device's"
     * again -- drop any swap-time suppression so the live check resumes. */
    m_suppressConflict = false;
    refreshPidConflictPill();
}

void AdvancedSettings::setPidConflictSuppressed(bool suppressed)
{
    m_suppressConflict = suppressed;
    /* Hide immediately when suppressing; un-suppressing is followed by the
     * caller's refreshOtherConnectedDevices(), which re-evaluates. */
    if (suppressed) refreshPidConflictPill();
}

void AdvancedSettings::refreshPidConflictPill()
{
    if (!m_pidConflictLabel) return;

    /* Suppressed mid-swap: the shown config doesn't belong to the selected
     * device yet, so any "conflict" would be against a stale PID. Keep the
     * banner down until the new config loads (or the user edits the PID). */
    if (m_suppressConflict) {
        m_pidConflictLabel->setText(QString());
        if (m_pidConflictRow) m_pidConflictRow->setVisible(false);
        ui->lineEdit_PID->setStyleSheet(QString());
        return;
    }

    const uint16_t curVid = uint16_t(ui->lineEdit_VID->text().toInt(nullptr, 16));
    const uint16_t curPid = uint16_t(ui->lineEdit_PID->text().toInt(nullptr, 16));

    /* Collect colliding siblings; append a short serial suffix when
     * names collide so two "F16 ICP" boards don't render as the same
     * string in the warning. The suffix takes the last 4 chars of the
     * serial -- enough to disambiguate visually without making the
     * warning text noisy. */
    QStringList conflictNames;
    for (const auto &d : m_otherConnectedDevices) {
        if (d.vid == curVid && d.pid == curPid) {
            QString label = d.name.isEmpty() ? tr("(unnamed)") : d.name;
            if (!d.serialHex.isEmpty()) {
                const QString tail = d.serialHex.right(4);
                label += QStringLiteral(" (#%1)").arg(tail);
            }
            conflictNames << label;
        }
    }

    if (!conflictNames.isEmpty()) {
        const QString joined = conflictNames.join(QStringLiteral(", "));
        m_pidConflictLabel->setText(tr(
            "This VID:PID is already used by: <b>%1</b>. "
            "Pick a unique PID to avoid Windows OEMName cache collisions and "
            "DirectInput confusion.").arg(joined));
        if (m_pidConflictRow) m_pidConflictRow->setVisible(true);
        ui->lineEdit_PID->setStyleSheet(
            QStringLiteral("border: 1px solid %1;").arg(freejoy_style::conflictColor().name()));
    } else {
        m_pidConflictLabel->setText(QString());
        if (m_pidConflictRow) m_pidConflictRow->setVisible(false);
        ui->lineEdit_PID->setStyleSheet(QString());
    }
}

// ---------------------------------------------------------------------------
// Default save directory
// ---------------------------------------------------------------------------
//
// Surface for the existing m_cfgDirPath mechanism in MainWindow: the line
// edit mirrors what MainWindow is currently using; Browse / Open folder /
// Reset all touch the line edit and emit saveDirectoryChanged() for
// MainWindow to update its own state. Configs land in <path>/, pre-flash
// device backups land in <path>/backups/.

namespace {

/* Default save directory: matches main.cpp's <Documents>/FreeJoy/configs.
 * Kept as a free helper so both the constructor (placeholder) and the Reset
 * button compute the same value without depending on AppSettings being
 * initialised in a particular order. */
QString defaultSaveDirectory()
{
    QString docLoc = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!docLoc.isEmpty()) docLoc += QStringLiteral("/FreeJoy/");
    return QDir::toNativeSeparators(docLoc + QStringLiteral("configs"));
}

} // namespace

void AdvancedSettings::setSaveDirectory(const QString &path)
{
    /* setText doesn't change the underlying value if the strings match, but
     * we run it through toNativeSeparators anyway so the displayed path
     * matches the platform convention regardless of how the caller phrased it. */
    ui->lineEdit_SaveDir->setText(QDir::toNativeSeparators(path));
}

void AdvancedSettings::on_pushButton_SaveDirBrowse_clicked()
{
    SelectFolder dlg(ui->lineEdit_SaveDir->text(), this);
    if (dlg.exec() != QDialog::Accepted) return;
    const QString picked = QDir::toNativeSeparators(dlg.folderPath());
    if (picked.isEmpty() || picked == ui->lineEdit_SaveDir->text()) return;
    ui->lineEdit_SaveDir->setText(picked);
    emit saveDirectoryChanged(dlg.folderPath());
}

void AdvancedSettings::on_pushButton_SaveDirOpen_clicked()
{
    const QString path = ui->lineEdit_SaveDir->text();
    if (path.isEmpty()) return;
    /* QDesktopServices::openUrl uses the user's default file manager. If the
     * directory doesn't exist yet, openUrl just fails silently -- create it
     * first so a freshly-reset path is browsable immediately. */
    QDir().mkpath(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void AdvancedSettings::on_pushButton_SaveDirReset_clicked()
{
    const QString def = defaultSaveDirectory();
    if (def == ui->lineEdit_SaveDir->text()) return;
    ui->lineEdit_SaveDir->setText(def);
    emit saveDirectoryChanged(def);
}
