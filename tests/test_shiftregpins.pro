QT += core widgets testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_shiftregpins

INCLUDEPATH += ../src

HEADERS += ../src/configtofile.h

SOURCES += \
    test_shiftregpins.cpp \
    ../src/configtofile.cpp
