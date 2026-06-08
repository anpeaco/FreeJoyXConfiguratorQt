#ifndef DEBUGWINDOW_H
#define DEBUGWINDOW_H

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

    /* Severity / category of a log line. Drives the colour each line is
     * rendered in and the [TAG] it carries. App messages map here from the
     * QtMsgType in main.cpp's CustomMessageHandler; button events and the
     * marker get their own categories. */
    enum class LogLevel { Debug, Info, Warn, Error, Button, Marker };

    void retranslateUi();

    void logicalButtonState(int buttonNumber, bool state);
    void physicalButtonState(int buttonNumber, bool state);

    /* Toggled from Advanced Settings (the checkbox moved out of this widget).
     * Gates appendToLogFile; the persisted OtherSettings/LogEnabled is read on
     * construction so logging is correct whether or not this pane was opened. */
    void setWriteToFile(bool on);

    /* Mirror an already-formatted (timestamped) line from the flash / DFU
     * install progress dialogs into the on-disk log, honouring the same
     * OtherSettings/LogEnabled gate the app log uses. The caller supplies the
     * line; this adds the trailing newline. No-op when file logging is off. */
    void appendProgressLine(const QString &line);

    Q_INVOKABLE // for multithreading -- CustomMessageHandler in main posts here
        void printMsg(const QString &msg, int level = int(LogLevel::Info));

private slots:
    void on_pushButton_LogMarker_clicked();
    void on_pushButton_LogClear_clicked();

private:
    Ui::DebugWindow *ui;

    /* Single sink for every log line: stamps, tags + colours by level, appends
     * to the combined view, and mirrors the plain text to the on-disk log. */
    void appendLine(LogLevel level, const QString &msg);
    void appendToLogFile(const QString &line);

    bool m_writeToFile;
};

#endif // DEBUGWINDOW_H
