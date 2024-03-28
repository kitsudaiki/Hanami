/**
 * @file        objects.h
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

#ifndef HANAMI_CORE_SEGMENT_OBJECTS_H
#define HANAMI_CORE_SEGMENT_OBJECTS_H

#include <hanami_common/structs.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>

#include <cstdlib>
#include <string>
#include <vector>

// const predefined values
#define UNINIT_STATE_64 0xFFFFFFFFFFFFFFFF
#define UNINIT_STATE_32 0xFFFFFFFF
#define UNINIT_STATE_24 0xFFFFFF
#define UNINIT_STATE_16 0xFFFF
#define UNINIT_STATE_8 0xFF

#define UNINTI_POINT_32 0x0FFFFFFF

// network-predefines
#define SYNAPSES_PER_SYNAPSESECTION 63
#define NUMBER_OF_SYNAPSESECTION 64
#define NEURONS_PER_NEURONSECTION 64
#define POSSIBLE_NEXT_AXON_STEP 80
#define NUMBER_OF_POSSIBLE_NEXT 64

// processing
#define NUMBER_OF_PROCESSING_UNITS 1
#define NUMBER_OF_RAND_VALUES 10485760

//==================================================================================================

enum ClusterProcessingMode {
    NORMAL_MODE = 0,
    TRAIN_FORWARD_MODE = 1,
    TRAIN_BACKWARD_MODE = 2,
    REDUCTION_MODE = 3,
};

//==================================================================================================

struct kuuid {
    char uuid[UUID_STR_LEN];
    uint8_t padding[3];

    const std::string toString() const { return std::string(uuid, UUID_STR_LEN - 1); }
};
static_assert(sizeof(kuuid) == 40);

//==================================================================================================

struct HeaderEntry {
    uint64_t bytePos = 0;
    uint64_t count = 0;
};
static_assert(sizeof(HeaderEntry) == 16);

//==================================================================================================

struct ClusterSettings {
    float backpropagationBorder = 0.01f;
    float potentialOverflow = 1.0f;

    float neuronCooldown = 1000000000.0f;
    uint32_t refractoryTime = 1;
    uint32_t maxConnectionDistance = 1;
    bool enableReduction = false;

    uint8_t padding[43];
};
static_assert(sizeof(ClusterSettings) == 64);

//==================================================================================================

struct ClusterHeader {
    uint8_t objectType = 0;
    uint8_t version = 1;
    uint8_t padding[2];

    char name[256];
    uint32_t nameSize = 0;

    uint64_t staticDataSize = 0;
    kuuid uuid;

    uint32_t numberOfInputs = 0;
    uint32_t numberOfOutputs = 0;

    // synapse-cluster
    HeaderEntry bricks;
    HeaderEntry neuronBlocks;

    ClusterSettings settings;

    uint8_t padding2[96];
};
static_assert(sizeof(ClusterHeader) == 512);

//==================================================================================================

struct Synapse {
    float weight = 0.0f;
    float border = 0.0f;

    uint8_t padding2[6];
    uint8_t activeCounter = 0;
    uint8_t targetNeuronId = UNINIT_STATE_8;
};
static_assert(sizeof(Synapse) == 16);

//==================================================================================================

struct SynapseSection {
    Synapse synapses[SYNAPSES_PER_SYNAPSESECTION];
    uint8_t padding[15];
    bool hasNext = false;

    SynapseSection() { std::fill_n(synapses, SYNAPSES_PER_SYNAPSESECTION, Synapse()); }
};
static_assert(sizeof(SynapseSection) == 1024);

//==================================================================================================

struct SynapseBlock {
    SynapseSection sections[NUMBER_OF_SYNAPSESECTION];
    float tempValues[NEURONS_PER_NEURONSECTION];

    SynapseBlock()
    {
        std::fill_n(sections, NUMBER_OF_SYNAPSESECTION, SynapseSection());
        std::fill_n(tempValues, NEURONS_PER_NEURONSECTION, 0.0f);
    }
};
static_assert(sizeof(SynapseBlock) == 64 * 1024 + 256);

//==================================================================================================

struct SourceLocationPtr {
    // HINT (kitsudaiki): not initialized here, because they are used in shared memory in cuda
    //                    which doesn't support initializing of the values, when defining the
    //                    shared-memory-object
    uint32_t blockId;
    uint16_t neuronId;
    uint8_t posInNeuron;
    uint8_t padding[1];
};
static_assert(sizeof(SourceLocationPtr) == 8);

//==================================================================================================

struct Neuron {
    float input = 0.0f;
    float border = 100.0f;
    float potential = 0.0f;
    float delta = 0.0f;

    uint8_t refractoryTime = 1;
    uint8_t active = 0;

    float newOffset = 0.0f;
    uint8_t isNew = 0;
    uint8_t inUse = 0;

    uint8_t padding[4];

    void setInUse(const uint8_t pos) { inUse |= (1 << pos); }

    void deleteInUse(const uint8_t pos) { inUse &= (~(1 << pos)); }

    uint8_t getFirstZeroBit()
    {
        for (int i = 0; i < 8; ++i) {
            if ((inUse & (1 << i)) == 0) {
                return i;
            }
        }

        return UNINIT_STATE_8;
    }
};
static_assert(sizeof(Neuron) == 32);

//==================================================================================================

struct NeuronBlock {
    uint32_t numberOfNeurons = 0;
    uint32_t brickId = 0;
    uint8_t padding[8];

    Neuron neurons[NEURONS_PER_NEURONSECTION];

    NeuronBlock() { std::fill_n(neurons, NEURONS_PER_NEURONSECTION, Neuron()); }
};
static_assert(sizeof(NeuronBlock) == 2064);

//==================================================================================================

struct TempNeuron {
    float delta[8];

    TempNeuron() { std::fill_n(delta, 8, 0.0f); }
};
static_assert(sizeof(TempNeuron) == 32);

//==================================================================================================

struct TempNeuronBlock {
    TempNeuron neurons[NEURONS_PER_NEURONSECTION];

    TempNeuronBlock() { std::fill_n(neurons, NEURONS_PER_NEURONSECTION, TempNeuron()); }
};
static_assert(sizeof(TempNeuronBlock) == 2048);

//==================================================================================================

struct SynapseConnection {
    SourceLocationPtr origin;
    float offset = 0.0f;
    bool inputConnected = false;
    uint8_t padding[3];

    SynapseConnection()
    {
        origin.blockId = UNINIT_STATE_32;
        origin.neuronId = UNINIT_STATE_16;
        origin.posInNeuron = 0;
    }
};
static_assert(sizeof(SynapseConnection) == 16);

//==================================================================================================

struct ConnectionBlock {
    SynapseConnection connections[NUMBER_OF_SYNAPSESECTION];
    uint64_t targetSynapseBlockPos = UNINIT_STATE_64;

    ConnectionBlock() { std::fill_n(connections, NUMBER_OF_SYNAPSESECTION, SynapseConnection()); }
};
static_assert(sizeof(ConnectionBlock) == 1032);

//==================================================================================================

struct Brick {
    // common
    uint32_t brickId = UNINIT_STATE_32;
    bool isOutputBrick = false;
    bool isInputBrick = false;
    bool wasResized = false;
    uint8_t padding1[1];

    char name[128];
    uint32_t nameSize = 0;

    uint8_t padding2[4];
    uint32_t ioBufferPos = UNINIT_STATE_32;
    uint32_t neuronBlockPos = UNINIT_STATE_32;

    uint32_t numberOfNeurons = 0;
    uint32_t numberOfNeuronBlocks = 0;

    Hanami::Position brickPos;
    uint32_t neighbors[12];
    uint32_t possibleTargetNeuronBrickIds[NUMBER_OF_POSSIBLE_NEXT];

    uint32_t dimX = 0;
    uint32_t dimY = 0;

    std::vector<ConnectionBlock> connectionBlocks;

    Brick() { std::fill_n(name, 128, '\0'); }

    /**
     * @brief get the name of the brick
     */
    const std::string getName() { return std::string(name, nameSize); }

    /**
     * @brief set new name for the brick
     *
     * @param newName new name
     *
     * @return true, if successful, else false
     */
    bool setName(const std::string& newName)
    {
        // precheck
        if (newName.size() > 127 || newName.size() == 0) {
            return false;
        }

        // copy string into char-buffer and set explicit the escape symbol to be absolut sure
        // that it is set to absolut avoid buffer-overflows
        strncpy(name, newName.c_str(), newName.size());
        name[newName.size()] = '\0';
        nameSize = newName.size();

        return true;
    }
};
static_assert(sizeof(Brick) == 512);

//==================================================================================================

struct CudaPointerHandle {
    uint32_t deviceId = 0;
    NeuronBlock* neuronBlocks = nullptr;
    TempNeuronBlock* tempNeuronBlock = nullptr;
    SynapseBlock* synapseBlocks = nullptr;
    std::vector<ConnectionBlock*> connectionBlocks;

    ClusterSettings* clusterSettings = nullptr;
};

//==================================================================================================

struct CheckpointHeader {
    uint64_t metaSize = 0;
    uint64_t blockSize = 0;

    char name[256];
    uint32_t nameSize = 0;
    char uuid[40];

    uint8_t padding[3780];

    CheckpointHeader()
    {
        std::fill_n(uuid, 40, '\0');
        std::fill_n(name, 256, '\0');
    }

    /**
     * @brief set new name for the brick
     *
     * @param newName new name
     *
     * @return true, if successful, else false
     */
    bool setName(const std::string& newName)
    {
        // precheck
        if (newName.size() > 255 || newName.size() == 0) {
            return false;
        }

        // copy string into char-buffer and set explicit the escape symbol to be absolut sure
        // that it is set to absolut avoid buffer-overflows
        strncpy(name, newName.c_str(), newName.size());
        name[newName.size()] = '\0';
        nameSize = newName.size();

        return true;
    }

    bool setUuid(const kuuid& uuid)
    {
        const std::string uuidStr = uuid.toString();

        strncpy(this->uuid, uuidStr.c_str(), uuidStr.size());
        this->uuid[uuidStr.size()] = '\0';

        return true;
    }
};
static_assert(sizeof(CheckpointHeader) == 4096);

//==================================================================================================

#endif  // HANAMI_CORE_SEGMENT_OBJECTS_H
