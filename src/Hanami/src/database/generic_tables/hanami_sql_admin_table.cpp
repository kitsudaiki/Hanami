/**
 * @file       hanami_sql_admin_table.cpp
 *
 * @author     Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 * @copyright  Apache License Version 2.0
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

#include "hanami_sql_admin_table.h"

#include <hanami_common/methods/string_methods.h>
#include <hanami_database/sql_database.h>
#include <uuid/uuid.h>

/**
 * @brief constructor, which add basic columns to the table
 *
 * @param db pointer to database
 */
HanamiSqlAdminTable::HanamiSqlAdminTable(Hanami::SqlDatabase* db) : SqlTable(db)
{
    DbHeaderEntry id;
    id.name = "id";
    id.maxLength = 256;
    id.isPrimary = true;
    m_tableHeader.push_back(id);

    DbHeaderEntry name;
    name.name = "name";
    name.maxLength = 256;
    m_tableHeader.push_back(name);

    DbHeaderEntry creatorId;
    creatorId.name = "creator_id";
    creatorId.maxLength = 256;
    m_tableHeader.push_back(creatorId);
}

/**
 * @brief destructor
 */
HanamiSqlAdminTable::~HanamiSqlAdminTable() {}
