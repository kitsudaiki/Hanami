/**
 * @file        rst_docu_generation.cpp
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

#include <rst_docu_generation.h>

#include <libKitsunemimiHanamiNetwork/hanami_messaging.h>
#include <libKitsunemimiHanamiCommon/component_support.h>

#include <libKitsunemimiHanamiNetwork/blossom.h>

#include <libKitsunemimiCommon/methods/string_methods.h>
#include <libKitsunemimiCrypto/common.h>

using namespace Kitsunemimi;

/**
 * @brief generate documenation for all fields
 *
 * @param docu reference to the complete document
 * @param defMap map with all field to ducument
 * @param isRequest true to say that the actual field is a request-field
 */
void
addFieldDocu_rst(std::string &docu,
                 const std::map<std::string, Hanami::FieldDef>* defMap,
                 const bool isRequest)
{
    for(auto const& [id, fieldDef] : *defMap)
    {
        const std::string field = id;
        const Hanami::FieldType fieldType = fieldDef.fieldType;
        const std::string comment = fieldDef.comment;
        const bool isRequired = fieldDef.isRequired;
        const DataItem* defaultVal = fieldDef.defaultVal;
        const DataItem* matchVal = fieldDef.match;
        const std::string regexVal = fieldDef.regex;
        const long lowerBorder = fieldDef.lowerBorder;
        const long upperBorder = fieldDef.upperBorder;

        docu.append("\n");
        docu.append("``" + field + "``\n");

        // comment
        if(comment != "")
        {
            docu.append("    **Description:**\n");
            docu.append("        ``" + comment + "``\n");
        }

        // type
        docu.append("    **Type:**\n");
        if(fieldType == Hanami::SAKURA_MAP_TYPE) {
            docu.append("        ``Map``\n");
        } else if(fieldType == Hanami::SAKURA_ARRAY_TYPE) {
            docu.append("        ``Array``\n");
        } else if(fieldType == Hanami::SAKURA_BOOL_TYPE) {
            docu.append("        ``Bool``\n");
        } else if(fieldType == Hanami::SAKURA_INT_TYPE) {
            docu.append("        ``Int``\n");
        } else if(fieldType == Hanami::SAKURA_FLOAT_TYPE) {
            docu.append("        ``Float``\n");
        } else if(fieldType == Hanami::SAKURA_STRING_TYPE) {
            docu.append("        ``String``\n");
        }

        if(isRequest)
        {
            // required
            docu.append("    **Required:**\n");
            if(isRequired) {
                docu.append("        ``True``\n");
            } else {
                docu.append("        ``False``\n");
            }

            // default
            if(defaultVal != nullptr
                    && isRequired == false)
            {
                docu.append("    **Default:**\n");
                docu.append("        ``" + defaultVal->toString() + "``\n");
            }

            // match
            if(matchVal != nullptr)
            {
                docu.append("    **Does have the value:**\n");
                docu.append("        ``" + matchVal->toString() + "``\n");
            }

            // match
            if(regexVal != "")
            {
                docu.append("    **Must match the regex:**\n");
                docu.append("        ``" + regexVal + "``\n");
            }

            // border
            if(lowerBorder != 0
                    || upperBorder != 0)
            {
                if(fieldType == Hanami::SAKURA_INT_TYPE)
                {
                    docu.append("    **Lower border of value:**\n");
                    docu.append("        ``" + std::to_string(lowerBorder) + "``\n");
                    docu.append("    **Upper border of value:**\n");
                    docu.append("        ``" + std::to_string(upperBorder) + "``\n");
                }
                if(fieldType == Hanami::SAKURA_STRING_TYPE)
                {
                    docu.append("    **Minimum string-length:**\n");
                    docu.append("        ``" + std::to_string(lowerBorder) + "``\n");
                    docu.append("    **Maximum string-length:**\n");
                    docu.append("        ``" + std::to_string(upperBorder) + "``\n");
                }
            }
        }
    }
}

/**
 * @brief generate documentation of a blossom-item
 *
 * @param docu reference to the complete document
 * @param langInterface pinter to the sakura-language-interface
 * @param groupName group of the blossom
 * @param itemName name of the blossom in group
 */
void
createBlossomDocu_rst(std::string &docu,
                      Hanami::HanamiMessaging* langInterface,
                      const std::string &groupName,
                      const std::string &itemName)
{
    Hanami::Blossom* blossom = langInterface->getBlossom(groupName, itemName);

    if(blossom == nullptr) {
        // TODO: handle error
        return;
    }

    // add comment/describtion
    docu.append(blossom->comment + "\n");

    // add input-fields
    docu.append("\n");
    docu.append("Request-Parameter\n");
    docu.append("~~~~~~~~~~~~~~~~~\n");
    addFieldDocu_rst(docu, blossom->getInputValidationMap(), true);

    // add output-fields
    docu.append("\n");
    docu.append("Response-Parameter\n");
    docu.append("~~~~~~~~~~~~~~~~~~\n");
    addFieldDocu_rst(docu, blossom->getOutputValidationMap(), false);
}

/**
 * @brief generate documentation for the endpoints
 *
 * @param docu reference to the complete document
 */
void
generateEndpointDocu_rst(std::string &docu)
{
    Hanami::HanamiMessaging* langInterface =
            Hanami::HanamiMessaging::getInstance();
    docu.append("\n");

    for(auto const& [endpoint, httpDef] : langInterface->endpointRules)
    {
        // add endpoint
        docu.append(endpoint);
        docu.append("\n");
        docu.append(std::string(endpoint.size(), '-'));
        docu.append("\n");

        for(auto const& [type, endpointEntry] : httpDef)
        {
            docu.append("\n");

            // add http-type
            if(type == Hanami::GET_TYPE) {
                docu.append("GET\n^^^\n\n");
            } else if(type == Hanami::POST_TYPE) {
                docu.append("POST\n^^^^\n\n");
            } else if(type == Hanami::DELETE_TYPE) {
                docu.append("DELETE\n^^^^^^\n\n");
            } else if(type == Hanami::PUT_TYPE) {
                docu.append("PUT\n^^^\n\n");
            }

            createBlossomDocu_rst(docu,
                                  langInterface,
                                  endpointEntry.group,
                                  endpointEntry.name);
        }
    }
}

/**
 * @brief createRstDocumentation
 * @param docu
 * @param localComponent
 */
void
createRstDocumentation(std::string &docu,
                       const std::string &localComponent)
{
    // create header
    docu.append(localComponent);
    toUpperCase(docu);
    docu.append("\n");
    docu.append(localComponent.size(), '=');
    docu.append("\n");

    generateEndpointDocu_rst(docu);
}
