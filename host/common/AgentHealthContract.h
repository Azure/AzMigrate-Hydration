#ifndef _AGENTHEALTHCONTRACT_H_
#define _AGENTHEALTHCONTRACT_H_

#include <string>
#include <vector>

#include "json_reader.h"
#include "json_writer.h"

#include "AgentHealthIssueNumbers.h"

#include <boost/algorithm/string/predicate.hpp>

#define EVENT_SOURCE_AGENT              "SourceAgent"
#define EVENT_SEVERITY_ERROR            "Error"
#define EVENT_SEVERITY_WARNING          "Warning"
#define EVENT_SEVERITY_INFORMATION      "Information"
#define EVENT_SEVERITY_UNKNOWN          "Unknown"

class HealthIssue
{
public:
    std::string                         IssueCode;
    std::string                         Severity;
    std::string                         Source;
    std::map<std::string, std::string>  MessageParams;

    HealthIssue() {}

    HealthIssue(std::string issueCode) :
        IssueCode(issueCode),
        Severity(EVENT_SEVERITY_WARNING),
        Source(EVENT_SOURCE_AGENT),
        MessageParams(std::map<std::string, std::string>())
    {
    }

    HealthIssue(std::string issueCode, std::map<std::string, std::string> params) :
        IssueCode(issueCode),
        Severity(EVENT_SEVERITY_WARNING),
        Source(EVENT_SOURCE_AGENT),
        MessageParams(params)
    {
    }

    bool is_equal(std::string issueCode) const
    {
        return boost::iequals(issueCode, IssueCode);
    }

    void serialize(JSON::Adapter& adapter)
    {
        JSON::Class root(adapter, "HealthIssue", false);

        JSON_E(adapter, IssueCode);
        JSON_E(adapter, Severity);
        JSON_E(adapter, Source);
        JSON_T(adapter, MessageParams);
    }
};

class AgentDiskLevelHealthIssue : public HealthIssue
{
public:
    std::string     DiskContext;

    AgentDiskLevelHealthIssue() {}

    AgentDiskLevelHealthIssue(std::string diskId, std::string issueCode) :
        DiskContext(diskId),
        HealthIssue(issueCode)
    {
    }

    AgentDiskLevelHealthIssue(std::string diskId, std::string issueCode, std::map<std::string, std::string> params) :
        DiskContext(diskId),
        HealthIssue(issueCode, params)
    {
    }

    bool is_equal(std::string issueCode) const
    {
        return boost::iequals(issueCode, IssueCode);
    }

    void serialize(JSON::Adapter& adapter)
    {
        JSON::Class root(adapter, "AgentDiskLevelHealthIssue", false);
        JSON_E(adapter, DiskContext);
        JSON_E(adapter, IssueCode);
        JSON_E(adapter, Severity);
        JSON_E(adapter, Source);
        JSON_T(adapter, MessageParams);
    }
};

class SourceAgentProtectionPairHealthIssues
{
public:
    std::vector<HealthIssue>                    HealthIssues;
    std::vector<AgentDiskLevelHealthIssue>      DiskLevelHealthIssues;

    /// \brief a serializer method for JSON
    void serialize(JSON::Adapter& adapter)
    {
        JSON::Class root(adapter, "SourceAgentProtectionPairHealthIssues", false);
        JSON_E(adapter, HealthIssues);
        JSON_T(adapter, DiskLevelHealthIssues);
    }
};

// a <key, value> pair of Health IssueCode and a serialized object of HealthIssue or AgentDiskLevelHealthIssue
typedef std::map<std::string, std::string> HealthIssueMap_t;

class SourceAgentProtectionPairHealthIssuesMap
{
public:
    //the second param in the map is a HealthIssue object serialized as string in VMLevelHIsMap
    HealthIssueMap_t VMLevelHIsMap;

    //std::The second param in the map is AgentDiskLevelHealthIssue object serialized as string in DiskLevelHIsMap
    HealthIssueMap_t DiskLevelHIsMap;
 

    /// \brief a serializer method for JSON
    void serialize(JSON::Adapter& adapter)
    {
        JSON::Class root(adapter, "SourceAgentProtectionpairHealthIssuesMap", false);
        JSON_E(adapter, VMLevelHIsMap);
        JSON_T(adapter, DiskLevelHIsMap);
    }
};

class OnPremToAzureSourceAgentDiskHealthMsg
{
public:
    std::string                 ReplicationPairContext;
    double                      DataPendingAtSourceAgentInMB;
    std::vector<HealthIssue>    HealthIssues;

    /// \brief a serializer method for JSON
    void serialize(JSON::Adapter& adapter)
    {
        JSON::Class root(adapter, "OnPremToAzureSourceAgentDiskHealthMsg", false);
        JSON_E(adapter, ReplicationPairContext);
        JSON_E(adapter, DataPendingAtSourceAgentInMB);
        JSON_T(adapter, HealthIssues);
    }
};

class OnPremToAzureSourceAgentVmHealthMsg
{
public:
    std::string                                         ProtectionPairContext;
    std::vector<HealthIssue>                            HealthIssues;
    std::vector<OnPremToAzureSourceAgentDiskHealthMsg>  DiskHealthDetails;

    // \brief a serializer method for JSON
    void serialize(JSON::Adapter& adapter)
    {
        JSON::Class root(adapter, "OnPremToAzureSourceAgentVmHealthMsg", false);
        JSON_E(adapter, ProtectionPairContext);
        JSON_E(adapter, HealthIssues);
        JSON_T(adapter, DiskHealthDetails);
    }
};
#endif
