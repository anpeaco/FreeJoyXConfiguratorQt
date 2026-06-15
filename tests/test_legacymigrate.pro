# Standalone test for the 0x0020 -> 0x0030 wire-format bump (MCP23017 i2c_gpio[]
# append) and its legacy migrator path in src/legacy/legacy_migrator.cpp.
# Build:
#   cd tests
#   qmake test_legacymigrate.pro
#   mingw32-make
#   ./release/test_legacymigrate.exe   (Windows MinGW puts it under release/)
#
# Links the migrator + InitConfig (stm_main.c); no GUI / FirmwareImage.

TEMPLATE = app
TARGET   = test_legacymigrate
CONFIG  += console c++17
CONFIG  -= app_bundle
QT      += testlib core
QT      -= gui

INCLUDEPATH += ../src ../src/legacy

SOURCES += \
    test_legacymigrate.cpp \
    ../src/legacy/legacy_migrator.cpp \
    ../src/stm_main.c

HEADERS += \
    ../src/legacy/legacy_migrator.h \
    ../src/legacy/legacy_types.h
