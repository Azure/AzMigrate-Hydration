#include<iostream>
#include<sstream>
#include<fstream>
#include<boost/filesystem.hpp>

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>

#include<boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

#include "logger.h"
#include "inmageex.h"
#include "portablehelpers.h"

#include "HealthCollator.h"

#define HEALTH_COLLATOR_CATCH_EXCEPTION(errMsg,status) \
    catch (const JSON::json_exception& jsone) \
    { \
        errMsg += std::string("JSON parser failed with exception: ") + jsone.what(); \
        status = false; \
        DebugPrintf(SV_LOG_ERROR,"%s: %s",FUNCTION_NAME,errMsg.c_str()); \
    } \
    catch (const std::exception &e) \
    { \
        errMsg += std::string("Standard Exception occurred:") + e.what(); \
        status = false; \
        DebugPrintf(SV_LOG_ERROR,"%s: %s",FUNCTION_NAME,errMsg.c_str()); \
    } \
    catch (...) \
    { \
        errMsg += std::string("Generic Exception occurred."); \
        status = false; \
        DebugPrintf(SV_LOG_ERROR,"%s: %s",FUNCTION_NAME,errMsg.c_str()); \
    }

// can change to SV_LOG_ALWAYS for debugging
SV_LOG_LEVEL HealthCollator::s_logLevel = SV_LOG_DEBUG;

HealthCollator::HealthCollator(const std::string& strHealthJsonFile)
    :m_HealthJsonFile(strHealthJsonFile),
    m_HealthJsonFileLock(strHealthJsonFile + std::string(".lck"))
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    Init();
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
}

void HealthCollator::Init()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    bool bStatus = true;
    std::string errMsg;

    boost::filesystem::path healthFilePath = m_HealthJsonFile;
    boost::system::error_code ec;
    // if the file is not found, there are no previous health issues
    if (!boost::filesystem::exists(healthFilePath.c_str(), ec))
    {
        DebugPrintf(s_logLevel, "%s: no previous health file %s found.\n", FUNCTION_NAME, m_HealthJsonFile.c_str());
        return;
    }

    //If already a health file is there with health issues then load them into memory
    ReadSerializedHealthIssuesFromJsonFile();

    if (m_strHealthIssuesAsJsonContent.empty())
    {
        DebugPrintf(s_logLevel, "%s: previous health file %s found is empty.\n", FUNCTION_NAME, m_HealthJsonFile.c_str());
        DeleteHealthFile();
        return;
    }

    // load any existing health issues
    try
    {
        m_allHealthIssuesMap = JSON::consumer<SourceAgentProtectionPairHealthIssuesMap>::convert(m_strHealthIssuesAsJsonContent);

        if (m_allHealthIssuesMap.VMLevelHIsMap.empty() && m_allHealthIssuesMap.DiskLevelHIsMap.empty())
        {
            DebugPrintf(s_logLevel, "%s: previous health file %s found has no health issues.\n", FUNCTION_NAME, m_HealthJsonFile.c_str());
            DeleteHealthFile();
        }
    }
    HEALTH_COLLATOR_CATCH_EXCEPTION(errMsg, bStatus);

    if (!bStatus)
    {
        DebugPrintf(SV_LOG_ERROR,"%s: deleting file %s as failed to deserialize existing health issues from file.\n",
            FUNCTION_NAME, m_HealthJsonFile.c_str());
        DeleteHealthFile();
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return;
}

void HealthCollator::ReadSerializedHealthIssuesFromJsonFile()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    std::string strException;
    //Use file lock to read the content
    std::ofstream lckFile(m_HealthJsonFileLock.c_str(), std::ofstream::out);
    if (!lckFile.good())
    {
        strException = "failed to create lock file.";
        throw INMAGE_EX(strException.c_str())(m_HealthJsonFileLock.c_str())(errno);
    }

    //Create a file lock object for the file
    boost::interprocess::file_lock flock(m_HealthJsonFileLock.c_str());

    //Assign a scoped lock object for the above file lock object
    boost::interprocess::sharable_lock<boost::interprocess::file_lock> sharableFlock(flock);

    //Read the file content
    std::ifstream hlthJsonFile(m_HealthJsonFile.c_str(), std::ifstream::in);
    if (!hlthJsonFile.good())
    {
        strException = "failed to read file.";
        throw INMAGE_EX(strException.c_str())(m_HealthJsonFileLock.c_str())(errno);
    }

    std::stringstream ssJsonContent;
    ssJsonContent << hlthJsonFile.rdbuf();
    hlthJsonFile.close();

    m_strHealthIssuesAsJsonContent = ssJsonContent.str();

    DebugPrintf(s_logLevel, "%s: Read the following Health Issues from the file %s as JSON content:%s\n",
        FUNCTION_NAME, m_HealthJsonFile.c_str(), m_strHealthIssuesAsJsonContent.c_str());

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return;
}

void HealthCollator::DeleteHealthFile()
{
    DebugPrintf(s_logLevel, "ENTERED %s: %s\n", FUNCTION_NAME, m_HealthJsonFile.c_str());

    int nRetryCounter = 0;
    // retry 3 times on any exception to delete the file
    while (nRetryCounter < 3)
    {
        try {
            if (boost::filesystem::exists(m_HealthJsonFile))
                boost::filesystem::remove(m_HealthJsonFile);

            if (boost::filesystem::exists(m_HealthJsonFileLock))
                boost::filesystem::remove(m_HealthJsonFileLock);

            break;
        }
        catch (const std::exception& ex)
        {
            DebugPrintf(SV_LOG_ERROR, "%s: caught excetpion %s\n", FUNCTION_NAME, ex.what());
            nRetryCounter++;
            boost::this_thread::sleep(boost::posix_time::seconds(1));
        }
        catch (...)
        {
            DebugPrintf(SV_LOG_ERROR, "%s: caught unknown excetpion\n", FUNCTION_NAME);
            nRetryCounter++;
            boost::this_thread::sleep(boost::posix_time::seconds(1));
        }
    }

    DebugPrintf(s_logLevel, "EXITED %s\n", FUNCTION_NAME);
}

bool HealthCollator::GetAllHealthIssues(SourceAgentProtectionPairHealthIssues& healthIssues)
{
    DebugPrintf(s_logLevel, "ENTERED %s\n", FUNCTION_NAME);

    boost::recursive_mutex::scoped_lock slock(m_hiMutex);

    bool bStatus = false;
    std::string errMsg;

    try {
        HealthIssueMap_t::const_iterator vmHIIter = m_allHealthIssuesMap.VMLevelHIsMap.begin();
        for (; vmHIIter != m_allHealthIssuesMap.VMLevelHIsMap.end(); vmHIIter++)
        {
            HealthIssue hi = JSON::consumer<HealthIssue>::convert(vmHIIter->second);
            healthIssues.HealthIssues.push_back(hi);
        }

        HealthIssueMap_t::const_iterator diskHIIter = m_allHealthIssuesMap.DiskLevelHIsMap.begin();
        for (; diskHIIter != m_allHealthIssuesMap.DiskLevelHIsMap.end(); diskHIIter++)
        {
            AgentDiskLevelHealthIssue diskHi = JSON::consumer<AgentDiskLevelHealthIssue>::convert(diskHIIter->second);
            healthIssues.DiskLevelHealthIssues.push_back(diskHi);
        }

        bStatus = true;
    }
    catch (const std::exception &ex)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: failed with exception: %s.\n", FUNCTION_NAME, ex.what());
    }
    catch (...)
    {
        DebugPrintf(SV_LOG_ERROR, "%s failed with an unknown exception.\n", FUNCTION_NAME);
    }

    DebugPrintf(s_logLevel, "EXITED %s\n", FUNCTION_NAME);
    return bStatus;
}

bool HealthCollator::SetVMHealthIssue(HealthIssue& vmHealthIssue)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    if (vmHealthIssue.IssueCode.empty())
    {
        DebugPrintf(SV_LOG_ERROR, "%s: the health issue code is empty.\n", FUNCTION_NAME);
        return false;
    }

    //mutex to synchronize the set and reset updates of health issues from across the process
    boost::recursive_mutex::scoped_lock slock(m_hiMutex);

    std::string errMsg;
    bool bPersisted = false;
    try
    {
        //Check if the same health issue code is already there, then don't overwrite the file
        //as we want to preserve observed time
        if (!IsHealthIssuePresent(vmHealthIssue.IssueCode, m_allHealthIssuesMap.VMLevelHIsMap))
        {
            //else if a new health issue is observed, add this as a new health issue
            std::string strSerializedHealthIssue = JSON::producer<HealthIssue>::convert(vmHealthIssue);
            m_allHealthIssuesMap.VMLevelHIsMap[vmHealthIssue.IssueCode] = strSerializedHealthIssue;

            std::string strContent =
                JSON::producer<SourceAgentProtectionPairHealthIssuesMap>::convert(m_allHealthIssuesMap);

            //and persist to the .json health file
            RemoveChar(strContent, '\r');
            RemoveChar(strContent, '\n');
            bPersisted = WriteToJsonHealthFileWithRetry(strContent);
            if (bPersisted)
            {
                m_strHealthIssuesAsJsonContent = strContent;

                DebugPrintf(s_logLevel, "%s:  A new VM Level Health Issue Code %s is added. file contents: %s\n",
                    FUNCTION_NAME, vmHealthIssue.IssueCode.c_str(), m_strHealthIssuesAsJsonContent.c_str());
            }
        }
        else
        {
            bPersisted = true;
            DebugPrintf(s_logLevel, "%s: existing VM Level Health Issue Code %s not added. file contents: %s\n",
                FUNCTION_NAME, vmHealthIssue.IssueCode.c_str(), m_strHealthIssuesAsJsonContent.c_str());
        }
    }
    HEALTH_COLLATOR_CATCH_EXCEPTION(errMsg, bPersisted);

    if (!bPersisted)
    {
        DebugPrintf(SV_LOG_ERROR, "%s:Failed to write the Health Issue %s to the file. restoring to old state.\n",
            FUNCTION_NAME, vmHealthIssue.IssueCode.c_str());

        RestoreOldState();
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bPersisted;
}

void HealthCollator::RestoreOldState()
{
    DebugPrintf(s_logLevel, "ENTERED %s: %s\n", FUNCTION_NAME, m_HealthJsonFile.c_str());

    std::string errMsg;
    bool bStatus = false;
    try {
        if (!m_strHealthIssuesAsJsonContent.empty())
        {
            m_allHealthIssuesMap = JSON::consumer< SourceAgentProtectionPairHealthIssuesMap>::convert(m_strHealthIssuesAsJsonContent);
        }

        DebugPrintf(s_logLevel, "%s: file %s, content %s.\n",
            FUNCTION_NAME, m_HealthJsonFile.c_str(), m_strHealthIssuesAsJsonContent.c_str());
    }
    HEALTH_COLLATOR_CATCH_EXCEPTION(errMsg, bStatus);

    DebugPrintf(s_logLevel, "EXITED %s\n", FUNCTION_NAME);
    return;
}

bool HealthCollator::ResetVMHealthIssue(const std::string &vmHealthIssueCode)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    if (vmHealthIssueCode.empty())
    {
        DebugPrintf(SV_LOG_ERROR, "%s: HealthIssue Code is empty.\n", FUNCTION_NAME);
        return false;
    }
    std::vector<std::string>VMHealthIssues;
    VMHealthIssues.push_back(vmHealthIssueCode);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return ResetVMHealthIssues(VMHealthIssues);
}

bool HealthCollator::ResetVMHealthIssues(const std::vector<std::string>&vHealthIssueCodes)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    if (vHealthIssueCodes.empty())
    {
        DebugPrintf(SV_LOG_ERROR, "%s: no HealthIssue Codes supplied.\n", FUNCTION_NAME);
        return false;
    }

    bool bFound = false;
    std::string errMsg;
    bool bPersisted = false;

    //synchronize with a scoped lock mutex
    boost::recursive_mutex::scoped_lock slock(m_hiMutex);

    try {

        std::map<std::string, std::string> tempVMHealthIssuesMap(m_allHealthIssuesMap.VMLevelHIsMap.begin(), m_allHealthIssuesMap.VMLevelHIsMap.end());
        std::map<std::string, std::string >::iterator mapIter;
        std::vector<std::string>::const_iterator resetIter;
        for (resetIter=vHealthIssueCodes.begin(); resetIter != vHealthIssueCodes.end(); resetIter++)
        {
            mapIter = m_allHealthIssuesMap.VMLevelHIsMap.find(*resetIter);
            if (mapIter != m_allHealthIssuesMap.VMLevelHIsMap.end())
            {
                bFound = true;
                m_allHealthIssuesMap.VMLevelHIsMap.erase(mapIter);
            }
        }

        if (!bFound)
        {
            DebugPrintf(s_logLevel, "EXITED %s as no health issue code found.\n", FUNCTION_NAME);
            return true;
        }

        if (m_allHealthIssuesMap.VMLevelHIsMap.empty() && m_allHealthIssuesMap.DiskLevelHIsMap.empty())
        {
            DebugPrintf(s_logLevel, "%s: deleting health file as there are no other health issues.\n", FUNCTION_NAME);
            m_strHealthIssuesAsJsonContent.clear();
            DeleteHealthFile();
            DebugPrintf(s_logLevel, "EXITED %s as health file is deleted.\n", FUNCTION_NAME);
            return true;
        }

        //get the serialized health content
        std::string strContent = JSON::producer<SourceAgentProtectionPairHealthIssuesMap>::convert(m_allHealthIssuesMap);

        //Write to the .json health file
        RemoveChar(strContent, '\r');
        RemoveChar(strContent, '\n');
        //even if this below write fails, the health issue will be cleaned in memory so that next write is attempted later and it can succeed
        bPersisted = WriteToJsonHealthFileWithRetry(strContent);
        if (bPersisted)
        {
            m_strHealthIssuesAsJsonContent = strContent;
        }
    }
    HEALTH_COLLATOR_CATCH_EXCEPTION(errMsg, bPersisted);

    if (!bPersisted)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: failed to persist to file, restoring to previous state.\n", FUNCTION_NAME);

        RestoreOldState();
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bPersisted;
}

bool HealthCollator::SetDiskHealthIssue(AgentDiskLevelHealthIssue& diskHealthIssue)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    if (diskHealthIssue.DiskContext.empty() || diskHealthIssue.IssueCode.empty())
    {
        DebugPrintf(SV_LOG_ERROR, "%s: Either DiskContext = \"%s\" or Disk Health Issue code = \"%s\" is empty. Cannot set disk health issue.\n",
            FUNCTION_NAME, diskHealthIssue.DiskContext.c_str(), diskHealthIssue.IssueCode.c_str());
        return false;
    }

    boost::recursive_mutex::scoped_lock slock(m_hiMutex);
    std::string errMsg;
    bool bPersisted = false;

    std::string strUniqDiskHlthIssue;
    COMBINE_DISKID_WITH_HEALTH_ISSUE_CODE(diskHealthIssue.DiskContext, diskHealthIssue.IssueCode, strUniqDiskHlthIssue);

    try {
        //Check if the same health issue code is already there, then don't overwrite the file as we want to preserve observed time
        if (!IsHealthIssuePresent(strUniqDiskHlthIssue, m_allHealthIssuesMap.DiskLevelHIsMap))
        {
            std::string strSerializedDiskLevelHealthIssue = JSON::producer<AgentDiskLevelHealthIssue>::convert(diskHealthIssue);
            m_allHealthIssuesMap.DiskLevelHIsMap[strUniqDiskHlthIssue] = strSerializedDiskLevelHealthIssue;

            std::string strContent =
                JSON::producer<SourceAgentProtectionPairHealthIssuesMap>::convert(m_allHealthIssuesMap);

            //and persist to the .json health file
            RemoveChar(strContent, '\r');
            RemoveChar(strContent, '\n');
            bPersisted = WriteToJsonHealthFileWithRetry(strContent);
            if (bPersisted)
            {
                m_strHealthIssuesAsJsonContent = strContent;
                DebugPrintf(s_logLevel, "%s:A new Disk Level Health Issue Code %s for disk %s is added. %s\n",
                    FUNCTION_NAME, diskHealthIssue.IssueCode.c_str(), diskHealthIssue.DiskContext.c_str(), strSerializedDiskLevelHealthIssue.c_str());
            }
        }
        else
        {
            //same health issue occurred
            bPersisted = true;
            DebugPrintf(s_logLevel, "%s: Health Issue %s for %s - already exists.\n", FUNCTION_NAME,
                diskHealthIssue.IssueCode.c_str(), diskHealthIssue.DiskContext.c_str());
        }
    }
    HEALTH_COLLATOR_CATCH_EXCEPTION(errMsg, bPersisted);

    if (!bPersisted)
    {
        //remove the disk health issue from the map which was added just above
        DebugPrintf(SV_LOG_ERROR, "%s: failed to persiste the disk health issue %s. restoring to old state.\n",
            FUNCTION_NAME, strUniqDiskHlthIssue.c_str());

        RestoreOldState();
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bPersisted;
}

bool HealthCollator::ResetDiskHealthIssue(const std::string &diskHealthIssueCode, const std::string& strDiskId)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    std::vector<std::string>vDiskIssues;
    if (diskHealthIssueCode.empty() || strDiskId.empty())
    {
        DebugPrintf(SV_LOG_ERROR, "%s:Either DiskId = %s or Disk Health Issue code = %s is empty.\n",
            FUNCTION_NAME,strDiskId.c_str(),diskHealthIssueCode.c_str());
        return false;
    }
    vDiskIssues.push_back(diskHealthIssueCode);

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return ResetDiskHealthIssues(vDiskIssues, strDiskId);
}

bool HealthCollator::ResetDiskHealthIssues(const std::vector<std::string>&vdiskHealthIssueCodes,
    const std::string& strDiskId)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    if (vdiskHealthIssueCodes.empty()||strDiskId.empty())
    {
        DebugPrintf(SV_LOG_ERROR, "%s:Either the health issues list count = %d is zero or the diskid = %s is empty.\n",
            FUNCTION_NAME,vdiskHealthIssueCodes.size(),strDiskId.c_str());
        return false;
    }

    boost::recursive_mutex::scoped_lock slock(m_hiMutex);

    bool bFound = false;
    std::string errMsg;
    bool bPersisted = false;
    try
    {
        const std::map < std::string, std::string>tempDiskLevelHealthIssuesMap(m_allHealthIssuesMap.DiskLevelHIsMap.begin(), m_allHealthIssuesMap.DiskLevelHIsMap.end());
        std::map<const std::string, std::string>::iterator diskIssuesMapIter;
        std::vector<std::string>::const_iterator resetIter = vdiskHealthIssueCodes.begin();
        std::string strUniqDiskHlthIssue;
        for (; resetIter != vdiskHealthIssueCodes.end(); resetIter++)
        {
            strUniqDiskHlthIssue = "";
            COMBINE_DISKID_WITH_HEALTH_ISSUE_CODE(strDiskId, *resetIter, strUniqDiskHlthIssue);

            diskIssuesMapIter = m_allHealthIssuesMap.DiskLevelHIsMap.find(strUniqDiskHlthIssue);
            if (diskIssuesMapIter != m_allHealthIssuesMap.DiskLevelHIsMap.end())
            {
                m_allHealthIssuesMap.DiskLevelHIsMap.erase(diskIssuesMapIter);
                bFound = true;
            }
        }

        if (!bFound)
        {
            DebugPrintf(s_logLevel, "EXITED %s as no health issue code found.\n", FUNCTION_NAME);
            return true;
        }

        if (m_allHealthIssuesMap.VMLevelHIsMap.empty() && m_allHealthIssuesMap.DiskLevelHIsMap.empty())
        {
            DebugPrintf(s_logLevel, "%s: deleting health file as there are no other health issues.\n", FUNCTION_NAME);
            m_strHealthIssuesAsJsonContent.clear();
            DeleteHealthFile();
            DebugPrintf(s_logLevel, "EXITED %s as health file is deleted.\n", FUNCTION_NAME);
            return true;
        }

        //get the serialized health content
        std::string strContent =
            JSON::producer<SourceAgentProtectionPairHealthIssuesMap>::convert(m_allHealthIssuesMap);

        //Write to the .json health file
        RemoveChar(strContent, '\r');
        RemoveChar(strContent, '\n');
        bPersisted = WriteToJsonHealthFileWithRetry(strContent);
        if (bPersisted)
        {
            m_strHealthIssuesAsJsonContent = strContent;
        }
    }
    HEALTH_COLLATOR_CATCH_EXCEPTION(errMsg, bPersisted);

    if (!bPersisted)
    {
        DebugPrintf(SV_LOG_ERROR,
            "%s: failed to persist the Disk Health Issues to file, restoring the old state.\n",
            FUNCTION_NAME);

        RestoreOldState();
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return (bPersisted);
}

bool HealthCollator::WriteToJsonHealthFileWithRetry(const std::string& strContent)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    int nRetryCounter = 0;
    bool bStatus = false;
    //Retrying for 3 times
    while (!bStatus && (nRetryCounter < 3))
    {
        if (bStatus = WriteToJsonHealthFile(strContent))
        {
            break;
        }
        else
        {
            nRetryCounter++;
            boost::this_thread::sleep(boost::posix_time::seconds(1));
        }
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bStatus;
}

bool HealthCollator::WriteToJsonHealthFile(const std::string& strContent)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    bool bStatus = false;
    try
    {
        boost::filesystem::path healthFilePath = m_HealthJsonFile;
        boost::system::error_code ec;

        //If the file is not found, then create the .json file
        if (!boost::filesystem::exists(healthFilePath.c_str(), ec))
        {
            DebugPrintf(s_logLevel, "%s: creating new health file %s.\n", FUNCTION_NAME, m_HealthJsonFile.c_str());

            if (!InitJsonFile())
            {
                DebugPrintf(SV_LOG_ERROR, "%s: failed to initialize new file %s.\n", FUNCTION_NAME, m_HealthJsonFile.c_str());
                return false;
            }
        }

        //Prepare to write the serialized content into a json health file
        std::ofstream lckFile(m_HealthJsonFileLock.c_str(), std::ofstream::out);
        if (!lckFile.good())
        {
            DebugPrintf(SV_LOG_ERROR, "%s:Failed to create lock file %s with error 0x%x\n",
                FUNCTION_NAME, m_HealthJsonFileLock.c_str(), errno);
            return false;
        }

        //Create a file lock object for the file
        boost::interprocess::file_lock flock(m_HealthJsonFileLock.c_str());

        //Assign a scoped lock object for the above file lock object
        boost::interprocess::scoped_lock<boost::interprocess::file_lock> slock(flock);

        //Now write the serialized content to the actual health json file
        std::ofstream healthJsonFile(m_HealthJsonFile.c_str(), std::ofstream::out);
        if (!healthJsonFile.good())
        {
            DebugPrintf(SV_LOG_ERROR, "%s:Failed to create the file %s with error 0x%x\n",
                FUNCTION_NAME, m_HealthJsonFile.c_str(), errno);
            healthJsonFile.close();
            return false;
        }

        //if file status is good, then dump the serialized content into the file
        healthJsonFile << strContent;

        if (healthJsonFile.good())
        {
            bStatus = true;
        }
        else
        {
            bStatus = false;
        }
        healthJsonFile.close();

        DebugPrintf(s_logLevel, "%s:Health Issues written to the file: %s :%s\n",
            FUNCTION_NAME, m_HealthJsonFile.c_str(), strContent.c_str());
    }
    catch (const boost::interprocess::interprocess_exception &ipc_ex)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: failed with boost::interprocess::interprocess_exception: %s.\n", FUNCTION_NAME, ipc_ex.what());
    }
    catch (const std::exception &ex)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: failed with exception: %s.\n", FUNCTION_NAME, ex.what());
    }
    catch (...)
    {
        DebugPrintf(SV_LOG_ERROR, "%s failed with an unknown exception.\n", FUNCTION_NAME);
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bStatus;
}

bool HealthCollator::InitJsonFile()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    bool bStatus = false;
    try
    {
        boost::filesystem::path healthFilePath = m_HealthJsonFile;
        boost::system::error_code ec;

        if (!boost::filesystem::exists(healthFilePath.parent_path().c_str(), ec))
        {
            DebugPrintf(s_logLevel, "%s:Creating directory %s for AgentHealth.\n", FUNCTION_NAME, healthFilePath.parent_path().c_str());
            if (!boost::filesystem::create_directories(healthFilePath.parent_path(), ec))
            {
                DebugPrintf(SV_LOG_ERROR, "%s:Failed to create directory %s with error code 0x%x. error message : %s\n",
                    FUNCTION_NAME, healthFilePath.parent_path().c_str(), ec.value(), ec.message().c_str());
                return false;
            }
            DebugPrintf(SV_LOG_ALWAYS, "%s: created directory %s\n", FUNCTION_NAME, healthFilePath.parent_path().c_str());
        }

        //Use file lock to create the file or read the content
        std::ofstream lckFile(m_HealthJsonFileLock.c_str(), std::ofstream::out);
        if (!lckFile.good())
        {
            DebugPrintf(SV_LOG_ERROR, "%s:Failed to create lock file %s with error 0x%x\n",
                FUNCTION_NAME, m_HealthJsonFileLock.c_str(), errno);
            return false;
        }

        //Create a file lock object for the file
        boost::interprocess::file_lock flock(m_HealthJsonFileLock.c_str());

        //Assign a scoped lock object for the above file lock object
        boost::interprocess::scoped_lock<boost::interprocess::file_lock> slock(flock);

        std::ofstream healthJsonFile(m_HealthJsonFile.c_str(), std::ofstream::out);
        if (!healthJsonFile.good())
        {
            DebugPrintf(SV_LOG_ERROR, "%s:Failed to create the file %s with error 0x%x\n",
                FUNCTION_NAME, m_HealthJsonFile.c_str(), errno);
            healthJsonFile.close();
            return false;
        }

        //fill the created file with the empty json content
        //{"VMLevelHIsMap":{},"DiskLevelHIsMap":{}}
        SourceAgentProtectionPairHealthIssuesMap sapphm;
        std::string strJsonContent = JSON::producer<SourceAgentProtectionPairHealthIssuesMap>::convert(sapphm);
        healthJsonFile << strJsonContent;
        healthJsonFile.flush();
        healthJsonFile.close();

        bStatus = true;

        DebugPrintf(s_logLevel, "%s: file %s persisted with content %s.\n", FUNCTION_NAME,
            m_HealthJsonFile.c_str(), strJsonContent.c_str());

    }
    catch (const boost::interprocess::interprocess_exception &ipc_ex)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: failed with boost::interprocess::interprocess_exception: %s.\n", FUNCTION_NAME, ipc_ex.what());
    }
    catch (const std::exception &ex)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: failed with exception: %s.\n", FUNCTION_NAME, ex.what());
    }
    catch (...)
    {
        DebugPrintf(SV_LOG_ERROR, "%s failed with an unknown exception.\n", FUNCTION_NAME);
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bStatus;
}

std::string HealthCollator::GetCurrentObservationTime()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    std::stringstream sstrLocalTime;
    boost::posix_time::ptime timeLocal = boost::posix_time::second_clock::universal_time();
    sstrLocalTime << boost::posix_time::to_simple_string(timeLocal);
    sstrLocalTime << " UTC";
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return sstrLocalTime.str();
}

bool HealthCollator::IsHealthIssuePresent(const std::string& healthIssueCode,
    const HealthIssueMap_t& mapHealthIssues) const
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    bool retval = false;
    if (!mapHealthIssues.empty())
    {
        retval = mapHealthIssues.find(healthIssueCode) != mapHealthIssues.end();
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return retval;
}

bool HealthCollator::IsVMHealthIssuePresent(const std::string& healthIssueCode) const
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return IsHealthIssuePresent(healthIssueCode, m_allHealthIssuesMap.VMLevelHIsMap);
}

bool HealthCollator::IsDiskHealthIssuePresent(const std::string& healthIssueCode, const std::string& diskId) const
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    std::string strUniqueDiskHealthIssue;
    COMBINE_DISKID_WITH_HEALTH_ISSUE_CODE(diskId, healthIssueCode, strUniqueDiskHealthIssue);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return IsHealthIssuePresent(strUniqueDiskHealthIssue, m_allHealthIssuesMap.DiskLevelHIsMap);
}
bool HealthCollator::IsThereAnyHealthIssue() const
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    boost::recursive_mutex::scoped_lock slock(m_hiMutex);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return(!(m_allHealthIssuesMap.VMLevelHIsMap.empty()) ||
        !(m_allHealthIssuesMap.DiskLevelHIsMap.empty()));
}

bool HealthCollator::IsThereAnyDiskLevelHealthIssue() const
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    boost::recursive_mutex::scoped_lock slock(m_hiMutex);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return !m_allHealthIssuesMap.DiskLevelHIsMap.empty();
}

bool HealthCollator::IsThisHealthIssueFound(const std::string& healthIssueCode)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    bool bFound = true;
    boost::recursive_mutex::scoped_lock slock(m_hiMutex);
    if (!IsHealthIssuePresent(healthIssueCode, m_allHealthIssuesMap.VMLevelHIsMap) &&
        !IsHealthIssuePresent(healthIssueCode, m_allHealthIssuesMap.DiskLevelHIsMap))
    {
        bFound = false;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bFound;
}
