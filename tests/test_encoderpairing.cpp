// Headless test for the Encoders-tab container logic (EncodersConfig): the
// positional auto-fill, the "fill empty slots only / never clobber" rule, and
// the drop-stale-pair behavior when a pin loses its "Encoder" marker.
//
// Drives EncodersConfig::onEncoderButtonsChanged() over a crafted dev_config_t
// and asserts what lands in config.slow_encoders[]. Slow encoder slots start at
// MAX_FAST_ENCODER_NUM (fast slots are unused here).
#include <QtTest>
#include "global.h"
#include "deviceconfig.h"
#include "common_defines.h"
#include "common_types.h"
#include "widgets/encoders/encodersconfig.h"
#include "widgets/encoders/encoders.h"

GlobalEnvironment gEnv;

class TestEncoderPairing : public QObject
{
    Q_OBJECT

    DeviceConfig *m_dc = nullptr;

    static const int FIRST = MAX_FAST_ENCODER_NUM;   // first slow-encoder slot

    void tagEncoder(int btn) { m_dc->config.buttons[btn].type = ENCODER_INPUT_A; }
    void setPair(int slot, int a, int b)
    {
        m_dc->config.slow_encoders[slot].btn_a = int8_t(a);
        m_dc->config.slow_encoders[slot].btn_b = int8_t(b);
    }
    int a(int slot) const { return m_dc->config.slow_encoders[slot].btn_a; }
    int b(int slot) const { return m_dc->config.slow_encoders[slot].btn_b; }

private slots:
    void init()
    {
        m_dc = new DeviceConfig();
        m_dc->resetConfig();                 // slow_encoders default to -1/-1
        gEnv.pDeviceConfig = m_dc;
    }
    void cleanup()
    {
        delete m_dc; m_dc = nullptr;
        gEnv.pDeviceConfig = nullptr;
    }

    // Four fresh encoder pins auto-fill the first two empty slow slots
    // positionally (lower index = Pin A).
    void autoFill_positional()
    {
        tagEncoder(5); tagEncoder(6); tagEncoder(10); tagEncoder(11);
        EncodersConfig w;
        w.onEncoderButtonsChanged();
        QCOMPARE(a(FIRST + 0), 5);   QCOMPARE(b(FIRST + 0), 6);
        QCOMPARE(a(FIRST + 1), 10);  QCOMPARE(b(FIRST + 1), 11);
        for (int s = FIRST + 2; s < MAX_ENCODERS_NUM; ++s) {
            QCOMPARE(a(s), -1); QCOMPARE(b(s), -1);
        }
    }

    // The critical rule: auto-fill only touches EMPTY slots. An explicit pair a
    // user already set is never re-paired; a newly added pin fills a *new* row.
    void autoFill_neverClobbersExistingPair()
    {
        tagEncoder(5); tagEncoder(6); tagEncoder(10); tagEncoder(11);
        setPair(FIRST + 0, 10, 11);          // user's explicit choice in slot 0
        EncodersConfig w;
        w.onEncoderButtonsChanged();
        // slot 0 is left exactly as set...
        QCOMPARE(a(FIRST + 0), 10);  QCOMPARE(b(FIRST + 0), 11);
        // ...and the remaining unused pins (5,6) fill the next empty slot.
        QCOMPARE(a(FIRST + 1), 5);   QCOMPARE(b(FIRST + 1), 6);
    }

    // An odd number of encoder pins: the unpaired one is left waiting, not
    // force-paired with something.
    void autoFill_oddPinLeftUnpaired()
    {
        tagEncoder(5); tagEncoder(6); tagEncoder(10);
        EncodersConfig w;
        w.onEncoderButtonsChanged();
        QCOMPARE(a(FIRST + 0), 5);   QCOMPARE(b(FIRST + 0), 6);
        QCOMPARE(a(FIRST + 1), -1);  QCOMPARE(b(FIRST + 1), -1);   // 10 is unpaired
    }

    // A pin that loses its "Encoder" marker takes its pair down with it: the
    // slot is cleared (a half-valid pair is useless).
    void dropStale_whenPinUntagged()
    {
        tagEncoder(5); tagEncoder(6);
        setPair(FIRST + 0, 5, 6);
        // pin 6 is no longer an encoder line:
        m_dc->config.buttons[6].type = BUTTON_NORMAL;
        EncodersConfig w;
        w.onEncoderButtonsChanged();
        QCOMPARE(a(FIRST + 0), -1);  QCOMPARE(b(FIRST + 0), -1);
    }

    // No encoder pins at all -> nothing is invented (guards against the
    // designated-init {0,0} phantom-encoder trap at the container level).
    void noEncoderButtons_staysEmpty()
    {
        EncodersConfig w;
        w.onEncoderButtonsChanged();
        for (int s = FIRST; s < MAX_ENCODERS_NUM; ++s) {
            QCOMPARE(a(s), -1); QCOMPARE(b(s), -1);
        }
    }

    // readFromConfig respects stored pairs and does NOT auto-fill: load must be
    // non-mutating (auto-fill is an edit-time convenience). A leftover tagged
    // pin is left available, not force-paired on load.
    void readFromConfig_respectsStoredPairs_noAutoFill()
    {
        tagEncoder(5); tagEncoder(6); tagEncoder(10); tagEncoder(11);
        setPair(FIRST + 0, 10, 11);          // as if loaded from a config
        EncodersConfig w;
        w.readFromConfig();
        QCOMPARE(a(FIRST + 0), 10);  QCOMPARE(b(FIRST + 0), 11);   // respected
        QCOMPARE(a(FIRST + 1), -1);  QCOMPARE(b(FIRST + 1), -1);   // NOT auto-filled
    }

    // Regression (review Bug 2): a migrated config where the old firmware left
    // some Encoder-tagged pins inert (mismatched A/B counts) must NOT gain a
    // phantom encoder on load. Here the stored pair is {5,8}; pins 6 and 7 are
    // tagged but were never paired. Load must leave them unpaired, not invent
    // slow_encoders={6,7}.
    void readFromConfig_noPhantomFromInertTaggedPins()
    {
        tagEncoder(5); tagEncoder(6); tagEncoder(7); tagEncoder(8);
        setPair(FIRST + 0, 5, 8);            // the only pair the old fw formed
        EncodersConfig w;
        w.readFromConfig();
        QCOMPARE(a(FIRST + 0), 5);   QCOMPARE(b(FIRST + 0), 8);    // stored pair kept
        for (int s = FIRST + 1; s < MAX_ENCODERS_NUM; ++s) {       // no phantom (6,7)
            QCOMPARE(a(s), -1); QCOMPARE(b(s), -1);
        }
    }

    // refreshDisplay (load / post-reorder path) also never auto-fills, even with
    // unpaired tagged pins available.
    void refreshDisplay_neverAutoFills()
    {
        tagEncoder(5); tagEncoder(6); tagEncoder(10); tagEncoder(11);
        EncodersConfig w;
        w.refreshDisplay();
        for (int s = FIRST; s < MAX_ENCODERS_NUM; ++s) {
            QCOMPARE(a(s), -1); QCOMPARE(b(s), -1);
        }
    }

    // The Swap button exchanges Pin A / Pin B (which reverses direction on the
    // device), persisting the exchange to slow_encoders[].
    void swapInputs_exchangesPinsAndPersists()
    {
        tagEncoder(5); tagEncoder(6);
        setPair(FIRST + 0, 5, 6);

        Encoders row(0);                 // configIndex == MAX_FAST_ENCODER_NUM
        QCOMPARE(row.configIndex(), FIRST);
        row.setEncoderButtons({ qMakePair(5, QStringLiteral("Button #6")),
                                qMakePair(6, QStringLiteral("Button #7")) });
        row.readFromConfig();
        QCOMPARE(row.inputA(), 5);  QCOMPARE(row.inputB(), 6);

        row.swapInputs();

        QCOMPARE(row.inputA(), 6);  QCOMPARE(row.inputB(), 5);   // UI exchanged
        QCOMPARE(a(FIRST + 0), 6);  QCOMPARE(b(FIRST + 0), 5);   // and persisted
    }
};

QTEST_MAIN(TestEncoderPairing)
#include "test_encoderpairing.moc"
