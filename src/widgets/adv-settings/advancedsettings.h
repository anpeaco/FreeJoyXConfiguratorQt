#ifndef ADVANCEDSETTINGS_H
#define ADVANCEDSETTINGS_H

#include "flasher.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
class QFile;
class QLabel;
class QPushButton;
class QCheckBox;
QT_END_NAMESPACE

namespace Ui {
class AdvancedSettings;
}

class AdvancedSettings : public QWidget
{
    Q_OBJECT

public:
    explicit AdvancedSettings(QWidget *parent = nullptr);
    ~AdvancedSettings();

    void setStyle(bool isDark);

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

    Flasher *flasher() const; // const?

    /* Single entry in the "other connected FreeJoy devices" list. Named
     * for the conflict pill so colliding sibling devices can be
     * identified by name, not just by count. Serial is included so
     * same-named siblings (the common case when the user has two
     * boards with identical device_name) get a disambiguating suffix
     * in the warning text. */
    struct OtherDevice {
        uint16_t vid;
        uint16_t pid;
        QString  name;
        QString  serialHex;
    };

    /* Push the latest "other connected FreeJoy devices" list into
     * AdvancedSettings so the PID input can flag collisions live.
     * The configurator's HidDevice worker is the source; MainWindow
     * forwards on every device-list rebuild. The list excludes the
     * currently-selected device so editing a device's PID doesn't
     * flag itself as a collision. */
    void setOtherConnectedDevices(const QList<OtherDevice> &devices);

    /* Push the currently-active save directory (the one MainWindow owns in
     * m_cfgDirPath) into the Default save directory line edit. Kept in sync
     * by MainWindow so the tab reflects out-of-band changes (e.g. the legacy
     * configs-dir icon button on the main window) without a restart. */
    void setSaveDirectory(const QString &path);

signals:
    void languageChanged(const QString &language);
    void themeChanged(bool dark);

    void fontChanged();

    /* User toggled "Auto-read config from device on connect" on this tab.
     * MainWindow caches the value (m_autoReadOnConnect) and consults it when
     * a device connects. The setting is also persisted to QSettings here, so
     * MainWindow's connect handler only needs to update its cached copy. */
    void autoReadOnConnectChanged(bool enabled);

    /* User clicked "Show all connected devices". MainWindow handles
     * it -- it has the full device list (including the selected
     * device, which AdvancedSettings's "other" snapshot excludes). */
    void showAllConnectedDevicesRequested();

    /* User picked a new default save directory (Browse or Reset on the
     * Advanced tab). MainWindow updates m_cfgDirPath + the configs combo;
     * the line edit on this widget is already updated by the time the signal
     * is emitted, so MainWindow doesn't need to call setSaveDirectory back. */
    void saveDirectoryChanged(const QString &path);

private slots:
    void on_pushButton_LangEnglish_clicked();
    void on_pushButton_LangRussian_clicked();
    void on_pushButton_LangSChinese_clicked();

    void on_spinBox_FontSize_valueChanged(int fontSize);
    void on_pushButton_About_clicked();

    void on_pushButton_removeName_clicked();

    void on_pushButton_RestartApp_clicked();

    void on_pushButton_LangDeutsch_clicked();

    /* Default-save-directory area: open the existing SelectFolder dialog
     * (pre-populated with the current path), reveal the path in the OS file
     * manager, or restore <Documents>/FreeJoy/configs. All three update the
     * line edit and emit saveDirectoryChanged() for MainWindow to act on. */
    void on_pushButton_SaveDirBrowse_clicked();
    void on_pushButton_SaveDirOpen_clicked();
    void on_pushButton_SaveDirReset_clicked();

private slots:
    void onPidTextChanged(const QString &);

private:
    Ui::AdvancedSettings *ui;

    Flasher *m_flasher;

    QString m_default_text;
    QString m_default_style;

    /* "Other connected FreeJoy devices" list, fed by MainWindow on
     * every device-list rebuild from HidDevice. Used by
     * onPidTextChanged to surface a live "PID in use by <name>" pill
     * on the PID input. */
    QList<OtherDevice> m_otherConnectedDevices;

    /* Refresh the conflict pill below lineEdit_PID against the current
     * VID+PID input vs. m_otherConnectedVidPids. Idempotent; safe to
     * call from textChanged or after any list update. */
    void refreshPidConflictPill();

    /* Dynamically-created widgets that visually surface the PID
     * conflict state. Created in the constructor and reparented into
     * the outer "USB settings" grid (gridLayout_3) so they span the
     * full group-box width. The pill row holds an icon + text in a
     * horizontal layout; the suggest button sits below it. */
    QWidget     *m_pidConflictRow   = nullptr;
    QLabel      *m_pidConflictIcon  = nullptr;
    QLabel      *m_pidConflictLabel = nullptr;

    /* Diagnostic button beneath the PID input -- always visible. Lets
     * the user dump the full connected-FreeJoy list when a phantom
     * conflict shows up they can't otherwise account for. */
    QPushButton *m_showAllDevicesButton = nullptr;

    /* "Auto-read config from device on connect" toggle. Added
     * programmatically into the "Other settings" group (gridLayout_7) so the
     * .ui grid doesn't need restructuring. Initial state + persistence live
     * under OtherSettings/AutoReadOnConnect; toggling emits
     * autoReadOnConnectChanged() for MainWindow. */
    QCheckBox *m_autoReadCheck = nullptr;
};

#endif // ADVANCEDSETTINGS_H
