/**
 * @file        save_cluster.cpp
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

#include "save_cluster.h"
#include <hanami_root.h>
#include <core/cluster/cluster_handler.h>
#include <core/cluster/cluster.h>
#include <core/cluster/add_tasks.h>

SaveCluster::SaveCluster()
    : Blossom("Save a cluster.")
{
    //----------------------------------------------------------------------------------------------
    // input
    //----------------------------------------------------------------------------------------------

    registerInputField("name",
                       SAKURA_STRING_TYPE,
                       true,
                       "Name for task, which is place in the task-queue and for the new snapshot.");
    assert(addFieldBorder("name", 4, 256));
    assert(addFieldRegex("name", NAME_REGEX));

    registerInputField("cluster_uuid",
                       SAKURA_STRING_TYPE,
                       true,
                       "UUID of the cluster, which should be saved as new snapstho to shiori.");
    assert(addFieldRegex("cluster_uuid", UUID_REGEX));

    //----------------------------------------------------------------------------------------------
    // output
    //----------------------------------------------------------------------------------------------

    registerOutputField("uuid",
                        SAKURA_STRING_TYPE,
                        "UUID of the save-task in the queue of the cluster.");
    registerOutputField("name",
                        SAKURA_STRING_TYPE,
                        "Name of the new created task and of the snapshot, "
                        "which should be created by the task.");

    //----------------------------------------------------------------------------------------------
    //
    //----------------------------------------------------------------------------------------------
}

/**
 * @brief runTask
 */
bool
SaveCluster::runTask(BlossomIO &blossomIO,
                     const Kitsunemimi::DataMap &context,
                     BlossomStatus &status,
                     Kitsunemimi::ErrorContainer &error)
{
    const std::string clusterUuid = blossomIO.input.get("cluster_uuid").getString();
    const std::string name = blossomIO.input.get("name").getString();
    const UserContext userContext(context);

    // get cluster
    Cluster* cluster = ClusterHandler::getInstance()->getCluster(clusterUuid);
    if(cluster == nullptr)
    {
        status.errorMessage = "Cluster with UUID '" + clusterUuid + "'not found";
        status.statusCode = NOT_FOUND_RTYPE;
        error.addMeesage(status.errorMessage);
        return false;
    }

    // init request-task
    const std::string taskUuid = addClusterSnapshotSaveTask(*cluster,
                                                            name,
                                                            userContext.userId,
                                                            userContext.projectId);
    blossomIO.output.insert("uuid", taskUuid);
    blossomIO.output.insert("name", name);

    return true;
}