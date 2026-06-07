#ifndef GROUPEDCOMBO_H
#define GROUPEDCOMBO_H

#include <QStyledItemDelegate>

class QComboBox;

// Shared popup styling + grouping for QComboBox drop-downs whose item lists are
// long enough to benefit from section headers (pin roles, button functions).
//
// A "header" row is a non-selectable, bold, banded divider that names the group
// of role rows beneath it (see GroupedComboDelegate). Headers may optionally be
// *collapsible*: clicking one folds/unfolds its rows in the popup, which keeps a
// long list (e.g. the 20 POV-hat entries) compact until the user wants it.
//
// The row->meaning mapping (which combo index backs which enum) stays the
// caller's responsibility -- callers already keep parallel index vectors and
// push a -1 sentinel for each header row added here.
namespace freejoy_ui {

// Item-data roles carried on combo items. UserRole+1 matches the value the pin
// dropdown used before this was extracted, so existing data keeps working.
constexpr int kGroupHeaderRole      = Qt::UserRole + 1;   // bool: row is a header
constexpr int kGroupCollapsibleRole = Qt::UserRole + 2;   // bool: header can fold
constexpr int kGroupCollapsedRole   = Qt::UserRole + 3;   // bool: header is folded
constexpr int kGroupSubGapRole      = Qt::UserRole + 4;   // bool: small gap above row
                                                          // (separates sub-clusters
                                                          // within one group, e.g. POV1/POV2)

// Popup-list delegate. Paints section headers as bold banded rows (a gap above
// and a smaller gap below, so each group reads as its own block) and role rows
// indented beneath, in their function colour (greyed if disabled). Collapsible
// headers also get a fold/unfold chevron at the right edge.
class GroupedComboDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *p, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};

// Installs the GroupedComboDelegate on a combo's popup view.
void installGroupedDelegate(QComboBox *box);

// Appends a non-selectable section-header row. Returns its row index. The caller
// must still push a matching sentinel into any parallel index vector it keeps.
int addGroupHeader(QComboBox *box, const QString &title, bool collapsible = false);

// Enables click-to-collapse on collapsible headers. Applies the initial fold
// state (every collapsible group folded unless it contains the current
// selection) and re-applies it whenever the selection changes, so a chosen
// row's group is always visible when the popup opens.
void enableGroupCollapse(QComboBox *box);

} // namespace freejoy_ui

#endif // GROUPEDCOMBO_H
