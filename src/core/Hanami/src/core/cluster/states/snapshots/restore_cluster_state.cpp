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
#include <hanami_root.h>
#include <core/cluster/task.h>
#include <core/cluster/cluster.h>
#include <core/cluster/cluster_init.h>
#include <core/cluster/statemachine_init.h>
#include <core/segments/abstract_segment.h>
#include <core/segments/core_segment/core_segment.h>

#include <libKitsunemimiCommon/files/binary_file.h>
#include <libKitsunemimiCrypto/hashes.h>

/**
 * @brief constructor
 *
 * @param cluster pointer to the cluster, where the event and the statemachine belongs to
 */
RestoreCluster_State::RestoreCluster_State(Cluster* cluster)
{
    m_cluster = cluster;
}

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
    Kitsunemimi::ErrorContainer error;
    const std::string originalUuid = m_cluster->getUuid();

    // get meta-infos of data-set from shiori
    Kitsunemimi::JsonItem parsedSnapshotInfo;
    parsedSnapshotInfo.parse(actualTask->snapshotInfo, error);

    // get other information
    const std::string location = parsedSnapshotInfo.get("location").toString();

    // get header
    const std::string header = parsedSnapshotInfo.get("header").toString();
    Kitsunemimi::JsonItem parsedHeader;
    if(parsedHeader.parse(header, error) == false)
    {
        m_cluster->goToNextState(FINISH_TASK);
        return false;
    }

    // get snapshot-data
    Kitsunemimi::BinaryFile snapshotFile(location);
    Kitsunemimi::DataBuffer snapshotBuffer;
    if(snapshotFile.readCompleteFile(snapshotBuffer, error) == false)
    {
        error.addMeesage("failed to load snapshot-data");
        m_cluster->goToNextState(FINISH_TASK);
        return false;
    }

    // copy meta-data of cluster
    const uint64_t headerSize = parsedHeader.get("header").getLong();
    if(Kitsunemimi::reset_DataBuffer(m_cluster->clusterData,
                                     Kitsunemimi::calcBytesToBlocks(headerSize)) == false)
    {
        // TODO: handle error
        m_cluster->goToNextState(FINISH_TASK);
        return false;
    }
    const uint8_t* u8Data = static_cast<const uint8_t*>(snapshotBuffer.data);
    memcpy(m_cluster->clusterData.data, &u8Data[0], headerSize);
    reinitPointer(m_cluster, originalUuid);

    // copy single segments
    uint64_t posCounter = headerSize;
    for(uint64_t i = 0; i < parsedHeader.get("segments").size(); i++)
    {
        Kitsunemimi::JsonItem segment = parsedHeader.get("segments").get(i);
        const SegmentTypes type = static_cast<SegmentTypes>(segment.get("type").getInt());
        const uint64_t size = static_cast<uint64_t>(segment.get("size").getLong());

        switch(type)
        {
            case CORE_SEGMENT:
            {
                CoreSegment* newSegment = new CoreSegment(&u8Data[posCounter], size);
                newSegment->reinitPointer(size);
                newSegment->parentCluster = m_cluster;
                m_cluster->coreSegments.push_back(newSegment);
                break;
            }
            case UNDEFINED_SEGMENT:
            {
                break;
            }
        }

        posCounter += size;
    }

    m_cluster->goToNextState(FINISH_TASK);

    return true;
}
