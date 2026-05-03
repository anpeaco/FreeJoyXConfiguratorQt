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
    /* Phase 7: warn when the loaded INI's board_id doesn't match the
     * connected device. No automatic conversion -- writes will be
     * refused by firmware until the user edits the config or connects
     * the matching board. */
    static void crossBoardCheck(QWidget *parent, dev_config_t &devC);

};

#endif // CONFIGTOFILE_H
