#ifndef PINTYPEHELPER_H
#define PINTYPEHELPER_H

#include <QWidget>
#include "common_types.h"

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
};

#endif // PINTYPEHELPER_H
