# Pure test of the board_id -> shown-board mapping (board_display::cardBoardId
# in src/boarddisplay.h). That mapping is the single source of truth BOTH
# getParamsPacket branches (recognised + unrecognised firmware) use for the
# Device card, so a wrong mapping -- like the old hard-coded F103 fallback that
# mislabeled an F411 mid firmware-bump -- is caught here.
#
# Build:
#   cd tests
#   qmake test_boarddisplay.pro
#   mingw32-make
#   ./release/test_boarddisplay.exe   (Windows MinGW puts it under release/)
#
# gui/widgets are needed only to COMPILE boarddisplay.h's QColor/QLabel/QPixmap
# includes; cardBoardId itself pulls in nothing GUI, and every freejoy_style
# helper is header-only inline, so there is no .cpp to link.

TEMPLATE = app
TARGET   = test_boarddisplay
CONFIG  += console c++17
CONFIG  -= app_bundle
QT      += testlib core gui widgets

INCLUDEPATH += ../src

SOURCES += test_boarddisplay.cpp

HEADERS += \
    ../src/boarddisplay.h \
    ../src/style_helpers.h
