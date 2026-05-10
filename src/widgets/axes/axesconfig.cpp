#include "axesconfig.h"
#include "ui_axesconfig.h"
#include <QCheckBox>
#include <QSettings>
#include "global.h"

AxesConfig::AxesConfig(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AxesConfig)
{
    ui->setupUi(this);
    m_a2bButtonsCount = 0;

    ui->layoutV_Axes->setAlignment(Qt::AlignTop);
    // axes spawn
    for (int i = 0; i < MAX_AXIS_NUM; i++) {
        Axes *axis = new Axes(i, this);
        ui->layoutV_Axes->addWidget(axis);
        m_axesPtrList.append(axis);
        connect(axis, SIGNAL(a2bCountChanged(int, int)),
                this, SLOT(a2bCountCalc(int, int)));
        // Re-mark source dropdowns whenever any axis's selection
        // changes -- ensures "(used by X)" suffixes track the live
        // global usage map without per-axis state.
        connect(axis, &Axes::mainSourceChanged,
                this, &AxesConfig::refreshSourceUsage);
        // Forward per-axis output-active changes so the Curves tab
        // can grey-out thumbnails for axes whose curves don't
        // currently affect device output.
        connect(axis, &Axes::outputActiveChanged,
                this, &AxesConfig::axisOutputActiveChanged);
        // added hidden axes checkboxes
        QCheckBox *chb = new QCheckBox(axesList()[i].guiName, this);
        ui->layoutH_HiddenAxes->addWidget(chb);
        chb->setProperty("index", i);
        m_hideChBoxes.append(chb);
        connect(chb, &QCheckBox::toggled, this, [&](bool hide) {
            hideAxis(sender()->property("index").toInt(), hide);
        });
    }

    gEnv.pAppSettings->beginGroup("OtherSettings");
    for (int i = 0; i < m_hideChBoxes.size(); ++i) {
        m_hideChBoxes[i]->setChecked(gEnv.pAppSettings->value(axesList()[i].guiName, "false").toBool());
    }
    gEnv.pAppSettings->endGroup();
}

AxesConfig::~AxesConfig()
{
    gEnv.pAppSettings->beginGroup("OtherSettings");
    for (int i = 0; i < m_hideChBoxes.size(); ++i) {
        gEnv.pAppSettings->setValue(axesList()[i].guiName, m_hideChBoxes[i]->isChecked());
    }
    gEnv.pAppSettings->endGroup();
    delete ui;
}

void AxesConfig::retranslateUi()
{
    ui->retranslateUi(this);
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        m_axesPtrList[i]->retranslateUi();
    }
}

void AxesConfig::a2bCountCalc(int count, int previousCount)
{
    m_a2bButtonsCount += count - previousCount;

    // Per-axis breakdown alongside the total -- emit FIRST so anyone
    // grouping UI off the breakdown has fresh data when the total triggers
    // a rebuild.
    QList<int> perAxis;
    perAxis.reserve(m_axesPtrList.size());
    for (auto *a : m_axesPtrList) {
        perAxis.append(a->a2bButtonCount());
    }
    emit a2bBreakdownChanged(perAxis);

    emit a2bCountChanged(m_a2bButtonsCount);
}

void AxesConfig::addOrDeleteMainSource(int sourceEnum, QString sourceName, bool isAdd)
{
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        m_axesPtrList[i]->addOrDeleteMainSource(sourceEnum, sourceName, isAdd);
    }
    // The set of candidate sources changed; refresh the "used by X"
    // annotations so newly-added items pick up suffixes immediately
    // and removed items don't leave dangling references in other
    // axes' dropdowns.
    refreshSourceUsage();
}

void AxesConfig::setConnectedBoard(int boardId)
{
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        m_axesPtrList[i]->setConnectedBoard(boardId);
    }
    // Each Axes::setConnectedBoard emits mainSourceChanged after
    // relabelling, which feeds refreshSourceUsage via the existing
    // signal wiring -- no explicit refresh needed here.
}

void AxesConfig::fastEncoderPinChanged(const QString &pinGuiName, bool isAdd)
{
    const QList<int> prevSlots = completedEncoderSlots();
    if (isAdd) {
        m_fastEncoderPins.insert(pinGuiName);
    } else {
        m_fastEncoderPins.remove(pinGuiName);
    }
    const QList<int> nowSlots = completedEncoderSlots();

    // Suppress per-pin churn -- only push when the set of *complete*
    // encoder pairs actually changes (e.g. assigning the second half
    // of a pair, or removing one half of a pair, both shift the slot
    // list, but adding the first half of an unpaired encoder doesn't).
    if (prevSlots == nowSlots) return;

    for (int i = 0; i < m_axesPtrList.size(); ++i) {
        m_axesPtrList[i]->setEncoderSlotsAvailable(nowSlots);
    }
    refreshSourceUsage();
}

QList<int> AxesConfig::completedEncoderSlots() const
{
    /* Encoder pin pairs are board-independent on F103 and F411:
     *   slot 0 = Enc 1 = PA8 + PA9 (TIM1)
     *   slot 1 = Enc 2 = PB6 + PB7 (TIM4)
     * pinList[].guiName arrives as a tr() string like "Pin A8", so
     * match by suffix (the same pattern encodersconfig.cpp::
     * fastEncoderSelected uses for its slot dispatch) -- this stays
     * correct under translation and any future "Pin "/"P"/"" prefix
     * shifts in the pin combobox. */
    auto hasPin = [&](const QString &suffix) {
        for (const QString &name : m_fastEncoderPins) {
            if (name.endsWith(suffix)) return true;
        }
        return false;
    };
    QList<int> completedSlots;
    if (hasPin(QStringLiteral("A8")) && hasPin(QStringLiteral("A9"))) {
        completedSlots.append(0);
    }
    if (hasPin(QStringLiteral("B6")) && hasPin(QStringLiteral("B7"))) {
        completedSlots.append(1);
    }
    return completedSlots;
}

void AxesConfig::refreshSourceUsage()
{
    // Build the global "source enum -> list of axis names" usage map.
    QMap<int, QStringList> globalUsed;
    for (int i = 0; i < m_axesPtrList.size(); ++i) {
        const int e = m_axesPtrList[i]->currentSource();
        if (Axes::isSharedSource(e)) continue;   // None / Encoder -- never count
        globalUsed[e].append(axesList()[i].guiName);
    }

    // Push to each axis a per-axis "used by OTHERS" map (own usage stripped).
    for (int i = 0; i < m_axesPtrList.size(); ++i) {
        const int ownEnum = m_axesPtrList[i]->currentSource();
        const QString ownName = axesList()[i].guiName;
        QMap<int, QStringList> usedByOthers = globalUsed;
        if (!Axes::isSharedSource(ownEnum) && usedByOthers.contains(ownEnum)) {
            usedByOthers[ownEnum].removeAll(ownName);
            if (usedByOthers[ownEnum].isEmpty()) {
                usedByOthers.remove(ownEnum);
            }
        }
        m_axesPtrList[i]->markSourcesInUse(usedByOthers);
    }
}

void AxesConfig::axesValueChanged()
{
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        m_axesPtrList[i]->updateAxisOut();
        m_axesPtrList[i]->updateAxisRaw();
    }
}

void AxesConfig::hideAxis(int index, bool hide)
{
    if (index < 0) index = 0;
    else if (index >= m_axesPtrList.size()) index = m_axesPtrList.size() - 1;

    if (hide) {
        m_axesPtrList[index]->hide();
    } else {
        m_axesPtrList[index]->show();
    }
}

void AxesConfig::readFromConfig()
{
    for (int i = 0; i < MAX_AXIS_NUM; i++) {
        m_axesPtrList[i]->readFromConfig();
    }
    // Annotate dropdowns now that the loaded source selections are
    // settled. axes whose source matched the combobox's current value
    // didn't fire mainSourceChanged during load, so do a single sweep.
    refreshSourceUsage();
}

void AxesConfig::writeToConfig()
{
    for (int i = 0; i < MAX_AXIS_NUM; i++) {
        m_axesPtrList[i]->writeToConfig();
    }
}
