# Guards the 0x0050 -> 0x0060 shift migration: shift_config[] (logical-button
# indices) must be lifted into the dedicated shift_buttons[] array so a user's
# shift mapping survives the wire-format bump, and the original buttons[] slots
# must be left intact (no HID renumber).
#
# Build:
#   cd tests
#   qmake test_shiftmigrate.pro
#   mingw32-make
#   ./release/test_shiftmigrate.exe
#
# Compiles the real migrator (legacy_migrator.cpp) + InitConfig (stm_main.c);
# legacy_types.h is header-only. QtCore only (no widgets).

TEMPLATE = app
TARGET   = test_shiftmigrate
CONFIG  += console c++17
CONFIG  -= app_bundle
QT      += testlib core

INCLUDEPATH += ../src ../src/legacy

SOURCES += \
    test_shiftmigrate.cpp \
    ../src/legacy/legacy_migrator.cpp \
    ../src/stm_main.c

HEADERS += \
    ../src/legacy/legacy_migrator.h \
    ../src/stm_main.h
