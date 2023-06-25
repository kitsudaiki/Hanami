/**
 * @file        create_cluster_template.h
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

#include "delete_template.h"

#include <libKitsunemimiJson/json_item.h>

#include <hanami_root.h>

DeleteTemplate::DeleteTemplate()
    : Blossom("Delete a template from the database.")
{
    //----------------------------------------------------------------------------------------------
    // input
    //----------------------------------------------------------------------------------------------

    registerInputField("uuid",
                       SAKURA_STRING_TYPE,
                       true,
                       "UUID of the template.");
    assert(addFieldRegex("uuid", UUID_REGEX));

    //----------------------------------------------------------------------------------------------
    //
    //----------------------------------------------------------------------------------------------
}

/**
 * @brief runTask
 */
bool
DeleteTemplate::runTask(BlossomIO &blossomIO,
                        const Kitsunemimi::DataMap &context,
                        BlossomStatus &status,
                        Kitsunemimi::ErrorContainer &error)
{
    // get information from request
    const std::string templateUuid = blossomIO.input.get("uuid").getString();
    const UserContext userContext(context);

    // check if user exist within the table
    Kitsunemimi::JsonItem getResult;
    if(TemplateTable::getInstance()->getTemplate(getResult,
                                              templateUuid,
                                              userContext,
                                              error) == false)
    {
        status.errorMessage = "Template with UUID '" + templateUuid + "' not found.";
        status.statusCode = NOT_FOUND_RTYPE;
        error.addMeesage(status.errorMessage);
        return false;
    }

    // remove data from table
    if(TemplateTable::getInstance()->deleteTemplate(templateUuid, userContext, error) == false)
    {
        status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
        error.addMeesage("Failed to delete template with UUID '"
                         + templateUuid
                         + "' from database");
        return false;
    }

    return true;
}
