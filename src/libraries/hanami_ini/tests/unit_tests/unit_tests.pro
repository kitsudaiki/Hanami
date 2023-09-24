include(../../defaults.pri)

QT -= qt core gui

CONFIG   -= app_bundle
CONFIG += c++17 console

LIBS += -L../../src -lhanami_ini

LIBS += -L../../../hanami_common/src -lhanami_common
LIBS += -L../../../hanami_common/src/debug -lhanami_common
LIBS += -L../../../hanami_common/src/release -lhanami_common
INCLUDEPATH += ../../../hanami_common/include

INCLUDEPATH += ../../../../third-party-libs/json/include

INCLUDEPATH += $$PWD


SOURCES += \
        main.cpp \
    hanami_ini/ini_item_test.cpp

HEADERS += \
    hanami_ini/ini_item_test.h

