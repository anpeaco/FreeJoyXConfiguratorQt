QT += core testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_reordermath

INCLUDEPATH += \
    ../src \
    ../src/widgets/buttons

SOURCES += \
    test_reordermath.cpp

HEADERS += \
    ../src/widgets/buttons/reordermath.h
