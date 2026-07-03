QT += core gui widgets svg testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_expandercs

INCLUDEPATH += \
    ../src \
    ../src/widgets \
    ../src/widgets/gpio-expander

HEADERS += \
    ../src/widgets/gpio-expander/gpioexpanderconfig.h \
    ../src/widgets/centered_cbox.h

SOURCES += \
    test_expandercs.cpp \
    ../src/widgets/gpio-expander/gpioexpanderconfig.cpp \
    ../src/widgets/centered_cbox.cpp \
    ../src/deviceconfig.cpp \
    ../src/converter.cpp \
    ../src/stm_main.c
