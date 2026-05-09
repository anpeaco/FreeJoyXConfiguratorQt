#ifndef REPORTCONVERTER_H
#define REPORTCONVERTER_H

#include "stdint.h"
#include <stddef.h>

#define BUFFERSIZE 64

namespace ReportConverter
{
    int paramReport(uint8_t *paramsBuffer);
    void resetReport();

    void getConfigFromDevice(uint8_t *hidBuffer);

    /* sendConfigToDevice now reads from an explicit source buffer + length
     * rather than the global current-shape dev_config_t, so the HID write
     * path can target older firmware shapes via the reverse migrator
     * (legacy_reverse_migrator.h). srcLen is the total wire size of the
     * config that will be written; requestConfigNumber is the 1-based
     * fragment index. */
    void sendConfigToDevice(uint8_t *hidBuf, uint8_t requestConfigNumber,
                            const uint8_t *src, size_t srcLen);
}

#endif // REPORTCONVERTER_H
