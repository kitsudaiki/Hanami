/**
 * @file        remove_project_from_user.cpp
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

#include "remove_project_from_user.h"

#include <hanami_root.h>
#include <database/users_table.h>

#include <hanami_crypto/hashes.h>
#include <hanami_common/methods/string_methods.h>
#include <hanami_json/json_item.h>

/**
 * @brief constructor
 */
RemoveProjectFromUser::RemoveProjectFromUser()
    : Blossom("Remove a project from a specific user")
{
    errorCodes.push_back(UNAUTHORIZED_RTYPE);
    errorCodes.push_back(NOT_FOUND_RTYPE);

    //----------------------------------------------------------------------------------------------
    // input
    //----------------------------------------------------------------------------------------------

    registerInputField("id", SAKURA_STRING_TYPE)
            .setComment("ID of the user.")
            .setLimit(4, 256)
            .setRegex(ID_EXT_REGEX);

    registerInputField("project_id", SAKURA_STRING_TYPE)
            .setComment("ID of the project, which has to be removed from the user.")
            .setLimit(4, 256)
            .setRegex(ID_REGEX);

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
RemoveProjectFromUser::runTask(BlossomIO &blossomIO,
                               const Hanami::DataMap &context,
                               BlossomStatus &status,
                               Hanami::ErrorContainer &error)
{
    // check if admin
    if(context.getBoolByKey("is_admin") == false)
    {
        status.statusCode = UNAUTHORIZED_RTYPE;
        return false;
    }

    const std::string userId = blossomIO.input.get("id").getString();
    const std::string projectId = blossomIO.input.get("project_id").getString();
    const std::string creatorId = context.getStringByKey("id");

    // check if user already exist within the table
    Hanami::JsonItem getResult;
    if(UsersTable::getInstance()->getUser(getResult, userId, error, false) == false)
    {
        status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
        return false;
    }

    // handle not found
    if(getResult.size() == 0)
    {
        status.errorMessage = "User with id '" + userId + "' not found";
        status.statusCode = NOT_FOUND_RTYPE;
        error.addMeesage(status.errorMessage);
        return false;
    }

    // check if project is assigned to user and remove it if found
    bool found = false;
    Hanami::JsonItem parsedProjects = getResult.get("projects");
    for(uint64_t i = 0; i < parsedProjects.size(); i++)
    {
        if(parsedProjects.get(i).get("project_id").getString() == projectId)
        {
            parsedProjects.remove(i);
            found = true;
            break;
        }
    }

    // handle error that project is not assigned to user
    if(found == false)
    {
        status.errorMessage = "Project with ID '"
                              + projectId
                              + "' is not assigned to user with id '"
                              + userId
                              + "' and so it can not be removed from the user.";
        error.addMeesage(status.errorMessage);
        status.statusCode = NOT_FOUND_RTYPE;
        return false;
    }

    // updated projects of user in database
    if(UsersTable::getInstance()->updateProjectsOfUser(userId,
                                                       parsedProjects.toString(),
                                                       error) == false)
    {
        error.addMeesage("Failed to update projects of user with id '"
                         + userId
                         + "'.");
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
