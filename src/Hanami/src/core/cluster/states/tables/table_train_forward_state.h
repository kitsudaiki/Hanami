/**
 * @file        table_train_forward_state.h
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

#ifndef TABLETRAINFORWARD_STATE_H
#define TABLETRAINFORWARD_STATE_H

#include <hanami_common/threading/event.h>

class Cluster;

class TableTrainForward_State : public Hanami::Event
{
   public:
    TableTrainForward_State(Cluster* cluster);
    ~TableTrainForward_State();

    bool processEvent();

   private:
    Cluster* m_cluster = nullptr;
};

#endif  // TABLETRAINFORWARD_STATE_H
