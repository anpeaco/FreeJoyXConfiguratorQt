# Standalone test executable for FlashSession + DeviceTransitionWatcher.
# Build:
#   cd tests
#   qmake test_flashsession.pro
#   mingw32-make
#   ./test_flashsession.exe
#
# The test doesn't link against the main app; it pulls in only the
# flash/* sources it actually exercises. HidDevice is forward-declared
# and never instantiated, so we don't need hidapi.

TEMPLATE = app
TARGET   = test_flashsession
CONFIG  += console c++17
CONFIG  -= app_bundle
QT      += testlib core
QT      -= gui

DEFINES += SRC_DIR=\\\"$$PWD/../src\\\"

INCLUDEPATH += \
    ../src \
    ../src/flash

SOURCES += \
    test_flashsession.cpp \
    ../src/flash/flashsession.cpp \
    ../src/flash/devicetransitionwatcher.cpp

HEADERS += \
    ../src/flash/flashsession.h \
    ../src/flash/devicetransitionwatcher.h \
    ../src/hiddevice.h

# DeviceTransitionWatcher takes &HidDevice::flasherFound (and friends) as
# member function pointers. Even though the test passes nullptr as the
# HidDevice and the connect() calls are guarded by `if (m_hid)`, the
# compiler emits relocations against HidDevice's signal symbols and
# staticMetaObject. Adding hiddevice.h to HEADERS makes qmake run moc on
# it; the resulting moc_hiddevice.cpp provides linkable definitions for
# the auto-generated signal stubs (`void HidDevice::flasherFound(bool)`,
# `staticMetaObject`, etc.) without us having to compile or link against
# the real hiddevice.cpp or hidapi.

