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
