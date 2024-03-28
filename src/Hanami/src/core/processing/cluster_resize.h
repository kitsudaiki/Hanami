/**
 * @file        cluster_resize.h
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

#ifndef HANAMI_SECTION_UPDATE_H
#define HANAMI_SECTION_UPDATE_H

#include <common.h>
#include <core/cluster/cluster.h>
#include <core/processing/cpu/cpu_host.h>
#include <core/processing/cuda/cuda_host.h>
#include <core/processing/logical_host.h>
#include <hanami_root.h>

#include "objects.h"

/**
 * @brief search for an empty target-connection within a target-brick
 *
 * @param targetBrick target-brick where to search
 * @param synapseBlockBuffer synapse-block-buffer to allocate new block,
 *                           if search-process was successful
 *
 * @return found empty connection, if seccessfule, else nullptr
 */
inline SynapseConnection*
searchTargetInBrick(Brick* targetBrick, ItemBuffer& synapseBlockBuffer)
{
    const uint64_t numberOfConnectionsBlocks = targetBrick->connectionBlocks.size();
    if (numberOfConnectionsBlocks == 0) {
        return nullptr;
    }

    uint64_t pos = rand() % numberOfConnectionsBlocks;
    for (uint64_t i = 0; i < numberOfConnectionsBlocks; i++) {
        ConnectionBlock* connectionBlock = &targetBrick->connectionBlocks[pos];

        for (uint16_t j = 0; j < NUMBER_OF_SYNAPSESECTION; j++) {
            if (connectionBlock->connections[j].origin.blockId == UNINIT_STATE_32) {
                // initialize a synapse-block if necessary
                if (connectionBlock->targetSynapseBlockPos == UNINIT_STATE_64) {
                    SynapseBlock block;
                    connectionBlock->targetSynapseBlockPos = synapseBlockBuffer.addNewItem(block);
                    if (connectionBlock->targetSynapseBlockPos == UNINIT_STATE_64) {
                        return nullptr;
                    }
                }
                return &connectionBlock->connections[j];
            }
        }
        pos = (pos + 1) % numberOfConnectionsBlocks;
    }

    return nullptr;
}

/**
 * @brief resize the number of connection-blocks of a brick
 *
 * @param targetBrick brick to resize
 */
inline void
resizeConnections(Brick* targetBrick)
{
    const uint32_t dimXold = targetBrick->dimX;
    const uint32_t dimYold = targetBrick->dimY;

    // output-bricks must cover the whole output
    if (targetBrick->isOutputBrick) {
        targetBrick->dimX = targetBrick->numberOfNeuronBlocks;
    }

    // update brick-dimensions
    if (targetBrick->dimX < targetBrick->numberOfNeuronBlocks) {
        targetBrick->dimX++;
    }
    targetBrick->dimY++;

    // resize list
    targetBrick->connectionBlocks.resize(targetBrick->dimX * targetBrick->dimY);

    // if there was no scaling in x-dimension, then no re-ordering necessary
    if (targetBrick->dimX == dimXold) {
        return;
    }

    LOG_DEBUG("resized connection-Block: " + std::to_string(dimXold) + ":" + std::to_string(dimYold)
              + " -> " + std::to_string(targetBrick->dimX) + ":"
              + std::to_string(targetBrick->dimY));
    uint32_t newPos = 0;
    uint32_t oldPos = 0;

    // update content of list for the new size
    for (int32_t y = dimYold - 1; y >= 1; y--) {
        for (int32_t x = dimXold - 1; x >= 0; x--) {
            newPos = (y * targetBrick->dimX) + x;
            oldPos = (y * dimXold) + x;

            targetBrick->connectionBlocks[newPos] = targetBrick->connectionBlocks[oldPos];
            targetBrick->connectionBlocks[oldPos] = ConnectionBlock();
        }
    }
}

/**
 * @brief allocate a new synapse-section
 *
 * @param cluster cluster to update
 * @param originLocation position of the soruce-neuron, which require the resize
 * @param offset action-offset of the new section
 * @param synapseBlockBuffer synapse-block-buffer to allocate new blocks, if necessary
 *
 * @return true, if successful, else false
 */
inline bool
createNewSection(Cluster& cluster,
                 const SourceLocationPtr& originLocation,
                 const float offset,
                 ItemBuffer& synapseBlockBuffer)
{
    // get origin object
    NeuronBlock* originBlock = &cluster.neuronBlocks[originLocation.blockId];
    Neuron* originNeuron = &originBlock->neurons[originLocation.neuronId];
    const bool inputConnected = cluster.bricks[originBlock->brickId].isInputBrick;
    const uint8_t newPosInNeuron = originNeuron->getFirstZeroBit();
    if (newPosInNeuron == UNINIT_STATE_8) {
        return false;
    }

    const uint32_t originBrickId = cluster.neuronBlocks[originLocation.blockId].brickId;
    const Brick* originBrick = &cluster.bricks[originBrickId];
    if (originBrick->isOutputBrick) {
        return false;
    }

    // get target objects
    const uint32_t targetBrickId
        = originBrick->possibleTargetNeuronBrickIds[rand() % NUMBER_OF_POSSIBLE_NEXT];
    Brick* targetBrick = &cluster.bricks[targetBrickId];

    // get target-connection
    SynapseConnection* targetConnection = searchTargetInBrick(targetBrick, synapseBlockBuffer);
    if (targetConnection == nullptr) {
        resizeConnections(targetBrick);
        targetConnection = searchTargetInBrick(targetBrick, synapseBlockBuffer);
        targetBrick->wasResized = true;
    }

    // initialize connection
    targetConnection->origin = originLocation;
    targetConnection->offset = offset;
    targetConnection->origin.posInNeuron = newPosInNeuron;
    targetConnection->inputConnected = inputConnected;
    originNeuron->setInUse(newPosInNeuron);

    return true;
}

/**
 * @brief iterate over all neuron and run the resize-process, if necessary. This function is used
 *        in case of a cuda host, where the resize has to be done after the processing
 *
 * @param cluster cluster to resize
 *
 * @return true, if a resize was performed, else false. This is used to avoid unnecessary data-
 *         transfers to the gpu
 */
inline bool
updateCluster(Cluster& cluster)
{
    NeuronBlock* neuronBlock = nullptr;
    Neuron* neuron = nullptr;
    Brick* brick = nullptr;
    bool found = false;

    // iterate over all neurons and add new synapse-section, if required
    const uint32_t numberOfBlocks = cluster.clusterHeader->neuronBlocks.count;
    for (uint32_t neuronBlockId = 0; neuronBlockId < numberOfBlocks; neuronBlockId++) {
        neuronBlock = &cluster.neuronBlocks[neuronBlockId];

        for (uint32_t sourceId = 0; sourceId < neuronBlock->numberOfNeurons; sourceId++) {
            neuron = &neuronBlock->neurons[sourceId];

            if (neuron->isNew > 0) {
                found = true;
                SourceLocationPtr originLocation;
                originLocation.blockId = neuronBlockId;
                originLocation.neuronId = sourceId;

                createNewSection(cluster,
                                 originLocation,
                                 neuron->newOffset,
                                 cluster.attachedHost->synapseBlocks);

                neuron->newOffset = 0.0f;
                neuron->isNew = 0;
            }
        }
    }

    return found;
}

#endif  // HANAMI_SECTION_UPDATE_H
