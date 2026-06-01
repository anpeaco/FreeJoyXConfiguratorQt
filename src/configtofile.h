#ifndef CONFIGTOFILE_H
#define CONFIGTOFILE_H

#include <QObject>

class QSettings;
#include "common_types.h"

class ConfigToFile : public QObject
{
    Q_OBJECT
public:
//    explicit ConfigToFile(QObject *parent = nullptr);

    static void loadDeviceConfigFromFile(QWidget *parent, const QString &fileName, dev_config_t &devC);
    static void saveDeviceConfigToFile(const QString &fileName, dev_config_t &devC);

    /* #60 (board_id provenance): the firmware doesn't persist board_id in
     * dev_config_t, so a config read from a device -- or an older / hand-built
     * file -- carries board_id = 0 (unknown). When the device reports a known
     * board, stamp it onto an UNKNOWN config board_id so the config round-trips
     * honestly and the firmware's board check doesn't reject the write with
     * 0xFD. A non-zero (already-known) config board_id is left untouched -- a
     * genuine cross-board mismatch is the convert prompt's responsibility, not
     * this. No-op when deviceBoardId is 0 (device board unknown too). */
    static void stampBoardIdFromDevice(dev_config_t &devC, uint8_t deviceBoardId);
//signals:

private:
    static void oldConfigHandler(QWidget *parent, dev_config_t &devC);
    /* If the loaded INI's board_id doesn't match the connected device,
     * prompt the user to convert. Conversion is just board_id +
     * firmware_version refresh -- the wire-format pin slots map
     * identically across BluePill / BlackPill except for slot 22
     * (PB11 vs PB2). I2C on slot 21/22 is cleared on forward
     * conversion to BlackPill since PB2 isn't I2C-bonded on F411
     * UFQFPN48. Other slot-22 usages are preserved with a
     * physical-pin warning. */
    static void crossBoardCheck(QWidget *parent, dev_config_t &devC);

};

#endif // CONFIGTOFILE_H
