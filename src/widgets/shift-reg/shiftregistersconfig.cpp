#include "shiftregistersconfig.h"
#include "ui_shiftregistersconfig.h"
#include <QDebug>
#include <QWidget>
#include <QGridLayout>
#include <QLabel>

ShiftRegistersConfig::ShiftRegistersConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ShiftRegistersConfig)
{
    ui->setupUi(this);
    m_shiftButtonsCount = 0;

    ui->layoutV_ShiftRegisters->setAlignment(Qt::AlignTop);
    ui->layoutV_ShiftRegisters->setSpacing(2);   // tight inter-row pitch, like the expander table

    // One shared column header for all registers (each register's own headers
    // are hidden). Same 7-column equal-stretch grid as a register row so the
    // headers line up, matching the Port Expanders table below.
    {
        auto *header = new QWidget(this);
        auto *hg = new QGridLayout(header);
        hg->setContentsMargins(9, 0, 9, 2);
        // Type leads (aligned above the Port Expanders' Type column), then the
        // register's pins, then the counts -- so both tables read
        // "# | Type | ... | Button count".
        m_headerLabels = {
            new QLabel(tr("Type"),            header),
            new QLabel(tr("Latch pin"),       header),
            new QLabel(tr("CLK pin"),         header),
            new QLabel(tr("Data pin"),        header),
            new QLabel(tr("Registers count"), header),
            new QLabel(tr("Button count"),    header),
        };
        for (int c = 0; c < m_headerLabels.size(); ++c)
            hg->addWidget(m_headerLabels[c], 0, c + 1, Qt::AlignCenter);
        for (int c = 0; c < 7; ++c) hg->setColumnStretch(c, 1);
        ui->layoutV_ShiftRegisters->addWidget(header);
    }

    // shift registers spawn
    for (int i = 0; i < MAX_SHIFT_REG_NUM; i++)
    {
        ShiftRegisters * shift_register = new ShiftRegisters(i, this);
        ui->layoutV_ShiftRegisters->addWidget(shift_register);
        m_shiftRegsPtrList.append(shift_register);
        connect(shift_register, &ShiftRegisters::buttonCountChanged,
                this, &ShiftRegistersConfig::shiftRegButtonsCalc);
    }
}

ShiftRegistersConfig::~ShiftRegistersConfig()
{
    delete ui;
}

void ShiftRegistersConfig::retranslateUi()
{
    ui->retranslateUi(this);
    for (int i = 0; i < m_shiftRegsPtrList.size(); ++i) {
        m_shiftRegsPtrList[i]->retranslateUi();
    }
    const QStringList hdr = { tr("Type"), tr("Latch pin"), tr("CLK pin"),
                             tr("Data pin"), tr("Registers count"),
                             tr("Button count") };
    for (int i = 0; i < m_headerLabels.size() && i < hdr.size(); ++i)
        m_headerLabels[i]->setText(hdr[i]);
}


void ShiftRegistersConfig::shiftRegButtonsCalc(int currentCount, int previousCount)
{
    m_shiftButtonsCount += currentCount - previousCount;

    // Per-register breakdown alongside the total -- emit FIRST so anyone
    // grouping UI off the breakdown has fresh data when the total triggers
    // a rebuild.
    QList<int> perRegister;
    perRegister.reserve(m_shiftRegsPtrList.size());
    for (auto *r : m_shiftRegsPtrList) {
        perRegister.append(r->buttonCount());
    }
    emit shiftRegBreakdownChanged(perRegister);

    emit shiftRegButtonsCountChanged(m_shiftButtonsCount);
}


bool ShiftRegistersConfig::sortByPinNumberAndNullLast(const ShiftRegData_t &lhs, const ShiftRegData_t &rhs)
{
    // sort null last
    if (lhs.pinNumber == 0) {
        return false;
    } else if (rhs.pinNumber == 0) {
        return true;
    } else { // sort ascending
        return lhs.pinNumber < rhs.pinNumber;
    }
}

// bullshit
void ShiftRegistersConfig::addPinAndSort(int pin, const QString &pinGuiName, std::array<ShiftRegData_t, MAX_SHIFT_REG_NUM + 1> &arr)
{
    // add shift reg latch/clk pin
    if (pin > 0) {
        arr.back().pinNumber = pin;
        arr.back().guiName = pinGuiName;
    }
    // delete shift reg latch pin
    else
    {
        pin = -pin;
        for (uint i = 0; i < arr.size(); ++i) {
            if (pin == arr[i].pinNumber) {
                arr[i].pinNumber = 0;
                arr[i].guiName = m_shiftRegsPtrList[0]->defaultText();      //?????????????????
            }
        }
    }

    std::sort(arr.begin(), arr.end(), sortByPinNumberAndNullLast);

    //all unused pins (if (m_latchPinsArray[i].pinNumber == 0)) = bigger pin
    for (int i = arr.size() -1; i >= 0; --i) {                        // bullshit // todo: refactoring
        if (arr[i].pinNumber > 0) {
            // example: we have selected ShiftLatch pins{A0(pinNumber = 1), A2(pinNumber = 3), A6(pinNumber = 7)}
            // in the end it should look like: m_latchPinsArray.pinNumber{1, 3, 7, 7, 7}
            for (uint k = 0; k < arr.size() -1; ++k) {
                if (arr[k].pinNumber == arr[k + 1].pinNumber && arr.back().pinNumber > 0) {
                    for (uint p = 0; p < arr.size() -(k + 2); ++p) {
                        arr[k + p + 1].pinNumber = arr.back().pinNumber;
                        arr[k + p + 1].guiName = arr.back().guiName;
                    }
                    break;
                }
            }
            for (int j = arr.size() -1; j > i; --j) {
                arr[j].pinNumber = arr[i].pinNumber;
                arr[j].guiName = arr[i].guiName;
            }
            break;
        }
    }
}

void ShiftRegistersConfig::shiftRegSelected(int latchPin, int clkPin, int dataPin, const QString &pinGuiName)
{
    // add shift reg latch pin
    if (latchPin != 0) {
        addPinAndSort(latchPin, pinGuiName, m_latchPinsArray);

        // update shiftreg ui
        for (uint i = 0; i < m_latchPinsArray.size() - 1; ++i) {
            m_shiftRegsPtrList[i]->setLatchPin(m_latchPinsArray[i].pinNumber, m_latchPinsArray[i].guiName);
        }
    }
    // add shift reg clk pin
    else if (clkPin != 0) {
        addPinAndSort(clkPin, pinGuiName, m_clkPinsArray);

        // update shiftreg ui
        for (uint i = 0; i < m_clkPinsArray.size() - 1; ++i) {
            m_shiftRegsPtrList[i]->setClkPin(m_clkPinsArray[i].pinNumber, m_clkPinsArray[i].guiName);
        }
    }
    // add shift reg data pin
    else if (dataPin != 0) {
        if (dataPin > 0) {
            m_dataPinsArray[m_dataPinsArray.size() - 1].pinNumber = dataPin;
            m_dataPinsArray[m_dataPinsArray.size() - 1].guiName = pinGuiName;
        } else {
            dataPin = -dataPin;
            for (uint i = 0; i < m_dataPinsArray.size(); ++i) {
                if (dataPin == m_dataPinsArray[i].pinNumber){
                    m_dataPinsArray[i].pinNumber = 0;
                    // m_dataPinsArray has MAX_SHIFT_REG_NUM+1 entries (trailing
                    // scratch slot for the add-and-sort dance above); the widget
                    // list has only MAX_SHIFT_REG_NUM, so [i] crashes when the
                    // match lands in scratch. defaultText() is the same shared
                    // placeholder on every widget -- mirror addPinAndSort's [0].
                    m_dataPinsArray[i].guiName = m_shiftRegsPtrList[0]->defaultText();
                }
            }
        }

        std::sort(m_dataPinsArray.begin(), m_dataPinsArray.end(), sortByPinNumberAndNullLast);

        for (uint i = 0; i < m_dataPinsArray.size() - 1; ++i) {
            m_shiftRegsPtrList[i]->setDataPin(m_dataPinsArray[i].pinNumber, m_dataPinsArray[i].guiName);
        }

    }
}

void ShiftRegistersConfig::readFromConfig()
{
    for (int i = 0; i < m_shiftRegsPtrList.size(); ++i) {
        m_shiftRegsPtrList[i]->readFromConfig();
    }
}

void ShiftRegistersConfig::writeToConfig()
{
    for (int i = 0; i < m_shiftRegsPtrList.size(); ++i) {
        m_shiftRegsPtrList[i]->writeToConfig();
    }
}
