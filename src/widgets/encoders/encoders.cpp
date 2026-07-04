#include "encoders.h"
#include "ui_encoders.h"

#include <QComboBox>
#include <QSignalBlocker>

#include "centered_cbox.h"
#include "common_defines.h"   // MAX_FAST_ENCODER_NUM, SLOW_ENC_*
#include "style_helpers.h"    // freejoy_style::fieldClashQss

Encoders::Encoders(int encodersNumber, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Encoders)
{
    ui->setupUi(this);

    m_encodersNumber = encodersNumber + 1;                    // 1-based user label
    m_configIndex    = encodersNumber + MAX_FAST_ENCODER_NUM; // slow slots sit after fast
    ui->label_EncoderIndex->setNum(m_encodersNumber);

    for (int i = 0; i < ENCODER_TYPE_COUNT; ++i)
        ui->comboBox_EncoderType->addItem(m_encoderTypeList[i].guiName);

    setEncoderButtons({});   // seed the "none" entry + disabled state

    connect(ui->comboBox_InputA, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Encoders::onUserEdited);
    connect(ui->comboBox_InputB, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Encoders::onUserEdited);
    connect(ui->comboBox_EncoderType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Encoders::onUserEdited);
    connect(ui->pushButton_Swap, &QPushButton::clicked, this, &Encoders::swapInputs);
}

Encoders::~Encoders()
{
    delete ui;
}

void Encoders::retranslateUi()
{
    ui->retranslateUi(this);
}

int Encoders::inputA() const
{
    QVariant v = ui->comboBox_InputA->currentData();
    return v.isValid() ? v.toInt() : -1;
}

int Encoders::inputB() const
{
    QVariant v = ui->comboBox_InputB->currentData();
    return v.isValid() ? v.toInt() : -1;
}

void Encoders::selectData(CenteredCBox *combo, int slotIndex)
{
    int idx = combo->findData(slotIndex);
    combo->setCurrentIndex(idx >= 0 ? idx : 0);   // 0 = the "none" entry
}

void Encoders::fillCombo(CenteredCBox *combo, const QVector<QPair<int, QString>> &buttons, int keepSel)
{
    QSignalBlocker block(combo);
    combo->clear();
    combo->addItem(QStringLiteral("—"), -1);   // em dash = "none"
    for (const auto &pr : buttons)
        combo->addItem(pr.second, pr.first);
    selectData(combo, keepSel);
}

void Encoders::setEncoderButtons(const QVector<QPair<int, QString>> &buttons)
{
    m_populating = true;
    const int keepA = inputA();
    const int keepB = inputB();
    fillCombo(ui->comboBox_InputA, buttons, keepA);
    fillCombo(ui->comboBox_InputB, buttons, keepB);
    m_populating = false;
    updateEnabledState();
}

void Encoders::updateEnabledState()
{
    // Pin combos are selectable whenever there is at least one encoder-line
    // button to choose (count() > 1 accounts for the leading "none" entry).
    const bool hasButtons = ui->comboBox_InputA->count() > 1;
    ui->comboBox_InputA->setEnabled(hasButtons);
    ui->comboBox_InputB->setEnabled(hasButtons);
    ui->text_InputA->setEnabled(hasButtons);
    ui->text_InputB->setEnabled(hasButtons);
    ui->label_EncoderIndex->setEnabled(hasButtons);

    // Mode + swap only matter once a full pair is chosen.
    const bool complete = inputA() >= 0 && inputB() >= 0;
    ui->comboBox_EncoderType->setEnabled(complete);
    ui->text_EncoderType->setEnabled(complete);
    ui->pushButton_Swap->setEnabled(complete);
    ui->text_Swap->setEnabled(complete);
}

void Encoders::readFromConfig()
{
    m_populating = true;
    const slow_encoder_t &se = gEnv.pDeviceConfig->config.slow_encoders[m_configIndex];
    selectData(ui->comboBox_InputA, se.btn_a);
    selectData(ui->comboBox_InputB, se.btn_b);

    // encoders[i] holds the detent mode. Mask off any legacy high bits (an
    // interim build packed a swap flag here before Swap became a pin exchange).
    const uint8_t enc = gEnv.pDeviceConfig->config.encoders[m_configIndex];
    ui->comboBox_EncoderType->setCurrentIndex(enc & SLOW_ENC_MODE_MASK);
    m_populating = false;

    updateEnabledState();
}

void Encoders::writeToConfig()
{
    gEnv.pDeviceConfig->config.slow_encoders[m_configIndex].btn_a = int8_t(inputA());
    gEnv.pDeviceConfig->config.slow_encoders[m_configIndex].btn_b = int8_t(inputB());
    gEnv.pDeviceConfig->config.encoders[m_configIndex] =
        uint8_t(ui->comboBox_EncoderType->currentIndex()) & SLOW_ENC_MODE_MASK;
}

void Encoders::swapInputs()
{
    // Exchange the Pin A / Pin B selections. Because the firmware decodes
    // rotation direction from pin order, swapping the pins reverses the encoder
    // -- no separate swap flag needed.
    const int a = inputA();
    const int b = inputB();
    m_populating = true;
    selectData(ui->comboBox_InputA, b);
    selectData(ui->comboBox_InputB, a);
    m_populating = false;
    writeToConfig();
    updateEnabledState();
    emit pairingEdited();
}

void Encoders::applyUsageMask(const QSet<int> &used)
{
    // Item state uses the Qt::UserRole-1 combo trick: 1|32 = selectable+enabled,
    // 0 = disabled (greyed, unpickable). Index 0 is the "none" entry -- always
    // enabled. A combo's own current pick stays enabled so it still displays.
    auto maskCombo = [&](CenteredCBox *combo, int keep) {
        for (int i = 1; i < combo->count(); ++i) {
            const int d = combo->itemData(i).toInt();
            const bool disable = used.contains(d) && d != keep;
            combo->setItemData(i, disable ? 0 : (1 | 32), Qt::UserRole - 1);
        }
    };
    maskCombo(ui->comboBox_InputA, inputA());
    maskCombo(ui->comboBox_InputB, inputB());
}

void Encoders::setInputClash(bool aClash, bool bClash)
{
    const QString clash = freejoy_style::fieldClashQss();
    ui->comboBox_InputA->setStyleSheet(aClash ? clash : QString());
    ui->comboBox_InputB->setStyleSheet(bClash ? clash : QString());
}

void Encoders::onUserEdited()
{
    if (m_populating) return;
    writeToConfig();
    updateEnabledState();
    emit pairingEdited();
}
