/**
 * @file        cpu_thread.h
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

#ifndef HANAMI_HARDWARE_CPUTHREAD_H
#define HANAMI_HARDWARE_CPUTHREAD_H

#include <hanami_common/logger.h>
#if (defined(__i386__)) || (defined(__x86_64__))
#include <hanami_cpu/rapl.h>
#endif

#include <iostream>
#include <string>
#include <vector>

namespace Hanami
{
class Host;

class CpuThread
{
   public:
    CpuThread(const uint32_t threadId);
    ~CpuThread();

    const uint64_t threadId;
    uint64_t minSpeed = 0;
    uint64_t maxSpeed = 0;

    uint64_t currentMinSpeed = 0;
    uint64_t currentMaxSpeed = 0;

    bool initThread(Host* host);
    uint64_t getCurrentThreadSpeed() const;

    double getThermalSpec() const;
    double getTotalPackagePower();

    const std::string toJsonString();
    json toJson();

   private:
    bool m_speedInitialized = true;
    bool m_powerInitialized = true;
#if (defined(__i386__)) || (defined(__x86_64__))
    Rapl m_rapl;
#endif
};

}  // namespace Hanami

#endif  // HANAMI_HARDWARE_CPUTHREAD_H
