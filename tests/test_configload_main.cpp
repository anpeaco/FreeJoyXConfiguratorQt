// Deterministic headless harness for the file-load path.
// gEnv.pDeviceConfig stays null, so crossBoardCheck() no-ops (it early-returns
// when deviceBoard == 0). This isolates QSettings parse + field mapping in
// loadDeviceConfigFromFile() from any device / GUI / race.
#include <QCoreApplication>
#include <QSettings>
#include <QDebug>
#include <cstring>
#include "configtofile.h"
#include "global.h"

GlobalEnvironment gEnv;   // satisfy the extern in global.h

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QString f = (argc > 1) ? QString::fromLocal8Bit(argv[1])
                           : QStringLiteral("../fixtures/f15e_ufc_upstream_1713.ini");

    // --- Hypothesis A, raw layer: can QSettings even read this file? ---
    QSettings raw(f, QSettings::IniFormat);
    qDebug() << "[HARNESS] QSettings.status() =" << raw.status()
             << "(0 == NoError)";
    raw.beginGroup("PinsConfig");
    qDebug() << "[HARNESS] raw A0 =" << raw.value("A0", "MISSING").toString()
             << " raw B3 =" << raw.value("B3", "MISSING").toString();
    raw.endGroup();
    raw.beginGroup("DeviceUsbConfig");
    qDebug() << "[HARNESS] raw DeviceName =" << raw.value("DeviceName", "MISSING").toString();
    raw.endGroup();

    // --- Full load path under test ---
    dev_config_t cfg;
    std::memset(&cfg, 0, sizeof(cfg));
    ConfigToFile::loadDeviceConfigFromFile(nullptr, f, cfg);

    qDebug() << "[HARNESS] === after loadDeviceConfigFromFile ===";
    qDebug() << "[HARNESS] firmware_version =" << Qt::hex << cfg.firmware_version << Qt::dec;
    qDebug() << "[HARNESS] board_id =" << cfg.board_id;
    qDebug() << "[HARNESS] device_name =" << QString::fromLatin1(cfg.device_name);
    qDebug() << "[HARNESS] pins[0..7] (expect 5 5 5 5 5 5 5 5 = AXIS_ANALOG):"
             << cfg.pins[0] << cfg.pins[1] << cfg.pins[2] << cfg.pins[3]
             << cfg.pins[4] << cfg.pins[5] << cfg.pins[6] << cfg.pins[7];
    qDebug() << "[HARNESS] pins[14..19] (expect 28 19 20 20 20 20 = shift-reg):"
             << cfg.pins[14] << cfg.pins[15] << cfg.pins[16]
             << cfg.pins[17] << cfg.pins[18] << cfg.pins[19];

    const bool pass = (cfg.pins[0] == 5 && cfg.pins[14] == 28 && cfg.pins[16] == 20);
    qDebug() << "[HARNESS] RESULT:" << (pass ? "PASS (load populated the struct)"
                                             : "FAIL (struct stayed empty -> load bug)");
    return pass ? 0 : 1;
}
