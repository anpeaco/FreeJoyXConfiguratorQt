#ifndef DEVICECONFIG_H
#define DEVICECONFIG_H

#include "common_defines.h"
#include "common_types.h"

class DeviceConfig
{
public:
    DeviceConfig();

    void resetConfig();

    dev_config_t config;
    //joy_report_t joyReport;
    params_report_t paramsReport;

    /* Single source of truth for "how many times has button N fired since the
     * last reset". Edge-counted once per params packet by tickFireCounts(),
     * UNGATED (independent of which tab is visible), so every consumer -- the
     * debug log's "fires=" field and the Encoders tab's per-row A/B squares --
     * reads the same tally and they can never drift apart. Indexed by 0-based
     * button slot (the same index as log_button_data / phy_button_data bits).
     * Previously each consumer ran its own edge-detector behind its own
     * visibility gate, so a frozen gate left stale prev-state and the two
     * counts diverged permanently. */
    int  logFireCount[MAX_BUTTONS_NUM] = {0};
    int  physFireCount[MAX_BUTTONS_NUM] = {0};

    /* Advance the fire tallies from the current paramsReport (rising edges on
     * log_button_data / phy_button_data). Call exactly once per params packet,
     * before any display refresh, with no tab/visibility gate. */
    void tickFireCounts();

    /* Zero every fire tally (the shared "since reset" point). Called by the
     * debug window's Log Clear and the Encoders tab's Reset -- one zero point
     * for both displays. */
    void resetFireCounts();

private:
    /* Rising-edge state for tickFireCounts(): last-seen bit per slot. */
    bool m_logFirePrev[MAX_BUTTONS_NUM] = {false};
    bool m_physFirePrev[MAX_BUTTONS_NUM] = {false};
};

#endif // DEVICECONFIG_H
