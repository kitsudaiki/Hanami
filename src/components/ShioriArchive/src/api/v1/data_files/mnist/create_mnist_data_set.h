/**
 * @file        create_mnist_data_set.h
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

#ifndef SHIORIARCHIVE_MNIST_CREATE_DATA_SET_H
#define SHIORIARCHIVE_MNIST_CREATE_DATA_SET_H

#include <libKitsunemimiHanamiNetwork/blossom.h>

class CreateMnistDataSet
        : public Kitsunemimi::Hanami::Blossom
{
public:
    CreateMnistDataSet();

protected:
    bool runTask(Kitsunemimi::Hanami::BlossomIO &blossomIO,
                 const Kitsunemimi::DataMap &context,
                 Kitsunemimi::Hanami::BlossomStatus &status,
                 Kitsunemimi::ErrorContainer &error);
};

#endif // SHIORIARCHIVE_MNIST_CREATE_DATA_SET_H