# Standalone test for the physical-button reference math
# (src/widgets/buttons/physref.h) -- regression for the C1 review finding
# (expander category missing from toRef/toAbs). physref.h is header-only, so
# this links nothing but Qt testlib.
#
# Build:
#   cd tests
#   qmake test_physref.pro
#   mingw32-make
#   ./release/test_physref.exe

TEMPLATE = app
TARGET   = test_physref
CONFIG  += console c++17
CONFIG  -= app_bundle
QT      += testlib core
QT      -= gui

INCLUDEPATH += ../src ../src/widgets/buttons

SOURCES += test_physref.cpp

HEADERS += ../src/widgets/buttons/physref.h
