/**
 * @file        validate_access.cpp
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

#include "validate_access.h"

#include <hanami_common/methods/string_methods.h>
#include <hanami_policies/policy.h>
#include <hanami_root.h>
#include <jwt-cpp/jwt.h>
// #include <jwt-cpp/traits/nlohmann-json/defaults.h>

using Hanami::HttpRequestType;

/**
 * @brief constructor
 */
ValidateAccess::ValidateAccess()
    : Blossom(
        "Checks if a JWT-access-token of a user is valid or not "
        "and optional check if the user is allowed by its roles "
        "and the policy to access a specific endpoint.",
        false)
{
    errorCodes.push_back(UNAUTHORIZED_RTYPE);

    //----------------------------------------------------------------------------------------------
    // input
    //----------------------------------------------------------------------------------------------

    registerInputField("token", SAKURA_STRING_TYPE)
        .setComment("User specific JWT-access-token.")
        .setRegex("[a-zA-Z_.\\-0-9]*");

    registerInputField("endpoint", SAKURA_STRING_TYPE)
        .setComment("Requesed endpoint within the component.")
        .setLimit(4, 256)
        .setRegex("[a-zA-Z][a-zA-Z_/0-9]*");

    registerInputField("http_type", SAKURA_INT_TYPE)
        .setComment(
            "Type of the HTTP-request as enum "
            "(DELETE = 1, GET = 2, HEAD = 3, POST = 4, PUT = 5).")
        .setLimit(1, 5);

    //----------------------------------------------------------------------------------------------
    // output
    //----------------------------------------------------------------------------------------------

    registerOutputField("id", SAKURA_STRING_TYPE).setComment("ID of the user.");

    registerOutputField("name", SAKURA_STRING_TYPE).setComment("Name of the user.");

    registerOutputField("is_admin", SAKURA_BOOL_TYPE)
        .setComment("Show if the user is an admin or not.");

    registerOutputField("project_id", SAKURA_STRING_TYPE)
        .setComment("Selected project of the user.");

    registerOutputField("role", SAKURA_STRING_TYPE)
        .setComment("Role of the user within the project.");

    registerOutputField("is_project_admin", SAKURA_BOOL_TYPE)
        .setComment("True, if the user is admin within the selected project.");

    //----------------------------------------------------------------------------------------------
    //
    //----------------------------------------------------------------------------------------------
}

/**
 * @brief runTask
 */
bool
ValidateAccess::runTask(BlossomIO& blossomIO,
                        const json&,
                        BlossomStatus& status,
                        Hanami::ErrorContainer& error)
{
    // collect information from the input
    const std::string token = blossomIO.input["token"];
    const std::string endpoint = blossomIO.input["endpoint"];

    try {
        auto decodedToken = jwt::decode(token);
        auto verifier = jwt::verify().allow_algorithm(
            jwt::algorithm::hs256{(const char*)HanamiRoot::tokenKey.data()});

        verifier.verify(decodedToken);

        // copy data of token into the output
        for (const auto& payload : decodedToken.get_payload_json()) {
            try {
                blossomIO.output = json::parse(payload.second.to_str());
            }
            catch (const json::parse_error& ex) {
                error.addMeesage("Error while parsing decoded token");
                error.addMeesage("json-parser error: " + std::string(ex.what()));
                status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
                return false;
            }
        }
    }
    catch (const std::exception& ex) {
        error.addMeesage("Failed to validate JWT-Token with error: " + std::string(ex.what()));
        status.errorMessage = "Failed to validate JWT-Token";
        status.statusCode = UNAUTHORIZED_RTYPE;
        LOG_DEBUG(status.errorMessage);
        return false;
    }

    if (blossomIO.input.contains("http_type") == false) {
        error.addMeesage("http_type is missing in token-request");
        status.statusCode = INTERNAL_SERVER_ERROR_RTYPE;
        return false;
    }

    const uint32_t httpTypeValue = blossomIO.input["http_type"];
    const HttpRequestType httpType = static_cast<HttpRequestType>(httpTypeValue);

    // check policy
    const std::string role = blossomIO.output["role"];
    if (Policy::getInstance()->checkUserAgainstPolicy(endpoint, httpType, role) == false) {
        status.errorMessage = "Access denied by policy";
        status.statusCode = UNAUTHORIZED_RTYPE;
        LOG_DEBUG(status.errorMessage);
        return false;
    }

    // remove irrelevant fields
    blossomIO.output.erase("pw_hash");
    blossomIO.output.erase("creator_id");
    blossomIO.output.erase("exp");
    blossomIO.output.erase("iat");
    blossomIO.output.erase("nbf");

    return true;
}
