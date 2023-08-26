/**
 * @file        create_cluster_template.cpp
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

#include "create_cluster.h"

#include <core/cluster/cluster_handler.h>
#include <core/cluster/cluster.h>

#include <libKitsunemimiCrypto/common.h>
#include <libKitsunemimiCommon/buffer/data_buffer.h>
#include <libKitsunemimiJson/json_item.h>

#include <hanami_root.h>

CreateCluster::CreateCluster()
    : Blossom("Create new cluster.")
{
    //----------------------------------------------------------------------------------------------
    // input
    //----------------------------------------------------------------------------------------------

    registerInputField("name",
                       SAKURA_STRING_TYPE,
                       true,
                       "Name for the new cluster.");
    assert(addFieldBorder("name", 4, 256));
    assert(addFieldRegex("name", NAME_REGEX));

    registerInputField("template",
                       SAKURA_STRING_TYPE,
                       false,
                       "Cluster-template as base64-string.");

    //----------------------------------------------------------------------------------------------
    // output
    //----------------------------------------------------------------------------------------------

    registerOutputField("uuid",
                        SAKURA_STRING_TYPE,
                        "UUID of the new created cluster.");
    registerOutputField("name",
                        SAKURA_STRING_TYPE,
                        "Name of the new created cluster.");
    registerOutputField("owner_id",
                        SAKURA_STRING_TYPE,
                        "ID of the user, who created the new cluster.");
    registerOutputField("project_id",
                        SAKURA_STRING_TYPE,
                        "ID of the project, where the new cluster belongs to.");
    registerOutputField("visibility",
                        SAKURA_STRING_TYPE,
                        "Visibility of the new created cluster (private, shared, public).");

    //----------------------------------------------------------------------------------------------
    //
    //----------------------------------------------------------------------------------------------
}

/**
 * @brief runTask
 */
bool
CreateCluster::runTask(BlossomIO &blossomIO,
                       const Kitsunemimi::DataMap &context,
                       BlossomStatus &status,
                       Kitsunemimi::ErrorContainer &error)
{
    const std::string clusterName = blossomIO.input.get("name").getString();
    const std::string base64Template = blossomIO.input.get("template").getString();
    const UserContext userContext(context);

    // check if user already exist within the table
    Kitsunemimi::JsonItem getResult;
    if(ClusterTable::getInstance()->getClusterByName(getResult, clusterName, userContext, error))
    {
        status.errorMessage = "Cluster with name '" + clusterName + "' already exist.";
        error.addMeesage(status.errorMessage);
        status.statusCode = CONFLICT_RTYPE;
        return false;
    }
    error._errorMessages.clear();
    error._possibleSolution.clear();

    Kitsunemimi::Hanami::ClusterMeta parsedCluster;
    if(base64Template != "")
    {
        // decode base64 formated template to check if valid base64-string
        Kitsunemimi::DataBuffer convertedTemplate;
        if(Kitsunemimi::decodeBase64(convertedTemplate, base64Template) == false)
        {
            status.errorMessage = "Uploaded template is not a valid base64-String.";
            status.statusCode = BAD_REQUEST_RTYPE;
            error.addMeesage(status.errorMessage);
            return false;
        }

        // parse cluster-template to validate syntax
        const std::string convertedTemplateStr(static_cast<const char*>(convertedTemplate.data),
                                               convertedTemplate.usedBufferSize);
        if(Kitsunemimi::Hanami::parseCluster(&parsedCluster, convertedTemplateStr, error) == false)
        {
            status.errorMessage = "Uploaded template is not a valid cluster-template: \n";
            status.errorMessage += error.toString();
            status.statusCode = BAD_REQUEST_RTYPE;
            error.addMeesage(status.errorMessage);
            return false;
        }
    }

    // convert values
    Kitsunemimi::JsonItem clusterData;
    clusterData.insert("name", clusterName);
    clusterData.insert("project_id", userContext.projectId);
    clusterData.insert("owner_id", userContext.userId);
    clusterData.insert("visibility", "private");

    // add new user to table
    if(ClusterTable::getInstance()->addCluster(clusterData, userContext, error) == false)
    {
        status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
        error.addMeesage("Failed to add cluster to database");
        return false;
    }

    // get new created user from database
    if(ClusterTable::getInstance()->getClusterByName(blossomIO.output,
                                                     clusterName,
                                                     userContext,
                                                     error) == false)
    {
        error.addMeesage("Failed to get cluster from database by name '" + clusterName + "'");
        status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
        return false;
    }

    const std::string uuid = blossomIO.output.get("uuid").getString();

    // create new cluster
    Cluster* newCluster = new Cluster();
    if(base64Template != "")
    {
        // generate and initialize the cluster based on the cluster-templates
        if(newCluster->init(parsedCluster, uuid) == false)
        {
            error.addMeesage("Failed to initialize cluster based on a template");
            status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
            delete newCluster;
            ClusterTable::getInstance()->deleteCluster(uuid, userContext, error);
            return false;
        }
    }

    ClusterHandler::getInstance()->addCluster(uuid, newCluster);

    // remove irrelevant fields
    blossomIO.output.remove("owner_id");
    blossomIO.output.remove("project_id");
    blossomIO.output.remove("visibility");

    return true;
}
