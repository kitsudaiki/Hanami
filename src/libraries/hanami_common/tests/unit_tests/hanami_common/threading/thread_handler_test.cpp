/**
 *  @file    thread_handler_test.h
 *
 *  @author  Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 *  @copyright MIT License
 */

#include "thread_handler_test.h"

#include <hanami_common/threading/thread_handler.h>

namespace Hanami
{

ThreadHandler_Test::ThreadHandler_Test() : Hanami::CompareTestHelper("ThreadHandler_Test")
{
    all_test();
}

/**
 * @brief all_test
 */
void
ThreadHandler_Test::all_test()
{
    std::vector<Thread*> threads;
    std::vector<std::string> keys = ThreadHandler::getInstance()->getRegisteredNames();
    TEST_EQUAL(keys.size(), 0);

    DummyThread* testThread = new DummyThread();
    testThread->startThread();
    DummyThread* testThread2 = new DummyThread();
    testThread2->startThread();

    keys = ThreadHandler::getInstance()->getRegisteredNames();
    TEST_EQUAL(keys.size(), 1);
    if (keys.size() < 1) {
        return;
    }

    sleep(1);

    // check state of the handler
    TEST_EQUAL(keys.at(0), "DummyThread");
    threads = ThreadHandler::getInstance()->getThreads("DummyThread");
    TEST_EQUAL(threads.size(), 2);

    // check after delete first thread
    delete testThread;
    threads = ThreadHandler::getInstance()->getThreads("DummyThread");
    TEST_EQUAL(threads.size(), 1);

    // check after delete second thread
    delete testThread2;
    threads = ThreadHandler::getInstance()->getThreads("DummyThread");
    TEST_EQUAL(threads.size(), 0);
    keys = ThreadHandler::getInstance()->getRegisteredNames();
    TEST_EQUAL(keys.size(), 0);
}

//==================================================================================================
// DummyThread for testing
//==================================================================================================
DummyThread::DummyThread() : Hanami::Thread("DummyThread") {}

void
DummyThread::run()
{
    while (m_abort == false) {
        sleepThread(1000000);
    }
}

}  // namespace Hanami
