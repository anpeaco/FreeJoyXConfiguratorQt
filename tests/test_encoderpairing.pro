QT += core gui widgets svg testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_encoderpairing

INCLUDEPATH += \
    ../src \
    ../src/widgets \
    ../src/widgets/encoders

HEADERS += \
    ../src/widgets/encoders/encodersconfig.h \
    ../src/widgets/encoders/encoders.h \
    ../src/widgets/encoders/fastencoder.h \
    ../src/widgets/centered_cbox.h

SOURCES += \
    test_encoderpairing.cpp \
    ../src/widgets/encoders/encodersconfig.cpp \
    ../src/widgets/encoders/encoders.cpp \
    ../src/widgets/encoders/fastencoder.cpp \
    ../src/widgets/centered_cbox.cpp \
    ../src/deviceconfig.cpp \
    ../src/converter.cpp \
    ../src/stm_main.c

FORMS += \
    ../src/widgets/encoders/encodersconfig.ui \
    ../src/widgets/encoders/encoders.ui \
    ../src/widgets/encoders/fastencoder.ui
