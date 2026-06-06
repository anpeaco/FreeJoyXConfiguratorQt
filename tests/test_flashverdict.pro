# Standalone test for the pure flash-decision logic (src/flashverdict.cpp).
# Build:
#   cd tests
#   qmake test_flashverdict.pro
#   mingw32-make
#   ./release/test_flashverdict.exe   (Windows MinGW puts it under release/)
#
# Pure: links only flashverdict.cpp -- no GUI, FirmwareImage, or legacy module.

TEMPLATE = app
TARGET   = test_flashverdict
CONFIG  += console c++17
CONFIG  -= app_bundle
QT      += testlib core
QT      -= gui

INCLUDEPATH += ../src

SOURCES += \
    test_flashverdict.cpp \
    ../src/flashverdict.cpp

HEADERS += \
    ../src/flashverdict.h
