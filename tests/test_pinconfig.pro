QT += core gui widgets svg testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_pinconfig

INCLUDEPATH += \
    ../src \
    ../src/widgets \
    ../src/widgets/pins \
    ../src/widgets/color \
    ../src/widgets/buttons \
    ../src/dialogs

HEADERS += \
    ../src/widgets/pins/pinconfig.h \
    ../src/widgets/pins/pincombobox.h \
    ../src/widgets/pins/pinsbluepill.h \
    ../src/widgets/pins/pinsblackpill.h \
    ../src/widgets/pins/pinscontrlite.h \
    ../src/widgets/pins/currentconfig.h \
    ../src/widgets/pins/pintypehelper.h \
    ../src/widgets/centered_cbox.h \
    ../src/dialogs/busremapconfirmationdialog.h

SOURCES += \
    test_pinconfig.cpp \
    test_pinconfig_stubs.cpp \
    ../src/widgets/pins/pinconfig.cpp \
    ../src/widgets/pins/pincombobox.cpp \
    ../src/widgets/pins/pinsbluepill.cpp \
    ../src/widgets/pins/pinsblackpill.cpp \
    ../src/widgets/pins/pinscontrlite.cpp \
    ../src/widgets/pins/currentconfig.cpp \
    ../src/widgets/pins/pintypehelper.cpp \
    ../src/widgets/centered_cbox.cpp \
    ../src/dialogs/busremapconfirmationdialog.cpp \
    ../src/deviceconfig.cpp \
    ../src/converter.cpp \
    ../src/stm_main.c

FORMS += \
    ../src/widgets/pins/pinconfig.ui \
    ../src/widgets/pins/pincombobox.ui \
    ../src/widgets/pins/pinsbluepill.ui \
    ../src/widgets/pins/pinsblackpill.ui \
    ../src/widgets/pins/pinscontrlite.ui \
    ../src/widgets/pins/currentconfig.ui \
    ../src/widgets/pins/pintypehelper.ui \
    ../src/dialogs/busremapconfirmationdialog.ui
