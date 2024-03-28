/**
 * @file        cluster.cpp
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

#include "cluster.h"

#include <api/websocket/cluster_io.h>
#include <core/cluster/cluster_init.h>
#include <core/cluster/statemachine_init.h>
#include <core/cluster/states/task_handle_state.h>
#include <core/cuda_functions.h>
#include <core/processing/logical_host.h>
#include <hanami_common/logger.h>
#include <hanami_common/statemachine.h>
#include <hanami_common/threading/thread.h>
#include <hanami_root.h>

/**
 * @brief constructor
 */
Cluster::Cluster(LogicalHost* host)
{
    attachedHost = host;
    stateMachine = new Hanami::Statemachine();
    taskHandleState = new TaskHandle_State(this);

    counter.store(0, std::memory_order_relaxed);

    initStatemachine(*stateMachine, this, taskHandleState);
}

/**
 * @brief constructor to create cluster from a checkpoint
 *
 * @param data pointer to data with checkpoint
 * @param dataSize size of checkpoint in number of bytes
 */
Cluster::Cluster(LogicalHost* host, const void* data, const uint64_t dataSize)
{
    attachedHost = host;

    counter.store(0, std::memory_order_relaxed);
    Hanami::allocateBlocks_DataBuffer(clusterData, Hanami::calcBytesToBlocks(dataSize));
    memcpy(clusterData.data, data, dataSize);
}

/**
 * @brief destructor
 */
Cluster::~Cluster()
{
    attachedHost->removeCluster(this);

    delete stateMachine;
    delete inputValues;
    delete outputValues;
    delete expectedValues;
    delete tempNeuronBlocks;
}

/**
 * @brief Cluster::incrementAndCompare
 * @param referenceValue
 */
bool
Cluster::incrementAndCompare(const uint32_t referenceValue)
{
    const int incrementedValue = counter.fetch_add(1, std::memory_order_relaxed);
    if (incrementedValue == referenceValue - 1) {
        counter.store(0, std::memory_order_relaxed);
        return true;
    }

    return false;
}

/**
 * @brief get uuid of the cluster
 *
 * @return uuid of the cluster
 */
const std::string
Cluster::getUuid()
{
    return clusterHeader->uuid.toString();
}

/**
 * @brief init the cluster
 *
 * @param parsedContent TODO
 * @param segmentTemplates TODO
 * @param uuid UUID of the new cluster
 *
 * @return true, if successful, else false
 */
bool
Cluster::init(const Hanami::ClusterMeta& clusterTemplate, const std::string& uuid)
{
    return initNewCluster(this, clusterTemplate, uuid);
}

/**
 * @brief get total size of data of the cluster
 *
 * @return size of cluster in bytes
 */
uint64_t
Cluster::getDataSize() const
{
    uint64_t size = clusterData.totalBufferSize;
    for (uint64_t i = 0; i < clusterHeader->bricks.count; i++) {
        const uint64_t numberOfConnections = bricks[i].connectionBlocks.size();
        size += numberOfConnections * sizeof(ConnectionBlock);
        size += numberOfConnections * sizeof(SynapseBlock);
    }

    return size;
}

/**
 * @brief get the name of the clsuter
 *
 * @return name of the cluster
 */
const std::string
Cluster::getName()
{
    // precheck
    if (clusterHeader == nullptr) {
        return std::string("");
    }

    return std::string(clusterHeader->name, clusterHeader->nameSize);
}

/**
 * @brief set new name for the cluster
 *
 * @param newName new name
 *
 * @return true, if successful, else false
 */
bool
Cluster::setName(const std::string& newName)
{
    // precheck
    if (clusterHeader == nullptr || newName.size() > 255 || newName.size() == 0) {
        return false;
    }

    // copy string into char-buffer and set explicit the escape symbol to be absolut sure
    // that it is set to absolut avoid buffer-overflows
    strncpy(clusterHeader->name, newName.c_str(), newName.size());
    clusterHeader->name[newName.size()] = '\0';
    clusterHeader->nameSize = newName.size();

    return true;
}

/**
 * @brief start a new forward train-cycle
 */
void
Cluster::startForwardCycle()
{
    attachedHost->addClusterToHost(this);
}

/**
 * @brief start a new backward train-cycle
 */
void
Cluster::startBackwardCycle()
{
    attachedHost->addClusterToHost(this);
}

/**
 * @brief Cluster::startReductionCycle
 */
void
Cluster::startReductionCycle()
{
    attachedHost->addClusterToHost(this);
}

/**
 * @brief switch state of the cluster between task and direct mode
 *
 * @param newState new desired state
 *
 * @return true, if switch in statemachine was successful, else false
 */
bool
Cluster::setClusterState(const std::string& newState)
{
    if (newState == "DIRECT") {
        return goToNextState(SWITCH_TO_DIRECT_MODE);
    }

    if (newState == "TASK") {
        return goToNextState(SWITCH_TO_TASK_MODE);
    }

    return false;
}

/**
 * @brief update state of the cluster, which is caled for each finalized cluster
 */
void
Cluster::updateClusterState()
{
    std::lock_guard<std::mutex> guard(m_clusterStateLock);

    enableCreation = false;

    // trigger next lerning phase, if already in phase 1
    if (mode == ClusterProcessingMode::TRAIN_FORWARD_MODE) {
        mode = ClusterProcessingMode::TRAIN_BACKWARD_MODE;
        startBackwardCycle();
    }
    else if (mode == ClusterProcessingMode::TRAIN_BACKWARD_MODE) {
        reductionCounter++;
        if (reductionCounter >= 100 && clusterHeader->settings.enableReduction) {
            mode = ClusterProcessingMode::REDUCTION_MODE;
            startReductionCycle();
            reductionCounter = 0;
        }
        else {
            sendClusterTrainEndMessage(this);
            goToNextState(NEXT);
        }
    }
    else if (mode == ClusterProcessingMode::REDUCTION_MODE) {
        sendClusterTrainEndMessage(this);
        goToNextState(NEXT);
    }
    else if (mode == ClusterProcessingMode::NORMAL_MODE) {
        sendClusterNormalEndMessage(this);
        goToNextState(NEXT);
    }
}

/**
 * @brief get actual task
 *
 * @return pointer to the actual task
 */
Task*
Cluster::getActualTask() const
{
    return taskHandleState->getActualTask();
}

/**
 * @brief get cycle of the actual task
 *
 * @return cycle of the actual task
 */
uint64_t
Cluster::getActualTaskCycle() const
{
    return taskHandleState->getActualTask()->actualCycle;
}

/**
 * @brief get task-progress
 *
 * @param taskUuid UUID of the task
 *
 * @return task-progress
 */
const TaskProgress
Cluster::getProgress(const std::string& taskUuid)
{
    return taskHandleState->getProgress(taskUuid);
}

/**
 * @brief remove task from queue of abort the task, if actual in progress
 *
 * @param taskUuid UUID of the task
 *
 * @return always true
 */
bool
Cluster::removeTask(const std::string& taskUuid)
{
    return taskHandleState->removeTask(taskUuid);
}

/**
 * @brief check if a task is finished
 *
 * @param taskUuid UUID of the task
 *
 * @return true, if task is finished, else false
 */
bool
Cluster::isFinish(const std::string& taskUuid)
{
    return taskHandleState->isFinish(taskUuid);
}

/**
 * @brief Cluster::getAllProgress
 * @param result
 */
void
Cluster::getAllProgress(std::map<std::string, TaskProgress>& result)
{
    return taskHandleState->getAllProgress(result);
}

/**
 * @brief switch statemachine of cluster to next state
 *
 * @param nextStateId id of the next state
 *
 * @return true, if statemachine switch was successful, else false
 */
bool
Cluster::goToNextState(const uint32_t nextStateId)
{
    return stateMachine->goToNextState(nextStateId);
}
