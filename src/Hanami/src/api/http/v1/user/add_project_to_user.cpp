/**
 * @file        add_project_to_user.cpp
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

#include "add_project_to_user.h"

#include <hanami_root.h>
#include <database/users_table.h>

#include <libKitsunemimiCrypto/hashes.h>
#include <libKitsunemimiCommon/methods/string_methods.h>
#include <libKitsunemimiJson/json_item.h>

/**
 * @brief constructor
 */
AddProjectToUser::AddProjectToUser()
    : Blossom("Add a project to a specific user.")
{
    //----------------------------------------------------------------------------------------------
    // input
    //----------------------------------------------------------------------------------------------

    registerInputField("id", SAKURA_STRING_TYPE)
            .setComment("ID of the user.")
            .setLimit(4, 256)
            .setRegex(ID_EXT_REGEX);

    registerInputField("project_id", SAKURA_STRING_TYPE)
            .setComment("ID of the project, which has to be added to the user.")
            .setLimit(4, 256)
            .setRegex(ID_REGEX);

    registerInputField("role", SAKURA_STRING_TYPE)
            .setComment("Role, which has to be assigned to the user within the project")
            .setLimit(4, 256)
            .setRegex(ID_REGEX);

    registerInputField("is_project_admin", SAKURA_BOOL_TYPE)
            .setComment("Set this to true, if the user should be an admin "
                        "within the assigned project.")
            .setDefault(new Kitsunemimi::DataValue(false));

    //----------------------------------------------------------------------------------------------
    // output
    //----------------------------------------------------------------------------------------------

    registerOutputField("id", SAKURA_STRING_TYPE)
            .setComment("ID of the user.");

    registerOutputField("name", SAKURA_STRING_TYPE)
            .setComment("Name of the user.");

    registerOutputField("is_admin", SAKURA_BOOL_TYPE)
            .setComment("True, if user is an admin.");

    registerOutputField("creator_id", SAKURA_STRING_TYPE)
            .setComment("Id of the creator of the user.");

    registerOutputField("projects", SAKURA_ARRAY_TYPE)
            .setComment("Json-array with all assigned projects "
                        "together with role and project-admin-status.");

    //----------------------------------------------------------------------------------------------
    //
    //----------------------------------------------------------------------------------------------
}

/**
 * @brief runTask
 */
bool
AddProjectToUser::runTask(BlossomIO &blossomIO,
                          const Kitsunemimi::DataMap &context,
                          BlossomStatus &status,
                          Kitsunemimi::ErrorContainer &error)
{
    // check if admin
    if(context.getBoolByKey("is_admin") == false)
    {
        status.statusCode = UNAUTHORIZED_RTYPE;
        return false;
    }

    const std::string userId = blossomIO.input.get("id").getString();
    const std::string projectId = blossomIO.input.get("project_id").getString();
    const std::string role = blossomIO.input.get("role").getString();
    const bool isProjectAdmin = blossomIO.input.get("is_project_admin").getBool();
    const std::string creatorId = context.getStringByKey("id");

    // check if user already exist within the table
    Kitsunemimi::JsonItem getResult;
    if(UsersTable::getInstance()->getUser(getResult, userId, error, false) == false)
    {
        status.errorMessage = "User with id '" + userId + "' not found.";
        status.statusCode = NOT_FOUND_RTYPE;
        return false;
    }

    // check if project is already assigned to user
    Kitsunemimi::JsonItem parsedProjects = getResult.get("projects");
    for(uint64_t i = 0; i < parsedProjects.size(); i++)
    {
        if(parsedProjects.get(i).get("project_id").getString() == projectId)
        {
            status.errorMessage = "Project with ID '"
                                  + projectId
                                  + "' is already assigned to user with id '"
                                  + userId
                                  + "'.";
            error.addMeesage(status.errorMessage);
            status.statusCode = CONFLICT_RTYPE;
            return false;
        }
    }

    // create new entry
    Kitsunemimi::JsonItem newEntry;
    newEntry.insert("project_id", projectId);
    newEntry.insert("role", role);
    newEntry.insert("is_project_admin", isProjectAdmin);
    parsedProjects.append(newEntry);

    // updated projects of user in database
    if(UsersTable::getInstance()->updateProjectsOfUser(userId,
                                                       parsedProjects.toString(),
                                                       error) == false)
    {
        error.addMeesage("Failed to update projects of user with id '" + userId + "'.");
        status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
        return false;
    }

    // get new created user from database
    if(UsersTable::getInstance()->getUser(blossomIO.output,
                                          userId,
                                          error,
                                          false) == false)
    {
        status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
        return false;
    }

    return true;
}
