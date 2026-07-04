QT += core widgets
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_configload

INCLUDEPATH += ../src

HEADERS += ../src/configtofile.h

SOURCES += \
    test_configload_main.cpp \
    ../src/configtofile.cpp \
    ../src/legacy/legacy_migrator.cpp \
    ../src/stm_main.c
