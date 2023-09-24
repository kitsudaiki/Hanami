/**
 * @file        http_processing.cpp
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

#include "http_processing.h"

#include <hanami_root.h>
#include <database/audit_log_table.h>
#include <api/endpoint_processing/http_processing/file_send.h>
#include <api/endpoint_processing/http_processing/response_builds.h>
#include <api/endpoint_processing/http_processing/string_functions.h>
#include <api/endpoint_processing/http_server.h>

#include <hanami_common/logger.h>

#include <jwt-cpp/jwt.h>
// #include <jwt-cpp/traits/nlohmann-json/defaults.h>

/**
 * @brief process request and build response
 */
bool
processRequest(http::request<http::string_body> &httpRequest,
               http::response<http::dynamic_body> &httpResponse,
               Hanami::ErrorContainer &error)
{
    // build default-header for response
    httpResponse.version(httpRequest.version());
    httpResponse.keep_alive(false);
    httpResponse.set(http::field::server, "ToriiGateway");
    httpResponse.result(http::status::ok);
    httpResponse.set(http::field::content_type, "text/plain");

    // collect and prepare relevant data
    const http::verb messageType = httpRequest.method();
    std::string path = httpRequest.target().to_string();
    std::string payload = "{}";

    // Request path must be absolute and not contain "..".
    if(checkPath(path) == false)
    {
        error.addMeesage("Path '" + path + "' is not valid");
        httpResponse.result(http::status::bad_request);
        return false;
    }

    // check if http-type is supported
    if(messageType != http::verb::get
            && messageType != http::verb::post
            && messageType != http::verb::put
            && messageType != http::verb::delete_)
    {
        httpResponse.result(http::status::bad_request);
        error.addMeesage("Invalid request-method '"
                         + std::string(httpRequest.method_string())
                         + "'");
        beast::ostream(httpResponse.body()) << error.toString();
        return false;
    }

    // check for dashboard-client-request
    if(messageType == http::verb::get
            && path.compare(0, 8, "/control") != 0)
    {
        if(processClientRequest(httpResponse, path, error) == false)
        {
            error.addMeesage("Failed to send dashboard-files");
            return false;
        }
        return true;
    }

    // get payload of message
    if(messageType == http::verb::post
            || messageType == http::verb::put)
    {
        payload = httpRequest.body().data();
    }

    // get token from request-header
    std::string token = "";
    if(httpRequest.count("X-Auth-Token") > 0) {
        token = httpRequest.at("X-Auth-Token").to_string();
    }

    // handle control-messages
    if(cutPath(path, "/control/"))
    {
        HttpRequestType hType = static_cast<HttpRequestType>(messageType);
        if(processControlRequest(httpResponse, path, token, payload, hType, error) == false)
        {
            error.addMeesage("Failed to process control-request");
            return false;
        }
        return true;
    }

    // handle default, if nothing was found
    error.addMeesage("no matching endpoint found for path '" + path + "'");
    genericError_ResponseBuild(httpResponse,
                               HttpResponseTypes::NOT_FOUND_RTYPE,
                               error.toString());

    return false;
}

/**
 * @brief request token from misaki
 *
 * @param hanamiRequest hanami-request for the token-request
 * @param error reference for error-output
 *
 * @return true, if successful, else false
 */
bool
requestToken(http::response<http::dynamic_body> &httpResponse,
             const RequestMessage &hanamiRequest,
             Hanami::ErrorContainer &error)
{
    json inputValues;
    try {
        inputValues = json::parse(hanamiRequest.inputValues);
    } catch(const json::parse_error& ex) {
        error.addMeesage("json-parser error: " + std::string(ex.what()));
        return false;
    }

    json result;
    BlossomStatus status;
    inputValues.erase("token");
    if(HanamiRoot::root->triggerBlossom(result,
                                        "create",
                                        "Token",
                                        json::object(),
                                        inputValues,
                                        status,
                                        error) == false)
    {
        return genericError_ResponseBuild(httpResponse,
                                          static_cast<HttpResponseTypes>(status.statusCode),
                                          status.errorMessage);
    }

    return success_ResponseBuild(httpResponse, result.dump());
}

/**
 * @brief send request to misaki to check permissions
 *
 * @param tokenData
 * @param token token to validate
 * @param hanamiRequest hanami-request to the requested endpoint
 * @param responseMsg reference for the response
 * @param error reference for error-output
 *
 * @return true, if successful, else false
 */
bool
checkPermission(json &tokenData,
                const std::string &token,
                const RequestMessage &hanamiRequest,
                ResponseMessage &responseMsg,
                Hanami::ErrorContainer &error)
{
    RequestMessage requestMsg;

    // collect information from the input
    const std::string endpoint = hanamiRequest.id;

    // handle empty token
    if(token.empty())
    {
        error.addMeesage("Failed to validate JWT-Token, because token is missing");
        responseMsg.success = false;
        responseMsg.type = UNAUTHORIZED_RTYPE;
        responseMsg.responseContent = "Failed to validate JWT-Token";
        return false;
    }

    // validate token
    try
    {
        auto decodedToken = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{(const char*)HanamiRoot::tokenKey.data()});
        verifier.verify(decodedToken);

        // copy data of token into the output
        for(const auto& e : decodedToken.get_payload_json())
        {
            const std::string tokenStr = e.second.to_str();
            try {
                tokenData = json::parse(tokenStr);
            } catch(const json::parse_error& ex) {
                error.addMeesage("json-parser error: " + std::string(ex.what()));
                return false;
            }
        }
    }
    catch (const std::exception& ex)
    {
        error.addMeesage("Failed to validate JWT-Token with error: " + std::string(ex.what()));
        responseMsg.success = false;
        responseMsg.type = UNAUTHORIZED_RTYPE;
        responseMsg.responseContent = "Failed to validate JWT-Token";
        return false;
    }

    const uint32_t httpTypeValue = static_cast<uint32_t>(hanamiRequest.httpType);
    const HttpRequestType httpType = static_cast<HttpRequestType>(httpTypeValue);

    // process payload to get role of user
    const std::string role = tokenData["role"];

    // check policy
    if(Policy::getInstance()->checkUserAgainstPolicy(endpoint,
                                                     httpType,
                                                     role) == false)
    {
        responseMsg.success = false;
        responseMsg.type = UNAUTHORIZED_RTYPE;
        responseMsg.responseContent = "Access denied by policy";
        error.addMeesage(responseMsg.responseContent);
        return false;
    }

    responseMsg.success = true;

    return true;
}

/**
 * @brief process control request
 *
 * @param uri requested uri
 * @param token given token coming from the http-header
 * @param inputValues json-formated input-values
 * @param httpType type of the http-request
 * @param error reference for error-output
 *
 * @return true, if successful, else false
 */
bool
processControlRequest(http::response<http::dynamic_body> &httpResponse,
                      const std::string &uri,
                      const std::string &token,
                      const std::string &inputValues,
                      const Hanami::HttpRequestType httpType,
                      Hanami::ErrorContainer &error)
{
    RequestMessage hanamiRequest;
    ResponseMessage hanamiResponse;

    // parse uri
    hanamiRequest.httpType = httpType;
    hanamiRequest.inputValues = inputValues;
    if(parseUri(token, hanamiRequest, uri, error) == false) {
        return invalid_ResponseBuild(httpResponse, error);
    }

    // handle token-request
    if(uri == "v1/token"
            && hanamiRequest.httpType == Hanami::POST_TYPE)
    {
        return requestToken(httpResponse, hanamiRequest, error);
    }

    // check authentication
    json tokenData = json::object();
    if(checkPermission(tokenData, token, hanamiRequest, hanamiResponse, error) == false) {
        return internalError_ResponseBuild(httpResponse, error);
    }

    // handle failed authentication
    if(hanamiResponse.type == UNAUTHORIZED_RTYPE
            || hanamiResponse.success == false)
    {
        return genericError_ResponseBuild(httpResponse,
                                          hanamiResponse.type,
                                          hanamiResponse.responseContent);
    }

    // convert http-type to string
    std::string httpTypeStr = "GET";
    if(hanamiRequest.httpType == Hanami::DELETE_TYPE) {
        httpTypeStr = "DELETE";
    }
    if(hanamiRequest.httpType == Hanami::GET_TYPE) {
        httpTypeStr = "GET";
    }
    if(hanamiRequest.httpType == Hanami::HEAD_TYPE) {
        httpTypeStr = "HEAD";
    }
    if(hanamiRequest.httpType == Hanami::POST_TYPE) {
        httpTypeStr = "POST";
    }
    if(hanamiRequest.httpType == Hanami::PUT_TYPE) {
        httpTypeStr = "PUT";
    }

    // write new audit-entry to database
    if(AuditLogTable::getInstance()->addAuditLogEntry(getDatetime(),
                                                      tokenData["id"],
                                                      hanamiRequest.id,
                                                      httpTypeStr,
                                                      error) == false)
    {
        error.addMeesage("ERROR: Failed to write audit-log into database");
        return internalError_ResponseBuild(httpResponse, error);
    }

    json inputValuesJson;
    try {
        inputValuesJson = json::parse(hanamiRequest.inputValues);
    } catch(const json::parse_error& ex) {
        error.addMeesage("json-parser error: " + std::string(ex.what()));
        return internalError_ResponseBuild(httpResponse, error);
    }

    if(hanamiRequest.id != "v1/auth") {
        inputValuesJson.erase("token");
    }

    EndpointEntry endpoint;
    if(HanamiRoot::root->mapEndpoint(endpoint, hanamiRequest.id, hanamiRequest.httpType) == false) {
        assert(false);
    }

    // make real request
    json result = json::object();
    BlossomStatus status;
    if(HanamiRoot::root->triggerBlossom(result,
                                        endpoint.name,
                                        endpoint.group,
                                        tokenData,
                                        inputValuesJson,
                                        status,
                                        error) == false)
    {
        return genericError_ResponseBuild(httpResponse,
                                          static_cast<HttpResponseTypes>(status.statusCode),
                                          status.errorMessage);
    }

    // handle error-response
    if(hanamiResponse.success == false)
    {
        return genericError_ResponseBuild(httpResponse,
                                          hanamiResponse.type,
                                          hanamiResponse.responseContent);
    }

    hanamiResponse.type = OK_RTYPE;
    hanamiResponse.responseContent = result.dump();

    // handle success
    return success_ResponseBuild(httpResponse, hanamiResponse.responseContent);
}
