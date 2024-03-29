﻿/**
 * @file        finalize_csv_dataset.cpp
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

#include "finalize_csv_dataset.h"

#include <core/temp_file_handler.h>
#include <database/dataset_table.h>
#include <hanami_common/files/binary_file.h>
#include <hanami_common/methods/file_methods.h>
#include <hanami_common/methods/string_methods.h>
#include <hanami_common/methods/vector_methods.h>
#include <hanami_crypto/common.h>
#include <hanami_files/dataset_files/dataset_file.h>
#include <hanami_files/dataset_files/table_dataset_file.h>
#include <hanami_root.h>

FinalizeCsvDataSet::FinalizeCsvDataSet()
    : Blossom(
        "Finalize uploaded dataset by checking completeness of the "
        "uploaded and convert into generic format.")
{
    errorCodes.push_back(NOT_FOUND_RTYPE);

    //----------------------------------------------------------------------------------------------
    // input
    //----------------------------------------------------------------------------------------------

    registerInputField("uuid", SAKURA_STRING_TYPE)
        .setComment("UUID of the new dataset.")
        .setRegex(UUID_REGEX);

    registerInputField("uuid_input_file", SAKURA_STRING_TYPE)
        .setComment("UUID to identify the file for date upload of input-data.")
        .setRegex(UUID_REGEX);

    //----------------------------------------------------------------------------------------------
    // output
    //----------------------------------------------------------------------------------------------

    registerOutputField("uuid", SAKURA_STRING_TYPE).setComment("UUID of the new dataset.");

    //----------------------------------------------------------------------------------------------
    //
    //----------------------------------------------------------------------------------------------
}

/**
 * @brief runTask
 */
bool
FinalizeCsvDataSet::runTask(BlossomIO& blossomIO,
                            const json& context,
                            BlossomStatus& status,
                            Hanami::ErrorContainer& error)
{
    const std::string uuid = blossomIO.input["uuid"];
    const std::string inputUuid = blossomIO.input["uuid_input_file"];
    const UserContext userContext(context);

    // get location from database
    json result;
    if (DataSetTable::getInstance()->getDataSet(result, uuid, userContext, error, true) == false) {
        status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
        return false;
    }

    // handle not found
    if (result.size() == 0) {
        status.errorMessage = "Data-set with uuid '" + uuid + "' not found";
        status.statusCode = NOT_FOUND_RTYPE;
        LOG_DEBUG(status.errorMessage);
        return false;
    }

    // read input-data from temp-file
    Hanami::DataBuffer inputBuffer;
    if (TempFileHandler::getInstance()->getData(inputBuffer, inputUuid) == false) {
        status.errorMessage = "Input-data with uuid '" + inputUuid + "' not found.";
        status.statusCode = NOT_FOUND_RTYPE;
        LOG_DEBUG(status.errorMessage);
        return false;
    }

    // write data to file
    if (convertCsvData(result["location"], result["name"], inputBuffer) == false) {
        status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
        error.addMessage("Failed to convert csv-data");
        return false;
    }

    // delete temp-files
    // TODO: error-handling
    TempFileHandler::getInstance()->removeData(inputUuid, userContext, error);

    // create output
    blossomIO.output["uuid"] = uuid;

    return true;
}

void
FinalizeCsvDataSet::convertField(float* segmentPos, const std::string& cell, const float lastVal)
{
    // true
    if (cell == "Null" || cell == "null" || cell == "NULL") {
        *segmentPos = lastVal;
    }
    // true
    else if (cell == "True" || cell == "true" || cell == "TRUE") {
        *segmentPos = 1.0f;
    }
    // false
    else if (cell == "False" || cell == "false" || cell == "FALSE") {
        *segmentPos = 0.0f;
    }
    // int/long
    else if (regex_match(cell, std::regex(INT_VALUE_REGEX))) {
        *segmentPos = static_cast<float>(std::stoi(cell.c_str()));
    }
    // float/double
    else if (regex_match(cell, std::regex(FLOAT_VALUE_REGEX))) {
        *segmentPos = std::strtof(cell.c_str(), NULL);
    }
    else {
        // ignore other lines
        *segmentPos = 0.0f;
    }
}

/**
 * @brief convert csv-data into generic format
 *
 * @param filePath path to the resulting file
 * @param name dataset name
 * @param inputBuffer buffer with input-data
 *
 * @return true, if successfull, else false
 */
bool
FinalizeCsvDataSet::convertCsvData(const std::string& filePath,
                                   const std::string& name,
                                   const Hanami::DataBuffer& inputBuffer)
{
    Hanami::ErrorContainer error;

    TableDataSetFile file(filePath);
    file.type = DataSetFile::TABLE_TYPE;
    file.name = name;

    // prepare content-processing
    const std::string stringContent(static_cast<char*>(inputBuffer.data),
                                    inputBuffer.usedBufferSize);

    // buffer for values to reduce write-access to file
    const uint32_t segmentSize = 10000000;
    std::vector<float> cluster(segmentSize, 0.0f);
    std::vector<float> lastLine;
    uint64_t segmentPos = 0;
    uint64_t segmentCounter = 0;

    // split content
    std::vector<std::string> lines;
    Hanami::splitStringByDelimiter(lines, stringContent, '\n');

    bool isHeader = true;

    for (uint64_t lineNum = 0; lineNum < lines.size(); lineNum++) {
        const std::string* line = &lines[lineNum];

        // check if the line is relevant to ignore broken lines
        const uint64_t numberOfColumns = std::count(line->begin(), line->end(), ',') + 1;
        if (numberOfColumns == 1) {
            continue;
        }

        // split line
        std::vector<std::string> lineContent;
        Hanami::splitStringByDelimiter(lineContent, *line, ',');

        if (isHeader) {
            file.tableHeader.numberOfColumns = numberOfColumns;
            file.tableHeader.numberOfLines = lines.size();

            for (const std::string& col : lineContent) {
                // create and add header-entry
                TableDataSetFile::TableHeaderEntry entry;
                entry.setName(col);
                file.tableColumns.push_back(entry);
            }
            isHeader = false;

            if (file.initNewFile(error) == false) {
                return false;
            }

            // this was the max value. While iterating over all lines, this value will be new
            // calculated with the correct value
            file.tableHeader.numberOfLines = 0;
            lastLine = std::vector<float>(numberOfColumns, 0.0f);
        }
        else {
            for (uint64_t colNum = 0; colNum < lineContent.size(); colNum++) {
                const std::string* cell = &lineContent[colNum];
                if (lastLine.size() > 0) {
                    const float lastVal = lastLine[colNum];
                    convertField(&cluster[segmentPos], *cell, lastVal);
                }
                else {
                    convertField(&cluster[segmentPos], *cell, 0.0f);
                }

                lastLine[colNum] = cluster[segmentPos];

                // write next cluster to file
                segmentPos++;
                if (segmentPos == segmentSize) {
                    if (file.addBlock(segmentCounter, &cluster[0], segmentSize, error) == false) {
                        return false;
                    }
                    segmentPos = 0;
                    segmentCounter++;
                }
            }

            file.tableHeader.numberOfLines++;
        }
    }

    // write last incomplete cluster to file
    if (segmentPos != 0) {
        if (file.addBlock(segmentCounter, &cluster[0], segmentPos, error) == false) {
            return false;
        }
    }

    // update header in file for the final number of lines for the case,
    // that there were invalid lines
    if (file.updateHeader(error) == false) {
        return false;
    }

    // debug-output
    // file.print();

    return true;
}
