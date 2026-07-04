QT += core widgets testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_configpersist

INCLUDEPATH += ../src

HEADERS += ../src/configtofile.h

SOURCES += \
    test_configpersist.cpp \
    ../src/configtofile.cpp
