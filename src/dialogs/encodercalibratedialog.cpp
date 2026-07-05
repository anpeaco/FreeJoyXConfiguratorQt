#include "encodercalibratedialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

#include "global.h"          // gEnv
#include "deviceconfig.h"    // DeviceConfig::paramsReport (gEnv.pDeviceConfig)
#include "common_types.h"    // ENCODER_CONF_*, params_report_t
#include "common_defines.h"  // MAX_FAST_ENCODER_NUM

EncoderCalibrateDialog::EncoderCalibrateDialog(int configIndex, int encoderNumber,
                                               const QString &pinAName, const QString &pinBName,
                                               QWidget *parent)
    : QDialog(parent)
    , m_configIndex(configIndex)
    , m_encoderNumber(encoderNumber)
    , m_pinA(pinAName)
    , m_pinB(pinBName)
{
    setWindowTitle(tr("Calibrate Encoder %1").arg(encoderNumber));
    setModal(true);
    // Drop the useless "?" context-help button from the title bar (Windows adds
    // it to dialogs by default; it does nothing here).
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    QVBoxLayout *root = new QVBoxLayout(this);

    QLabel *title = new QLabel(tr("<b>Encoder %1 — find the right mode</b>").arg(encoderNumber), this);
    root->addWidget(title);

    m_instruction = new QLabel(this);
    m_instruction->setWordWrap(true);
    m_instruction->setMinimumWidth(400);
    m_instruction->setTextFormat(Qt::RichText);
    root->addWidget(m_instruction);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    m_status->setTextFormat(Qt::RichText);
    m_status->setMinimumHeight(48);
    root->addWidget(m_status);

    root->addStretch(1);

    QHBoxLayout *btns = new QHBoxLayout();
    btns->addStretch(1);
    m_btnSecondary = new QPushButton(this);
    m_btnPrimary   = new QPushButton(this);
    m_btnCancel    = new QPushButton(tr("Cancel"), this);
    btns->addWidget(m_btnSecondary);
    btns->addWidget(m_btnPrimary);
    btns->addWidget(m_btnCancel);
    root->addLayout(btns);

    connect(m_btnPrimary,   &QPushButton::clicked, this, &EncoderCalibrateDialog::onPrimary);
    connect(m_btnSecondary, &QPushButton::clicked, this, &EncoderCalibrateDialog::onSecondary);
    connect(m_btnCancel,    &QPushButton::clicked, this, &QDialog::reject);

    m_poll = new QTimer(this);
    m_poll->setInterval(50);
    connect(m_poll, &QTimer::timeout, this, &EncoderCalibrateDialog::onTick);
    m_poll->start();

    setState(Branch);
}

QString EncoderCalibrateDialog::modeName(int mode)
{
    switch (mode) {
    case ENCODER_CONF_1x: return QStringLiteral("1x");
    case ENCODER_CONF_2x: return QStringLiteral("2x");
    case ENCODER_CONF_4x: return QStringLiteral("4x");
    default:              return QStringLiteral("?");
    }
}

void EncoderCalibrateDialog::beginMeasure()
{
    const params_report_t &pr = gEnv.pDeviceConfig->paramsReport;
    m_prevValid = pr.enc_mon_valid;
    m_prevNet   = pr.enc_mon_net;
    m_prevJumps = pr.enc_mon_invalid;
    m_accValid = m_accNet = m_accJumps = 0;
    m_wrongEncoder = false;
}

void EncoderCalibrateDialog::accumulate()
{
    const params_report_t &pr = gEnv.pDeviceConfig->paramsReport;
    const int dv = int(pr.enc_mon_valid)   - m_prevValid;
    const int dn = int(pr.enc_mon_net)     - m_prevNet;
    const int dj = int(pr.enc_mon_invalid) - m_prevJumps;
    m_prevValid = pr.enc_mon_valid;
    m_prevNet   = pr.enc_mon_net;
    m_prevJumps = pr.enc_mon_invalid;

    // enc_mon_slot names the encoder that produced this movement. Only fold the
    // delta into our totals when it's THIS encoder -- so turning the wrong knob
    // (dv < 0 guards a counter reset/reconnect too) can't inflate the count.
    const bool moved = (dv > 0) || (dj > 0) || (dn != 0);
    const bool mine  = (pr.enc_mon_slot == (m_configIndex & 0xFF));
    m_wrongEncoder = moved && !mine;
    if (!mine) return;
    if (dv > 0) m_accValid += dv;
    if (dj > 0) m_accJumps += dj;
    m_accNet += dn;
}

void EncoderCalibrateDialog::setState(State s)
{
    m_state = s;
    switch (s) {
    case Branch:
        m_instruction->setText(tr("Does this encoder <b>click</b> as you turn it (has detents), "
                                  "or spin <b>smoothly</b> with no clicks?"));
        m_status->clear();
        m_btnSecondary->setText(tr("It's smooth"));
        m_btnSecondary->setVisible(true);
        m_btnPrimary->setEnabled(true);
        m_btnPrimary->setText(tr("It clicks"));
        m_btnPrimary->setDefault(true);
        break;

    case Measure:
        beginMeasure();
        m_instruction->setText(tr("Slowly turn <b>Encoder %1</b> exactly <b>10 clicks</b> in ONE "
                                  "direction, then press <b>Done</b>."
                                  "<br><span style=\"color:gray\">Pin A: %2 &nbsp;&nbsp; Pin B: %3</span>")
                               .arg(m_encoderNumber).arg(m_pinA, m_pinB));
        m_status->setText(tr("Waiting for movement…"));
        m_btnSecondary->setVisible(false);
        m_btnPrimary->setEnabled(true);
        m_btnPrimary->setText(tr("Done"));
        break;

    case Smooth:
        m_recMode = ENCODER_CONF_4x;
        m_recQueue = false;   // a smooth encoder has no clicks to queue
        m_instruction->setText(tr("A smooth encoder has no detent to lock onto, so use "
                                  "<b>4x</b> (full resolution)."));
        m_status->setText(tr("Queue mode: <b>Off</b> — there are no discrete clicks to "
                             "queue; the button simply follows the rotation."));
        m_btnSecondary->setVisible(false);
        m_btnPrimary->setEnabled(true);
        m_btnPrimary->setText(tr("Apply 4x, Queue off"));
        break;

    case Result:
        // m_instruction / m_status were filled by computeResult(); set buttons here.
        m_btnSecondary->setText(tr("Try again"));
        m_btnSecondary->setVisible(true);
        m_btnPrimary->setEnabled(m_recMode >= 0);
        m_btnPrimary->setText(m_recMode >= 0 ? tr("Apply %1, Queue on").arg(modeName(m_recMode))
                                             : tr("Apply"));
        break;
    }
}

void EncoderCalibrateDialog::onTick()
{
    if (m_state != Measure)
        return;

    accumulate();

    // Warn (but keep the running total intact) when the wrong encoder is turned;
    // its steps are deliberately NOT counted.
    if (m_wrongEncoder) {
        m_status->setText(tr("<span style=\"color:#c0392b\">⚠ That's a different encoder — its "
                             "turns are NOT being counted. Turn <b>Encoder %1</b> "
                             "(Pin A: %2, Pin B: %3).</span>")
                          .arg(m_encoderNumber).arg(m_pinA, m_pinB));
        return;
    }
    if (m_accValid == 0) {
        m_status->setText(tr("Waiting for movement…"));
        return;
    }
    m_status->setText(tr("Movement detected: <b>%1</b> steps — keep going to 10 clicks, then Done.")
                      .arg(m_accValid));
}

void EncoderCalibrateDialog::computeResult()
{
    accumulate();                 // fold in any final movement since the last poll
    const int steps = m_accValid; // all attributed to THIS encoder only
    const int net   = m_accNet;
    const int jumps = m_accJumps;

    m_instruction->setText(tr("Result for <b>Encoder %1</b>:").arg(m_encoderNumber));

    if (steps < 5) {
        m_recMode = -1;
        m_status->setText(tr("<span style=\"color:#c0392b\">Hardly any movement detected (%1 steps). "
                             "Check the device is connected and that you turned this encoder a full "
                             "10 clicks, then Try again.</span>").arg(steps));
        setState(Result);
        return;
    }

    const bool reversed = qAbs(net) < steps * 3 / 4;
    const double ratio = steps / 10.0;

    QString perClick;
    if (ratio >= 3.0)      { m_recMode = ENCODER_CONF_1x; perClick = tr("~4 steps per click"); }
    else if (ratio >= 1.5) { m_recMode = ENCODER_CONF_2x; perClick = tr("~2 steps per click"); }
    else                   { m_recMode = ENCODER_CONF_4x; perClick = tr("~1 step per click"); }

    // A detented encoder driving buttons wants Queue on: each click becomes one
    // clean, discrete press.
    m_recQueue = true;

    QString msg;
    if (jumps > qMax(2, steps / 6)) {
        msg += tr("<span style=\"color:#c0392b\">⚠ Noisy signal: %1 un-decodable transitions "
                  "detected — the encoder may have a dirty/failing channel or a wiring problem. The "
                  "suggestion below may be unreliable.</span><br><br>").arg(jumps);
    }
    if (reversed) {
        msg += tr("<span style=\"color:#c0392b\">It looks like the encoder was turned back and forth. "
                  "For the most accurate result, Try again turning steadily in ONE direction."
                  "</span><br><br>");
    }
    msg += tr("Measured <b>%1</b> steps over 10 clicks (%2).<br>"
              "Recommended mode: <b>%3</b>.<br>"
              "Queue mode: <b>On</b> — each click sends one clean, discrete press "
              "(best for stepping a value in a sim).")
           .arg(steps).arg(perClick, modeName(m_recMode));
    m_status->setText(msg);
    setState(Result);
}

void EncoderCalibrateDialog::onPrimary()
{
    switch (m_state) {
    case Branch:  setState(Measure);       break;
    case Measure: computeResult();         break;
    case Smooth:
        m_chosenMode = m_recMode; m_chosenQueue = m_recQueue; accept();
        break;
    case Result:
        if (m_recMode >= 0) { m_chosenMode = m_recMode; m_chosenQueue = m_recQueue; accept(); }
        break;
    }
}

void EncoderCalibrateDialog::onSecondary()
{
    switch (m_state) {
    case Branch: setState(Smooth);  break;
    case Result: setState(Measure); break;   // Try again
    default:                        break;
    }
}
