#include "deviceconfig.h"

extern "C" {
#include "stm_main.h"
}

DeviceConfig::DeviceConfig()
{
    config = InitConfig();
    //joyReport = {};
    paramsReport = {};
}

void DeviceConfig::resetConfig()
{
    config = InitConfig();
}

void DeviceConfig::tickFireCounts()
{
    /* One edge-detect pass over the current params bitmaps. Runs every params
     * packet, ungated -- the single point where fire tallies advance, so the
     * debug log and the Encoders-tab squares always agree. log_button_data /
     * phy_button_data are MAX_BUTTONS_NUM bits packed 8-per-byte. */
    for (int slot = 0; slot < MAX_BUTTONS_NUM; ++slot) {
        const bool logOn = paramsReport.log_button_data[slot >> 3] & (1 << (slot & 0x07));
        const bool phyOn = paramsReport.phy_button_data[slot >> 3] & (1 << (slot & 0x07));
        if (logOn && !m_logFirePrev[slot]) ++logFireCount[slot];
        if (phyOn && !m_physFirePrev[slot]) ++physFireCount[slot];
        m_logFirePrev[slot] = logOn;
        m_physFirePrev[slot] = phyOn;
    }
}

void DeviceConfig::resetFireCounts()
{
    for (int slot = 0; slot < MAX_BUTTONS_NUM; ++slot) {
        logFireCount[slot] = 0;
        physFireCount[slot] = 0;
        /* Re-seed prev-state from the live bitmap so a button already held down
         * at reset isn't counted as a fresh edge on the next packet. */
        m_logFirePrev[slot] = paramsReport.log_button_data[slot >> 3] & (1 << (slot & 0x07));
        m_physFirePrev[slot] = paramsReport.phy_button_data[slot >> 3] & (1 << (slot & 0x07));
    }
}
