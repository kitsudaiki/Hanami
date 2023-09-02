/**
 * @file        list_request_result.cpp
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

#include "list_request_result.h"

#include <hanami_root.h>
#include <database/request_result_table.h>

ListRequestResult::ListRequestResult()
    : Blossom("List all visilbe request-results.")
{
    //----------------------------------------------------------------------------------------------
    // output
    //----------------------------------------------------------------------------------------------

    registerOutputField("header", SAKURA_ARRAY_TYPE)
            .setComment("Array with the namings all columns of the table.")
            .setMatch(new Hanami::DataValue("[\"uuid\","
                                                 "\"project_id\","
                                                 "\"owner_id\","
                                                 "\"visibility\","
                                                 "\"name\"]"));

    registerOutputField("body", SAKURA_ARRAY_TYPE)
            .setComment("Array with all rows of the table, which array arrays too.");

    //----------------------------------------------------------------------------------------------
    //
    //----------------------------------------------------------------------------------------------
}

/**
 * @brief runTask
 */
bool
ListRequestResult::runTask(BlossomIO &blossomIO,
                           const Hanami::DataMap &context,
                           BlossomStatus &status,
                           Hanami::ErrorContainer &error)
{
    const UserContext userContext(context);

    // get data from table
    Hanami::TableItem table;
    if(RequestResultTable::getInstance()->getAllRequestResult(table, userContext, error) == false)
    {
        status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
        return false;
    }

    // create output
    Hanami::DataArray* headerInfo = table.getInnerHeader();
    blossomIO.output.insert("header", headerInfo);
    blossomIO.output.insert("body", table.getBody());
    delete headerInfo;

    return true;
}
