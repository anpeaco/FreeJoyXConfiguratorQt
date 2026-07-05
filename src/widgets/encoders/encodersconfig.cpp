#include "encodersconfig.h"
#include "ui_encodersconfig.h"

#include <QSet>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QGridLayout>

#include "common_defines.h"   // MAX_ENCODERS_NUM, MAX_FAST_ENCODER_NUM, MAX_BUTTONS_NUM
#include "common_types.h"     // ENCODER_INPUT_A
#include "widgets/debugwindow.h"   // shared fire-count reset (gEnv.pDebugWindow)

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

    // One shared column header for all slow-encoder rows (each row carries no
    // headers of its own) -- same single-header table style as the Shift
    // Registers screen. The grid's column stretches / minimum widths MUST match
    // encoders.ui so the header lines up above the row controls.
    {
        auto *header = new QWidget(this);
        auto *hg = new QGridLayout(header);
        hg->setContentsMargins(0, 0, 0, 2);
        hg->setHorizontalSpacing(6);
        m_encHeaderLabels = {
            new QLabel(tr("Pin A"),     header),
            new QLabel(tr("Pin B"),     header),
            new QLabel(tr("Swap"),      header),
            new QLabel(tr("Type"),      header),
            new QLabel(tr("Queue"),     header),
            new QLabel(tr("Calibrate"), header),
        };
        // Labels occupy columns 1..6 (col 0 = row index, col 7 = trailing spacer).
        for (int i = 0; i < m_encHeaderLabels.size(); ++i)
            hg->addWidget(m_encHeaderLabels[i], 0, i + 1, Qt::AlignCenter);
        static const int stretch[8] = { 0, 10, 10, 0, 8, 0, 0, 4 };
        static const int minw[8]    = { 30, 0, 0, 50, 0, 52, 58, 0 };
        for (int c = 0; c < 8; ++c) {
            hg->setColumnStretch(c, stretch[c]);
            if (minw[c]) hg->setColumnMinimumWidth(c, minw[c]);
        }
        ui->layoutV_Encoders->addWidget(header);
    }

    for (int i = 0; i < MAX_ENCODERS_NUM - MAX_FAST_ENCODER_NUM; i++) {
        Encoders *encoder = new Encoders(i, this);
        ui->layoutV_Encoders->addWidget(encoder);
        m_encodersPtrList.append(encoder);
        connect(encoder, &Encoders::pairingEdited,
                this, &EncodersConfig::onRowPairingEdited);
    }

    // Per-row A/B press-count reset -- built in code (kept out of the .ui). Each
    // row shows its own A/B counts in the indicator squares; this one button
    // zeroes them all for a fresh test.
    m_resetCountsBtn = new QPushButton(tr("Reset press counts"), this);
    m_resetCountsBtn->setToolTip(tr("Zero the A / B press counts on every encoder row."));
    m_resetCountsBtn->setMaximumWidth(170);
    connect(m_resetCountsBtn, &QPushButton::clicked, this, &EncodersConfig::resetCounts);

    QWidget *countRow = new QWidget(this);
    QHBoxLayout *countLayout = new QHBoxLayout(countRow);
    countLayout->setContentsMargins(4, 2, 4, 2);
    countLayout->addStretch(1);
    countLayout->addWidget(m_resetCountsBtn, 0);

    if (QGridLayout *g = qobject_cast<QGridLayout *>(layout()))
        g->addWidget(countRow, g->rowCount(), 0);
    else if (layout())
        layout()->addWidget(countRow);

    resetCounts();   // size + zero the per-row counters now the rows exist
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
    const QStringList hdr = { tr("Pin A"), tr("Pin B"), tr("Swap"),
                             tr("Type"), tr("Queue"), tr("Calibrate") };
    for (int i = 0; i < m_encHeaderLabels.size() && i < hdr.size(); ++i)
        m_encHeaderLabels[i]->setText(hdr[i]);
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
    applyUsageMasks();
    applyClashHighlight();
}

void EncodersConfig::applyUsageMasks()
{
    QSet<int> used;
    for (Encoders *row : m_encodersPtrList) {
        if (row->inputA() >= 0) used.insert(row->inputA());
        if (row->inputB() >= 0) used.insert(row->inputB());
    }
    for (Encoders *row : m_encodersPtrList) row->applyUsageMask(used);
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

void EncodersConfig::updateActivity()
{
    // The encoder fires the logical button at its A/B button slot (A on one
    // rotation direction, B on the other). Read those bits from the device's
    // logical-button bitmap: flash the row's Pin A / Pin B square AND edge-count
    // each activation (0->1) into the running press count shown in the square.
    // Runs on every params packet so brief pulses are caught the same way the
    // debug log catches them.
    const uint8_t *log = gEnv.pDeviceConfig->paramsReport.log_button_data;
    const int *fires = gEnv.pDeviceConfig->logFireCount;
    auto bitSet = [log](int slot) {
        return slot >= 0 && slot < MAX_BUTTONS_NUM &&
               (log[slot >> 3] & (1 << (slot & 0x07)));
    };
    auto fireCount = [fires](int slot) {
        return (slot >= 0 && slot < MAX_BUTTONS_NUM) ? fires[slot] : 0;
    };
    const int n = m_encodersPtrList.size();
    for (int r = 0; r < n; ++r) {
        Encoders *row = m_encodersPtrList[r];
        // Flash from the live bit; the press count is the SHARED tally that
        // DeviceConfig edge-counts every packet (same number the debug log's
        // "fires=" shows), so the square and the log can never disagree.
        row->setPressCounts(fireCount(row->inputA()), fireCount(row->inputB()));
        row->setActivity(bitSet(row->inputA()), bitSet(row->inputB()));
    }
}

void EncodersConfig::resetCounts()
{
    // The tally is the shared DeviceConfig counter (same one the debug log
    // reads), so zeroing is a single call there. The squares repaint to 0 on
    // the next params packet via updateActivity().
    if (gEnv.pDeviceConfig)
        gEnv.pDeviceConfig->resetFireCounts();
    const int n = m_encodersPtrList.size();
    for (int r = 0; r < n; ++r)
        m_encodersPtrList[r]->setPressCounts(0, 0);
}

void EncodersConfig::onRowPairingEdited()
{
    // A row already wrote its own slot to config; re-grey the now-used pins
    // across all rows and re-run the clash highlight.
    applyUsageMasks();
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
