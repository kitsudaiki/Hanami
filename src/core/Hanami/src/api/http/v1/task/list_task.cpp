/**
 * @file        list_task.cpp
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

#include "list_task.h"

#include <core/cluster/cluster_handler.h>
#include <core/cluster/cluster.h>
#include <hanami_root.h>

ListTask::ListTask()
    : Blossom("List all visible tasks of a specific cluster.")
{
    //----------------------------------------------------------------------------------------------
    // input
    //----------------------------------------------------------------------------------------------

    registerInputField("cluster_uuid",
                       SAKURA_STRING_TYPE,
                       true,
                       "UUID of the cluster, whos tasks should be listed");
    assert(addFieldRegex("cluster_uuid", UUID_REGEX));

    //----------------------------------------------------------------------------------------------
    // output
    //----------------------------------------------------------------------------------------------

    registerOutputField("header",
                        SAKURA_ARRAY_TYPE,
                        "Array with the namings all columns of the table.");
    assert(addFieldMatch("header", new Kitsunemimi::DataValue("[\"uuid\","
                                                              "\"state\","
                                                              "\"percentage\","
                                                              "\"queued\","
                                                              "\"start\","
                                                              "\"end\"]")));

    registerOutputField("body",
                        SAKURA_ARRAY_TYPE,
                        "Array with all rows of the table, which array arrays too.");

    //----------------------------------------------------------------------------------------------
    //
    //----------------------------------------------------------------------------------------------
}

/**
 * @brief runTask
 */
bool
ListTask::runTask(BlossomIO &blossomIO,
                  const Kitsunemimi::DataMap &context,
                  BlossomStatus &status,
                  Kitsunemimi::ErrorContainer &error)
{
    const UserContext userContext(context);
    const std::string clusterUuid = blossomIO.input.get("cluster_uuid").getString();

    // get cluster
    Cluster* cluster = ClusterHandler::getInstance()->getCluster(clusterUuid);
    if(cluster == nullptr)
    {
        status.errorMessage = "Cluster with UUID '" + clusterUuid + "'not found";
        status.statusCode = NOT_FOUND_RTYPE;
        error.addMeesage(status.errorMessage);
        return false;
    }

    // get progress of all tasks
    std::map<std::string, TaskProgress> progressOverview;
    cluster->getAllProgress(progressOverview);

    // init table-header
    Kitsunemimi::TableItem result;
    result.addColumn("uuid");
    result.addColumn("state");
    result.addColumn("percentage");
    result.addColumn("queued");
    result.addColumn("start");
    result.addColumn("end");

    // build table-content
    for(auto const& [id, progress] : progressOverview)
    {
        if(progress.state == QUEUED_TASK_STATE)
        {
            result.addRow(std::vector<std::string>{
                              id,
                              "queued",
                              std::to_string(progress.percentageFinished),
                              serializeTimePoint(progress.queuedTimeStamp),
                              "-",
                              "-"
                          });
        }
        else if(progress.state == ACTIVE_TASK_STATE)
        {
            result.addRow(std::vector<std::string>{
                              id,
                              "active",
                              std::to_string(progress.percentageFinished),
                              serializeTimePoint(progress.queuedTimeStamp),
                              serializeTimePoint(progress.startActiveTimeStamp),
                              "-"
                          });
        }
        else if(progress.state == ABORTED_TASK_STATE)
        {
            result.addRow(std::vector<std::string>{
                              id,
                              "aborted",
                              std::to_string(progress.percentageFinished),
                              serializeTimePoint(progress.queuedTimeStamp),
                              serializeTimePoint(progress.startActiveTimeStamp),
                              "-"
                          });
        }
        else if(progress.state == FINISHED_TASK_STATE)
        {
            result.addRow(std::vector<std::string>{
                              id,
                              "finished",
                              std::to_string(progress.percentageFinished),
                              serializeTimePoint(progress.queuedTimeStamp),
                              serializeTimePoint(progress.startActiveTimeStamp),
                              serializeTimePoint(progress.endActiveTimeStamp)
                          });
        }
    }

    // prepare for output
    blossomIO.output.insert("header", result.getInnerHeader());
    blossomIO.output.insert("body", result.getBody());

    return true;
}