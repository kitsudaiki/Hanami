/**
 * @file        checkpoint_delete_test.h
 *
 * @author      Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 * @copyright   Apache License Version 2.0
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

#ifndef TSUGUMITESTER_CHECKPOINTDELETETEST_H
#define TSUGUMITESTER_CHECKPOINTDELETETEST_H

#include <common/test_step.h>

class CheckpointDeleteTest
        : public TestStep
{
public:
    CheckpointDeleteTest(const bool expectSuccess);

    bool runTest(Hanami::JsonItem &inputData,
                 Hanami::ErrorContainer &error);
};

#endif // TSUGUMITESTER_CHECKPOINTDELETETEST_H
