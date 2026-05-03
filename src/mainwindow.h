#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QTranslator>

#include "hiddevice.h"
#include "reportconverter.h"

#include "advancedsettings.h"
#include "axesconfig.h"
#include "axescurvesconfig.h"
#include "buttonconfig.h"
#include "debugwindow.h"
#include "encodersconfig.h"
#include "ledconfig.h"
#include "pinconfig.h"
#include "shiftregistersconfig.h"
#include "shiftstimersconfig.h"
#include "switchbutton.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setDefaultStyleSheet();

signals:
    void getConfigDone(bool success);
    void sendConfigDone(bool success);

private slots:
    void showConnectDeviceInfo();
    void hideConnectDeviceInfo();
    void flasherConnected();
    //void getParamsPacket(uint8_t *buffer);
    void getParamsPacket(bool firmwareCompatible);

    void configReceived(bool success);
    void configSent(bool success);
    void blockWRConfigToDevice(bool block);

    void deviceFlasherController(bool isStartFlash);

    void hidDeviceList(const QList<QPair<bool, QString>> &deviceNames, int preferredIndex);
    void hidDeviceListChanged(int index);

    /* Receives USB identity (vid hex, pid hex, serial) for the currently
     * opened device from HidDevice and populates the device-info card
     * below the dropdown. Empty strings (on disconnect) reset the card
     * to "—". Firmware version is filled separately from getParamsPacket
     * since it arrives later via the params report. */
    void setDeviceInfo(const QString &vidHex, const QString &pidHex, const QString &serial);

    void languageChanged(const QString &language);
    void setFont();

    void finalInitialization();

    void on_pushButton_ResetAllPins_clicked();

    void on_pushButton_ReadConfig_clicked();
    void on_pushButton_WriteConfig_clicked();

    void on_pushButton_SaveToFile_clicked();
    void on_pushButton_LoadFromFile_clicked();

    void on_pushButton_ShowDebug_clicked();

    void on_pushButton_TestButton_clicked();
    void on_pushButton_TestButton_2_clicked();

    void on_pushButton_Wiki_clicked();

    void themeChanged(bool dark);

    void on_toolButton_ConfigsDir_clicked();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    Ui::MainWindow *ui;

    QThread *m_thread;
    HidDevice *m_hidDeviceWorker;

    QThread *m_threadGetSendConfig;

    PinConfig *m_pinConfig;
    ButtonConfig *m_buttonConfig;
    ShiftsTimersConfig *m_shiftsTimersConfig;
    LedConfig *m_ledConfig;
    EncodersConfig *m_encoderConfig;
    ShiftRegistersConfig *m_shiftRegConfig;
    AxesConfig *m_axesConfig;
    AxesCurvesConfig *m_axesCurvesConfig;
    AdvancedSettings *m_advSettings;

    DebugWindow *m_debugWindow = nullptr;
    bool m_debugIsEnable;

    bool m_deviceChanged;

    QString m_cfgDirPath;
    void curCfgFileChanged(const QString &fileName);
    QStringList cfgFilesList(const QString &dirPath);
    QIcon pixmapToIcon(QPixmap pixmap, const QColor &color);
    void updateColor();

    void UiReadFromConfig();
    void UiWriteToConfig();

    /* Returns true if every logical button slot using Function = Logic
     * has its operator and (for binary operators) Source B set to real
     * values rather than the "-" UI sentinels. On false, this also pops
     * a warning naming the first incomplete slot. */
    bool confirmLogicConfigComplete();

    void loadAppConfig();
    void saveAppConfig();
};
#endif // MAINWINDOW_H
