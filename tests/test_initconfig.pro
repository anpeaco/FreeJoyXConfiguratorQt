# Guards the factory-default seed produced by InitConfig() (src/stm_main.c).
#
# Regression target: tap_cutoff_ms / double_tap_window_ms were missing from the
# designated initializer, so they silently seeded to 0 -- which collapses the
# gesture window and makes TAP / DOUBLE_TAP never fire on any config that comes
# from InitConfig() (new/blank configs, old INIs missing the keys, and legacy
# migrations that overlay onto the InitConfig base). This test fails if anyone
# drops those fields again.
#
# Build:
#   cd tests
#   qmake test_initconfig.pro
#   mingw32-make
#   ./release/test_initconfig.exe   (Windows MinGW puts it under release/)
#
# stm_main.c is C; it links cleanly here because the test wraps its include in
# extern "C" (same bridge the app uses in deviceconfig.cpp).

TEMPLATE = app
TARGET   = test_initconfig
CONFIG  += console c++17
CONFIG  -= app_bundle
QT      += testlib core

INCLUDEPATH += ../src

SOURCES += \
    test_initconfig.cpp \
    ../src/stm_main.c

HEADERS += ../src/stm_main.h
