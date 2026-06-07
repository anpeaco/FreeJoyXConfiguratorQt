TEMPLATE = app

QT       += core gui svg network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += static \
          c++17

RC_ICONS = Images/icon.ico

TARGET = FreeJoyXConfiguratorQt

# --- Configurator app version (window title / App card / .exe properties) ---
# CI (release.yml) exports FREEJOYX_APP_VERSION = the release tag minus "v"
# (e.g. "0.1.8") before qmake, so the app states the exact version downloaded.
# Absent on local/dev builds, version.h falls back to FREEJOYX_VERSION "-dev".
# Intentionally decoupled from the firmware/wire FREEJOYX_VERSION (synced across
# repos, reported by the device) -- see memory project_version_display_sync.
APP_VER = $$(FREEJOYX_APP_VERSION)
!isEmpty(APP_VER) {
    APP_VER_PARTS = $$split(APP_VER, .)
    # Numeric components are safe for BOTH the C++ compiler and windres.
    APP_VER_NUM_DEFINES = \
        FREEJOYX_APP_VER_MAJOR=$$member(APP_VER_PARTS, 0) \
        FREEJOYX_APP_VER_MINOR=$$member(APP_VER_PARTS, 1) \
        FREEJOYX_APP_VER_PATCH=$$member(APP_VER_PARTS, 2)
    DEFINES += $$APP_VER_NUM_DEFINES
    RC_DEFINES += $$APP_VER_NUM_DEFINES
    # The quoted-string define goes ONLY to the C++ compiler -- windres can't
    # parse a quoted-string -D (it breaks the resource preprocessor's command
    # line); winapp.rc builds its version string from the numerics instead
    # (FREEJOYX_APP_VERSION_NUM in version.h).
    DEFINES += FREEJOYX_APP_VERSION=\\\"$$APP_VER\\\"
    message("FreeJoyX configurator app version: $$APP_VER")
}

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS \
WIN_DESKTOP

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH +="widgets" \
    "dialogs" \
    "flash" \
    "widgets/adv-settings" \
    "widgets/axes" \
    "widgets/axes-curves" \
    "widgets/buttons" \
    "widgets/encoders" \
    "widgets/led" \
    "widgets/pins" \
    "widgets/shift-reg" \
    "widgets/shifts-timers" \
    "widgets/color" \
    "widgets/led_rgb"

SOURCES += \
    mainwindow_style.cpp \
    windowthemehelper.cpp \
    widgets/altspinbox.cpp \
    widgets/axes-curves/axescurvesbutton.cpp \
    widgets/axes-curves/axescurvesprofiles.cpp \
    widgets/centered_cbox.cpp \
    configtofile.cpp \
    converter.cpp \
    deviceconfig.cpp \
    deviceversion.cpp \
    dialogs/busremapconfirmationdialog.cpp \
    dialogs/dfuinstalldialog.cpp \
    dialogs/flashconfirmationdialog.cpp \
    dialogs/flashprogressdialog.cpp \
    flash/devicetransitionwatcher.cpp \
    flash/dfuinstallsession.cpp \
    flash/flashsession.cpp \
    firmwareimage.cpp \
    flashverdict.cpp \
    firmwareupdater.cpp \
    hiddevice.cpp \
    main.cpp \
    mainwindow.cpp \
    mousewheelguard.cpp \
    reportconverter.cpp \
    stm_main.c \
    widgets/color/colorcells.cpp \
    widgets/color/colorpicker.cpp \
    widgets/color/colorvalueslider.cpp \
    widgets/color/colorwheel.cpp \
    widgets/groupedcombo.cpp \
    widgets/infolabel.cpp \
    widgets/led_rgb/ledfunction.cpp \
    widgets/led_rgb/ledrgb.cpp \
    widgets/led_rgb/ledrgbconfig.cpp \
    widgets/pins/pintypehelper.cpp \
    widgets/selectfolder.cpp \
    widgets/switchbutton.cpp \
    widgets/adv-settings/advancedsettings.cpp \
    widgets/axes/axes.cpp \
    widgets/axes/axesconfig.cpp \
    widgets/axes-curves/axescurves.cpp \
    widgets/axes-curves/axescurvesconfig.cpp \
    widgets/axes-curves/axescurvesplot.cpp \
    widgets/axes/axesextended.cpp \
    widgets/axes/axestobuttonsslider.cpp \
    widgets/buttons/buttonconfig.cpp \
    widgets/buttons/buttonlogical.cpp \
    widgets/buttons/buttonphysical.cpp \
    widgets/pins/currentconfig.cpp \
    widgets/debugwindow.cpp \
    widgets/encoders/encoders.cpp \
    widgets/encoders/encodersconfig.cpp \
    widgets/encoders/fastencoder.cpp \
    widgets/adv-settings/flasher.cpp \
    widgets/led/led.cpp \
    widgets/led/ledconfig.cpp \
    widgets/pins/pincombobox.cpp \
    widgets/pins/pinconfig.cpp \
    widgets/pins/pinsbluepill.cpp \
    widgets/pins/pinsblackpill.cpp \
    widgets/pins/pinscontrlite.cpp \
    widgets/shift-reg/shiftregisters.cpp \
    widgets/shift-reg/shiftregistersconfig.cpp \
    widgets/shifts-timers/shiftstimersconfig.cpp \
    legacy/legacy_migrator.cpp \
    legacy/legacy_reverse_migrator.cpp \
    legacy/upstream_layout_check.cpp \
    firmwarelibrary.cpp

HEADERS += \
    style_helpers.h \
    windowthemehelper.h \
    widgets/altspinbox.h \
    widgets/axes-curves/axescurvesbutton.h \
    widgets/axes-curves/axescurvesprofiles.h \
    widgets/centered_cbox.h \
    common_defines.h \
    common_types.h \
    configtofile.h \
    converter.h \
    deviceconfig.h \
    deviceversion.h \
    devicesync.h \
    dialogs/busremapconfirmationdialog.h \
    dialogs/dfuinstalldialog.h \
    dialogs/flashconfirmationdialog.h \
    dialogs/flashprogressdialog.h \
    flash/devicetransitionwatcher.h \
    flash/dfuinstallsession.h \
    flash/flashsession.h \
    firmwareimage.h \
    flashverdict.h \
    firmwareupdater.h \
    global.h \
    hidapi.h \
    hiddevice.h \
    mainwindow.h \
    mousewheelguard.h \
    reportconverter.h \
    stm_main.h \
    widgets/color/colorcells.h \
    widgets/color/colorpicker.h \
    widgets/color/colorvalueslider.h \
    widgets/color/colorwheel.h \
    widgets/groupedcombo.h \
    widgets/infolabel.h \
    widgets/led_rgb/ledfunction.h \
    widgets/led_rgb/ledrgb.h \
    widgets/led_rgb/ledrgbconfig.h \
    widgets/pins/pintypehelper.h \
    widgets/selectfolder.h \
    widgets/switchbutton.h \
    version.h \
    widgets/adv-settings/advancedsettings.h \
    widgets/axes/axes.h \
    widgets/axes/axesconfig.h \
    widgets/axes-curves/axescurves.h \
    widgets/axes-curves/axescurvesconfig.h \
    widgets/axes-curves/axescurvesplot.h \
    widgets/axes/axesextended.h \
    widgets/axes/axestobuttonsslider.h \
    widgets/buttons/buttonconfig.h \
    widgets/buttons/buttonlogical.h \
    widgets/buttons/buttonphysical.h \
    widgets/buttons/physref.h \
    widgets/pins/currentconfig.h \
    widgets/debugwindow.h \
    widgets/encoders/encoders.h \
    widgets/encoders/encodersconfig.h \
    widgets/encoders/fastencoder.h \
    widgets/adv-settings/flasher.h \
    widgets/led/led.h \
    widgets/led/ledconfig.h \
    widgets/pins/pincombobox.h \
    widgets/pins/pinconfig.h \
    widgets/pins/pinsbluepill.h \
    widgets/pins/pinsblackpill.h \
    widgets/pins/pinscontrlite.h \
    widgets/shift-reg/shiftregisters.h \
    widgets/shift-reg/shiftregistersconfig.h \
    widgets/shifts-timers/shiftstimersconfig.h \
    legacy/legacy_types.h \
    legacy/legacy_migrator.h \
    legacy/legacy_reverse_migrator.h \
    firmwarelibrary.h

FORMS += \
    dialogs/busremapconfirmationdialog.ui \
    dialogs/flashconfirmationdialog.ui \
    dialogs/flashprogressdialog.ui \
    mainwindow.ui \
    widgets/adv-settings/advancedsettings.ui \
    widgets/axes-curves/axescurvesprofiles.ui \
    widgets/axes/axes.ui \
    widgets/axes/axesconfig.ui \
    widgets/axes-curves/axescurves.ui \
    widgets/axes-curves/axescurvesconfig.ui \
    widgets/axes/axesextended.ui \
    widgets/axes/axestobuttonsslider.ui \
    widgets/buttons/buttonconfig.ui \
    widgets/buttons/buttonlogical.ui \
    widgets/buttons/buttonphysical.ui \
    widgets/color/colorpicker.ui \
    widgets/led_rgb/ledfunction.ui \
    widgets/led_rgb/ledrgbconfig.ui \
    widgets/pins/currentconfig.ui \
    widgets/debugwindow.ui \
    widgets/encoders/encoders.ui \
    widgets/encoders/encodersconfig.ui \
    widgets/encoders/fastencoder.ui \
    widgets/adv-settings/flasher.ui \
    widgets/led/led.ui \
    widgets/led/ledconfig.ui \
    widgets/pins/pincombobox.ui \
    widgets/pins/pinconfig.ui \
    widgets/pins/pinsbluepill.ui \
    widgets/pins/pinsblackpill.ui \
    widgets/pins/pinscontrlite.ui \
    widgets/pins/pintypehelper.ui \
    widgets/selectfolder.ui \
    widgets/shift-reg/shiftregisters.ui \
    widgets/shift-reg/shiftregistersconfig.ui \
    widgets/shifts-timers/shiftstimersconfig.ui

TRANSLATIONS += \
    FreeJoyQt_ru.ts \
    FreeJoyQt_zh_CN.ts \
    FreeJoyQt_de_DE.ts


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

linux {
    QMAKE_LFLAGS += -no-pie
    LIBS += -ludev
    SOURCES += \
        linux/hidapi.c
}

win32 {
    RC_FILE = winapp.rc
    LIBS += -lhid -lsetupapi -ldwmapi
    SOURCES += \
        windows/hidapi.c
}

macx {
    LIBS += -framework CoreFoundation -framework IOkit
    SOURCES += \
    mac/hidapi.c
}
