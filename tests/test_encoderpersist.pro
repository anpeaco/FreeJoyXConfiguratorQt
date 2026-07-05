QT += core widgets testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_encoderpersist

INCLUDEPATH += ../src

HEADERS += ../src/configtofile.h

SOURCES += \
    test_encoderpersist.cpp \
    ../src/configtofile.cpp \
    ../src/legacy/legacy_migrator.cpp \
    ../src/stm_main.c
