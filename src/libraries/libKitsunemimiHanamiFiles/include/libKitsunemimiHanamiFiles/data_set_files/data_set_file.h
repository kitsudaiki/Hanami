/**
 * @file        data_set_file.cpp
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

#ifndef HANAMI_DATASETFILE_H
#define HANAMI_DATASETFILE_H

#include <string>
#include <vector>
#include <stdint.h>
#include <cstring>

#include <libKitsunemimiCommon/logger.h>
#include <libKitsunemimiCommon/buffer/data_buffer.h>

namespace Kitsunemimi {
struct DataBuffer;
class BinaryFile;
}

class DataSetFile
{
public:
    enum DataSetType
    {
        UNDEFINED_TYPE = 0,
        IMAGE_TYPE = 1,
        TABLE_TYPE = 2
    };

    struct DataSetHeader
    {
        uint8_t type = UNDEFINED_TYPE;
        char name[256];
    };

    DataSetFile(const std::string &filePath);
    DataSetFile(Kitsunemimi::BinaryFile* file);
    virtual ~DataSetFile();

    bool initNewFile(Kitsunemimi::ErrorContainer &error);
    bool readFromFile(Kitsunemimi::ErrorContainer &error);

    bool addBlock(const uint64_t pos,
                  const float* data,
                  const u_int64_t numberOfValues,
                  Kitsunemimi::ErrorContainer &error);
    virtual bool getPayload(Kitsunemimi::DataBuffer &result,
                            Kitsunemimi::ErrorContainer &error,
                            const std::string &columnName = "") = 0;
    virtual bool updateHeader(Kitsunemimi::ErrorContainer &error) = 0;

    DataSetType type = UNDEFINED_TYPE;
    std::string name = "";

protected:
    virtual void initHeader() = 0;
    virtual void readHeader(const uint8_t* u8buffer) = 0;

    Kitsunemimi::BinaryFile* m_targetFile = nullptr;

    uint64_t m_headerSize = 0;
    uint64_t m_totalFileSize = 0;
};

DataSetFile* readDataSetFile(const std::string &filePath,
                             Kitsunemimi::ErrorContainer &error);

#endif // HANAMI_DATASETFILE_H
