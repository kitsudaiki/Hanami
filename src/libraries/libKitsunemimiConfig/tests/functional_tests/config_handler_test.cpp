/**
 *  @file       config_handler_test.cpp
 *
 *  @author     Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 *  @copyright  Apache License Version 2.0
 *
 *      Copyright 2022 Tobias Anker
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "config_handler_test.h"

#include <libKitsunemimiConfig/config_handler.h>
#include <libKitsunemimiCommon/files/text_file.h>
#include <libKitsunemimiCommon/methods/file_methods.h>

namespace Kitsunemimi
{

ConfigHandler_Test::ConfigHandler_Test()
    : Kitsunemimi::CompareTestHelper("ConfigHandler_Test")
{
    runTest();
}

/**
 * @brief runTest
 */
void
ConfigHandler_Test::runTest()
{
    ErrorContainer error;
    Kitsunemimi::writeFile(m_testFilePath, getTestString(), error, true);

    REGISTER_STRING_CONFIG("DEFAULT", "string_val", "", error, "");
    REGISTER_INT_CONFIG("DEFAULT", "int_val", "", error, 42);
    REGISTER_INT_CONFIG("DEFAULT", "another_int_val", "", error, 42);

    // init config
    TEST_EQUAL(Kitsunemimi::initConfig(m_testFilePath, error), true);

    bool success = false;
    TEST_EQUAL(GET_STRING_CONFIG("DEFAULT", "string_val", success), "asdf.asdf");
    TEST_EQUAL(success, true);
    TEST_EQUAL(GET_INT_CONFIG("DEFAULT", "int_val", success), 2);
    TEST_EQUAL(success, true);
    TEST_EQUAL(GET_INT_CONFIG("DEFAULT", "another_int_val", success), 42);
    TEST_EQUAL(success, true);
    TEST_EQUAL(GET_STRING_CONFIG("DEFAULT", "fail", success), "");
    TEST_EQUAL(success, false);

    Kitsunemimi::deleteFileOrDir(m_testFilePath, error);

}

/**
 * @brief ConfigHandler_Test::getTestString
 * @return
 */
const std::string
ConfigHandler_Test::getTestString()
{
    const std::string testString(
                "[DEFAULT]\n"
                "string_val = asdf.asdf\n"
                "int_val = 2\n"
                "float_val = 123.0\n"
                "string_list = a,b,c\n"
                "bool_value = true\n"
                "\n");
    return testString;
}

}
