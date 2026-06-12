# Standalone test for the pure tooltip formatter (src/tip_format.h).
# Build:
#   cd tests
#   qmake test_tipformat.pro
#   mingw32-make
#   ./release/test_tipformat.exe   (Windows MinGW puts it under release/)
#
# Header-only unit: links nothing -- no GUI, no widgets.

TEMPLATE = app
TARGET   = test_tipformat
CONFIG  += console c++17
CONFIG  -= app_bundle
QT      += testlib core
QT      -= gui

INCLUDEPATH += ../src

SOURCES += \
    test_tipformat.cpp

HEADERS += \
    ../src/tip_format.h
