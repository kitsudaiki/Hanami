/**
 *  @file       hashes_test.h
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

#ifndef HASHES_TEST_H
#define HASHES_TEST_H

#include <hanami_common/test_helper/compare_test_helper.h>

namespace Hanami
{

class Hashes_Test
        : public Hanami::CompareTestHelper
{
public:
    Hashes_Test();

private:
    void generate_SHA_256_test();
};

}

#endif // HASHES_TEST_H