#ifndef ENCODERSCONFIG_H
#define ENCODERSCONFIG_H

#include <QWidget>
#include <QVector>
#include <QPair>
#include <QString>

#include "deviceconfig.h"
#include "encoders.h"
#include "fastencoder.h"
#include "global.h"

namespace Ui {
class EncodersConfig;
}

class EncodersConfig : public QWidget
{
    Q_OBJECT

public:
    explicit EncodersConfig(QWidget *parent = nullptr);
    ~EncodersConfig();

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

    /* Pull the FastEncoder's Enable checkbox back into sync with the
     * actual pin state. Used by MainWindow's toggle handler after the
     * user dismisses the "pin already in use" confirmation, so the
     * checkbox doesn't get stuck on "checked" when no pin assignment
     * happened. */
    void refreshFastEncoderUi(int slotIndex);

signals:
    /* Forwarded from each FastEncoder when the user toggles its
     * Enable checkbox. MainWindow handles the cross-tab work
     * (checking PinConfig for current usage, prompting on conflict,
     * applying / clearing the FAST_ENCODER assignment on both pins). */
    void fastEncoderEnableToggleRequested(int slotIndex, bool desiredEnabled);

public slots:
    /* A pin gained or lost the "Encoder" marker on the Buttons tab. Rescan the
     * encoder-line buttons, drop pairs that reference a pin that's no longer an
     * encoder line, auto-fill empty slots with unused encoder buttons, then
     * refresh the rows. Auto-fill only ever touches EMPTY slots -- it never
     * re-pairs a slot the user has explicitly set. */
    void onEncoderButtonsChanged();
    void fastEncoderSelected(const QString &pinGuiName, bool isSelected);

private slots:
    /* A row's Pin A / Pin B / swap / mode was edited -> refresh clash
     * highlighting across all rows. */
    void onRowPairingEdited();

private:
    /* Scan dev_config.buttons[] for pins tagged "Encoder" (ENCODER_INPUT_A);
     * rebuild m_encoderButtons as sorted (slotIndex, "Button #N") pairs. */
    void rebuildEncoderButtonList();
    /* Clear any slow_encoders[] pair that points at a button no longer tagged
     * "Encoder" (both indices must be current encoder lines). */
    void dropStalePairs();
    /* Fill empty slow_encoders[] slots (MAX_FAST..) with consecutive pairs of
     * as-yet-unused encoder buttons (lower index = A). Never overwrites a slot
     * that already holds a pair. */
    void autoFillEmptyPairs();
    /* Push m_encoderButtons into every row and re-read the config into them. */
    void refreshRows();
    /* Red-border a row's Pin combos when A==B or the same button is used by
     * two different encoders. */
    void applyClashHighlight();

    Ui::EncodersConfig *ui;

    QList<Encoders *> m_encodersPtrList;
    QList<FastEncoder *> m_fastEncodersPtrList;
    QVector<QPair<int, QString>> m_encoderButtons;   // (button slot index, display name)
};

#endif // ENCODERSCONFIG_H
