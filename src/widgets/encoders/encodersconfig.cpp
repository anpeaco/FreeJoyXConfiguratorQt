#include "encodersconfig.h"
#include "ui_encodersconfig.h"

#include <QSet>

#include "common_defines.h"   // MAX_ENCODERS_NUM, MAX_FAST_ENCODER_NUM, MAX_BUTTONS_NUM
#include "common_types.h"     // ENCODER_INPUT_A

EncodersConfig::EncodersConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EncodersConfig)
{
    ui->setupUi(this);

    // spawn fast-encoder rows -- one per fast encoder slot.
    ui->layoutV_FastEncoders->setAlignment(Qt::AlignTop);
    for (int i = 0; i < MAX_FAST_ENCODER_NUM; ++i) {
        FastEncoder *fe = new FastEncoder(i, this);
        ui->layoutV_FastEncoders->addWidget(fe);
        m_fastEncodersPtrList.append(fe);
        connect(fe, &FastEncoder::enableToggleRequested,
                this, &EncodersConfig::fastEncoderEnableToggleRequested);
    }

    // spawn slow-encoder rows below
    ui->layoutV_Encoders->setAlignment(Qt::AlignTop);
    for (int i = 0; i < MAX_ENCODERS_NUM - MAX_FAST_ENCODER_NUM; i++) {
        Encoders *encoder = new Encoders(i, this);
        ui->layoutV_Encoders->addWidget(encoder);
        m_encodersPtrList.append(encoder);
        connect(encoder, &Encoders::pairingEdited,
                this, &EncodersConfig::onRowPairingEdited);
    }
}

EncodersConfig::~EncodersConfig()
{
    delete ui;
}

void EncodersConfig::retranslateUi()
{
    ui->retranslateUi(this);
    for (int i = 0; i < m_fastEncodersPtrList.size(); ++i) {
        m_fastEncodersPtrList[i]->retranslateUi();
    }
    for (int i = 0; i < m_encodersPtrList.size(); ++i) {
        m_encodersPtrList[i]->retranslateUi();
    }
}

void EncodersConfig::refreshFastEncoderUi(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= m_fastEncodersPtrList.size()) return;
    m_fastEncodersPtrList[slotIndex]->refreshEnableCheckbox();
}

void EncodersConfig::fastEncoderSelected(const QString &pinGuiName, bool isSelected)
{
    // FAST_ENCODER role is allowed on PA8/PA9 (Encoder 1, TIM1, fast slot 0)
    // and PB6/PB7 (Encoder 2, TIM4, fast slot 1). Dispatch by pin name suffix.
    int slot = (pinGuiName.endsWith("B6") || pinGuiName.endsWith("B7")) ? 1 : 0;

    if (slot >= m_fastEncodersPtrList.size()) {
        return;
    }
    FastEncoder *fe = m_fastEncodersPtrList[slot];
    if (isSelected) {
        fe->setInput(pinGuiName);
    } else {
        fe->clearInput(pinGuiName);
    }
}

void EncodersConfig::rebuildEncoderButtonList()
{
    m_encoderButtons.clear();
    const button_t *b = gEnv.pDeviceConfig->config.buttons;
    for (int i = 0; i < MAX_BUTTONS_NUM; ++i) {
        // ENCODER_INPUT_A (219) is the single "Encoder" marker; also accept the
        // retired ENCODER_INPUT_B (220) so a not-yet-normalised legacy pin still
        // shows up as an encoder line.
        if (b[i].type == ENCODER_INPUT_A || b[i].type == ENCODER_INPUT_B) {
            m_encoderButtons.append(qMakePair(i, tr("Button #%1").arg(i + 1)));
        }
    }
    // buttons[] is scanned in index order, so m_encoderButtons is already sorted.
}

void EncodersConfig::dropStalePairs()
{
    QSet<int> valid;
    for (const auto &pr : m_encoderButtons) valid.insert(pr.first);

    slow_encoder_t *se = gEnv.pDeviceConfig->config.slow_encoders;
    for (int s = MAX_FAST_ENCODER_NUM; s < MAX_ENCODERS_NUM; ++s) {
        const bool bothEmpty = (se[s].btn_a < 0 && se[s].btn_b < 0);
        const bool bothValid = (se[s].btn_a >= 0 && valid.contains(se[s].btn_a) &&
                                se[s].btn_b >= 0 && valid.contains(se[s].btn_b));
        if (!bothEmpty && !bothValid) {   // half-set or references a non-encoder pin
            se[s].btn_a = -1;
            se[s].btn_b = -1;
        }
    }
}

void EncodersConfig::autoFillEmptyPairs()
{
    slow_encoder_t *se = gEnv.pDeviceConfig->config.slow_encoders;

    // Encoder buttons already claimed by an existing pair.
    QSet<int> used;
    for (int s = MAX_FAST_ENCODER_NUM; s < MAX_ENCODERS_NUM; ++s) {
        if (se[s].btn_a >= 0) used.insert(se[s].btn_a);
        if (se[s].btn_b >= 0) used.insert(se[s].btn_b);
    }
    // Unused encoder buttons, in slot order (lower index becomes Pin A).
    QList<int> unused;
    for (const auto &pr : m_encoderButtons)
        if (!used.contains(pr.first)) unused.append(pr.first);

    // Fill empty slots with consecutive pairs; never touch a slot already set.
    int k = 0;
    for (int s = MAX_FAST_ENCODER_NUM; s < MAX_ENCODERS_NUM; ++s) {
        if (se[s].btn_a < 0 && se[s].btn_b < 0) {   // empty slot
            if (k + 1 < unused.size()) {
                se[s].btn_a = int8_t(unused[k]);
                se[s].btn_b = int8_t(unused[k + 1]);
                k += 2;
            }
        }
    }
}

void EncodersConfig::refreshRows()
{
    for (Encoders *row : m_encodersPtrList) {
        row->setEncoderButtons(m_encoderButtons);
        row->readFromConfig();
    }
    applyClashHighlight();
}

void EncodersConfig::applyClashHighlight()
{
    const int n = m_encodersPtrList.size();
    for (int r = 0; r < n; ++r) {
        const int a = m_encodersPtrList[r]->inputA();
        const int b = m_encodersPtrList[r]->inputB();

        auto usedElsewhere = [&](int pin) -> bool {
            if (pin < 0) return false;
            for (int q = 0; q < n; ++q) {
                if (q == r) continue;
                if (m_encodersPtrList[q]->inputA() == pin ||
                    m_encodersPtrList[q]->inputB() == pin) return true;
            }
            return false;
        };

        const bool aClash = a >= 0 && (a == b || usedElsewhere(a));
        const bool bClash = b >= 0 && (b == a || usedElsewhere(b));
        m_encodersPtrList[r]->setInputClash(aClash, bClash);
    }
}

void EncodersConfig::resync(bool autoFill)
{
    rebuildEncoderButtonList();
    dropStalePairs();
    if (autoFill) autoFillEmptyPairs();
    refreshRows();
}

void EncodersConfig::onEncoderButtonsChanged()
{
    resync(true);    // a pin was just tagged/untagged -> auto-fill new pairs
}

void EncodersConfig::refreshDisplay()
{
    resync(false);   // load / reorder -> respect stored pairs, no invention
}

void EncodersConfig::onRowPairingEdited()
{
    // A row already wrote its own slot to config; just re-run clash highlight
    // (a pin now used twice, or A==B, must light up across rows).
    applyClashHighlight();
}

void EncodersConfig::readFromConfig()
{
    for (int i = 0; i < m_fastEncodersPtrList.size(); ++i) {
        m_fastEncodersPtrList[i]->readFromConfig();
    }
    // Materialise the encoder-line list from the freshly loaded buttons and
    // render the stored pairs. NO auto-fill on load: the config (migrated or
    // saved) is authoritative -- inventing pairs here would mark it dirty and
    // could resurrect encoder-line buttons the old firmware left inert.
    resync(false);
}

void EncodersConfig::writeToConfig()
{
    for (int i = 0; i < m_fastEncodersPtrList.size(); ++i) {
        m_fastEncodersPtrList[i]->writeToConfig();
    }
    for (int i = 0; i < m_encodersPtrList.size(); ++i) {
        m_encodersPtrList[i]->writeToConfig();
    }
}
