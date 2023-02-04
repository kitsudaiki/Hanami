/**
 *  @file       memory.cpp
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

#include <libKitsunemimiCpu/memory.h>

namespace Kitsunemimi
{

/**
 * @brief get total amount of main-memory of the system in bytes
 */
uint64_t
getTotalMemory()
{
    const uint64_t pages = sysconf(_SC_PHYS_PAGES);
    const uint64_t page_size = getPageSize();
    return pages * page_size;
}

/**
 * @brief get amound of free main-memory of the system in bytes
 */
uint64_t
getFreeMemory()
{
    const uint64_t pages = sysconf(_SC_AVPHYS_PAGES);
    const uint64_t page_size = getPageSize();
    return pages * page_size;
}

/**
 * @brief get page-size of the main-memory of the system in bytes
 */
uint64_t
getPageSize()
{
    return sysconf(_SC_PAGE_SIZE);
}

} // namespace Kitsunemimi
