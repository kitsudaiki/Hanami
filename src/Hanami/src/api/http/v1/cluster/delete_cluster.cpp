/**
 * @file        delete_cluster.cpp
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

#include "delete_cluster.h"

#include <core/cluster/cluster.h>
#include <core/cluster/cluster_handler.h>
#include <database/cluster_table.h>
#include <hanami_root.h>

DeleteCluster::DeleteCluster() : Blossom("Delete a cluster.")
{
    errorCodes.push_back(NOT_FOUND_RTYPE);

    //----------------------------------------------------------------------------------------------
    // input
    //----------------------------------------------------------------------------------------------

    registerInputField("uuid", SAKURA_STRING_TYPE)
        .setComment("UUID of the cluster.")
        .setRegex(UUID_REGEX);

    //----------------------------------------------------------------------------------------------
    //
    //----------------------------------------------------------------------------------------------
}

/**
 * @brief runTask
 */
bool
DeleteCluster::runTask(BlossomIO& blossomIO,
                       const json& context,
                       BlossomStatus& status,
                       Hanami::ErrorContainer& error)
{
    const UserContext userContext(context);
    const std::string clusterUuid = blossomIO.input["uuid"];

    // check if user exist within the table
    json getResult;
    if (ClusterTable::getInstance()->getCluster(getResult, clusterUuid, userContext, error)
        == false)
    {
        status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
        return false;
    }

    // handle not found
    if (getResult.size() == 0) {
        status.errorMessage = "Cluster with uuid '" + clusterUuid + "' not found";
        status.statusCode = NOT_FOUND_RTYPE;
        LOG_DEBUG(status.errorMessage);
        return false;
    }

    // remove data from table
    if (ClusterTable::getInstance()->deleteCluster(clusterUuid, userContext, error) == false) {
        status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
        error.addMessage("Failed to delete cluster with UUID '" + clusterUuid + "' from database");
        return false;
    }

    // remove internal data
    const std::string uuid = getResult["uuid"];
    if (ClusterHandler::getInstance()->removeCluster(uuid) == false) {
        // should never be false, because the uuid is already defined as unique by the database
        status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
        error.addMessage("Failed to delete cluster with UUID '" + clusterUuid
                         + "' from cluster-handler");
        return false;
    }

    return true;
}
