#ifndef VERSION_H
#define VERSION_H

/* The configurator's user-facing version is FREEJOYX_VERSION, defined
 * in the manually-synced common_defines.h pair (also visible to the
 * firmware). Pre-2026-05 this file carried two parallel version
 * concepts inherited from upstream (APP_VERSION = "1.7.3b0",
 * FORK_VERSION = "0.2.0"); both are retired. See issue
 * anpeaco/FreeJoyX#18 for the rationale.
 *
 * FORK_NAME stays here -- it's used as the USB manufacturer-string
 * filter in HidDevice's enumeration (FreeJoyX vs upstream FreeJoy)
 * and isn't versioned, so it's a separate concern from the semver. */

#include "common_defines.h"

#define FORK_NAME    "FreeJoyX"

/* APP_VERSION kept as an alias for FREEJOYX_VERSION so the .rc file
 * and any third-party reference still compile. Prefer FREEJOYX_VERSION
 * in new code. */
#define APP_VERSION  FREEJOYX_VERSION

#endif // VERSION_H
