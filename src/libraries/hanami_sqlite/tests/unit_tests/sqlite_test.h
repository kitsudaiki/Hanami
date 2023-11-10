/**
 *  @file    sqlite_test.h
 *
 *  @author  Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 *  @copyright MIT License
 */

#ifndef SQLITE_TEST_H
#define SQLITE_TEST_H

#include <hanami_common/test_helper/compare_test_helper.h>

#include <filesystem>

namespace Hanami
{

class Sqlite_Test : public Hanami::CompareTestHelper
{
   public:
    Sqlite_Test();

   private:
    void initTest();
    void initDB_test();
    void execSqlCommand_test();
    void closeDB_test();
    void closeTest();

    std::string m_filePath = "";
    void deleteFile();
};

}  // namespace Hanami

#endif  // SQLITE_TEST_H
