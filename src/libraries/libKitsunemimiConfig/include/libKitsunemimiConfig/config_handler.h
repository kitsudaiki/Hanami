/**
 *  @file       config_handler.h
 *
 *  @author     Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 *  @copyright  Apache License Version 2.0
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

#ifndef CONFIG_HANDLER_H
#define CONFIG_HANDLER_H

#include <iostream>
#include <vector>
#include <map>
#include <libKitsunemimiCommon/logger.h>

#define REGISTER_STRING_CONFIG Kitsunemimi::registerString
#define REGISTER_INT_CONFIG Kitsunemimi::registerInteger
#define REGISTER_FLOAT_CONFIG Kitsunemimi::registerFloat
#define REGISTER_BOOL_CONFIG Kitsunemimi::registerBoolean
#define REGISTER_STRING_ARRAY_CONFIG Kitsunemimi::registerStringArray

#define GET_STRING_CONFIG Kitsunemimi::getString
#define GET_INT_CONFIG Kitsunemimi::getInteger
#define GET_FLOAT_CONFIG Kitsunemimi::getFloat
#define GET_BOOL_CONFIG Kitsunemimi::getBoolean
#define GET_STRING_ARRAY_CONFIG Kitsunemimi::getStringArray

namespace Kitsunemimi
{
class DataItem;
class IniItem;

class ConfigHandler_Test;

bool initConfig(const std::string &configFilePath,
                ErrorContainer &error);

// register config-options
bool registerString(const std::string &groupName,
                    const std::string &itemName,
                    const std::string &comment,
                    ErrorContainer &error,
                    const std::string &defaultValue = "",
                    const bool required = false);
bool registerInteger(const std::string &groupName,
                     const std::string &itemName,
                     const std::string &comment,
                     ErrorContainer &error,
                     const long defaultValue = 0,
                     const bool required = false);
bool registerFloat(const std::string &groupName,
                   const std::string &itemName,
                   const std::string &comment,
                   ErrorContainer &error,
                   const double defaultValue = 0.0,
                   const bool required = false);
bool registerBoolean(const std::string &groupName,
                     const std::string &itemName,
                     const std::string &comment,
                     ErrorContainer &error,
                     const bool defaultValue = false,
                     const bool required = false);
bool registerStringArray(const std::string &groupName,
                         const std::string &itemName,
                         const std::string &comment,
                         ErrorContainer &error,
                         const std::vector<std::string> &defaultValue = {},
                         const bool required = false);

// getter
const std::string getString(const std::string &groupName,
                            const std::string &itemName,
                            bool &success);
long getInteger(const std::string &groupName,
                const std::string &itemName,
                bool &success);
double getFloat(const std::string &groupName,
                const std::string &itemName,
                bool &success);
bool getBoolean(const std::string &groupName,
                const std::string &itemName,
                bool &success);
const std::vector<std::string> getStringArray(const std::string &groupName,
                                              const std::string &itemName,
                                              bool &success);

//==================================================================================================

class ConfigHandler
{
public:
    ConfigHandler();
    ~ConfigHandler();

    bool initConfig(const std::string &configFilePath,
                    ErrorContainer &error);

    // register config-options
    bool registerString(const std::string &groupName,
                        const std::string &itemName,
                        const std::string &comment,
                        ErrorContainer &error,
                        const std::string &defaultValue = "",
                        const bool required = false);
    bool registerInteger(const std::string &groupName,
                         const std::string &itemName,
                         const std::string &comment,
                         ErrorContainer &error,
                         const long defaultValue = 0,
                         const bool required = false);
    bool registerFloat(const std::string &groupName,
                       const std::string &itemName,
                       const std::string &comment,
                       ErrorContainer &error,
                       const double defaultValue = 0.0,
                       const bool required = false);
    bool registerBoolean(const std::string &groupName,
                         const std::string &itemName,
                         const std::string &comment,
                         ErrorContainer &error,
                         const bool defaultValue = false,
                         const bool required = false);
    bool registerStringArray(const std::string &groupName,
                             const std::string &itemName,
                             const std::string &comment,
                             ErrorContainer &error,
                             const std::vector<std::string> &defaultValue = {},
                             const bool required = false);

    // getter
    const std::string getString(const std::string &groupName,
                                const std::string &itemName,
                                bool &success);
    long getInteger(const std::string &groupName,
                    const std::string &itemName,
                    bool &success);
    double getFloat(const std::string &groupName,
                    const std::string &itemName,
                    bool &success);
    bool getBoolean(const std::string &groupName,
                    const std::string &itemName,
                    bool &success);
    const std::vector<std::string> getStringArray(const std::string &groupName,
                                                  const std::string &itemName,
                                                  bool &success);

    static Kitsunemimi::ConfigHandler* m_config;

private:
    friend ConfigHandler_Test;

    enum ConfigType
    {
        UNDEFINED_TYPE,
        STRING_TYPE,
        INT_TYPE,
        FLOAT_TYPE,
        BOOL_TYPE,
        STRING_ARRAY_TYPE
    };

    struct ConfigEntry
    {
        bool isRequired = false;
        ConfigType type = UNDEFINED_TYPE;
        DataItem* value = nullptr;
        std::string comment = "";
    };

    bool checkEntry(const std::string &groupName,
                    const std::string &itemName,
                    ConfigEntry &entry,
                    ErrorContainer &error);
    bool checkType(const std::string &groupName,
                   const std::string &itemName,
                   const ConfigType type);
    bool isRegistered(const std::string &groupName,
                      const std::string &itemName);

    ConfigType getRegisteredType(const std::string &groupName,
                                 const std::string &itemName);

    bool registerValue(std::string &groupName,
                       const std::string &itemName,
                       const std::string &comment,
                       const ConfigType type,
                       const bool required,
                       DataItem* defaultValue,
                       ErrorContainer &error);

    std::string m_configFilePath = "";
    IniItem* m_iniItem = nullptr;
    std::map<std::string, std::map<std::string, ConfigEntry>> m_registeredConfigs;
};

}


#endif // CONFIG_HANDLER_H
