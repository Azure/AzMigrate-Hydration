#ifndef TAG_TELEMETRY_H
#define TAG_TELEMETRY_H

#include <string>
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/iter_find.hpp>

#include "json_reader.h"
#include "json_writer.h"

#include "portablehelpers.h"
#include "Telemetry.h"
#include "TagTelemetryKeys.h"
#include "VacpErrorCodes.h"

using namespace AgentTelemetry;

typedef boost::tokenizer<boost::char_separator<char> > tokenizer_t;

static const std::size_t TAG_GUID_LEN = 36;

namespace TagTelemetry
{
    enum TagFinalStatus {
        TAG_SKIPPED = 1,
        TAG_SUCCEEDED,
        TAG_PRECHECK_FAILED,
        TAG_VACP_SPAWN_FAILED,
        TAG_FAILED,
        TAG_VACP_ABORT_TIMEOUT,
        TAG_VACP_CRASHED
    };

    enum PolicyID {
        POLICY_UNKNOWN = 0,
        POLICY_CRASH = 1,
        POLICY_APP = 2,
        POLICY_BASELINE = 3
    };

    enum TagType {
        SINGLE_NODE = 0,
        CRASH_CONSISTENCY  = 1,
        APP_CONSISTENCY = 2,
        MULTI_NODE = 4,
        BASELINE_CONSISTENCY = 8,
        INVALID_TAGTYPE = 1024
    };

    enum NodeType {
        MULTI_VM_NODE_TYPE_MASTER = 1,
        MULTI_VM_NODE_TYPE_CLIENT
    };

    enum MessageTypes {
        TAG_TABLE_AGENT_LOG = 1 + SOURCE_TELEMETRY_BASE_MESSAGE_TYPE,
        /* MESSAGE_TYPE_LOG_TRUNCATE = 2*/
        TAG_TABLE_AGENT_LOG_V2 = 3 + SOURCE_TELEMETRY_BASE_MESSAGE_TYPE,
        TAG_TABLE_AGENT_LOG_BAD_FILE,
        TAG_TABLE_AGENT_LOG_EMPTY_FILE,
        TAG_TABLE_AGENT_LOG_KEY_ABSENT_IS_CRASH_TAG_KEY, // not used
        TAG_TABLE_AGENT_LOG_KEY_ABSENT_VACP_START_TIME_KEY, // not used
        TAG_TABLE_AGENT_LOG_KEY_ABSENT_VACP_EXIT_STATUS, // not used
        TAG_TABLE_AGENT_LOG_REQUIRED_KEYS_NOT_FOUND = 9,
        TAG_TABLE_AGENT_LOG_APP_RETRY_ON_REBOOT
    };

    const std::string IPADDR("IpAddr");
    const std::string SYSTIMETAGT("SysTime");
    const std::string POLICYID("PolicyId");
    const std::string TAGTYPECONF("TagTypeConf");

    const std::string TAGTYPEINSERT("TagTypeInsert");
    const std::string TAGGUID("TagGuid");
    const std::string TAGNAME("TagName");
    const std::string NUMOFDISKINPOLICY("NumOfDiskInPolicy");
    const std::string DISKIDLISTINPOLICY("DiskIdListInPolicy");
    
    const std::string NODETYPE("NodeType");
    const std::string MASTERNODE("MasterNode");
    const std::string NUMOFNODES("NumOfNodes");
    const std::string CLIENTNODES("ClientNodes");

    const std::string VACPSTARTTIME("VacpStartTime");
    const std::string VACPENDTIME("VacpEndTime");
    const std::string PREPAREELAPSED("PrepareElapsed");
    const std::string FREEZEELAPSED("FreezeElapsed");

    const std::string APPFREEZEELAPSED("AppFreezeElapsed");
    const std::string APPPREPAREELAPSED("AppPrepareElapsed");
    const std::string BARRIERELAPSED("BarrierElapsed");

    const std::string TAGINSERTTIME("TagInsertTime");
    const std::string TAGCOMMITTIME("TagCommitTime");
    const std::string TAGPHASEELAPSED("TagPhaseElapsed");
    const std::string RESUMEELAPSED("ResumeElapsed");

    const std::string FINALSTATUS("FinalStatus");
    const std::string REASON("Reason");
    const std::string MULTIVMSTATUS("MultiVmStatus");
    const std::string MULTIVMREASON("MultiVmReason");

    const std::string NUMOFDISKWITHTAG("NumOfDiskWithTag");
    const std::string FAILEDDISKID("FailedDiskId");
    const std::string LOCALIPS("LocalIPs");

    // Azure to Azure replication specific
    const std::string APPCONSINTERVAL("AppConsistencyInterval");
    const std::string CRASHCONSINTERVAL("CrashConsistencyInterval");

    class TagStatus
    {
    public:

        std::map<std::string, std::string> Map;

        void serialize(JSON::Adapter& adapter)
        {
            JSON::Class root(adapter, "TagStatus", false);

            JSON_T(adapter, Map);
        }
        std::string getValue(std::string key)
        {
            if (Map.end() != Map.find(key)) {
                return Map[key];
            }
            return "";
        }
    };

    // \\\ brief
    // Finds first pattern in the input and finds its value. The input is considered in the following 
    // format. The key and value are seperated by a space. The end of value is identified by a space
    // or expected length.
    // ... <key> <value> ....
    // ... <key><value> ....
    static void GetValueForPropertyKey(const std::string& input, const::std::string& key, std::string &value, std::size_t valueSize = 0)
    {
        if (!key.length())
        {
            DebugPrintf(SV_LOG_ERROR, "%s: key is empty.\n", FUNCTION_NAME);
            return;
        }

        value.clear();
        std::size_t keyPos = input.find(key);
        if (std::string::npos != keyPos)
        {
            keyPos += key.length();
            std::size_t valueStart = input.find_first_not_of(" ", keyPos);

            if (std::string::npos != valueStart)
            {
                std::size_t valueEnd;
                if (0 == valueSize)
                {
                    valueEnd = input.find_first_of(" ", valueStart);
                    if (std::string::npos != valueEnd)
                        valueSize = valueEnd - valueStart;
                    if (valueSize == input.length())
                        valueSize = input.length() - valueStart;
                }

                value = input.substr(valueStart, valueSize);
                boost::trim(value);
            }
        }
    }

    static void GetValueForPropertyKey(tokenizer_t& inputLines, const::std::string& key, std::string &value)
    {
        tokenizer_t::iterator lineIter(inputLines.begin());
        tokenizer_t::iterator lineIterEnd(inputLines.end());
        for (/* empty*/; lineIter != lineIterEnd; ++lineIter) {
            if (boost::algorithm::icontains(*lineIter, key))
            {
                return GetValueForPropertyKey(*lineIter, key, value, lineIter->length());
            }
        }
    }

    static void ParsegVacpFailMsgs(const std::string &strVacpFailMsgs, TagStatus& tagStatus, const uint32_t exitCode)
    {
        DebugPrintf(SV_LOG_DEBUG, "%s: strVacpFailMsgs:%s.\n", FUNCTION_NAME, strVacpFailMsgs.c_str());

        std::vector<std::string> vVacpFailMsg;
        boost::iter_split(vVacpFailMsg, strVacpFailMsgs, boost::first_finder(VACP_FAIL_MSG_DELM));

        BOOST_FOREACH(std::string strVFMsg, vVacpFailMsg)
        {
            std::vector<std::string> vVFMsgContent;
            boost::iter_split(vVFMsgContent, strVFMsg, boost::first_finder(FAIL_MSG_DELM));
            unsigned long failureModule = 0;
            try
            {
                failureModule = boost::lexical_cast<unsigned long>(vVFMsgContent[0]);
            }
            catch (const boost::bad_lexical_cast &e)
            {
                std::stringstream ss;
                ss << "Caught exception in parsing vacp failureModule: " << e.what();
                DebugPrintf(SV_LOG_ERROR, "%s: %s.\n", FUNCTION_NAME, ss.str().c_str());
            }
            
            if (VACP_MODULE_INFO != failureModule)
            {
                unsigned long errorCode = 0;
                try
                {
                    errorCode = boost::lexical_cast<unsigned long>(vVFMsgContent[2]);
                }
                catch (const boost::bad_lexical_cast &e)
                {
                    std::stringstream ss;
                    ss << "Caught exception in parsing vacp errorCode: " << e.what();
                    DebugPrintf(SV_LOG_ERROR, "%s: %s.\n", FUNCTION_NAME, ss.str().c_str());
                }
                DebugPrintf(SV_LOG_DEBUG, "%s: failureModule:%lu, exitCode=%lu, errorCode:%lu.\n", FUNCTION_NAME, failureModule, exitCode, errorCode);
                if (errorCode != exitCode)
                {
                    ADD_INT_TO_MAP(tagStatus, REASON, errorCode);
                }
                break;
            }
        }
        std::string strMessage = strVacpFailMsgs;
        if (!tagStatus.Map[MESSAGE].empty())
        {
            strMessage += "\n";
            strMessage += tagStatus.Map[MESSAGE];
        }
        ADD_STRING_TO_MAP(tagStatus, MESSAGE, strMessage);
    }

    static void ParsegMultivmFailMsgs(const std::string &strMultivmFailMsgs, TagStatus& tagStatus)
    {
        DebugPrintf(SV_LOG_DEBUG, "%s: strMultivmFailMsgs:%s.\n", FUNCTION_NAME, strMultivmFailMsgs.c_str());

        std::vector<std::string> vMultivmFailMsg;
        boost::iter_split(vMultivmFailMsg, strMultivmFailMsgs, boost::first_finder(MULTIVM_FAIL_MSG_DELM));

        BOOST_FOREACH(std::string strMFMsg, vMultivmFailMsg)
        {
            std::vector<std::string> vMFMsgContent;
            boost::iter_split(vMFMsgContent, strMFMsg, boost::first_finder(FAIL_MSG_DELM));
            unsigned long mvFailureModule = 0;
            unsigned long mvErrorCode = 0;
            try
            {
                mvFailureModule = boost::lexical_cast<unsigned long>(vMFMsgContent[0]);
            }
            catch (const boost::bad_lexical_cast &e)
            {
                std::stringstream ss;
                ss << "Caught exception in parsing mvFailureModule: " << e.what();
                DebugPrintf(SV_LOG_ERROR, "%s: %s.\n", FUNCTION_NAME, ss.str().c_str());
            }

            try
            {
                mvErrorCode = boost::lexical_cast<unsigned long>(vMFMsgContent[2]);
            }
            catch (const boost::bad_lexical_cast &e)
            {
                std::stringstream ss;
                ss << "Caught exception in parsing mvErrorCode: " << e.what();
                DebugPrintf(SV_LOG_ERROR, "%s: %s.\n", FUNCTION_NAME, ss.str().c_str());
            }
            DebugPrintf(SV_LOG_DEBUG, "%s: mvFailureModule:%lu, mvErrorCode=%lu.\n", FUNCTION_NAME, mvFailureModule, mvErrorCode);
            ADD_INT_TO_MAP(tagStatus, MULTIVMSTATUS, mvFailureModule);

            std::stringstream ssMVReason;
            ssMVReason << vMFMsgContent[1] << ":" << mvErrorCode;
            if (!tagStatus.Map[MULTIVMREASON].empty())
            {
                ssMVReason << " ";
                ssMVReason << tagStatus.Map[MULTIVMREASON];
            }
            ADD_STRING_TO_MAP(tagStatus, MULTIVMREASON, ssMVReason.str());
            break;
        }
    }

    static void ParseCmdOutput(const std::string& cmdOutput, TagStatus& tagStatus, const SV_ULONG &exitCode)
    {

        std::string strValue;

        GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::TagGuidKey, strValue, TAG_GUID_LEN);
        ADD_STRING_TO_MAP(tagStatus, TAGGUID, strValue);

        GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::TagInsertKey, strValue);
        ADD_STRING_TO_MAP(tagStatus, NUMOFDISKWITHTAG, strValue);

        std::string tagName;

        uint32_t tagTypeInsert = INVALID_TAGTYPE;
        std::string strTagVal;
        GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::TagType, strTagVal);
        if (0 == strTagVal.compare(BASELINE_TAG))
        {
            tagTypeInsert = BASELINE_CONSISTENCY;
            GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::BaselineTagKey, tagName);
            ADD_STRING_TO_MAP(tagStatus, TAGNAME, tagName);
        }
        else
        {
            GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::TagKey, tagName, NsVacpOutputKeys::TIMESTAMP_LEN);
            if (std::string::npos != cmdOutput.find(NsVacpOutputKeys::FsTagKey))
            {
                tagTypeInsert = APP_CONSISTENCY;
                tagName = std::string("FileSystem") + tagName;

                ADD_STRING_TO_MAP(tagStatus, TAGNAME, tagName);
            }

            if (std::string::npos != cmdOutput.find(NsVacpOutputKeys::CcTagKey))
            {
                tagTypeInsert = CRASH_CONSISTENCY;
                tagName = std::string("CrashTag") + tagName;
                ADD_STRING_TO_MAP(tagStatus, TAGNAME, tagName);
            }

            if (std::string::npos != cmdOutput.find(NsVacpOutputKeys::LocalTagKey))
                tagTypeInsert |= SINGLE_NODE;

            if (std::string::npos != cmdOutput.find(NsVacpOutputKeys::MultivmTagKey))
                tagTypeInsert |= MULTI_NODE;
        }

        ADD_INT_TO_MAP(tagStatus, TAGTYPEINSERT, tagTypeInsert);

        GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::TagInsertTimeKey, strValue);
        ADD_STRING_TO_MAP(tagStatus, TAGINSERTTIME, strValue);

        if (CRASH_CONSISTENCY == (tagTypeInsert & CRASH_CONSISTENCY))
        {
            GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::IoBarrierKey, strValue);
            ADD_STRING_TO_MAP(tagStatus, APPFREEZEELAPSED, strValue);
        }
        else if (APP_CONSISTENCY == (tagTypeInsert & APP_CONSISTENCY))
        {
            GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::TagCommitTimeKey, strValue);
            ADD_STRING_TO_MAP(tagStatus, TAGCOMMITTIME, strValue);

            GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::AppQuiesceKey, strValue);
            ADD_STRING_TO_MAP(tagStatus, APPFREEZEELAPSED, strValue);

            GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::DrainBarrierKey, strValue);
            ADD_STRING_TO_MAP(tagStatus, BARRIERELAPSED, strValue);

            GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::AppPrepPhKey, strValue);
            ADD_STRING_TO_MAP(tagStatus, APPPREPAREELAPSED, strValue);
        }

        if (MULTI_NODE == (tagTypeInsert & MULTI_NODE))
        {
            GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::PrepPhKey, strValue);
            ADD_STRING_TO_MAP(tagStatus, PREPAREELAPSED, strValue);
            GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::TagPhKey, strValue);
            ADD_STRING_TO_MAP(tagStatus, TAGPHASEELAPSED, strValue);
            GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::QuiescePhKey, strValue);
            ADD_STRING_TO_MAP(tagStatus, FREEZEELAPSED, strValue);
            GetValueForPropertyKey(cmdOutput, NsVacpOutputKeys::ResumePhKey, strValue);
            ADD_STRING_TO_MAP(tagStatus, RESUMEELAPSED, strValue);
        }

        ADD_INT_TO_MAP(tagStatus, REASON, exitCode);

        if (0 == exitCode)
        {
            ADD_INT_TO_MAP(tagStatus, FINALSTATUS, TAG_SUCCEEDED);
        }
        else if (VACP_ONE_OR_MORE_DISK_IN_IR == exitCode)
        {
            ADD_INT_TO_MAP(tagStatus, FINALSTATUS, TAG_SKIPPED);
        }
        else if (VACP_E_PREPARE_PHASE_FAILED == exitCode)
        {
            ADD_INT_TO_MAP(tagStatus, FINALSTATUS, TAG_PRECHECK_FAILED);
        }
        else if (std::string::npos != cmdOutput.find(NsVacpOutputKeys::VacpSpawnFailKey))
        {
            ADD_INT_TO_MAP(tagStatus, FINALSTATUS, TAG_VACP_SPAWN_FAILED);
        }
        else if (std::string::npos != cmdOutput.find(NsVacpOutputKeys::VacpAbortKey))
        {
            ADD_INT_TO_MAP(tagStatus, FINALSTATUS, TAG_VACP_ABORT_TIMEOUT);
        }
        else
        {
            ADD_INT_TO_MAP(tagStatus, FINALSTATUS, TAG_FAILED);
        }

        int first = 0;
        int len = 0;
        if (std::string::npos != (first = cmdOutput.find(vacpFailMsgsTok)))
        {
            first += vacpFailMsgsTok.length();
            if (std::string::npos != (len = cmdOutput.rfind(VACP_FAIL_MSG_DELM)))
            {
                len -= first;
                ParsegVacpFailMsgs(cmdOutput.substr(first, len), tagStatus, exitCode);
            }
        }
        else if (0 != exitCode)
        {
            DebugPrintf(SV_LOG_ERROR, "%s: vacp failed but token \"%s\" not found in vacp cmdOutput, exitCode=%d.\n", FUNCTION_NAME, vacpFailMsgsTok.c_str(), exitCode);
        }

        if (boost::lexical_cast<uint32_t>(tagStatus.Map[TAGTYPECONF]) & MULTI_NODE)
        {
            if (std::string::npos != cmdOutput.find(NsVacpOutputKeys::MasterNodeRoleKey))
            {
                ADD_INT_TO_MAP(tagStatus, NODETYPE, MULTI_VM_NODE_TYPE_MASTER);
            }
            else if (std::string::npos != cmdOutput.find(NsVacpOutputKeys::ClientNodeRoleKey))
            {
                ADD_INT_TO_MAP(tagStatus, NODETYPE, MULTI_VM_NODE_TYPE_CLIENT);
            }

            // Check multivm failure here as exitCode is 0 when multivm fails and local tag successful
            std::size_t first = cmdOutput.find(multivmFailMsgsTok);
            if (std::string::npos != first)
            {
                std::size_t end = cmdOutput.rfind(MULTIVM_FAIL_MSG_DELM);
                first += multivmFailMsgsTok.length();
                if (std::string::npos != end)
                {
                    ParsegMultivmFailMsgs(cmdOutput.substr(first, end - first), tagStatus);
                }
            }
        }
    }

}


#endif
