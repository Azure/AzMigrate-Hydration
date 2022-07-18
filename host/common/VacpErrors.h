#pragma once

#include <vector>
#include <string>
#include <map>
#include <fstream>

#include "json_reader.h"
#include "json_writer.h"

#include "AgentHealthIssueNumbers.h"

namespace VacpErrorAttributes
{
    const char  VERSION[] = "Version";
    const char PARAMS[] = "params";
    const char ERROR_NAME[] = "ErrorName";
    const char ELMENT[] = "Error";
}

class VacpError
{
public:
    VacpError() {}

    VacpError(std::string errorName, std::map<std::string, std::string> params) :
        IssueCode(errorName),
        MessageParams(params)
    {
    }

    std::string                         IssueCode;
    std::map<std::string, std::string>  MessageParams;

    /// \brief a serializer method for JSON
    void serialize(JSON::Adapter& adapter)
    {
        JSON::Class root(adapter, "Error", false);
        JSON_E(adapter, IssueCode);
        JSON_T(adapter, MessageParams);
    }

};

#define VACP_ERROR_VERSION              "1.0"

namespace VacpAttributes
{
    const char LOG_VERSION[] = "Version";
    const char TAG_TYPE[] = "TagType";
    const char TAG_GUID[] = "TagGuid";
    const char LOG_PATH[] = "LogPath";
    const char END_TIME[] = "EndTime";
}

class VacpErrorInfo
{
public:
    VacpErrorInfo()
    {
        Attributes[VacpAttributes::LOG_VERSION] = VACP_ERROR_VERSION;
    }

    void    Add(std::string   errorCode, std::map<std::string, std::string> params)
    {
        std::vector<VacpError>::iterator iter = Errors.begin();
        for (; iter != Errors.end(); iter++)
        {
            if (iter->IssueCode == errorCode)
            {
                DebugPrintf("%s:A duplicate error Code %s is already found in current VSS Health Issues and hence not adding the same error.\n",FUNCTION_NAME,errorCode.c_str());
                return;//RCM limitation: RCM cannot accept multiple Health Issues with same HealthCode.
            }
        }
        Errors.push_back(VacpError(errorCode, params));
    }

    void clear()
    {
        Errors.erase(Errors.begin(), Errors.end());
        Attributes.clear();
    }
    void    SetAttribute(std::string name, std::string value)
    {
        if (!name.empty())
        {
            Attributes[name] = value;
        }
    }

    std::string get(std::string attribute)
    {
        if (Attributes.end() != Attributes.find(attribute)) 
        {
            return Attributes[attribute];
        }

        return "";
    }

    bool empty()
    {
        return Errors.empty();
    }
    std::vector<VacpError> getVacpErrors()
    {
        return Errors;
    }

    /// \brief a serializer method for JSON
    void serialize(JSON::Adapter& adapter)
    {
        JSON::Class root(adapter, "Errors", false);
        JSON_E(adapter, Attributes);
        JSON_T(adapter, Errors);
    }

private:
    std::vector<VacpError>                  Errors;
    std::map<std::string, std::string>      Attributes;
};

