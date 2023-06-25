/**
 * @file        cluster_table.h
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

#ifndef TEMPLATETABLE_H
#define TEMPLATETABLE_H

#include <libKitsunemimiCommon/logger.h>
#include <database/generic_tables/hanami_sql_table.h>

namespace Kitsunemimi {
class JsonItem;
}


class TemplateTable
        : public HanamiSqlTable
{
public:
    static TemplateTable* getInstance()
    {
        if(instance == nullptr) {
            instance = new TemplateTable();
        }
        return instance;
    }

    ~TemplateTable();

    bool addTemplate(Kitsunemimi::JsonItem &clusterData,
                     const UserContext &userContext,
                     Kitsunemimi::ErrorContainer &error);
    bool getTemplate(Kitsunemimi::JsonItem &result,
                     const std::string &templateUuid,
                     const UserContext &userContext,
                     Kitsunemimi::ErrorContainer &error,
                     const bool showHiddenValues = false);
    bool getTemplateByName(Kitsunemimi::JsonItem &result,
                           const std::string &templateName,
                           const UserContext &userContext,
                           Kitsunemimi::ErrorContainer &error,
                           const bool showHiddenValues = false);
    bool getAllTemplate(Kitsunemimi::TableItem &result,
                        const UserContext &userContext,
                        Kitsunemimi::ErrorContainer &error);
    bool deleteTemplate(const std::string &templateUuid,
                        const UserContext &userContext,
                        Kitsunemimi::ErrorContainer &error);

private:
    TemplateTable();
    static TemplateTable* instance;
};

#endif // TEMPLATETABLE_H
