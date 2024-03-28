/**
 * @file        processing.h
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

#ifndef HANAMI_CORE_PROCESSING_H
#define HANAMI_CORE_PROCESSING_H

#include <api/websocket/cluster_io.h>
#include <common.h>
#include <core/cluster/cluster.h>
#include <core/processing/cluster_io_functions.h>
#include <core/processing/cluster_resize.h>
#include <core/processing/objects.h>
#include <hanami_root.h>
#include <math.h>

#include <cmath>

/**
 * @brief process all neurons of an input-brick
 *
 * @param cluster cluster, where the brick belongs to
 * @param brick input-brick to process
 */
template <bool doTrain>
inline void
processNeuronsOfInputBrick(Cluster& cluster, const Brick* brick)
{
    NeuronBlock* neuronBlocks = cluster.neuronBlocks;
    float* inputValues = cluster.inputValues;
    Neuron* neuron = nullptr;
    NeuronBlock* block = nullptr;
    uint32_t counter = 0;
    float* brickBuffer = &inputValues[brick->ioBufferPos];

    // iterate over all neurons within the brick
    for (uint32_t blockId = brick->neuronBlockPos;
         blockId < brick->numberOfNeuronBlocks + brick->neuronBlockPos;
         ++blockId)
    {
        block = &neuronBlocks[blockId];

        for (uint32_t neuronIdInBlock = 0; neuronIdInBlock < block->numberOfNeurons;
             neuronIdInBlock++)
        {
            neuron = &block->neurons[neuronIdInBlock];
            neuron->potential = brickBuffer[counter];
            neuron->active = neuron->potential > 0.0f;

            if constexpr (doTrain) {
                if (neuron->active != 0 && neuron->inUse == 0) {
                    SourceLocationPtr originLocation;
                    originLocation.blockId = blockId;
                    originLocation.neuronId = neuronIdInBlock;
                    createNewSection(
                        cluster, originLocation, 0.0f, cluster.attachedHost->synapseBlocks);
                }
            }
            counter++;
        }
    }
}

/**
 * @brief initialize a new synpase
 *
 * @param block source-neuron-block, which is only used to hold the randamo-value
 * @param synapse pointer to the synapse, which should be (re-) initialized
 * @param clusterSettings pointer to the cluster-settings
 * @param remainingW new weight for the synapse
 * @param randomSeed reference to the current seed of the randomizer
 */
inline void
createNewSynapse(NeuronBlock* block,
                 Synapse* synapse,
                 const ClusterSettings* clusterSettings,
                 const float remainingW,
                 uint32_t& randomSeed)
{
    const float randMax = static_cast<float>(RAND_MAX);
    uint32_t signRand = 0;
    const float sigNeg = 0.5f;

    // set activation-border
    synapse->border = remainingW;

    // set initial active-counter for reduction-process
    synapse->activeCounter = 5;

    // set target neuron
    randomSeed = pcg_hash(randomSeed);
    synapse->targetNeuronId = static_cast<uint16_t>(randomSeed % block->numberOfNeurons);

    randomSeed = pcg_hash(randomSeed);
    synapse->weight = (static_cast<float>(randomSeed) / randMax) / 10.0f;

    // update weight with sign
    randomSeed = pcg_hash(randomSeed);
    signRand = randomSeed % 1000;
    synapse->weight *= static_cast<float>(1.0f - (1000.0f * sigNeg > signRand) * 2);
}

/**
 * @brief process a single synapse-section
 *
 * @param cluster cluster, where the synapseSection belongs to
 * @param synapseSection current synapse-section to process
 * @param connection pointer to the connection-object, which is related to the section
 * @param targetNeuronBlock neuron-block, which is the target for all synapses in the section
 * @param sourceNeuron pointer to source-neuron, which had triggered the section
 * @param originLocation location of the source-neuron to mark updates
 * @param clusterSettings pointer to cluster-settings
 * @param randomSeed reference to the current seed of the randomizer
 */
template <bool doTrain>
inline void
synapseProcessingBackward(Cluster& cluster,
                          SynapseSection* synapseSection,
                          SynapseConnection* connection,
                          NeuronBlock* targetNeuronBlock,
                          Neuron* sourceNeuron,
                          const SourceLocationPtr originLocation,
                          ClusterSettings* clusterSettings,
                          const bool inputConnected,
                          uint32_t& randomSeed)
{
    float potential = sourceNeuron->potential - connection->offset;
    float val = 0.0f;
    uint8_t pos = 0;
    Synapse* synapse = nullptr;
    Neuron* targetNeuron = nullptr;
    bool condition = false;

    // iterate over all synapses in the section
    while (pos < SYNAPSES_PER_SYNAPSESECTION && potential > 0.01f) {
        synapse = &synapseSection->synapses[pos];

        if constexpr (doTrain) {
            if(inputConnected || cluster.enableCreation)
            {
                // create new synapse if necesarry and training is active
                if (synapse->targetNeuronId == UNINIT_STATE_8) {
                    createNewSynapse(
                        targetNeuronBlock, synapse, clusterSettings, potential, randomSeed);
                    cluster.enableCreation = true;
                }
            }
        }

        if (synapse->targetNeuronId != UNINIT_STATE_8) {
            // update target-neuron
            targetNeuron = &targetNeuronBlock->neurons[synapse->targetNeuronId];
            val = synapse->weight;
            if (potential < synapse->border) {
                val *= ((1.0f / synapse->border) * potential);
            }
            targetNeuron->input += val;
        }

        // update loop-counter
        potential -= synapse->border;

        if constexpr (doTrain) {
            if(potential < 0.0f)
            {
                if((inputConnected)
                        && (potential + synapse->border) < 0.66f * synapse->border
                        && (potential + synapse->border) > 0.33f * synapse->border)
                {
                    synapse->border /= 2.0f;
                    cluster.enableCreation = true;
                }
            }
        }
        ++pos;
    }

    if constexpr (doTrain) {
        sourceNeuron->isNew = potential > 0.01f && synapseSection->hasNext == false;
        sourceNeuron->newOffset = (sourceNeuron->potential - potential) + connection->offset;
        synapseSection->hasNext = synapseSection->hasNext || sourceNeuron->isNew;
    }
}

/**
 * @brief process all synapes of a brick
 *
 * @param cluster cluster, where the brick belongs to
 * @param brick pointer to current brick
 * @param blockId id of the current block within the brick
 */
template <bool doTrain>
inline void
processSynapses(Cluster& cluster, Brick* brick, const uint32_t blockId)
{
    NeuronBlock* neuronBlocks = cluster.neuronBlocks;
    SynapseBlock* synapseBlocks = getItemData<SynapseBlock>(cluster.attachedHost->synapseBlocks);
    ClusterSettings* clusterSettings = &cluster.clusterHeader->settings;
    SynapseConnection* scon = nullptr;
    NeuronBlock* sourceNeuronBlock = nullptr;
    NeuronBlock* targetNeuronBlock = nullptr;
    ConnectionBlock* connectionBlock = nullptr;
    Neuron* sourceNeuron = nullptr;
    SynapseSection* section = nullptr;
    uint32_t randomeSeed = rand();
    const uint32_t dimY = brick->dimY;
    const uint32_t dimX = brick->dimX;

    bool inputConnected = false;

    if (blockId >= dimX) {
        return;
    }

    for (uint32_t c = blockId * dimY; c < (blockId * dimY) + dimY; ++c) {
        assert(c < brick->connectionBlocks.size());
        connectionBlock = &brick->connectionBlocks[c];

        for (uint16_t i = 0; i < NUMBER_OF_SYNAPSESECTION; i++) {
            scon = &connectionBlock->connections[i];
            if (scon->origin.blockId == UNINIT_STATE_32) {
                continue;
            }

            inputConnected = scon->inputConnected;
            sourceNeuronBlock = &neuronBlocks[scon->origin.blockId];
            sourceNeuron = &sourceNeuronBlock->neurons[scon->origin.neuronId];
            if (sourceNeuron->active == 0) {
                continue;
            }

            section = &synapseBlocks[connectionBlock->targetSynapseBlockPos].sections[i];
            targetNeuronBlock = &neuronBlocks[brick->neuronBlockPos + (c / brick->dimY)];
            randomeSeed += (c * NUMBER_OF_SYNAPSESECTION) + i;

            synapseProcessingBackward<doTrain>(cluster,
                                               section,
                                               scon,
                                               targetNeuronBlock,
                                               sourceNeuron,
                                               scon->origin,
                                               clusterSettings,
                                               inputConnected,
                                               randomeSeed);
        }
    }
}

/**
 * @brief process all neurons of a brick
 *
 * @param cluster cluster, where the brick belongs to
 * @param brick pointer to current brick
 * @param blockId id of the current block within the brick
 */
template <bool doTrain>
inline void
processNeurons(Cluster& cluster, Brick* brick, const uint32_t blockId)
{
    NeuronBlock* neuronBlocks = cluster.neuronBlocks;
    ClusterSettings* clusterSettings = &cluster.clusterHeader->settings;
    Neuron* neuron = nullptr;
    NeuronBlock* targetNeuronBlock = nullptr;
    const uint32_t neuronBlockId = brick->neuronBlockPos + blockId;

    targetNeuronBlock = &neuronBlocks[neuronBlockId];
    for (uint32_t neuronId = 0; neuronId < NEURONS_PER_NEURONSECTION; ++neuronId) {
        neuron = &targetNeuronBlock->neurons[neuronId];
        neuron->potential /= clusterSettings->neuronCooldown;
        neuron->refractoryTime = neuron->refractoryTime >> 1;

        if (neuron->refractoryTime == 0) {
            neuron->potential += clusterSettings->potentialOverflow * neuron->input;
            neuron->refractoryTime = clusterSettings->refractoryTime;
        }

        neuron->potential -= neuron->border;
        neuron->active = neuron->potential > 0.0f;
        neuron->potential = static_cast<float>(neuron->active) * neuron->potential;
        neuron->input = 0.0f;
        neuron->potential = log2(neuron->potential + 1.0f);

        if constexpr (doTrain) {
            neuron->isNew = neuron->active != 0 && neuron->inUse == 0;
            neuron->newOffset = 0.0f;
        }
    }
}

#endif  // HANAMI_CORE_PROCESSING_H
