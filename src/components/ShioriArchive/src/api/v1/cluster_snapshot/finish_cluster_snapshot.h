/**
 * @file        get_cluster_snapshot.h
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

#ifndef FINISH_CLUSTER_SNAPSHOT_H
#define FINISH_CLUSTER_SNAPSHOT_H

#include <libKitsunemimiHanamiNetwork/blossom.h>

class FinalizeClusterSnapshot
        : public Kitsunemimi::Hanami::Blossom
{
public:
    FinalizeClusterSnapshot();

protected:
    bool runTask(Kitsunemimi::Hanami::BlossomIO &blossomIO,
                 const Kitsunemimi::DataMap &,
                 Kitsunemimi::Hanami::BlossomStatus &status,
                 Kitsunemimi::ErrorContainer &error);
};

#endif // FINISH_CLUSTER_SNAPSHOT_H