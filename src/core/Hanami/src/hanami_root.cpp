﻿/**
 * @file        hanami_root.cpp
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

#include <hanami_root.h>

#include <core/cluster/cluster_init.h>

#include <core/processing/processing_unit_handler.h>

#include <core/cluster/cluster_handler.h>
#include <core/cluster/cluster.h>

#include <libKitsunemimiCommon/logger.h>
#include <libKitsunemimiCommon/files/text_file.h>
#include <libKitsunemimiConfig/config_handler.h>

#include <libKitsunemimiOpencl/gpu_interface.h>
#include <libKitsunemimiOpencl/gpu_handler.h>
#include <api/endpoint_processing/http_server.h>
#include <api/endpoint_processing/http_websocket_thread.h>
#include <api/endpoint_processing/blossom.h>
#include <api/endpoint_processing/items/item_methods.h>

#include <libKitsunemimiSakuraDatabase/sql_database.h>
#include <database/data_set_table.h>
#include <database/cluster_snapshot_table.h>
#include <database/request_result_table.h>
#include <database/error_log_table.h>
#include <database/audit_log_table.h>
#include <core/temp_file_handler.h>
#include <core/thread_binder.h>
#include <core/power_measuring.h>
#include <core/speed_measuring.h>
#include <core/temperature_measuring.h>

// init static variables
uint32_t* HanamiRoot::m_randomValues = nullptr;
Kitsunemimi::GpuInterface* HanamiRoot::gpuInterface = nullptr;
Kitsunemimi::Jwt* HanamiRoot::jwt = nullptr;
HanamiRoot* HanamiRoot::root = nullptr;

// static flag to switch to experimental gpu-support (see issue #44 and #76)
bool HanamiRoot::useOpencl = false;
bool HanamiRoot::useCuda = true;
HttpServer* HanamiRoot::httpServer = nullptr;

/**
 * @brief constructor
 */
HanamiRoot::HanamiRoot() {}

/**
 * @brief destructor
 */
HanamiRoot::~HanamiRoot() {}

/**
 * @brief init root-object
 *
 * @param error reference for error-output
 *
 * @return true, if successful, else false
 */
bool
HanamiRoot::init(Kitsunemimi::ErrorContainer &error)
{
    root = this;

    if(useOpencl)
    {
        Kitsunemimi::GpuHandler oclHandler;
        assert(oclHandler.initDevice(error));
        assert(oclHandler.m_interfaces.size() == 1);
        gpuInterface = oclHandler.m_interfaces.at(0);
    }

    // init predefinde random-values
    m_randomValues = new uint32_t[NUMBER_OF_RAND_VALUES];
    srand(time(NULL));
    for(uint32_t i = 0; i < NUMBER_OF_RAND_VALUES; i++) {
        m_randomValues[i] = static_cast<uint32_t>(rand());
    }

    // init db
    if(initDatabase(error) == false) {
        return false;
    }

    if(initDatabase(error) == false)
    {
        error.addMeesage("Failed to initialize database");
        return false;
    }

    if(initJwt(error) == false)
    {
        error.addMeesage("Failed to initialize jwt");
        return false;
    }

    if(initPolicies(error) == false)
    {
        error.addMeesage("Failed to initialize policies");
        return false;
    }

    if(initHttpServer() == false)
    {
        error.addMeesage("initializing http-server failed");
        LOG_ERROR(error);
        return false;
    }

    // init overview of all resources of the host
    //Kitsunemimi::Sakura::Host* host = Kitsunemimi::Sakura::Host::getInstance();
    //if(host->initHost(error) == false)
    //{
    //    error.addMeesage("Failed read resource-information of the local host");
    //    LOG_ERROR(error);
    //    return 1;
    //}

    // create thread-binder
    //threadBinder = new ThreadBinder();
    //threadBinder->startThread();

    // create power-measuring-loop
    //powerMeasuring = new PowerMeasuring();
    //powerMeasuring->startThread();

    // create speed-measuring-loop
    //speedMeasuring = new SpeedMeasuring();
    //speedMeasuring->startThread();

    // create temperature-measuring-loop
    //temperatureMeasuring = new TemperatureMeasuring();
    //temperatureMeasuring->startThread();

    return true;
}

/**
 * @brief create processing-threads
 *
 * @return true, if successful, else false
 */
bool
HanamiRoot::initThreads()
{
    ProcessingUnitHandler* processingUnitHandler = ProcessingUnitHandler::getInstance();
    if(processingUnitHandler->initProcessingUnits(1) == false) {
        return false;
    }

    return true;
}

/**
 * @brief init database
 *
 * @param error reference for error-output
 *
 * @return true, if successful, else false
 */
bool
HanamiRoot::initDatabase(Kitsunemimi::ErrorContainer &error)
{
    bool success = false;

    // read database-path from config
    Kitsunemimi::Sakura::SqlDatabase* database = Kitsunemimi::Sakura::SqlDatabase::getInstance();
    const std::string databasePath = GET_STRING_CONFIG("DEFAULT", "database", success);
    LOG_DEBUG("database-path: '" + databasePath + "'");
    if(success == false)
    {
        error.addMeesage("No database-path defined in config.");
        LOG_ERROR(error);
        return false;
    }

    // initalize database
    if(database->initDatabase(databasePath, error) == false)
    {
        error.addMeesage("Failed to initialize sql-database.");
        LOG_ERROR(error);
        return false;
    }

    // initialize cluster-table
    ClusterTable* clustersTable = ClusterTable::getInstance();
    if(clustersTable->initTable(error) == false)
    {
        error.addMeesage("Failed to initialize cluster-table in database.");
        LOG_ERROR(error);
        return false;
    }

    // initialize template-table
    TemplateTable* templateTable = TemplateTable::getInstance();
    if(templateTable->initTable(error) == false)
    {
        error.addMeesage("Failed to initialize template-table in database.");
        LOG_ERROR(error);
        return false;
    }

    // initialize projects-table
    ProjectsTable* projectsTable = ProjectsTable::getInstance();
    if(projectsTable->initTable(error) == false)
    {
        error.addMeesage("Failed to initialize project-table in database.");
        return false;
    }

    // initialize users-table
    UsersTable* usersTable = UsersTable::getInstance();
    if(usersTable->initTable(error) == false)
    {
        error.addMeesage("Failed to initialize user-table in database.");
        return false;
    }
    if(usersTable->initNewAdminUser(error) == false)
    {
        error.addMeesage("Failed to initialize new admin-user even this is necessary.");
        return false;
    }

    // initialize dataset-table
    DataSetTable* dataSetTable = DataSetTable::getInstance();
    if(dataSetTable->initTable(error) == false)
    {
        error.addMeesage("Failed to initialize dataset-table in database.");
        LOG_ERROR(error);
        return false;
    }

    // initialize request-result-table
    RequestResultTable* requestResultTable = RequestResultTable::getInstance();
    if(requestResultTable->initTable(error) == false)
    {
        error.addMeesage("Failed to initialize request-result-table in database.");
        LOG_ERROR(error);
        return false;
    }

    // initialize cluster-snapshot-table
    ClusterSnapshotTable* clusterSnapshotTable = ClusterSnapshotTable::getInstance();
    if(clusterSnapshotTable->initTable(error) == false)
    {
        error.addMeesage("Failed to initialize cluster-snapshot-table in database.");
        LOG_ERROR(error);
        return false;
    }

    // initialize error-log-table
    ErrorLogTable* errorLogTable = ErrorLogTable::getInstance();
    if(errorLogTable->initTable(error) == false)
    {
        error.addMeesage("Failed to initialize error-log-table in database.");
        LOG_ERROR(error);
        return false;
    }

    // initialize audit-log-table
    AuditLogTable* auditLogTable = AuditLogTable::getInstance();
    if(auditLogTable->initTable(error) == false)
    {
        error.addMeesage("Failed to initialize audit-log-table in database.");
        LOG_ERROR(error);
        return false;
    }

    return true;
}

/**
 * @brief initialze http server
 *
 * @return true, if successful, else false
 */
bool
HanamiRoot::initHttpServer()
{
    bool success = false;

    // check if http is enabled
    if(GET_BOOL_CONFIG("http", "enable", success) == false) {
        return true;
    }

    // get stuff from config
    const uint16_t port =            GET_INT_CONFIG(    "http", "port",              success);
    const std::string ip =           GET_STRING_CONFIG( "http", "ip",                success);
    const std::string cert =         GET_STRING_CONFIG( "http", "certificate",       success);
    const std::string key =          GET_STRING_CONFIG( "http", "key",               success);
    const uint32_t numberOfThreads = GET_INT_CONFIG(    "http", "number_of_threads", success);

    // create server
    httpServer = new HttpServer(ip, port, cert, key);
    httpServer->startThread();

    // start threads
    for(uint32_t i = 0; i < numberOfThreads; i++)
    {
        const std::string name = "HttpWebsocketThread";
        HttpWebsocketThread* httpWebsocketThread = new HttpWebsocketThread(name);
        httpWebsocketThread->startThread();
        m_threads.push_back(httpWebsocketThread);
    }

    return true;
}

/**
 * @brief init policies
 *
 * @param error reference for error-output
 *
 * @return true, if successful, else false
 */
bool
HanamiRoot::initPolicies(Kitsunemimi::ErrorContainer &error)
{
    bool success = false;

    // read policy-file-path from config
    const std::string policyFilePath = GET_STRING_CONFIG("auth", "policies", success);
    if(success == false)
    {
        error.addMeesage("No policy-file defined in config.");
        return false;
    }

    // read policy-file
    std::string policyFileContent;
    if(Kitsunemimi::readFile(policyFileContent, policyFilePath, error) == false)
    {
        error.addMeesage("Failed to read policy-file");
        return false;
    }

    // parse policy-file
    Kitsunemimi::Hanami::Policy* policies = Kitsunemimi::Hanami::Policy::getInstance();
    if(policies->parse(policyFileContent, error) == false)
    {
        error.addMeesage("Failed to parser policy-file");
        return false;
    }

    return true;
}

/**
 * @brief init jwt-class to validate incoming requested
 *
 * @param error reference for error-output
 *
 * @return true, if successful, else false
 */
bool
HanamiRoot::initJwt(Kitsunemimi::ErrorContainer &error)
{
    bool success = false;

    // read jwt-token-key from config
    const std::string tokenKeyPath = GET_STRING_CONFIG("auth", "token_key_path", success);
    if(success == false)
    {
        error.addMeesage("No token_key_path defined in config.");
        return false;
    }

    std::string tokenKeyString;
    if(Kitsunemimi::readFile(tokenKeyString, tokenKeyPath, error) == false)
    {
        error.addMeesage("Failed to read token-file '" + tokenKeyPath + "'");
        return false;
    }

    // init jwt for token create and sign
    CryptoPP::SecByteBlock tokenKey((unsigned char*)tokenKeyString.c_str(), tokenKeyString.size());
    jwt = new Kitsunemimi::Jwt(tokenKey);

    return true;
}

/**
 * @brief check if a specific blossom was registered
 *
 * @param groupName group-identifier of the blossom
 * @param itemName item-identifier of the blossom
 *
 * @return true, if blossom with the given group- and item-name exist, else false
 */
bool
HanamiRoot::doesBlossomExist(const std::string &groupName,
                             const std::string &itemName)
{
    auto groupIt = m_registeredBlossoms.find(groupName);
    if(groupIt != m_registeredBlossoms.end())
    {
        if(groupIt->second.find(itemName) != groupIt->second.end()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief SakuraLangInterface::addBlossom
 *
 * @param groupName group-identifier of the blossom
 * @param itemName item-identifier of the blossom
 * @param newBlossom pointer to the new blossom
 *
 * @return true, if blossom was registered or false, if the group- and item-name are already
 *         registered
 */
bool
HanamiRoot::addBlossom(const std::string &groupName,
                       const std::string &itemName,
                       Blossom* newBlossom)
{
    // check if already used
    if(doesBlossomExist(groupName, itemName) == true) {
        return false;
    }

    // create internal group-map, if not already exist
    auto groupIt = m_registeredBlossoms.find(groupName);
    if(groupIt == m_registeredBlossoms.end())
    {
        std::map<std::string, Blossom*> newMap;
        m_registeredBlossoms.insert(std::make_pair(groupName, newMap));
    }

    // add item to group
    groupIt = m_registeredBlossoms.find(groupName);
    groupIt->second.insert(std::make_pair(itemName, newBlossom));

    return true;
}

/**
 * @brief request a registered blossom
 *
 * @param groupName group-identifier of the blossom
 * @param itemName item-identifier of the blossom
 *
 * @return pointer to the blossom or
 *         nullptr, if blossom the given group- and item-name was not found
 */
Blossom*
HanamiRoot::getBlossom(const std::string &groupName,
                       const std::string &itemName)
{
    // search for group
    auto groupIt = m_registeredBlossoms.find(groupName);
    if(groupIt != m_registeredBlossoms.end())
    {
        // search for item within group
        auto itemIt = groupIt->second.find(itemName);
        if(itemIt != groupIt->second.end()) {
            return itemIt->second;
        }
    }

    return nullptr;
}

/**
 * @brief trigger existing blossom
 *
 * @param result map with resulting items
 * @param blossomName id of the blossom to trigger
 * @param blossomGroupName id of the group of the blossom to trigger
 * @param initialValues input-values for the tree
 * @param status reference for status-output
 * @param error reference for error-output
 *
 * @return true, if successfule, else false
 */
bool
HanamiRoot::triggerBlossom(Kitsunemimi::DataMap &result,
                           const std::string &blossomName,
                           const std::string &blossomGroupName,
                           const Kitsunemimi::DataMap &context,
                           const Kitsunemimi::DataMap &initialValues,
                           BlossomStatus &status,
                           Kitsunemimi::ErrorContainer &error)
{
    LOG_DEBUG("trigger blossom");

    // get initial blossom-item
    Blossom* blossom = getBlossom(blossomGroupName, blossomName);
    if(blossom == nullptr)
    {
        error.addMeesage("No blosom found for the id " + blossomName);
        return false;
    }

    // inialize a new blossom-leaf for processing
    BlossomIO blossomIO;
    blossomIO.blossomName = blossomName;
    blossomIO.blossomPath = blossomName;
    blossomIO.blossomGroupType = blossomGroupName;
    blossomIO.input = &initialValues;
    blossomIO.parentValues = blossomIO.input.getItemContent()->toMap();
    blossomIO.nameHirarchie.push_back("BLOSSOM: " + blossomName);

    std::string errorMessage;
    // check input to be complete
    if(blossom->validateFieldsCompleteness(initialValues,
                                           *blossom->getInputValidationMap(),
                                           FieldDef::INPUT_TYPE,
                                           errorMessage) == false)
    {
        error.addMeesage(errorMessage);
        error.addMeesage("check of completeness of input-fields failed");
        status.statusCode = 400;
        status.errorMessage = errorMessage;
        LOG_ERROR(error);
        return false;
    }

    // process blossom
    if(blossom->growBlossom(blossomIO, &context, status, error) == false)
    {
        error.addMeesage("trigger blossom failed.");
        LOG_ERROR(error);
        return false;
    }

    // check output to be complete
    Kitsunemimi::DataMap* output = blossomIO.output.getItemContent()->toMap();
    if(blossom->validateFieldsCompleteness(*output,
                                           *blossom->getOutputValidationMap(),
                                           FieldDef::OUTPUT_TYPE,
                                           errorMessage) == false)
    {
        error.addMeesage(errorMessage);
        error.addMeesage("check of completeness of output-fields failed");
        status.statusCode = 500;
        status.errorMessage = errorMessage;
        LOG_ERROR(error);
        return false;
    }

    // TODO: override only with the output-values to avoid unnecessary conflicts
    result.clear();
    overrideItems(result, *output, ALL);

    return true;
}

/**
 * @brief map the endpoint to the real target
 *
 * @param result reference to the result to identify the target
 * @param id request-id
 * @param type requested http-request-type
 *
 * @return false, if mapping failes, else true
 */
bool
HanamiRoot::mapEndpoint(EndpointEntry &result,
                        const std::string &id,
                        const HttpRequestType type)
{
    const auto id_it = endpointRules.find(id);
    if(id_it != endpointRules.end())
    {
        auto type_it = id_it->second.find(type);
        if(id_it->second.find(type) != id_it->second.end())
        {
            result.type = type_it->second.type;
            result.group = type_it->second.group;
            result.name = type_it->second.name;

            return true;
        }
    }

    return false;
}

/**
 * @brief add new custom-endpoint without the parser
 *
 * @param id identifier for the new entry
 * @param httpType http-type (get, post, put, delete)
 * @param sakuraType sakura-type (tree or blossom)
 * @param group blossom-group
 * @param name tree- or blossom-id
 *
 * @return false, if id together with http-type is already registered, else true
 */
bool
HanamiRoot::addEndpoint(const std::string &id,
                        const HttpRequestType &httpType,
                        const SakuraObjectType &sakuraType,
                        const std::string &group,
                        const std::string &name)
{
    EndpointEntry newEntry;
    newEntry.type = sakuraType;
    newEntry.group = group;
    newEntry.name = name;

    // search for id
    auto id_it = endpointRules.find(id);
    if(endpointRules.find(id) != endpointRules.end())
    {
        // search for http-type
        if(id_it->second.find(httpType) != id_it->second.end()) {
            return false;
        }

        // add new
        id_it->second.emplace(httpType, newEntry);
    }
    else
    {
        // add new
        std::map<HttpRequestType, EndpointEntry> typeEntry;
        typeEntry.emplace(httpType, newEntry);
        endpointRules.emplace(id, typeEntry);
    }

    return true;
}
