#include "groupedcombo.h"

#include "style_helpers.h"

#include <QComboBox>
#include <QEvent>
#include <QFont>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QStyleOptionViewItem>

namespace freejoy_ui {

namespace {

// Blank space painted above every section header so the groups read as
// distinct blocks rather than a continuous list.
constexpr int kGroupGap = 10;
// Small blank space below the header band, separating it from its role rows.
constexpr int kHeaderBelowGap = 4;
// Small blank space above a sub-cluster row (kGroupSubGapRole), e.g. between
// the POV1 and POV2 blocks inside the POV-hats group.
constexpr int kSubGap = 6;

} // namespace

void GroupedComboDelegate::paint(QPainter *p, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const bool header      = index.data(kGroupHeaderRole).toBool();
    const bool collapsible = index.data(kGroupCollapsibleRole).toBool();
    const bool collapsed   = index.data(kGroupCollapsedRole).toBool();
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
    QRect bandRect = opt.rect;
    if (header) {
        leftPad = 8;           // headers flush-left so the indent reads
        // Leave a blank gap above (and a smaller one below) the band -- the
        // gaps stay the plain base colour painted above -- so each group
        // reads as its own block separated from the row above and below.
        bandRect.setTop(opt.rect.top() + kGroupGap);
        bandRect.setBottom(opt.rect.bottom() - kHeaderBelowGap);
        // Banded header: a darker grey on dark themes, a very light grey on
        // light themes, with matching-contrast text. Bold + the band + the
        // extra row padding (see sizeHint) make the section dividers clear.
        p->fillRect(bandRect, base.darker(darkUi ? 140 : 108));
        fg = darkUi ? QColor(238, 238, 238) : QColor(45, 45, 45);
        f.setBold(true);
    } else {
        // A sub-cluster row carries a small gap above it (stays plain base);
        // the row content + any hover wash sit below the gap.
        if (index.data(kGroupSubGapRole).toBool()) {
            bandRect.setTop(opt.rect.top() + kSubGap);
        }
        if (active) {
            // subtle neutral hover/selection -- not the loud function-colour fill
            p->fillRect(bandRect, darkUi ? QColor(255, 255, 255, 28)
                                         : QColor(0, 0, 0, 22));
        }
        fg = !enabled ? opt.palette.color(QPalette::Disabled, QPalette::Text)
                      : (role.isValid() ? role : opt.palette.color(QPalette::Text));
    }

    p->setPen(fg);
    p->setFont(f);

    // Collapsible headers get a lucide chevron at the right edge: chevron-right
    // when folded, chevron-down when expanded (tinted to the header ink so it
    // tracks the theme, matching the combo's own drop-arrow and the disclosure
    // toggles).
    int rightPad = 8;
    if (header && collapsible) {
        const int sz = 14;
        const QPixmap chev = freejoy_style::tintedSvgPixmap(
            collapsed ? QStringLiteral(":/Images/icons/lucide/chevron-right-neutral.svg")
                      : QStringLiteral(":/Images/icons/lucide/chevron-down-neutral.svg"),
            QSize(sz, sz), fg);
        p->drawPixmap(bandRect.right() - 8 - sz,
                      bandRect.center().y() - sz / 2 + 1, chev);
        rightPad = 8 + sz + 8;
    }

    // Text sits inside the band (below the gap for headers).
    const QRect tr = bandRect.adjusted(leftPad, 0, -rightPad, 0);
    p->drawText(tr, Qt::AlignVCenter | Qt::AlignLeft,
                opt.fontMetrics.elidedText(text, Qt::ElideRight, tr.width()));
    p->restore();
}

QSize GroupedComboDelegate::sizeHint(const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    // Guarantee the row clears the full font height + a little breathing room,
    // so descenders (g, y, p) aren't clipped at the small default UI font size.
    s.setHeight(qMax(s.height(), option.fontMetrics.height() + 6));
    if (index.data(kGroupHeaderRole).toBool()) {
        // +2px top/bottom band padding, a gap above the band, and a smaller
        // gap below it separating the header from its role rows.
        s.setHeight(s.height() + 4 + kGroupGap + kHeaderBelowGap);
    } else if (index.data(kGroupSubGapRole).toBool()) {
        s.setHeight(s.height() + kSubGap);   // gap above a sub-cluster row
    }
    return s;
}

namespace {

// Returns the model rows belonging to a header (header row + 1 up to but not
// including the next header row or the end of the list).
void groupRange(const QComboBox *box, int headerRow, int &firstChild, int &endRow)
{
    firstChild = headerRow + 1;
    endRow = box->count();
    for (int r = headerRow + 1; r < box->count(); ++r) {
        if (box->itemData(r, kGroupHeaderRole).toBool()) { endRow = r; break; }
    }
}

// Owns the click-to-collapse behaviour for one combo.
//
// Everything is LAZY: the controller only watches the combo until the user
// first opens the popup. It never touches view() at construction (creating 128
// popup containers up front -- one per button row -- froze startup), and it does
// NOT re-fold on every currentIndexChanged (that ran a full doItemsLayout for
// every one of 128 rows on each config load -- the "stopped reading" stall).
// Instead the fold state is recomputed just before each popup opens, and only
// rows whose state actually changed trigger a relayout.
class GroupCollapseController : public QObject
{
public:
    explicit GroupCollapseController(QComboBox *box)
        : QObject(box), m_box(box)
    {
        m_box->installEventFilter(this);   // cheap: no view() yet
    }

    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        // On the combo itself: a click or key press may open the popup. Wire up
        // the viewport filter (once) and refresh the fold state BEFORE the popup
        // computes its geometry, so it opens at the right height.
        if (obj == m_box) {
            const QEvent::Type t = ev->type();
            if (t == QEvent::MouseButtonPress || t == QEvent::KeyPress) {
                ensureViewportHooked();
                applyFromSelection();
            }
            return QObject::eventFilter(obj, ev);
        }

        QListView *lv = listView();
        if (lv && obj == lv->viewport()) {
            const QEvent::Type t = ev->type();
            if (t == QEvent::MouseButtonPress || t == QEvent::MouseButtonRelease
                || t == QEvent::MouseButtonDblClick) {
                auto *me = static_cast<QMouseEvent *>(ev);
                const QModelIndex idx = lv->indexAt(me->pos());
                if (idx.isValid()
                    && idx.data(kGroupHeaderRole).toBool()
                    && idx.data(kGroupCollapsibleRole).toBool()) {
                    if (t == QEvent::MouseButtonRelease) {
                        const bool collapsed = idx.data(kGroupCollapsedRole).toBool();
                        setCollapsed(idx.row(), !collapsed);
                    }
                    return true;   // never let a header click select or close
                }
            }
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    QListView *listView() const { return qobject_cast<QListView *>(m_box->view()); }

    // Install the header-click filter on the popup viewport the first time the
    // popup is about to open (this is what forces view() creation -- once, on
    // demand, not 128x at startup).
    void ensureViewportHooked()
    {
        if (m_viewportHooked) return;
        if (QListView *lv = listView()) {
            lv->viewport()->installEventFilter(this);
            m_viewportHooked = true;
        }
    }

    void setCollapsed(int headerRow, bool collapsed)
    {
        // Skip when nothing changes -- avoids a needless relayout (the hot path
        // during a config load, where the fold state is almost always stable).
        if (m_box->itemData(headerRow, kGroupCollapsedRole).toBool() == collapsed) {
            return;
        }
        m_box->setItemData(headerRow, collapsed, kGroupCollapsedRole);
        QListView *lv = listView();
        if (!lv) return;
        int first, end;
        groupRange(m_box, headerRow, first, end);
        for (int r = first; r < end; ++r) {
            lv->setRowHidden(r, collapsed);
        }
        lv->doItemsLayout();
        lv->viewport()->update();
    }

    // Fold every collapsible group except the one holding the current
    // selection, so a chosen row is always visible when the popup opens.
    void applyFromSelection()
    {
        const int cur = m_box->currentIndex();
        for (int h = 0; h < m_box->count(); ++h) {
            if (!m_box->itemData(h, kGroupHeaderRole).toBool()) continue;
            if (!m_box->itemData(h, kGroupCollapsibleRole).toBool()) continue;
            int first, end;
            groupRange(m_box, h, first, end);
            const bool contains = (cur >= first && cur < end);
            setCollapsed(h, !contains);
        }
    }

    QPointer<QComboBox> m_box;
    bool m_viewportHooked = false;
};

} // namespace

void installGroupedDelegate(QComboBox *box)
{
    box->setItemDelegate(new GroupedComboDelegate(box));
}

int addGroupHeader(QComboBox *box, const QString &title, bool collapsible)
{
    const int row = box->count();
    box->addItem(title);
    box->setItemData(row, 0, Qt::UserRole - 1);            // non-selectable
    box->setItemData(row, true, kGroupHeaderRole);
    if (collapsible) {
        box->setItemData(row, true, kGroupCollapsibleRole);
        box->setItemData(row, false, kGroupCollapsedRole);
    }
    return row;
}

void enableGroupCollapse(QComboBox *box)
{
    new GroupCollapseController(box);   // parented to box; self-managing
}

} // namespace freejoy_ui
