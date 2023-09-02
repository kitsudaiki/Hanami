QT -= qt core gui

TARGET = hanami_cpu
TEMPLATE = lib
CONFIG += c++17
VERSION = 0.3.0

LIBS += -L../../hanami_common/src -lhanami_common
LIBS += -L../../hanami_common/src/debug -lhanami_common
LIBS += -L../../hanami_common/src/release -lhanami_common
INCLUDEPATH += ../../hanami_common/include

INCLUDEPATH += $$PWD \
               $$PWD/../include

HEADERS += \
    ../include/hanami_cpu/cpu.h \
    ../include/hanami_cpu/memory.h \
    ../include/hanami_cpu/rapl.h

SOURCES += \
    cpu.cpp \
    memory.cpp \
    rapl.cpp

