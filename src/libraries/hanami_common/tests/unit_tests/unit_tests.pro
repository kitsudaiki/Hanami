include(../../defaults.pri)

QT -= qt core gui

CONFIG   -= app_bundle
CONFIG += c++17 console

LIBS += -L../../src -lhanami_common
INCLUDEPATH += $$PWD

HEADERS += \
    hanami_common/methods/file_methods_test.h \
    hanami_common/files/binary_file_with_directIO_test.h \
    hanami_common/files/binary_file_without_directIO_test.h \
    hanami_common/files/text_file_test.h \
    hanami_common/logger_test.h \
    hanami_common/progress_bar_test.h \
    hanami_common/state_test.h \
    hanami_common/statemachine_test.h \
    hanami_common/methods/string_methods_test.h \
    hanami_common/methods/vector_methods_test.h \
    hanami_common/items/data_items_DataArray_test.h \
    hanami_common/items/data_items_DataValue_test.h \
    hanami_common/items/table_item_test.h \
    hanami_common/items/data_items_DataMap_test.h \
    hanami_common/buffer/data_buffer_test.h \
    hanami_common/buffer/ring_buffer_test.h \
    hanami_common/buffer/stack_buffer_reserve_test.h \
    hanami_common/buffer/stack_buffer_test.h \
    hanami_common/buffer/item_buffer_test.h \
    hanami_common/threading/thread_handler_test.h

SOURCES += \
    hanami_common/methods/file_methods_test.cpp \
    hanami_common/files/binary_file_with_directIO_test.cpp \
    hanami_common/files/binary_file_without_directIO_test.cpp \
    hanami_common/files/text_file_test.cpp \
    hanami_common/logger_test.cpp \
    hanami_common/progress_bar_test.cpp \
    hanami_common/threading/thread_handler_test.cpp \
    main.cpp \
    hanami_common/state_test.cpp \
    hanami_common/statemachine_test.cpp \
    hanami_common/methods/string_methods_test.cpp \
    hanami_common/methods/vector_methods_test.cpp \
    hanami_common/items/data_items_DataArray_test.cpp \
    hanami_common/items/data_items_DataValue_test.cpp \
    hanami_common/items/table_item_test.cpp \
    hanami_common/items/data_items_DataMap_test.cpp \
    hanami_common/buffer/data_buffer_test.cpp \
    hanami_common/buffer/ring_buffer_test.cpp \
    hanami_common/buffer/stack_buffer_reserve_test.cpp \
    hanami_common/buffer/stack_buffer_test.cpp \
    hanami_common/buffer/item_buffer_test.cpp
