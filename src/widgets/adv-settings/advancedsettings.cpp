#include "advancedsettings.h"
#include "ui_advancedsettings.h"

//#include <QFile>
#include <QComboBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTextStream>
#include <QTimer>
#include <QProcess>

#include "version.h"
#include "deviceconfig.h"
#include "global.h"

#include <QDebug>

AdvancedSettings::AdvancedSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AdvancedSettings)
{
    ui->setupUi(this);

    m_flasher = new Flasher(this);
    ui->layoutH_Flasher->addWidget(m_flasher);

    gEnv.pAppSettings->beginGroup("FontSettings");
    ui->spinBox_FontSize->setValue(gEnv.pAppSettings->value("FontSize", "8").toInt());
    gEnv.pAppSettings->endGroup();

    gEnv.pAppSettings->beginGroup("StyleSettings");
    QString style = gEnv.pAppSettings->value("StyleSheet", "default").toString();
    gEnv.pAppSettings->endGroup();
    if (style == "dark") {
        ui->widget_StyleSwitch->setChecked(true);
    } else {
        ui->widget_StyleSwitch->setChecked(false);
    }
    connect(ui->widget_StyleSwitch, &SwitchButton::stateChanged, this, &AdvancedSettings::themeChanged);

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
    m_pidConflictRow = new QWidget(this);
    auto *rowLayout = new QHBoxLayout(m_pidConflictRow);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(8);

    m_pidConflictIcon = new QLabel(m_pidConflictRow);
    /* The lucide SVG renders to a pixmap at the host font's height for
     * crisp visual pairing with the text. The pixmap is recoloured by
     * pixmapToIcon-equivalent painting so it matches the warning hue
     * regardless of theme. We paint once during construction; refresh
     * just toggles visibility. */
    {
        const int h = qMax(16, fontMetrics().height());
        QPixmap pix(":/Images/icons/lucide/triangle-alert.svg");
        if (!pix.isNull()) {
            pix = pix.scaledToHeight(h, Qt::SmoothTransformation);
            QPixmap colored(pix.size());
            colored.fill(Qt::transparent);
            QPainter p(&colored);
            p.setCompositionMode(QPainter::CompositionMode_Source);
            p.drawPixmap(0, 0, pix);
            p.setCompositionMode(QPainter::CompositionMode_SourceIn);
            p.fillRect(colored.rect(), QColor(0xCC, 0x33, 0x33));
            p.end();
            m_pidConflictIcon->setPixmap(colored);
        }
    }

    m_pidConflictLabel = new QLabel(m_pidConflictRow);
    m_pidConflictLabel->setText(QString());
    m_pidConflictLabel->setStyleSheet(QStringLiteral(
        "color: #cc3333; font-weight: bold;"));
    m_pidConflictLabel->setWordWrap(true);
    m_pidConflictLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    rowLayout->addWidget(m_pidConflictIcon, 0, Qt::AlignTop);
    rowLayout->addWidget(m_pidConflictLabel, 1);
    m_pidConflictRow->setVisible(false);

    m_suggestPidButton = new QPushButton(tr("Suggest unique PID"), this);
    m_suggestPidButton->setToolTip(tr(
        "Pick a free PID from the FreeJoyX-reserved range that doesn't "
        "collide with any currently-connected device."));
    connect(m_suggestPidButton, &QPushButton::clicked,
            this, &AdvancedSettings::suggestFreePidRequested);

    /* Attach to the OUTER "USB settings" grid (gridLayout_3) which has
     * 7 columns; gridLayout_2 inside it is the narrow VID/PID column.
     * We span all columns so the warning text gets the full group-box
     * width to wrap into. */
    QGridLayout *outerGrid = findChild<QGridLayout *>(QStringLiteral("gridLayout_3"));
    if (outerGrid) {
        const int row = outerGrid->rowCount();
        outerGrid->addWidget(m_pidConflictRow,    row,     0, 1, outerGrid->columnCount());
        outerGrid->addWidget(m_suggestPidButton,  row + 1, 0, 1, outerGrid->columnCount(),
                             Qt::AlignLeft);
    }

    /* Live conflict check on every PID edit. */
    connect(ui->lineEdit_PID, &QLineEdit::textChanged,
            this, &AdvancedSettings::onPidTextChanged);

    /* Initial state -- pill hidden, button shown. */
    refreshPidConflictPill();
}

AdvancedSettings::~AdvancedSettings()
{
    delete ui;
}

void AdvancedSettings::retranslateUi()
{
    ui->retranslateUi(this);
    m_flasher->retranslateUi();
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

void AdvancedSettings::on_pushButton_RestartApp_clicked()
{
    qApp->quit();
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
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
    const QString version = QString("<p align=\"center\">%1 Configurator v%2 "
                                    "(fork of FreeJoy v%3)")
                                .arg(FORK_NAME, FORK_VERSION, QStringLiteral(APP_VERSION));
    const QString source = tr("<br>Built with Qt %1 (%2)<br>"
                              R"(Fork source on <a style="color: #03A9F4; text-decoration:none;"
                                href="https://github.com/anpeaco/FreeJoyConfiguratorQtX">GitHub</a>;
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
    // PID
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
    const QList<QPair<uint16_t, uint16_t>> &vidPids)
{
    m_otherConnectedVidPids = vidPids;
    refreshPidConflictPill();
}

void AdvancedSettings::onPidTextChanged(const QString &)
{
    refreshPidConflictPill();
}

void AdvancedSettings::refreshPidConflictPill()
{
    if (!m_pidConflictLabel || !m_suggestPidButton) return;

    const uint16_t curVid = uint16_t(ui->lineEdit_VID->text().toInt(nullptr, 16));
    const uint16_t curPid = uint16_t(ui->lineEdit_PID->text().toInt(nullptr, 16));

    int conflicts = 0;
    for (const auto &vp : m_otherConnectedVidPids) {
        if (vp.first == curVid && vp.second == curPid) ++conflicts;
    }

    if (conflicts > 0) {
        m_pidConflictLabel->setText(tr(
            "This VID:PID is already used by %1 other connected device%2. "
            "Pick a unique PID to avoid Windows OEMName cache collisions and "
            "DirectInput confusion.")
                .arg(conflicts).arg(conflicts == 1 ? "" : "s"));
        if (m_pidConflictRow) m_pidConflictRow->setVisible(true);
        ui->lineEdit_PID->setStyleSheet(QStringLiteral(
            "border: 1px solid #cc3333;"));
    } else {
        m_pidConflictLabel->setText(QString());
        if (m_pidConflictRow) m_pidConflictRow->setVisible(false);
        ui->lineEdit_PID->setStyleSheet(QString());
    }
}
