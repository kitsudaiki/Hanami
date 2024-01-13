/**
 * @file        cuda_host.cpp
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

#include "cuda_host.h"

#include <core/cuda_functions.h>
#include <core/processing/cluster_io_functions.h>
#include <core/processing/cluster_resize.h>

CudaHost::CudaHost(const uint32_t localId) : LogicalHost(localId)
{
    m_hostType = CUDA_HOST_TYPE;
    initBuffer(localId);
}

CudaHost::~CudaHost() {}

uint64_t
CudaHost::getAvailableMemory()
{
    return 0;
}

void
CudaHost::hostSpecificCleanup(Cluster* cluster)
{
    removeFromDevice_CUDA(&cluster->gpuPointer);
}

/**
 * @brief CudaHost::moveCluster
 * @param originHost
 * @param cluster
 * @return
 */
bool
CudaHost::moveCluster(Cluster* cluster)
{
    LogicalHost* originHost = cluster->attachedHost;
    SynapseBlock* cpuSynapseBlocks = Hanami::getItemData<SynapseBlock>(synapseBlocks);
    SynapseBlock tempBlock;

    for (uint64_t i = 0; i < cluster->clusterHeader->bricks.count; i++) {
        for (ConnectionBlock& block : cluster->bricks[i].connectionBlocks) {
            if (block.targetSynapseBlockPos != UNINIT_STATE_64) {
                tempBlock = cpuSynapseBlocks[block.targetSynapseBlockPos];
                originHost->synapseBlocks.deleteItem(block.targetSynapseBlockPos);
                const uint64_t newPos = synapseBlocks.addNewItem(tempBlock);
                // TODO: make roll-back possible in error-case
                if (newPos == UNINIT_STATE_64) {
                    return false;
                }
                block.targetSynapseBlockPos = newPos;
            }
        }
    }

    copyToDevice_CUDA(&cluster->gpuPointer,
                      &cluster->clusterHeader->settings,
                      cluster->neuronBlocks,
                      cluster->tempNeuronBlocks,
                      cluster->clusterHeader->neuronBlocks.count,
                      getItemData<SynapseBlock>(synapseBlocks),
                      synapseBlocks.metaData->itemCapacity,
                      cluster->bricks,
                      cluster->clusterHeader->bricks.count);

    cluster->attachedHost = this;

    return true;
}

/**
 * @brief run forward-propagation on a cluster
 *
 * @param cluster cluster to process
 */
void
CudaHost::trainClusterForward(Cluster* cluster)
{
    Hanami::ErrorContainer error;

    // process input-bricks
    for (uint32_t brickId = 0; brickId < cluster->clusterHeader->bricks.count; ++brickId) {
        Brick* brick = &cluster->bricks[brickId];
        if (brick->isInputBrick == false) {
            continue;
        }

        processNeuronsOfInputBrickBackward<true>(
            brick, cluster->inputValues, cluster->neuronBlocks);
    }

    processing_CUDA(&cluster->gpuPointer,
                    cluster->bricks,
                    cluster->clusterHeader->bricks.count,
                    cluster->neuronBlocks,
                    cluster->numberOfNeuronBlocks,
                    true);

    // process output-bricks
    for (uint32_t brickId = 0; brickId < cluster->clusterHeader->bricks.count; ++brickId) {
        Brick* brick = &cluster->bricks[brickId];
        if (brick->isOutputBrick == false) {
            continue;
        }

        processNeuronsOfOutputBrick(brick, cluster->outputValues, cluster->neuronBlocks);
    }

    if (updateCluster(*cluster)) {
        update_CUDA(&cluster->gpuPointer,
                    cluster->neuronBlocks,
                    cluster->numberOfNeuronBlocks,
                    cluster->bricks,
                    cluster->clusterHeader->bricks.count);
    }
}

/**
 * @brief run back-propagation on a cluster
 *
 * @param cluster cluster to process
 */
void
CudaHost::trainClusterBackward(Cluster* cluster)
{
    Hanami::ErrorContainer error;

    // process output-bricks on cpu
    for (uint32_t brickId = 0; brickId < cluster->clusterHeader->bricks.count; ++brickId) {
        Brick* brick = &cluster->bricks[brickId];
        if (brick->isOutputBrick) {
            if (backpropagateOutput(brick,
                                    cluster->neuronBlocks,
                                    cluster->tempNeuronBlocks,
                                    cluster->outputValues,
                                    cluster->expectedValues,
                                    &cluster->clusterHeader->settings)
                == false)
            {
                return;
            }
        }
    }

    backpropagation_CUDA(&cluster->gpuPointer,
                         cluster->bricks,
                         cluster->clusterHeader->bricks.count,
                         cluster->neuronBlocks,
                         cluster->tempNeuronBlocks,
                         cluster->numberOfNeuronBlocks);

    if (reductionCounter == 100) {
        reduction_CUDA(&cluster->gpuPointer,
                       cluster->bricks,
                       cluster->clusterHeader->bricks.count,
                       cluster->neuronBlocks,
                       cluster->numberOfNeuronBlocks);
        if (updateCluster(*cluster)) {
            update_CUDA(&cluster->gpuPointer,
                        cluster->neuronBlocks,
                        cluster->numberOfNeuronBlocks,
                        cluster->bricks,
                        cluster->clusterHeader->bricks.count);
        }
        reductionCounter = 0;
    }
    reductionCounter++;
}

/**
 * @brief process segments
 *
 * @param cluster cluster to process
 */
void
CudaHost::requestCluster(Cluster* cluster)
{
    Hanami::ErrorContainer error;

    // process input-bricks
    for (uint32_t brickId = 0; brickId < cluster->clusterHeader->bricks.count; ++brickId) {
        Brick* brick = &cluster->bricks[brickId];
        if (brick->isInputBrick == false) {
            continue;
        }

        processNeuronsOfInputBrickBackward<false>(
            brick, cluster->inputValues, cluster->neuronBlocks);
    }

    processing_CUDA(&cluster->gpuPointer,
                    cluster->bricks,
                    cluster->clusterHeader->bricks.count,
                    cluster->neuronBlocks,
                    cluster->numberOfNeuronBlocks,
                    false);

    // process output-bricks
    for (uint32_t brickId = 0; brickId < cluster->clusterHeader->bricks.count; ++brickId) {
        Brick* brick = &cluster->bricks[brickId];
        if (brick->isOutputBrick == false) {
            continue;
        }

        processNeuronsOfOutputBrick(brick, cluster->outputValues, cluster->neuronBlocks);
    }
}

void
CudaHost::initBuffer(const uint32_t id)
{
    uint64_t sizeOfMemory = getAvailableMemory_CUDA(id);
    sizeOfMemory = (sizeOfMemory / 100) * 80;  // use 80% for synapse-blocks
    synapseBlocks.initBuffer<SynapseBlock>(sizeOfMemory / sizeof(SynapseBlock));
    synapseBlocks.deleteAll();

    LOG_INFO("Initialized number of syanpse-blocks on gpu-device with id '" + std::to_string(id)
             + "': " + std::to_string(synapseBlocks.metaData->itemCapacity));
}
