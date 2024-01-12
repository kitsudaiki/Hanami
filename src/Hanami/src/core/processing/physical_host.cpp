/**
 * @file        physical_host.cpp
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

#include "physical_host.h"

#include <core/cuda_functions.h>
#include <core/processing/cpu/cpu_host.h>
#include <core/processing/cuda/cuda_host.h>
#include <hanami_hardware/host.h>

PhysicalHost::PhysicalHost() {}

/**
 * @brief PhysicalHost::init
 * @param error
 * @return
 */
bool
PhysicalHost::init(Hanami::ErrorContainer& error)
{
    if (Hanami::Host::getInstance()->initHost(error) == false) {
        return false;
    }

    const uint32_t numberOfSockets = Hanami::Host::getInstance()->cpuPackages.size();
    for (uint32_t i = 0; i < numberOfSockets; i++) {
        CpuHost* newHost = new CpuHost();
        newHost->startThread();
        m_cpuHosts.push_back(newHost);
    }

    const uint32_t numberOfCudaGpus = getNumberOfDevices_CUDA();
    for (uint32_t i = 0; i < numberOfCudaGpus; i++) {
        CudaHost* newHost = new CudaHost();
        newHost->startThread();
        m_cudaHosts.push_back(newHost);
    }

    LOG_INFO("Initialized " + std::to_string(m_cpuHosts.size()) + " CPU-sockets");
    LOG_INFO("Initialized " + std::to_string(m_cudaHosts.size()) + " CUDA-GPUs");

    return true;
}

/**
 * @brief PhysicalHost::getFirstHost
 * @return
 */
LogicalHost*
PhysicalHost::getFirstHost() const
{
    if (m_cpuHosts.size() == 0) {
        return nullptr;
    }

    return m_cpuHosts.at(0);
}
