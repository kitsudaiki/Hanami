/**
 * @file        processing_unit_handler.h
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

#ifndef HANAMI_PROCESSING_UNIT_HANDLER_H
#define HANAMI_PROCESSING_UNIT_HANDLER_H

#include <stdint.h>

#include <vector>

class CpuProcessingUnit;

class ProcessingUnitHandler
{
   public:
    static ProcessingUnitHandler* getInstance()
    {
        if (instance == nullptr) {
            instance = new ProcessingUnitHandler();
        }
        return instance;
    }
    ~ProcessingUnitHandler();

    void addProcessingUnit(const uint64_t threadId);
    bool initProcessingUnits(const uint16_t numberOfThreads);

   private:
    ProcessingUnitHandler();
    static ProcessingUnitHandler* instance;

    std::vector<CpuProcessingUnit*> m_processingUnits;
};

#endif  // HANAMI_PROCESSING_UNIT_HANDLER_H
