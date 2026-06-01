#ifndef PINTYPEHELPER_H
#define PINTYPEHELPER_H

#include <QHash>
#include <QTimer>
#include <QWidget>
#include "common_types.h"

class QLabel;

namespace Ui {
class PinTypeHelper;
}

class PinTypeHelper : public QWidget
{
    Q_OBJECT

public:
    explicit PinTypeHelper(QWidget *parent = nullptr);
    ~PinTypeHelper();

    //! Bus identifiers for the quick-setup toggles.
    enum BusType { BUS_I2C = 0, BUS_SPI = 1 };

    //! Reflect a bus's current state in its toggle without re-emitting:
    //! `checked` ticks the box, `enabled` greys it out (used to enforce the
    //! F411 SPI/I2C mutex and to lock SPI while a sensor owns the bus).
    void setBusState(int bus, bool checked, bool enabled);

signals:
    void helpHovered(pin_t pinType, bool hovered);
    //! Emitted when the user clicks a bus quick-setup checkbox.
    void busToggleRequested(int bus, bool enable);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::PinTypeHelper *ui;

    /* Each Pin Info row's wiring-detail HTML, moved out of the widget tooltip
     * so it only pops when the row's info icon (an <a> link) is hovered, not
     * anywhere on the row. Keyed by the row label. */
    QHash<QLabel *, QString> m_rowHelp;

    /* Delay timer for the row tooltip: started on icon-hover, and only when it
     * fires (after ~600 ms) does the pending row's tooltip actually show. */
    QTimer m_tipTimer;
    QLabel *m_pendingTipLabel = nullptr;
};

#endif // PINTYPEHELPER_H
