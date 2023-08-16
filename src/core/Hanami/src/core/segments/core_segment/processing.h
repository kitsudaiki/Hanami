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

#include <common.h>
#include <math.h>
#include <cmath>

#include <core/cluster/cluster.h>
#include <api/websocket/cluster_io.h>

#include <hanami_root.h>
#include "objects.h"
#include "core_segment.h"
#include "section_update.h"

/**
 * @brief get position of the highest output-position
 *
 * @param segment output-segment to check
 *
 * @return position of the highest output.
 */
inline uint32_t
getHighestOutput(const CoreSegment &segment)
{
    float hightest = -0.1f;
    uint32_t hightestPos = 0;
    float value = 0.0f;

    for(uint32_t outputNeuronId = 0;
        outputNeuronId < segment.segmentHeader->outputValues.count;
        outputNeuronId++)
    {
        value = segment.outputValues[outputNeuronId];
        if(value > hightest)
        {
            hightest = value;
            hightestPos = outputNeuronId;
        }
    }

    return hightestPos;
}

/**
 * @brief initialize a new specific synapse
 */
inline void
createNewSynapse(NeuronBlock* block,
                 Synapse* synapse,
                 const SegmentSettings* segmentSettings,
                 const float remainingW)
{
    const uint32_t* randomValues = HanamiRoot::m_randomValues;
    const float randMax = static_cast<float>(RAND_MAX);
    uint32_t signRand = 0;
    const float sigNeg = segmentSettings->signNeg;

    // set activation-border
    synapse->border = remainingW;

    // set target neuron
    block->randomPos = (block->randomPos + 1) % NUMBER_OF_RAND_VALUES;
    synapse->targetNeuronId = static_cast<uint16_t>(randomValues[block->randomPos]
                              % block->numberOfNeurons);


    block->randomPos = (block->randomPos + 1) % NUMBER_OF_RAND_VALUES;
    synapse->weight = (static_cast<float>(randomValues[block->randomPos]) / randMax) / 10.0f;

    // update weight with sign
    block->randomPos = (block->randomPos + 1) % NUMBER_OF_RAND_VALUES;
    signRand = randomValues[block->randomPos] % 1000;
    synapse->weight *= static_cast<float>(1.0f - (1000.0f * sigNeg > signRand) * 2);

    synapse->activeCounter = 1;
}

/**
 * @brief process synapse-section
 */
inline void
synapseProcessing(CoreSegment &segment,
                  Synapse* section,
                  SynapseConnection* connection,
                  LocationPtr* sourceLocation,
                  const float outH,
                  NeuronBlock* neuronBlocks,
                  SynapseBlock* synapseBlocks,
                  SynapseConnection* synapseConnections,
                  SegmentSettings* segmentSettings)
{
    uint32_t pos = 0;
    Synapse* synapse = nullptr;
    Neuron* targetNeuron = nullptr;
    uint8_t active = 0;
    float counter = outH - connection->offset[sourceLocation->sectionId];
    NeuronBlock* neuronBlock = &neuronBlocks[connection->targetNeuronBlockId];

    // iterate over all synapses in the section
    while(pos < SYNAPSES_PER_SYNAPSESECTION
          && counter > 0.01f)
    {
        synapse = &section[pos];

        // create new synapse if necesarry and learning is active
        if(synapse->targetNeuronId == UNINIT_STATE_16) {
            createNewSynapse(neuronBlock, synapse, segmentSettings, counter);
        }

        if(synapse->border > 2.0f * counter
                && pos < SYNAPSES_PER_SYNAPSESECTION-2)
        {
            const float val = synapse->border / 2.0f;
            section[pos + 1].border += val;
            synapse->border -= val;
        }

        // update target-neuron
        targetNeuron = &neuronBlock->neurons[synapse->targetNeuronId];
        targetNeuron->input += synapse->weight;

        // update active-counter
        active = (synapse->weight > 0) == (targetNeuron->potential > targetNeuron->border);
        synapse->activeCounter += active * static_cast<uint8_t>(synapse->activeCounter < 126);

        // update loop-counter
        counter -= synapse->border;
        pos++;
    }

    LocationPtr* targetLocation = &connection->next[sourceLocation->sectionId];
    if(counter > 0.01f
            && targetLocation->sectionId == UNINIT_STATE_16)
    {
        const float newOffset = (outH - counter) + connection->offset[sourceLocation->sectionId];
        processUpdatePositon_CPU(segment, sourceLocation, targetLocation, newOffset);
    }

    if(targetLocation->sectionId != UNINIT_STATE_16)
    {
        Synapse* nextSection = synapseBlocks[targetLocation->blockId].synapses[targetLocation->sectionId];
        SynapseConnection* nextConnection = &synapseConnections[targetLocation->blockId];
        synapseProcessing(segment,
                          nextSection,
                          nextConnection,
                          targetLocation,
                          outH,
                          neuronBlocks,
                          synapseBlocks,
                          synapseConnections,
                          segmentSettings);
    }
}

/**
 * @brief process only a single neuron
 */
inline void
processSingleNeuron(CoreSegment &segment,
                    Neuron* neuron,
                    const uint32_t blockId,
                    const uint32_t neuronIdInBlock,
                    NeuronBlock* neuronBlocks,
                    SynapseBlock* synapseBlocks,
                    SynapseConnection* synapseConnections,
                    SegmentSettings* segmentSettings)
{
    // handle active-state
    if(neuron->active == 0) {
        return;
    }

    LocationPtr* targetLocation = &neuron->target;
    if(targetLocation->blockId == UNINIT_STATE_32)
    {
        LocationPtr sourceLocation;
        sourceLocation.blockId = blockId;
        sourceLocation.sectionId = neuronIdInBlock;
        sourceLocation.isNeuron = true;
        if(processUpdatePositon_CPU(segment, &sourceLocation, targetLocation, 0.0f) == false) {
            return;
        }
    }

    Synapse* nextSection = synapseBlocks[targetLocation->blockId].synapses[targetLocation->sectionId];
    SynapseConnection* nextConnection = &synapseConnections[targetLocation->blockId];
    synapseProcessing(segment,
                      nextSection,
                      nextConnection,
                      targetLocation,
                      neuron->potential,
                      neuronBlocks,
                      synapseBlocks,
                      synapseConnections,
                      segmentSettings);
}

/**
 * @brief process output brick
 */
inline void
processNeuronsOfOutputBrick(CoreSegment &segment,
                            const Brick* brick,
                            float* outputValues,
                            NeuronBlock* neuronBlocks,
                            SegmentSettings* segmentSettings)
{
    Neuron* neuron = nullptr;
    NeuronBlock* block = nullptr;
    uint32_t counter = 0;

    // iterate over all neurons within the brick
    for(uint32_t blockId = brick->brickBlockPos;
        blockId < brick->numberOfNeuronSections + brick->brickBlockPos;
        blockId++)
    {
        block = &neuronBlocks[blockId];
        for(uint32_t neuronIdInBlock = 0;
            neuronIdInBlock < block->numberOfNeurons;
            neuronIdInBlock++)
        {
            neuron = &block->neurons[neuronIdInBlock];
            neuron->potential = segmentSettings->potentialOverflow * neuron->input;
            if(neuron->potential != 0.0f) {
                neuron->potential = 1.0f / (1.0f + exp(-1.0f * neuron->potential));
            }
            outputValues[counter] = neuron->potential;
            neuron->input = 0.0f;
            counter++;
        }

        // send output back if a client-connection is set
        if(segment.parentCluster->msgClient != nullptr
                && segment.parentCluster->mode == Cluster::NORMAL_MODE)
        {
             sendClusterOutputMessage(segment);
        }
    }
}

/**
 * @brief process input brick
 */
inline void
processNeuronsOfInputBrick(CoreSegment &segment,
                           const Brick* brick,
                           float* inputValues,
                           NeuronBlock* neuronBlocks,
                           SynapseBlock* synapseBlocks,
                           SynapseConnection* synapseConnections,
                           SegmentSettings* segmentSettings)
{
    Neuron* neuron = nullptr;
    NeuronBlock* block = nullptr;
    uint32_t counter = 0;

    // iterate over all neurons within the brick
    for(uint32_t blockId = brick->brickBlockPos;
        blockId < brick->numberOfNeuronSections + brick->brickBlockPos;
        blockId++)
    {
        block = &neuronBlocks[blockId];
        for(uint32_t neuronIdInBlock = 0;
            neuronIdInBlock < block->numberOfNeurons;
            neuronIdInBlock++)
        {
            neuron = &block->neurons[neuronIdInBlock];
            neuron->potential = inputValues[counter];
            neuron->active = neuron->potential > 0.0f;

            processSingleNeuron(segment,
                                neuron,
                                blockId,
                                neuronIdInBlock,
                                neuronBlocks,
                                synapseBlocks,
                                synapseConnections,
                                segmentSettings);

            counter++;
        }
    }
}

/**
 * @brief process normal internal brick
 */
inline void
processNeuronsOfNormalBrick(CoreSegment &segment,
                            const Brick* brick,
                            NeuronBlock* neuronBlocks,
                            SynapseBlock* synapseBlocks,
                            SynapseConnection* synapseConnections,
                            SegmentSettings* segmentSettings)
{
    Neuron* neuron = nullptr;
    NeuronBlock* blocks = nullptr;

    // iterate over all neurons within the brick
    for(uint32_t blockId = brick->brickBlockPos;
        blockId < brick->numberOfNeuronSections + brick->brickBlockPos;
        blockId++)
    {
        blocks = &neuronBlocks[blockId];
        for(uint32_t neuronIdInBlock = 0;
            neuronIdInBlock < blocks->numberOfNeurons;
            neuronIdInBlock++)
        {
            neuron = &blocks->neurons[neuronIdInBlock];

            neuron->potential /= segmentSettings->neuronCooldown;
            neuron->refractionTime = neuron->refractionTime >> 1;

            if(neuron->refractionTime == 0)
            {
                neuron->potential = segmentSettings->potentialOverflow * neuron->input;
                neuron->refractionTime = segmentSettings->refractionTime;
            }

            // update neuron
            neuron->potential -= neuron->border;
            neuron->active = neuron->potential > 0.0f;
            neuron->input = 0.0f;
            neuron->potential = log2(neuron->potential + 1.0f);

            processSingleNeuron(segment,
                                neuron,
                                blockId,
                                neuronIdInBlock,
                                neuronBlocks,
                                synapseBlocks,
                                synapseConnections,
                                segmentSettings);
        }
    }
}

/**
 * @brief process all neurons within a segment
 */
inline void
prcessCoreSegment(CoreSegment &segment)
{
    float* inputValues = segment.inputValues;
    float* outputValues = segment.outputValues;
    NeuronBlock* neuronBlocks = segment.neuronBlocks;
    SynapseConnection* synapseConnections = segment.synapseConnections;
    SynapseBlock* synapseBlocks = segment.synapseBlocks;
    SegmentSettings* segmentSettings = segment.segmentSettings;

    const uint32_t numberOfBricks = segment.segmentHeader->bricks.count;
    for(uint32_t pos = 0; pos < numberOfBricks; pos++)
    {
        const uint32_t brickId = segment.brickOrder[pos];
        Brick* brick = &segment.bricks[brickId];
        if(brick->isInputBrick)
        {
            processNeuronsOfInputBrick(segment,
                                       brick,
                                       inputValues,
                                       neuronBlocks,
                                       synapseBlocks,
                                       synapseConnections,
                                       segmentSettings);
        }
        else if(brick->isOutputBrick)
        {
            processNeuronsOfOutputBrick(segment,
                                        brick,
                                        outputValues,
                                        neuronBlocks,
                                        segmentSettings);
        }
        else
        {
            processNeuronsOfNormalBrick(segment,
                                        brick,
                                        neuronBlocks,
                                        synapseBlocks,
                                        synapseConnections,
                                        segmentSettings);
        }
    }
}

#endif // HANAMI_CORE_PROCESSING_H
