/**
 * @file       hanami_sql_table.h
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

#ifndef HANAMI_DATABASE_SQL_TABLE_H
#define HANAMI_DATABASE_SQL_TABLE_H

#include <vector>
#include <string>
#include <uuid/uuid.h>
#include <common/structs.h>

#include <libKitsunemimiCommon/logger.h>

#include <libKitsunemimiSakuraDatabase/sql_table.h>

class SqlDatabase;

class HanamiSqlTable
        : public Kitsunemimi::Sakura::SqlTable
{
public:
    HanamiSqlTable(Kitsunemimi::Sakura::SqlDatabase* db);
    virtual ~HanamiSqlTable();

    bool add(Kitsunemimi::JsonItem &values,
             const UserContext &userContext,
             Kitsunemimi::ErrorContainer &error);
    bool get(Kitsunemimi::JsonItem &result,
             const UserContext &userContext,
             std::vector<RequestCondition> &conditions,
             Kitsunemimi::ErrorContainer &error,
             const bool showHiddenValues = false);
    bool update(Kitsunemimi::JsonItem &values,
                const UserContext &userContext,
                std::vector<RequestCondition> &conditions,
                Kitsunemimi::ErrorContainer &error);
    bool getAll(Kitsunemimi::TableItem &result,
                const UserContext &userContext,
                std::vector<RequestCondition> &conditions,
                Kitsunemimi::ErrorContainer &error,
                const bool showHiddenValues = false);
    bool del(std::vector<RequestCondition> &conditions,
             const UserContext &userContext,
             Kitsunemimi::ErrorContainer &error);

private:
    void fillCondition(std::vector<RequestCondition> &conditions,
                       const UserContext &userContext);
};

#endif // HANAMI_DATABASE_SQL_TABLE_H
