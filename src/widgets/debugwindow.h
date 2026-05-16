#ifndef DEBUGWINDOW_H
#define DEBUGWINDOW_H

#include <QElapsedTimer>
#include <QWidget>

namespace Ui {
class DebugWindow;
}

class DebugWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DebugWindow(QWidget *parent = nullptr);
    ~DebugWindow();

    void retranslateUi();

    void devicePacketReceived();
    void resetPacketsCount();

    void logicalButtonState(int buttonNumber, bool state);

    Q_INVOKABLE // for multithreading -- unsure if correct, but works. CustomMessageHandler in main
        void printMsg(const QString &msg); // unsure about the reference; may need to receive a copy under multithreading

private slots:
    void on_checkBox_WriteLog_clicked(bool checked);
    void on_pushButton_LogMarker_clicked();

private:
    Ui::DebugWindow *ui;
    void buttonLogReset();

    int m_packetsCount;
    QElapsedTimer m_timer;
    bool m_writeToFile;
};

#endif // DEBUGWINDOW_H
