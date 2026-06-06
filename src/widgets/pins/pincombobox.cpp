#include "pincombobox.h"
#include "ui_pincombobox.h"

#include <QFont>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyleOptionViewItem>

namespace {

// Marks a section-header row in the pin-role popup (see addType / the delegate).
constexpr int kPinHeaderRole = Qt::UserRole + 1;

// Popup-list delegate for the pin-role dropdown. Makes the section grouping
// obvious without a loud selection:
//   - section header  -> UPPERCASE bold full-contrast label, a thin separator
//                         line above it, sitting flush-left
//   - role rows        -> indented under their header (so the hierarchy reads),
//                         function-coloured (or greyed+struck if disabled) label
//   - hovered/selected -> a subtle neutral wash only; the function-coloured text
//                         stays, so selection is clear but not in-your-face
class PinRoleDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *p, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);

        const bool header  = index.data(kPinHeaderRole).toBool();
        const bool enabled = index.flags().testFlag(Qt::ItemIsEnabled);
        const bool active  = (opt.state.testFlag(QStyle::State_Selected)
                              || opt.state.testFlag(QStyle::State_MouseOver))
                             && enabled && !header;
        const QColor role  = qvariant_cast<QColor>(index.data(Qt::ForegroundRole));
        const QColor base  = opt.palette.color(QPalette::Base);
        const bool darkUi  = base.lightness() < 128;

        QFont f = qvariant_cast<QFont>(index.data(Qt::FontRole));
        if (f.family().isEmpty()) {
            f = opt.font;
        }

        p->save();
        p->fillRect(opt.rect, base);

        int leftPad = 22;          // role rows indented beneath their header
        QColor fg;
        QString text = index.data(Qt::DisplayRole).toString();
        if (header) {
            leftPad = 8;           // headers flush-left so the indent reads
            p->setPen(opt.palette.color(QPalette::Mid));   // thin divider above the header
            p->drawLine(opt.rect.left() + 6, opt.rect.top(),
                        opt.rect.right() - 6, opt.rect.top());
            fg = opt.palette.color(QPalette::Text);        // full contrast (white on dark)
            f.setBold(true);
            text = text.toUpper();
        } else {
            if (active) {
                // subtle neutral hover/selection -- not the loud function-colour fill
                p->fillRect(opt.rect, darkUi ? QColor(255, 255, 255, 28)
                                             : QColor(0, 0, 0, 22));
            }
            fg = !enabled ? opt.palette.color(QPalette::Disabled, QPalette::Text)
                          : (role.isValid() ? role : opt.palette.color(QPalette::Text));
        }

        p->setPen(fg);
        p->setFont(f);
        const QRect tr = opt.rect.adjusted(leftPad, 0, -8, 0);
        p->drawText(tr, Qt::AlignVCenter | Qt::AlignLeft,
                    opt.fontMetrics.elidedText(text, Qt::ElideRight, tr.width()));
        p->restore();
    }
};

} // namespace

//! pinNumber cannot be less 1 and more than PINS_COUNT
PinComboBox::PinComboBox(uint pinNumber, QWidget *parent) : // pin handling was the first thing I coded in the configurator, and in hindsight
    QWidget(parent),                                        // it's hard to follow even for me
    ui(new Ui::PinComboBox)                                 // condolences to anyone trying to understand it
{
    ui->setupUi(this);
    // Custom popup rendering: section headers vs. function-coloured hover/select.
    ui->comboBox_PinsType->setItemDelegate(new PinRoleDelegate(ui->comboBox_PinsType));
    // minimum pinNumber = enum PA_0 = 1
    if (pinNumber < 1 || pinNumber > PINS_COUNT) {
        qFatal("(pinNumber < 1 || pinNumber > PINS_COUNT) in pincombobox.cpp");
    }
    m_pinNumber = pinNumber;
    // set object name for for placement in PinConfig layout
    setObjectName(m_pinList[m_pinNumber-1].objectName);

    m_previousIndex = 0;
    m_interactCount = 0;
    m_currentDevEnum = 0;

    m_isCall_Interaction = false;
    m_isInteracts = false;

    initializationPins(m_pinNumber);

    connect(ui->comboBox_PinsType, SIGNAL(currentIndexChanged(int)),
                this, SLOT(indexChanged(int)));

    // After the user commits a selection the combobox retains keyboard focus, so
    // the :focus blue border lingers on every configured pin (a column of blue
    // outlines). Drop focus once a choice is made -- the focus ring still shows
    // while the dropdown is open/being navigated, just not after. activated() is
    // user-interaction only, so programmatic loads (Read / auto-read) don't trip it.
    connect(ui->comboBox_PinsType, QOverload<int>::of(&QComboBox::activated),
            this, [this]() { ui->comboBox_PinsType->clearFocus(); });
}

PinComboBox::~PinComboBox()
{
    m_pinTypesIndex.clear();         // Fault tolerant heap??
    m_pinTypesIndex.shrink_to_fit(); // Fault tolerant heap??
    m_enumIndex.clear();
    m_enumIndex.shrink_to_fit();
    delete ui;
}

void PinComboBox::retranslateUi()
{
    ui->retranslateUi(this);
}

const pins *PinComboBox::pinList() const
{
    return m_pinList;
}


const QVector<int> & PinComboBox::pinTypeIndex() const
{
    return m_pinTypesIndex;
}

const QVector<int> & PinComboBox::enumIndex() const
{
    return m_enumIndex;
}

int PinComboBox::rowForEnum(int deviceEnum) const
{
    for (int i = 0; i < m_enumIndex.size(); ++i) {
        if (m_enumIndex[i] == deviceEnum) return i;
    }
    return -1;
}

QString PinComboBox::currentRoleText() const
{
    return ui->comboBox_PinsType->currentText();
}

bool PinComboBox::setRoleByEnum(int deviceEnum)
{
    const int row = rowForEnum(deviceEnum);
    if (row < 0) return false;
    if (ui->comboBox_PinsType->currentIndex() == row) {
        return true;     // already in the requested role; nothing to do
    }
    /* setCurrentIndex emits currentIndexChanged on the QComboBox,
     * which fires PinComboBox::indexChanged -> currentIndexChanged
     * (this widget's own signal) and feeds the existing
     * pinIndexChanged dispatch in PinConfig. So all downstream wiring
     * (fastEncoderSelected, axesSourceChanged, pin-type-limit, etc.)
     * runs exactly as it would for a user-driven role change. */
    ui->comboBox_PinsType->setCurrentIndex(row);
    return true;
}

uint PinComboBox::interactCount() const
{
    return m_interactCount;
}

void PinComboBox::setInteractCount(const uint &count)
{
    m_interactCount = count;
}

bool PinComboBox::isInteracts() const
{
    return m_isInteracts;
}

int PinComboBox::liveDevEnum() const
{
    const int idx = ui->comboBox_PinsType->currentIndex();
    if (idx >= 0 && idx < m_enumIndex.size() && m_enumIndex[idx] >= 0) {
        return m_enumIndex[idx];
    }
    return NOT_USED;
}

int PinComboBox::currentDevEnum() const
{
    return liveDevEnum();
}
//! Set selected index enable or disable. Also strikes through the item's
//! text when disabled so the conflict is visually obvious -- Qt's default
//! disabled rendering is too subtle in this styled combobox.
void PinComboBox::setIndexStatus(int index, bool status)
{
    QFont font = ui->comboBox_PinsType->font();
    if (status == true){
        ui->comboBox_PinsType->setItemData(index, 1 | 32, Qt::UserRole - 1);
        font.setStrikeOut(false);
    } else {
        ui->comboBox_PinsType->setItemData(index, 0, Qt::UserRole - 1);
        font.setStrikeOut(true);
    }
    ui->comboBox_PinsType->setItemData(index, font, Qt::FontRole);
}

void PinComboBox::setLocked(bool locked)
{
    ui->comboBox_PinsType->setEnabled(!locked);
}

bool PinComboBox::isComboBoxEnabled() const
{
    return ui->comboBox_PinsType->isEnabled();
}

bool PinComboBox::isRoleOptionEnabled(int deviceEnum) const
{
    for (int i = 0; i < m_enumIndex.size(); ++i) {
        if (m_enumIndex[i] == deviceEnum) {
            // setIndexStatus() stores 0 at Qt::UserRole-1 to disable an item;
            // an item never touched has no such data and is selectable.
            const QVariant v = ui->comboBox_PinsType->itemData(i, Qt::UserRole - 1);
            return !v.isValid() || v.toInt() != 0;
        }
    }
    return false;   // role not offered on this pin
}

void PinComboBox::resetPin()
{
    ui->comboBox_PinsType->setCurrentIndex(0);
    if (m_isInteracts == true)
    {
        ui->comboBox_PinsType->setEnabled(true);
        m_isInteracts = false;
    }
}

//! Apply `color` as the closed-combobox text colour and cache it. Set both
//! ButtonText and Text -- QStyleSheetStyle draws the closed combobox label
//! from Text on some paths, ButtonText on others. An invalid QColor restores
//! the inherited default palette ("Not Used" / interaction reset).
void PinComboBox::applyTextColor(const QColor &color)
{
    m_roleColor = color;
    if (color.isValid()) {
        QPalette pal = ui->comboBox_PinsType->palette();
        pal.setColor(QPalette::ButtonText, color);
        pal.setColor(QPalette::Text,       color);
        ui->comboBox_PinsType->setPalette(pal);
    } else {
        ui->comboBox_PinsType->setPalette(palette());
    }
    // Under the app stylesheet, QStyleSheetStyle owns the draw and setPalette()
    // alone often doesn't trigger a repaint -- on interactive selection one
    // happens anyway, but on a programmatic load (Read / auto-read / device
    // swap) the closed combobox keeps painting the old (black) colour. Force
    // the repaint so the role colour shows immediately.
    ui->comboBox_PinsType->update();
}

void PinComboBox::reapplyRoleColor()
{
    applyTextColor(m_roleColor);
}

QColor PinComboBox::colorForRole(int deviceEnum) const
{
    for (const cBox &c : m_pinTypes) {
        if (c.deviceEnumIndex == deviceEnum) {
            return c.color;
        }
    }
    return QColor();
}

void PinComboBox::setHoverOutline(const QColor &border)
{
    /* Inline stylesheet on the inner combobox: a thicker coloured border in the
     * hovered role's group colour, with padding trimmed 1px to absorb the extra
     * border so the control doesn't shift. Clearing removes the stylesheet. The
     * setStyleSheet polish wipes the palette-driven role text colour, so restore
     * it via reapplyRoleColor() afterwards (same reason flashAutoAssignedPin /
     * highlightPins do). border-radius matches the global QComboBox rule. */
    if (border.isValid()) {
        ui->comboBox_PinsType->setStyleSheet(
            QStringLiteral("QComboBox { border: 2px solid %1; border-radius: 4px; "
                           "padding: 1px 5px; }").arg(border.name()));
    } else {
        ui->comboBox_PinsType->setStyleSheet(QString());
    }
    reapplyRoleColor();
}

void PinComboBox::setIndex_iteraction(int index, int senderIndex)
{
    if(m_isInteracts == false && m_isCall_Interaction == false)     // ui->comboBox_PinsType->isEnabled()
    {
        /* Disable the combobox while this pin is owned by an interaction
         * (a sensor's auto-claimed lead). Upstream exempted TLE5011_GEN here
         * so its box stayed clickable -- but that made the GEN pin (B6) read
         * as "not locked" next to its fully-disabled SCK/MOSI siblings when a
         * TLE is active (GEN is only ever auto-claimed by a TLE). Lock it like
         * the others; the release branch below re-enables it when the sensor's
         * CS clears. The #65 option-greying in blockEncoder2TLE5011 stays as
         * belt-and-suspenders. */
        ui->comboBox_PinsType->setEnabled(false);
        // change text color
        applyTextColor(m_pinTypes[senderIndex].color);

        m_isInteracts = true;
        ui->comboBox_PinsType->setCurrentIndex(index);
    }
    else if (m_isInteracts == true)// && interact_count_ <= 0)
    {
        ui->comboBox_PinsType->setEnabled(true);
        m_isInteracts = false;
        // reset color
        applyTextColor(QColor());

        ui->comboBox_PinsType->setCurrentIndex(index);
    }
}

//! Set pin items
void PinComboBox::initializationPins(uint pin)      // pin_number_ - 1 -- not great
{                                                   // this is because the empty values of const cBox pin_types_[PIN_TYPE_COUNT]
    m_pinNumber = pin;                              // initialise to 0, making the code hard to follow. May rewrite.

    /* Clear combobox + parallel index arrays so re-init from setExcludedRoles
     * doesn't accumulate items / duplicate sentinel rows on top of the
     * existing universal set. The original (single-call) shape relied on the
     * arrays being empty by virtue of being freshly constructed; now we
     * re-populate from scratch each time. */
    {
        QSignalBlocker bl(ui->comboBox_PinsType);
        ui->comboBox_PinsType->clear();
    }
    m_pinTypesIndex.clear();
    m_enumIndex.clear();

    int typeExceptSize = sizeof(m_pinTypes->pinExcept) / sizeof(m_pinTypes->pinExcept[0]);
    int typeSize = sizeof(m_pinTypes->pinType) / sizeof(m_pinTypes->pinType[0]);
    int pinListTypeSize = sizeof(m_pinList->pinType) / sizeof(m_pinList->pinType[0]);
    bool tmp = false;

    // Human-readable section title for a PinGroup. Empty -> no header row.
    auto sectionTitle = [this](int group) -> QString {
        switch (group) {
        case GRP_BUTTON:     return tr("Buttons");
        case GRP_SHIFTREG:   return tr("Shift Registers");
        case GRP_SPI_SENSOR: return tr("SPI Devices");
        case GRP_LED:        return tr("LEDs");
        case GRP_AXIS:       return tr("Analog Axis");
        case GRP_ENCODER:    return tr("Encoder");
        case GRP_I2C:        return tr("I2C Devices");
        case GRP_UART:       return tr("Serial Output");
        case GRP_SPI_BUS:    return tr("SPI Bus (auto-assigned)");
        default:             return QString();
        }
    };

    /* Adds one role item, keeping the QComboBox row indices aligned 1:1 with
     * m_pinTypesIndex / m_enumIndex (every consumer relies on that alignment).
     * Whenever the role group changes from the previously-added item, inserts a
     * non-selectable, bold section header (e.g. "SPI Devices") and pushes a -1
     * sentinel into both parallel vectors for that header row so the alignment
     * survives the insert. */
    int lastGroup = -1;
    auto addType = [&](int i) {
        const int group = m_pinTypes[i].group;
        const QString title = sectionTitle(group);
        if (lastGroup != -1 && group != lastGroup && !title.isEmpty()) {
            const int hdr = ui->comboBox_PinsType->count();
            ui->comboBox_PinsType->addItem(title);
            ui->comboBox_PinsType->setItemData(hdr, 0, Qt::UserRole - 1);   // non-selectable
            // Mark as a section header; PinRoleDelegate paints it (UPPERCASE
            // bold + a divider line, flush-left) so it never looks like a row.
            ui->comboBox_PinsType->setItemData(hdr, true, kPinHeaderRole);
            m_pinTypesIndex.push_back(-1);   // sentinel: header row, no backing type
            m_enumIndex.push_back(-1);
        }
        ui->comboBox_PinsType->addItem(m_pinTypes[i].guiName);
        // Dropdown tooltip explaining the device + how its pins bind. Wrapped in
        // <qt> (escaped) so Qt treats it as rich text and word-wraps the long
        // descriptions instead of showing one very wide line.
        ui->comboBox_PinsType->setItemData(
            ui->comboBox_PinsType->count() - 1,
            QStringLiteral("<qt>%1</qt>").arg(m_pinTypes[i].description.toHtmlEscaped()),
            Qt::ToolTipRole);
        m_pinTypesIndex.push_back(i);
        m_enumIndex.push_back(m_pinTypes[i].deviceEnumIndex);
        lastGroup = group;
    };

    for (int i = 0; i < PIN_TYPE_COUNT; ++i) {
        // except
        tmp = false;
        for (int c = 0; c < typeExceptSize; ++c) {
            if (m_pinTypes[i].pinExcept[c] == 0){
                break;
            }
            if (m_pinTypes[i].pinExcept[c] == m_pinNumber){
                //++i;  // why?
                tmp = true;
                break;
            }
            for (int k = 0; k < pinListTypeSize; ++k)
            {
                if (m_pinList[m_pinNumber-1].pinType[k] == 0){
                    break;
                }
                if (m_pinTypes[i].pinExcept[c] == m_pinList[m_pinNumber-1].pinType[k]){
                    //++i;  // why?
                    tmp = true;
                    break;
                }
            }
        }
        if (tmp){
            continue;
        }

        // decide whether this role is legal for this pin -- added at most once
        bool shouldAdd = false;
        for (int j = 0; j < typeSize && !shouldAdd; ++j) {
            if (m_pinTypes[i].pinType[j] == 0) {
                break;
            }
            if (m_pinTypes[i].pinType[j] == ALL || m_pinTypes[i].pinType[j] == int(m_pinNumber)){
                shouldAdd = true;
                break;
            }
            for (int k = 0; k < pinListTypeSize; ++k)
            {
                if (m_pinList[m_pinNumber-1].pinType[k] == 0){
                    break;
                }
                if (m_pinTypes[i].pinType[j] == m_pinList[m_pinNumber-1].pinType[k]){
                    shouldAdd = true;
                    break;
                }
            }
        }
        /* Per-board veto: skip roles the active board's firmware can't honour
         * on this pin (set by PinConfig::applyBoardSpecificRoleFilters via
         * setExcludedRoles -- e.g. F411 strips I2C_SDA from slot 22 / PB2). */
        if (shouldAdd && m_excludedRoles.contains(m_pinTypes[i].deviceEnumIndex)) {
            shouldAdd = false;
        }
        if (shouldAdd) {
            addType(i);
        }
    }
    // change items text color // i = 1 -skip "NOT_USED"
    for (int i = 1; i < m_pinTypesIndex.size(); ++i) {
        if (m_pinTypesIndex[i] < 0) {   // skip separator sentinel rows
            continue;
        }
        ui->comboBox_PinsType->setItemData(i, QBrush(m_pinTypes[m_pinTypesIndex[i]].color), Qt::ForegroundRole);
    }
}

void PinComboBox::setExcludedRoles(const QSet<int> &excludedDeviceEnums)
{
    /* No-op if the set hasn't changed -- avoids an unnecessary combobox rebuild
     * (which also resets the current selection) on a redundant filter apply. */
    if (m_excludedRoles == excludedDeviceEnums) return;
    m_excludedRoles = excludedDeviceEnums;
    initializationPins(m_pinNumber);
}

void PinComboBox::indexChanged(int index)
{
    // separators carry a -1 sentinel and have no backing type; they aren't
    // selectable, but guard anyway so we never deref m_pinTypes[-1].
    if (index < 0 || index >= m_pinTypesIndex.size() || m_pinTypesIndex[index] < 0) {
        return;
    }
    if(m_pinTypesIndex.empty() == false && m_isInteracts == false)
    {
        // change text color
        if (index == 0) {
            applyTextColor(QColor());
        } else {
            applyTextColor(m_pinTypes[m_pinTypesIndex[index]].color);
        }

        int iteractionSize = sizeof(m_pinTypes->interaction) / sizeof(m_pinTypes->interaction[0]);
        int tmp = 0;
        for (int i = 0; i < iteractionSize; ++i) {
            if (m_isCall_Interaction == true && tmp == 0)
            {
                m_isCall_Interaction = false;
                // add
                if (m_pinTypes[m_pinTypesIndex[index]].interaction[i] > 0)
                {
                    m_isCall_Interaction = true;
                    for (int t = 0; t < 10; ++t)
                    {
                        if (m_pinTypes[m_pinTypesIndex[index]].interaction[t] > 0)
                        {
                            for (int k = 0; k < PIN_TYPE_COUNT; ++k) {
                                if(m_pinTypes[k].deviceEnumIndex == m_pinTypes[m_pinTypesIndex[index]].interaction[t]){
                                    emit valueChangedForInteraction(k, m_pinTypesIndex[index], m_pinNumber);
                                    break;
                                }
                            }
                        }
                    }
                }
                // delete
                for (int n = 0; n < iteractionSize; ++n) {
                    if (m_pinTypes[m_call_interaction].interaction[n] <= 0){
                        break;
                    }
                    if(m_pinTypes[m_call_interaction].interaction[n] > 0){

                        for (int m = 0; m < PIN_TYPE_COUNT; ++m) {
                            if(m_pinTypes[m].deviceEnumIndex == m_pinTypes[m_call_interaction].interaction[n]){
                                emit valueChangedForInteraction(NOT_USED, m, m_pinNumber);
                            }
                        }
                    }
                }

                m_call_interaction = m_pinTypesIndex[index];
                break;
            }

            // add
            else if (m_pinTypes[m_pinTypesIndex[index]].interaction[i] > 0)
            {
                m_isCall_Interaction = true;
                for (int k = 0; k < PIN_TYPE_COUNT; ++k) {
                    if(m_pinTypes[k].deviceEnumIndex == m_pinTypes[m_pinTypesIndex[index]].interaction[i]){
                        m_call_interaction = m_pinTypesIndex[index];
                        emit valueChangedForInteraction(k, m_pinTypesIndex[index], m_pinNumber);
                        tmp++;
                        break;
                    }
                }
            }
        }
    }

    // set current config
    if(m_pinTypesIndex.empty() == false){
        emit currentIndexChanged(m_enumIndex[index], m_previousIndex, m_pinNumber, ui->comboBox_PinsType->currentText());
        m_previousIndex = m_enumIndex[index];
        m_currentDevEnum = m_enumIndex[index];
    }

}


void PinComboBox::readFromConfig(uint pin)
{
    /* Default to "Not Used" (index 0) and only override when the stored role
     * is a valid option for this pin. Without the default, a pin the incoming
     * config does NOT assign would keep the previously-loaded config's value
     * -- so swapping or re-reading a device merged the old bindings with the
     * new ones instead of replacing them. Setting index 0 first guarantees a
     * clean slate per read. (setCurrentIndex is a no-op when the index is
     * already correct, so unchanged pins don't churn signals.) */
    int target = 0;
    for (int i = 0; i < m_enumIndex.size(); ++i) {
        if (gEnv.pDeviceConfig->config.pins[pin] == m_enumIndex[i])
        {
            target = int(i);
            break;
        }
    }
    ui->comboBox_PinsType->setCurrentIndex(target);
}

void PinComboBox::writeToConfig(uint pin)
{
    // Use the live combobox selection, not the cached m_currentDevEnum, which can
    // be stale after an interaction-driven clear (would otherwise re-write a
    // just-removed sensor role).
    gEnv.pDeviceConfig->config.pins[pin] = liveDevEnum();
}
