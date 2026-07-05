#ifndef ENCODERCALIBRATEDIALOG_H
#define ENCODERCALIBRATEDIALOG_H

#include <QDialog>

class QLabel;
class QPushButton;
class QTimer;

/* Guided helper that measures an encoder's steps-per-detent from the firmware's
 * live decode monitor (params_report.enc_mon_*) and recommends the 1x / 2x / 4x
 * mode. Configurator-only -- it just reads counters the device already reports,
 * so no firmware/wire change is involved.
 *
 * Flow: ask detented-vs-smooth; for a detented encoder have the user turn
 * exactly 10 clicks and read the change in the "valid step" counter (immune to
 * sample-rate aliasing because it's a running total); map ~4/2/1 steps-per-click
 * to 1x/2x/4x; flag a noisy signal (un-decodable transitions) or a back-and-forth
 * turn. On Apply, chosenMode() returns the ENCODER_CONF_* index for the caller to
 * write to that encoder row. Smooth encoders skip measurement -> 4x. */
class EncoderCalibrateDialog : public QDialog
{
    Q_OBJECT
public:
    EncoderCalibrateDialog(int configIndex, int encoderNumber,
                           const QString &pinAName, const QString &pinBName,
                           QWidget *parent = nullptr);

    // Valid only after exec() == Accepted: the recommended ENCODER_CONF_* index.
    int chosenMode() const { return m_chosenMode; }
    // Recommended Queue-mode state to apply alongside the mode.
    bool chosenQueue() const { return m_chosenQueue; }

private slots:
    void onPrimary();
    void onSecondary();
    void onTick();

private:
    enum State { Branch, Measure, Result, Smooth };
    void setState(State s);
    void beginMeasure();   // reset the per-encoder accumulators at Measure start
    void accumulate();     // fold the newest counter delta in, if it's OUR encoder
    void computeResult();
    static QString modeName(int mode);

    int m_configIndex;
    int m_encoderNumber;
    QString m_pinA, m_pinB;

    State m_state = Branch;
    int m_chosenMode = -1;      // set on Apply
    bool m_chosenQueue = true;  // set on Apply
    int m_recMode = -1;         // measured recommendation
    bool m_recQueue = true;     // recommended Queue state

    // Per-encoder step accounting. The firmware's enc_mon_* counters are GLOBAL
    // (shared across encoders; enc_mon_slot names whoever last moved), so we
    // accumulate deltas ONLY while THIS encoder is the active one -- turning the
    // wrong encoder can't pollute the measurement. m_prev* = last raw poll.
    int m_prevValid = 0, m_prevNet = 0, m_prevJumps = 0;
    int m_accValid = 0,  m_accNet = 0,  m_accJumps = 0;
    bool m_wrongEncoder = false;   // last poll saw movement on a different encoder

    QLabel *m_instruction = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_btnPrimary = nullptr;
    QPushButton *m_btnSecondary = nullptr;
    QPushButton *m_btnCancel = nullptr;
    QTimer *m_poll = nullptr;
};

#endif // ENCODERCALIBRATEDIALOG_H
