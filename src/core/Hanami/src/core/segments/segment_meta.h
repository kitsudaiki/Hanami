/**
 * @file        segment_meta.h
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

#ifndef HANAMI_SEGMENT_META_H
#define HANAMI_SEGMENT_META_H

#include <common.h>

#include <libKitsunemimiHanamiClusterParser/segment_meta.h>

enum SegmentTypes
{
    UNDEFINED_SEGMENT = 0,
    INPUT_SEGMENT = 1,
    OUTPUT_SEGMENT = 2,
    CORE_SEGMENT = 3,
};

struct SegmentHeaderEntry
{
    uint64_t bytePos = 0;
    uint64_t count = 0;

    // total size: 16 Byte
};

struct SegmentHeader
{
    uint8_t objectType = SEGMENT_OBJECT;
    uint8_t version = 1;
    uint8_t segmentType = UNDEFINED_SEGMENT;
    uint8_t padding;
    uint32_t segmentID = UNINIT_STATE_32;
    uint64_t staticDataSize = 0;
    Kitsunemimi::Position position;

    kuuid parentClusterId;

    // synapse-segment
    SegmentHeaderEntry name;
    SegmentHeaderEntry settings;
    SegmentHeaderEntry slotList;
    SegmentHeaderEntry inputValues;
    SegmentHeaderEntry outputValues;
    SegmentHeaderEntry expectedValues;

    SegmentHeaderEntry bricks;
    SegmentHeaderEntry brickOrder;
    SegmentHeaderEntry neuronBlocks;
    SegmentHeaderEntry synapseConnections;
    SegmentHeaderEntry synapseBlocks;

    uint8_t padding2[262];
};
static_assert(sizeof(SegmentHeader) == 512);

enum SlotDirection
{
    UNDEFINED_DIRECTION = 0,
    INPUT_DIRECTION = 1,
    OUTPUT_DIRECTION = 2,
};

struct SegmentName
{
    char name[248];
    uint64_t length = 0;

    /**
     * @brief get name of the segment
     *
     * @return name of the segment
     */
    const std::string
    getName() const
    {
        return std::string(name, length);
    }

    /**
     * @brief set new name for the segment
     *
     * @param newName new name
     *
     * @return false, if name is too long or empty, else true
     */
    bool
    setName(const std::string &newName)
    {
        if(newName.size() == 0
                || newName.size() >= 248)
        {
            return false;
        }

        memcpy(name, newName.c_str(), newName.size());
        name[newName.size()] = '\0';
        // I know, that the \0 should be enough, but I like string limitations more explicit to
        // avoid the risk of buffer overflows
        length = newName.size();

        return true;
    }
};
static_assert(sizeof(SegmentName) == 256);

#endif // SEGMENT_META_H
