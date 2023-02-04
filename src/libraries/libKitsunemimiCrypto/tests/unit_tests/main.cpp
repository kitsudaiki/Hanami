/**
 *  @file       main.cpp
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

#include <iostream>
#include <vector>

#include <common_test.h>
#include <hashes_test.h>
#include <signing_test.h>
#include <symmetric_encryption_test.h>

int main()
{
    Kitsunemimi::Common_Test();
    Kitsunemimi::Hashes_Test();
    Kitsunemimi::Signing_Test();
    Kitsunemimi::Symmetric_Encryption_Test();

    return 0;
}

