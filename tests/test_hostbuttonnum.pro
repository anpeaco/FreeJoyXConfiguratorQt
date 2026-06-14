QT += core testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_hostbuttonnum

INCLUDEPATH += \
    ../src \
    ../src/widgets/buttons

SOURCES += \
    test_hostbuttonnum.cpp

HEADERS += \
    ../src/widgets/buttons/hostbuttonnum.h
