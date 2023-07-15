/**
 * @file        md_docu_generation.cpp
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

#include <docu_generation/openapi_docu_generation.h>

#include <hanami_root.h>
#include <api/endpoint_processing/blossom.h>

#include <libKitsunemimiCommon/methods/string_methods.h>
#include <libKitsunemimiCrypto/common.h>

using namespace Kitsunemimi;

/**
 * @brief addTokenRequirement
 * @param parameters
 */
void
addTokenRequirement(JsonItem &parameters)
{
    JsonItem param;
    param.insert("in", "header");
    param.insert("description", "JWT-Token for authentication");
    param.insert("name", "X-Auth-Token");
    param.insert("required", true);

    JsonItem schema;
    schema.insert("type","string");

    param.insert("schema", schema);
    parameters.append(param);
}

/**
 * @brief createQueryParams_openapi
 * @param schema
 * @param defMap
 * @param isRequest
 */
void
createQueryParams_openapi(JsonItem &parameters,
                          const std::map<std::string, FieldDef>* defMap)
{
    for(const auto& [field, fieldDef] : *defMap)
    {
        const FieldType fieldType = fieldDef.fieldType;
        const std::string comment = fieldDef.comment;
        const bool isRequired = fieldDef.isRequired;
        const DataItem* defaultVal = fieldDef.defaultVal;
        const DataItem* matchVal = fieldDef.match;
        std::string regexVal = fieldDef.regex;
        const long lowerBorder = fieldDef.lowerBorder;
        const long upperBorder = fieldDef.upperBorder;

        JsonItem param;
        param.insert("in", "query");
        param.insert("name", field);

        // required
        if(isRequired) {
            param.insert("required", isRequired);
        }

        // comment
        if(comment != "") {
            param.insert("description", comment);
        }

        JsonItem schema;

        // type
        if(fieldType == SAKURA_MAP_TYPE) {
            schema.insert("type","object");
        } else if(fieldType == SAKURA_ARRAY_TYPE) {
            schema.insert("type","array");
        } else if(fieldType == SAKURA_BOOL_TYPE) {
            schema.insert("type","boolean");
        } else if(fieldType == SAKURA_INT_TYPE) {
            schema.insert("type","integer");
        } else if(fieldType == SAKURA_FLOAT_TYPE) {
            schema.insert("type","number");
        } else if(fieldType == SAKURA_STRING_TYPE) {
            schema.insert("type","string");
        }

        // default
        if(defaultVal != nullptr
                && isRequired == false)
        {
            schema.insert("default", defaultVal->toString());
        }

        // match
        if(regexVal != "")
        {
            Kitsunemimi::replaceSubstring(regexVal, "\\", "\\\\");
            schema.insert("pattern", regexVal);
        }

        // border
        if(lowerBorder != 0
                || upperBorder != 0)
        {
            if(fieldType == SAKURA_INT_TYPE)
            {
                schema.insert("minimum", std::to_string(lowerBorder));
                schema.insert("maximum", std::to_string(upperBorder));
            }
            if(fieldType == SAKURA_STRING_TYPE)
            {
                schema.insert("minLength", std::to_string(lowerBorder));
                schema.insert("maxLength", std::to_string(upperBorder));
            }
        }

        // match
        if(matchVal != nullptr)
        {
            JsonItem match;
            std::string content = matchVal->toString();
            Kitsunemimi::replaceSubstring(content, "\"", "\\\"");
            match.append(content);
            schema.insert("enum", match);
        }

        param.insert("schema", schema);
        parameters.append(param);
    }
}

/**
 * @brief generate documenation for all fields
 *
 * @param docu reference to the complete document
 * @param defMap map with all field to ducument
 * @param isRequest true to say that the actual field is a request-field
 */
void
createBodyParams_openapi(JsonItem &schema,
                         const std::map<std::string, FieldDef>* defMap,
                         const bool isRequest)
{
    std::vector<std::string> requiredFields;

    JsonItem properties;
    for(const auto& [id, fieldDef] : *defMap)
    {
        JsonItem temp;

        const std::string field = id;
        const FieldType fieldType = fieldDef.fieldType;
        const std::string comment = fieldDef.comment;
        const bool isRequired = fieldDef.isRequired;
        const DataItem* defaultVal = fieldDef.defaultVal;
        const DataItem* matchVal = fieldDef.match;
        std::string regexVal = fieldDef.regex;
        const long lowerBorder = fieldDef.lowerBorder;
        const long upperBorder = fieldDef.upperBorder;

        // type
        if(fieldType == SAKURA_MAP_TYPE) {
            temp.insert("type","object");
        } else if(fieldType == SAKURA_ARRAY_TYPE) {
            temp.insert("type","array");
            JsonItem array;
            array.insert("type", "string");

            // match
            if(matchVal != nullptr)
            {
                JsonItem match;
                Kitsunemimi::ErrorContainer error;
                match.parse(matchVal->toString(), error);
                array.insert("enum", match);
            }

            temp.insert("items", array);
        } else if(fieldType == SAKURA_BOOL_TYPE) {
            temp.insert("type","boolean");
        } else if(fieldType == SAKURA_INT_TYPE) {
            temp.insert("type","integer");
        } else if(fieldType == SAKURA_FLOAT_TYPE) {
            temp.insert("type","number");
        } else if(fieldType == SAKURA_STRING_TYPE) {
            temp.insert("type","string");
        }

        // comment
        if(comment != "") {
            temp.insert("description", comment);
        }

        if(isRequest)
        {
            // required
            if(isRequired) {
                requiredFields.push_back(field);
            }

            // default
            if(defaultVal != nullptr
                    && isRequired == false)
            {
                temp.insert("default", defaultVal->toString());
            }

            // match
            if(regexVal != "")
            {
                Kitsunemimi::replaceSubstring(regexVal, "\\", "\\\\");
                temp.insert("pattern", regexVal);
            }

            // border
            if(lowerBorder != 0
                    || upperBorder != 0)
            {
                if(fieldType == SAKURA_INT_TYPE)
                {
                    temp.insert("minimum", std::to_string(lowerBorder));
                    temp.insert("maximum", std::to_string(upperBorder));
                }
                if(fieldType == SAKURA_STRING_TYPE)
                {
                    temp.insert("minLength", std::to_string(lowerBorder));
                    temp.insert("maxLength", std::to_string(upperBorder));
                }
            }

            // match
            if(matchVal != nullptr)
            {
                JsonItem match;
                std::string content = matchVal->toString();
                Kitsunemimi::replaceSubstring(content, "\"", "\\\"");
                match.append(content);
                temp.insert("enum", match);
            }

        }

        properties.insert(field, temp);
    }

    schema.insert("properties", properties);

    if(isRequest)
    {
        JsonItem required;
        for(const std::string& field : requiredFields) {
            required.append(field);
        }
        schema.insert("required", required);
    }
}

/**
 * @brief generate documentation for the endpoints
 *
 * @param docu reference to the complete document
 */
void
generateEndpointDocu_openapi(JsonItem &result)
{
    for(const auto& [endpointPath, httpDef] : HanamiRoot::root->endpointRules)
    {
        // add endpoint
        JsonItem endpoint;

        for(const auto& [type, endpointEntry] : httpDef)
        {
            JsonItem endpointType;

            Blossom* blossom = HanamiRoot::root->getBlossom(endpointEntry.group,
                                                            endpointEntry.name);
            if(blossom == nullptr) {
                // TODO: handle error
                return;
            }

            // add comment/describtion
            endpointType.insert("summary", blossom->comment);

            JsonItem tags;
            tags.append(endpointEntry.group);
            endpointType.insert("tags", tags);

            JsonItem parameters;

            if(blossom->requiresAuthToken) {
                addTokenRequirement(parameters);
            }

            if(type == POST_TYPE
                    || type == PUT_TYPE)
            {
                JsonItem requestBody;
                requestBody.insert("required", true);
                JsonItem content;
                JsonItem jsonApplication;
                JsonItem schema;
                schema.insert("type", "object");
                createBodyParams_openapi(schema, blossom->getInputValidationMap(), true);
                jsonApplication.insert("schema", schema);
                content.insert("application/json", jsonApplication);
                requestBody.insert("content", content);
                endpointType.insert("requestBody", requestBody);
            }

            if(type == GET_TYPE
                    || type == DELETE_TYPE)
            {
                createQueryParams_openapi(parameters, blossom->getInputValidationMap());
            }
            endpointType.insert("parameters", parameters);

            {
                JsonItem responses;
                JsonItem resp200;
                resp200.insert("description", "Successful response");
                JsonItem content;
                JsonItem jsonApplication;
                JsonItem schema;
                schema.insert("type", "object");
                createBodyParams_openapi(schema, blossom->getOutputValidationMap(), false);
                jsonApplication.insert("schema", schema);
                content.insert("application/json", jsonApplication);
                resp200.insert("content", content);
                responses.insert("200", resp200);
                endpointType.insert("responses", responses);
            }

            // add http-type
            if(type == GET_TYPE) {
                endpoint.insert("get", endpointType);
            } else if(type == POST_TYPE) {
                endpoint.insert("post", endpointType);
            } else if(type == DELETE_TYPE) {
                endpoint.insert("delete", endpointType);
            } else if(type == PUT_TYPE) {
                endpoint.insert("put", endpointType);
            }
        }

        result.insert(endpointPath, endpoint);
    }
}

/**
 * @brief createMdDocumentation
 * @param docu
 */
void
createOpenApiDocumentation(std::string &docu)
{
    JsonItem result;
    result.insert("openapi", "3.0.0");

    JsonItem info;
    info.insert("title", "API documentation");
    info.insert("version", "unreleased");
    result.insert("info", info);

    JsonItem contact;
    info.insert("name", "Tobias Anker");
    info.insert("email", "tobias.anker@kitsunemimi.moe");
    result.insert("contact", contact);

    JsonItem license;
    license.insert("name", "Apache 2.0");
    license.insert("url", "https://www.apache.org/licenses/LICENSE-2.0.html");
    result.insert("license", license);

    JsonItem paths;
    generateEndpointDocu_openapi(paths);
    result.insert("paths", paths);

    docu = result.toString();
}
