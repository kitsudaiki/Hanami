/**
 *  @file    main.cpp
 *
 *  @author  Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 *  @copyright MIT License
 */

#include <iostream>

#include <hanami_common/buffer/data_buffer_test.h>
#include <hanami_common/buffer/item_buffer_test.h>
#include <hanami_common/buffer/ring_buffer_test.h>
#include <hanami_common/buffer/stack_buffer_reserve_test.h>
#include <hanami_common/buffer/stack_buffer_test.h>

#include <hanami_common/methods/string_methods_test.h>
#include <hanami_common/methods/vector_methods_test.h>
#include <hanami_common/methods/file_methods_test.h>

#include <hanami_common/state_test.h>
#include <hanami_common/statemachine_test.h>
#include <hanami_common/progress_bar_test.h>
#include <hanami_common/logger_test.h>

#include <hanami_common/threading/thread_handler_test.h>

#include <hanami_common/items/table_item_test.h>

#include <hanami_common/files/text_file_test.h>
#include <hanami_common/files/binary_file_with_directIO_test.h>
#include <hanami_common/files/binary_file_without_directIO_test.h>

int main()
{
    Hanami::DataBuffer_Test();
    Hanami::ItemBuffer_Test();
    Hanami::RingBuffer_Test();
    Hanami::StackBufferReserve_Test();
    Hanami::StackBuffer_Test();

    Hanami::StringMethods_Test();
    Hanami::VectorMethods_Test();
    Hanami::FileMethods_Test();

    Hanami::State_Test();
    Hanami::Statemachine_Test();
    Hanami::ProgressBar_Test();
    Hanami::Logger_Test();

    Hanami::ThreadHandler_Test();

    Hanami::TextFile_Test();
    Hanami::BinaryFile_withDirectIO_Test();
    Hanami::BinaryFile_withoutDirectIO_Test();

    Hanami::TableItem_test();
}
