// Round-trip + legacy-synthesis tests for slow-encoder persistence
// (wire gen 0x0040). Covers:
//   - slow_encoders[] {btn_a, btn_b} and the encoders[] swap/mode byte survive
//     a .cfg save/load;
//   - a pre-0x0040 file (no BtnA/BtnB keys) with positional ENCODER_INPUT_A/_B
//     buttons has its pairs SYNTHESISED on load (encoders not lost).
//
// Drives ConfigToFile directly; gEnv.pDeviceConfig stays null so
// crossBoardCheck() early-returns and this isolates the QSettings paths.
#include <QtTest>
#include <QSettings>
#include <QTemporaryDir>
#include <cstring>
#include "configtofile.h"
#include "common_defines.h"   // FIRMWARE_VERSION, SLOW_ENC_*, MAX_*
#include "common_types.h"     // ENCODER_INPUT_A/_B, ENCODER_CONF_*
#include "global.h"

GlobalEnvironment gEnv;   // satisfy the extern in global.h

class TestEncoderPersist : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;
    QString path(const char *name) { return m_dir.filePath(QString::fromLatin1(name)); }

    // Zeroed config stamped current, so oldConfigHandler() is a no-op.
    static void zeroCurrent(dev_config_t &c)
    {
        std::memset(&c, 0, sizeof(c));
        c.firmware_version = FIRMWARE_VERSION;
    }

private slots:
    // Explicit pairs + the detent-mode/swap byte round-trip verbatim.
    void slowEncoderPairs_and_swap_roundTrip()
    {
        dev_config_t a; zeroCurrent(a);
        a.slow_encoders[MAX_FAST_ENCODER_NUM + 0].btn_a = 5;
        a.slow_encoders[MAX_FAST_ENCODER_NUM + 0].btn_b = 6;
        a.slow_encoders[MAX_FAST_ENCODER_NUM + 1].btn_a = 40;
        a.slow_encoders[MAX_FAST_ENCODER_NUM + 1].btn_b = 41;
        // detent mode + direction swap packed into encoders[i]
        a.encoders[MAX_FAST_ENCODER_NUM + 0] = ENCODER_CONF_4x | SLOW_ENC_SWAP;
        a.encoders[MAX_FAST_ENCODER_NUM + 1] = ENCODER_CONF_2x;   // no swap

        const QString f = path("enc_roundtrip.ini");
        ConfigToFile::saveDeviceConfigToFile(f, a);

        dev_config_t b; zeroCurrent(b);
        for (int i = 0; i < MAX_ENCODERS_NUM; ++i) {             // poison
            b.slow_encoders[i].btn_a = 99;
            b.slow_encoders[i].btn_b = 99;
            b.encoders[i] = 0x7F;
        }
        ConfigToFile::loadDeviceConfigFromFile(nullptr, f, b);

        for (int i = 0; i < MAX_ENCODERS_NUM; ++i) {
            QCOMPARE(b.slow_encoders[i].btn_a, a.slow_encoders[i].btn_a);
            QCOMPARE(b.slow_encoders[i].btn_b, a.slow_encoders[i].btn_b);
            QCOMPARE(b.encoders[i], a.encoders[i]);
        }
        // Spot-check the swap bit specifically.
        QVERIFY(b.encoders[MAX_FAST_ENCODER_NUM + 0] & SLOW_ENC_SWAP);
        QVERIFY(!(b.encoders[MAX_FAST_ENCODER_NUM + 1] & SLOW_ENC_SWAP));
        QCOMPARE(b.encoders[MAX_FAST_ENCODER_NUM + 0] & SLOW_ENC_MODE_MASK,
                 static_cast<uint8_t>(ENCODER_CONF_4x));
    }

    // A pre-0x0040 file: positional ENCODER_INPUT_A/_B buttons, no BtnA/BtnB
    // keys. On load the pairs must be synthesised from the button layout.
    void legacyFile_synthesizesPairsFromPositional()
    {
        // Build a config with two positional encoders, save it, then strip the
        // BtnA/BtnB keys to mimic a file written before slow_encoders existed.
        dev_config_t a; zeroCurrent(a);
        a.firmware_version = 0x0030;                 // pre-0x0040 stamp
        a.buttons[5].type  = ENCODER_INPUT_A;        // encoder: A=5, B=6
        a.buttons[6].type  = ENCODER_INPUT_B;
        a.buttons[10].type = ENCODER_INPUT_A;        // encoder: A=10, B=11
        a.buttons[11].type = ENCODER_INPUT_B;

        const QString f = path("enc_legacy.ini");
        ConfigToFile::saveDeviceConfigToFile(f, a);
        {
            QSettings s(f, QSettings::IniFormat);
            for (int i = 0; i < MAX_ENCODERS_NUM; ++i) {
                s.beginGroup("EncodersConfig_" + QString::number(i));
                s.remove("BtnA");
                s.remove("BtnB");
                s.endGroup();
            }
        }

        dev_config_t b; zeroCurrent(b);
        ConfigToFile::loadDeviceConfigFromFile(nullptr, f, b);

        // Fast slots stay unwired; the two positional encoders materialise at
        // slots MAX_FAST_ENCODER_NUM + 0/1.
        for (int i = 0; i < MAX_FAST_ENCODER_NUM; ++i) {
            QCOMPARE(b.slow_encoders[i].btn_a, static_cast<int8_t>(-1));
            QCOMPARE(b.slow_encoders[i].btn_b, static_cast<int8_t>(-1));
        }
        QCOMPARE(b.slow_encoders[MAX_FAST_ENCODER_NUM + 0].btn_a, static_cast<int8_t>(5));
        QCOMPARE(b.slow_encoders[MAX_FAST_ENCODER_NUM + 0].btn_b, static_cast<int8_t>(6));
        QCOMPARE(b.slow_encoders[MAX_FAST_ENCODER_NUM + 1].btn_a, static_cast<int8_t>(10));
        QCOMPARE(b.slow_encoders[MAX_FAST_ENCODER_NUM + 1].btn_b, static_cast<int8_t>(11));
    }

    // A current file with stored pairs must NOT be re-synthesised: a new config
    // stores both encoder pins as ENCODER_INPUT_A (219), so positional
    // synthesis would find no B and wipe the real pairs. The key-presence gate
    // must take the stored pairs.
    void currentFile_keepsStoredPairs_noResynthesis()
    {
        dev_config_t a; zeroCurrent(a);
        a.buttons[7].type = ENCODER_INPUT_A;         // both pins marked "Encoder"
        a.buttons[8].type = ENCODER_INPUT_A;         // (single-type UI, value 219)
        a.slow_encoders[MAX_FAST_ENCODER_NUM].btn_a = 7;
        a.slow_encoders[MAX_FAST_ENCODER_NUM].btn_b = 8;

        const QString f = path("enc_current.ini");
        ConfigToFile::saveDeviceConfigToFile(f, a);

        dev_config_t b; zeroCurrent(b);
        ConfigToFile::loadDeviceConfigFromFile(nullptr, f, b);

        // Stored pair survives even though there is no ENCODER_INPUT_B button
        // for the positional synthesis to pair with.
        QCOMPARE(b.slow_encoders[MAX_FAST_ENCODER_NUM].btn_a, static_cast<int8_t>(7));
        QCOMPARE(b.slow_encoders[MAX_FAST_ENCODER_NUM].btn_b, static_cast<int8_t>(8));
    }
};

QTEST_MAIN(TestEncoderPersist)
#include "test_encoderpersist.moc"
