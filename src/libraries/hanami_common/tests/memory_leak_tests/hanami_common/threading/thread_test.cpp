/**
 *  @file      thread_test.cpp
 *
 *  @author    Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 *  @copyright MIT License
 */

#include "thread_test.h"

#include <hanami_common/threading/event.h>
#include <hanami_common/threading/thread.h>

#include "bogus_event.h"
#include "bogus_thread.h"

namespace Hanami
{

Thread_Test::Thread_Test() : Hanami::MemoryLeakTestHelpter("DataBuffer_Test")
{
    // The first created thread initialize a static instance of a central thread-handler to
    // track all threads. This will not be deleted anytime, so one thread has to be created
    // outside of the test-case.
    Hanami::Thread* testThread = new BogusThread();
    delete testThread;

    create_delete_test();
    create_delete_with_events_test();
}

/**
 * @brief create_delete_test
 */
void
Thread_Test::create_delete_test()
{
    REINIT_TEST();

    Hanami::Thread* testThread = new BogusThread();
    testThread->startThread();
    usleep(100000);
    delete testThread;

    CHECK_MEMORY();
}

/**
 * @brief create_delete_with_events_test
 */
void
Thread_Test::create_delete_with_events_test()
{
    REINIT_TEST();

    Hanami::Thread* testThread = new BogusThread();
    testThread->startThread();
    Event* testEvent = new BogusEvent();
    testThread->addEventToQueue(testEvent);
    usleep(100000);
    delete testThread;

    CHECK_MEMORY();
}

}  // namespace Hanami
