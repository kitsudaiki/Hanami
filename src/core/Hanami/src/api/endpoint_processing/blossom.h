/**
 * @file        blossom.h
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

#ifndef KITSUNEMIMI_SAKURA_LANG_BLOSSOM_H
#define KITSUNEMIMI_SAKURA_LANG_BLOSSOM_H

#include <common/structs.h>

#include <libKitsunemimiCommon/items/data_items.h>
#include <libKitsunemimiCommon/logger.h>
#include <libKitsunemimiJson/json_item.h>

class BlossomItem;
class SakuraThread;
class InitialValidator;
class HanamiRoot;
class ValueItemMap;

//--------------------------------------------------------------------------------------------------

struct BlossomIO
{
    std::string blossomType = "";
    std::string blossomName = "";
    std::string blossomPath = "";
    std::string blossomGroupType = "";
    std::vector<std::string> nameHirarchie;

    Kitsunemimi::JsonItem output;
    Kitsunemimi::JsonItem input;

    Kitsunemimi::DataMap* parentValues = nullptr;

    std::string terminalOutput = "";

    BlossomIO()
    {
        std::map<std::string, Kitsunemimi::JsonItem> temp;
        output = Kitsunemimi::JsonItem(temp);
        input = Kitsunemimi::JsonItem(temp);
    }
};

//--------------------------------------------------------------------------------------------------

enum FieldType
{
    SAKURA_UNDEFINED_TYPE = 0,
    SAKURA_INT_TYPE = 1,
    SAKURA_FLOAT_TYPE = 2,
    SAKURA_BOOL_TYPE = 3,
    SAKURA_STRING_TYPE = 4,
    SAKURA_ARRAY_TYPE = 5,
    SAKURA_MAP_TYPE = 6
};

//--------------------------------------------------------------------------------------------------

struct FieldDef
{
    enum IO_ValueType
    {
        UNDEFINED_VALUE_TYPE = 0,
        INPUT_TYPE = 1,
        OUTPUT_TYPE = 2,
    };

    const IO_ValueType ioType;
    const FieldType fieldType;
    const bool isRequired;
    const std::string comment;
    Kitsunemimi::DataItem* match = nullptr;
    Kitsunemimi::DataItem* defaultVal = nullptr;
    std::string regex = "";
    long lowerBorder = 0;
    long upperBorder = 0;

    FieldDef(const IO_ValueType ioType,
             const FieldType fieldType,
             const bool isRequired,
             const std::string &comment)
        : ioType(ioType),
          fieldType(fieldType),
          isRequired(isRequired),
          comment(comment) { }
};

//--------------------------------------------------------------------------------------------------

class Blossom
{
public:
    Blossom(const std::string &comment, const bool requiresToken = true);
    virtual ~Blossom();

    const std::string comment;
    const bool requiresAuthToken;

    const std::map<std::string, FieldDef>* getInputValidationMap() const;
    const std::map<std::string, FieldDef>* getOutputValidationMap() const;

protected:
    virtual bool runTask(BlossomIO &blossomIO,
                         const Kitsunemimi::DataMap &context,
                         BlossomStatus &status,
                         Kitsunemimi::ErrorContainer &error) = 0;
    bool allowUnmatched = false;

    bool registerInputField(const std::string &name,
                            const FieldType fieldType,
                            const bool required,
                            const std::string &comment);
    bool registerOutputField(const std::string &name,
                             const FieldType fieldType,
                             const std::string &comment);
    bool addFieldMatch(const std::string &name,
                       Kitsunemimi::DataItem* match);
    bool addFieldDefault(const std::string &name,
                         Kitsunemimi::DataItem* defaultValue);
    bool addFieldRegex(const std::string &name,
                       const std::string &regex);
    bool addFieldBorder(const std::string &name,
                        const long lowerBorder,
                        const long upperBorder);

private:
    friend SakuraThread;
    friend InitialValidator;
    friend HanamiRoot;

    std::map<std::string, FieldDef> m_inputValidationMap;
    std::map<std::string, FieldDef> m_outputValidationMap;

    bool growBlossom(BlossomIO &blossomIO,
                     const Kitsunemimi::DataMap* context,
                     BlossomStatus &status,
                     Kitsunemimi::ErrorContainer &error);
    bool validateFieldsCompleteness(const Kitsunemimi::DataMap &input,
                                    const std::map<std::string, FieldDef> &validationMap,
                                    const FieldDef::IO_ValueType valueType,
                                    std::string &errorMessage);
    bool validateInput(BlossomItem &blossomItem,
                       const std::map<std::string, FieldDef> &validationMap,
                       const std::string &filePath,
                       Kitsunemimi::ErrorContainer &error);
    void getCompareMap(std::map<std::string, FieldDef::IO_ValueType> &compareMap,
                       const ValueItemMap &valueMap);
    void fillDefaultValues(Kitsunemimi::DataMap &values);
};

#endif // KITSUNEMIMI_SAKURA_LANG_BLOSSOM_H
