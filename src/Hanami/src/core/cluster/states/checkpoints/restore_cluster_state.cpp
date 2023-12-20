/**
 * @file        restore_cluster_state.cpp
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

#include "restore_cluster_state.h"

#include <core/cluster/cluster.h>
#include <core/cluster/cluster_init.h>
#include <core/cluster/statemachine_init.h>
#include <core/cluster/task.h>
#include <hanami_common/files/binary_file.h>
#include <hanami_crypto/hashes.h>
#include <hanami_root.h>

/**
 * @brief constructor
 *
 * @param cluster pointer to the cluster, where the event and the statemachine belongs to
 */
RestoreCluster_State::RestoreCluster_State(Cluster* cluster) { m_cluster = cluster; }

/**
 * @brief destructor
 */
RestoreCluster_State::~RestoreCluster_State() {}

/**
 * @brief prcess event
 *
 * @return alway true
 */
bool
RestoreCluster_State::processEvent()
{
    Task* actualTask = m_cluster->getActualTask();
    Hanami::ErrorContainer error;
    const std::string originalUuid = m_cluster->getUuid();

    bool success = false;

    do {
        // get meta-infos of dataset from shiori
        json parsedCheckpointInfo;
        try {
            parsedCheckpointInfo = json::parse(actualTask->checkpointInfo);
        }
        catch (const json::parse_error& ex) {
            error.addMeesage("json-parser error: " + std::string(ex.what()));
            break;
        }

        // get other information
        const std::string location = parsedCheckpointInfo["location"];

        // get checkpoint-data
        Hanami::BinaryFile checkpointFile(location);
        Hanami::DataBuffer checkpointBuffer;
        if (checkpointFile.readCompleteFile(checkpointBuffer, error) == false) {
            error.addMeesage("failed to load checkpoint-data");
            break;
        }

        uint8_t* u8Data = static_cast<uint8_t*>(checkpointBuffer.data);
        uint64_t position = 0;

        // convert checkpoint-header
        CheckpointHeader header;
        memcpy(&header, &u8Data[position], sizeof(CheckpointHeader));
        position += sizeof(CheckpointHeader);

        // convert cluster-body
        if (Hanami::reset_DataBuffer(m_cluster->clusterData,
                                     Hanami::calcBytesToBlocks(header.metaSize))
            == false)
        {
            error.addMeesage("failed to allocate cluster-data for write-back of checkpoint");
            break;
        }
        memcpy(m_cluster->clusterData.data, &u8Data[position], header.metaSize);
        if (reinitPointer(m_cluster, header.metaSize) == false) {
            error.addMeesage("failed to re-init cluster from checkpoint");
            break;
        }
        position += header.metaSize;

        // convert payload of bricks
        for (uint32_t i = 0; i < m_cluster->clusterHeader->bricks.count; i++) {
            Brick* brick = &m_cluster->bricks[i];
            brick->connectionBlocks.resize(brick->dimX * brick->dimY);

            const uint64_t numberOfConnections = brick->connectionBlocks.size();
            for (uint64_t c = 0; c < numberOfConnections; c++) {
                // convert connection-block
                memcpy(&brick->connectionBlocks[c], &u8Data[position], sizeof(ConnectionBlock));
                position += sizeof(ConnectionBlock);

                // convert synapse-block
                SynapseBlock newSynapseBlock;
                memcpy(&newSynapseBlock, &u8Data[position], sizeof(SynapseBlock));
                const uint64_t itemPos = HanamiRoot::m_synapseBlocks.addNewItem(newSynapseBlock);
                if (itemPos == UNINIT_STATE_64) {
                    error.addMeesage("failed allocate synapse-block for checkpoint");
                    break;
                }

                // write new position into the related connection-block
                brick->connectionBlocks[c].targetSynapseBlockPos = itemPos;

                position += sizeof(SynapseBlock);
            }
        }

        // update uuid
        strncpy(m_cluster->clusterHeader->uuid.uuid, originalUuid.c_str(), originalUuid.size());
        success = true;
        break;
    }
    while (true);

    m_cluster->goToNextState(FINISH_TASK);

    return success;
}
