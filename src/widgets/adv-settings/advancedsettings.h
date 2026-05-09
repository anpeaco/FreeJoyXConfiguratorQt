#ifndef ADVANCEDSETTINGS_H
#define ADVANCEDSETTINGS_H

#include "flasher.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
class QFile;
class QLabel;
class QPushButton;
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

    /* Push the latest "other connected FreeJoy devices'" VID/PID list
     * into AdvancedSettings so the PID input can flag collisions live.
     * The configurator's HidDevice worker is the source; MainWindow
     * forwards on every device-list rebuild. The list excludes the
     * currently-selected device so editing a device's PID doesn't
     * flag itself as a collision. */
    void setOtherConnectedDevices(const QList<QPair<uint16_t, uint16_t>> &vidPids);

signals:
    void languageChanged(const QString &language);
    void themeChanged(bool dark);

    void fontChanged();

    /* Fired when the user clicks the "Suggest unique PID" button next
     * to the PID input. MainWindow handles it -- it has the connected-
     * device list and chooses a free slot in the FreeJoyX-reserved
     * range. */
    void suggestFreePidRequested();

private slots:
    void on_pushButton_LangEnglish_clicked();
    void on_pushButton_LangRussian_clicked();
    void on_pushButton_LangSChinese_clicked();

    void on_spinBox_FontSize_valueChanged(int fontSize);
    void on_pushButton_About_clicked();

    void on_pushButton_removeName_clicked();

    void on_pushButton_RestartApp_clicked();

    void on_pushButton_LangDeutsch_clicked();

private slots:
    void onPidTextChanged(const QString &);

private:
    Ui::AdvancedSettings *ui;

    Flasher *m_flasher;

    QString m_default_text;
    QString m_default_style;

    /* "Other connected FreeJoy devices" list, fed by MainWindow on
     * every device-list rebuild from HidDevice. Used by
     * onPidTextChanged to surface a live "PID in use by N device(s)"
     * pill on the PID input. */
    QList<QPair<uint16_t, uint16_t>> m_otherConnectedVidPids;

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
    QPushButton *m_suggestPidButton = nullptr;
};

#endif // ADVANCEDSETTINGS_H
