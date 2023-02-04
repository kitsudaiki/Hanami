/**
 * @file        cluster_handler.h
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

#ifndef KYOUKOMIND_CLUSTERHANDLER_H
#define KYOUKOMIND_CLUSTERHANDLER_H

#include <common.h>

class Cluster;

class ClusterHandler
{
public:
    ClusterHandler();

    bool addCluster(const std::string uuid, Cluster* newCluster);
    bool removeCluster(const std::string uuid);
    Cluster* getCluster(const std::string uuid);

private:
    std::map<std::string, Cluster*> m_allCluster;
};

#endif // KYOUKOMIND_CLUSTERHANDLER_H
