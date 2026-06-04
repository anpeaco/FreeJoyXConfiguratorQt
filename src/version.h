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

/* FREEJOYX_APP_VERSION is the *configurator's own* release version -- what's
 * shown in the window title, the App info card, and the .exe file properties.
 * CI (release.yml) injects it from the git release tag, e.g.
 * -DFREEJOYX_APP_VERSION=\"0.1.8\", so the app always states the version the
 * user downloaded. Local/dev builds fall back to the firmware-gen
 * FREEJOYX_VERSION with a "-dev" suffix (honestly "not a tagged release").
 *
 * This is deliberately DECOUPLED from FREEJOYX_VERSION (the firmware/wire
 * version, synced across both repos by header-sync CI and reported by the
 * device): the configurator releases on its own cadence -- see the memory note
 * project_version_display_sync and issue anpeaco/FreeJoyX#18. The numeric
 * components feed winapp.rc's FILEVERSION; CI passes them too, else they fall
 * back to FREEJOYX_VERSION_*. */
#ifndef FREEJOYX_APP_VERSION
#  define FREEJOYX_APP_VERSION  FREEJOYX_VERSION "-dev"
#endif
#ifndef FREEJOYX_APP_VER_MAJOR
#  define FREEJOYX_APP_VER_MAJOR  FREEJOYX_VERSION_MAJOR
#endif
#ifndef FREEJOYX_APP_VER_MINOR
#  define FREEJOYX_APP_VER_MINOR  FREEJOYX_VERSION_MINOR
#endif
#ifndef FREEJOYX_APP_VER_PATCH
#  define FREEJOYX_APP_VER_PATCH  FREEJOYX_VERSION_PATCH
#endif

/* String form of the app version built from the NUMERIC components. windres
 * can't ingest the injected FREEJOYX_APP_VERSION quoted-string -D (it breaks the
 * resource preprocessor's command line), so winapp.rc uses this instead -- the
 * same stringize trick FREEJOYX_VERSION uses, which the .rc has always handled.
 * C++ keeps using FREEJOYX_APP_VERSION directly (it carries any tag suffix). */
#define FREEJOYX_APP_VERSION_NUM \
    FREEJOYX_VER_STR(FREEJOYX_APP_VER_MAJOR) "." \
    FREEJOYX_VER_STR(FREEJOYX_APP_VER_MINOR) "." \
    FREEJOYX_VER_STR(FREEJOYX_APP_VER_PATCH)

/* APP_VERSION kept as an alias for the app version so any third-party
 * reference still compiles. Prefer FREEJOYX_APP_VERSION in new code. */
#define APP_VERSION  FREEJOYX_APP_VERSION

#endif // VERSION_H
