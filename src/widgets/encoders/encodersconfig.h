#ifndef ENCODERSCONFIG_H
#define ENCODERSCONFIG_H

#include <QWidget>
#include <QVector>
#include <QPair>
#include <QString>

class QLabel;
class QPushButton;

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
    /* Live activity refresh: for each encoder, light Pin A / Pin B from the
     * device's logical-button bitmap (params report) so the user sees which
     * side fires and can verify direction. Call on every params update while
     * the Encoders tab is visible. */
    void updateActivity();
    /* Re-render the rows from the current config WITHOUT auto-filling -- used on
     * load and after a Buttons-tab reorder (which already remapped the stored
     * pairs). Auto-fill is an edit-time convenience only; inventing pairs here
     * would mutate a freshly-loaded config (spurious dirty / phantom encoders). */
    void refreshDisplay();
    void fastEncoderSelected(const QString &pinGuiName, bool isSelected);
    /* Zero all per-row A/B press counters (and their edge-detect state). Called
     * on construct, from the Reset button, and from the debug window's Log Clear
     * so both counts share one reset point. Public so MainWindow can connect it. */
    void resetCounts();

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
    /* Shared resync: rebuild the button list, drop stale pairs, optionally
     * auto-fill empty slots, then refresh the rows. */
    void resync(bool autoFill);
    /* Grey out, in every row, the encoder buttons already assigned to a pair so
     * they can't be picked twice (each combo keeps its own pick selectable). */
    void applyUsageMasks();
    /* Red-border a row's Pin combos when A==B or the same button is used by
     * two different encoders. */
    void applyClashHighlight();

    Ui::EncodersConfig *ui;

    // One shared column header for the slow-encoder table (rows carry no headers
    // of their own) -- matches the Shift Registers screen's single-header style.
    QVector<QLabel *> m_encHeaderLabels;

    /* Per-row A/B press counts come from the SHARED DeviceConfig fire tally
     * (edge-counted once per packet, ungated) -- updateActivity() just reads
     * logFireCount[slot] for each row's Pin A / Pin B and shows it in the row's
     * square, so it always equals the debug log's "fires=" for that slot. The
     * Reset button zeroes the shared tally (which also zeroes the log's). */
    QPushButton *m_resetCountsBtn = nullptr;

    QList<Encoders *> m_encodersPtrList;
    QList<FastEncoder *> m_fastEncodersPtrList;
    QVector<QPair<int, QString>> m_encoderButtons;   // (button slot index, display name)
};

#endif // ENCODERSCONFIG_H
