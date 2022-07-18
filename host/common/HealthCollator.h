#ifndef __HEALTHCOLLATOR_H
#define __HEALTHCOLLATOR_H

#include<string>

#include<boost/shared_ptr.hpp>
#include<boost/atomic.hpp>
#include<boost/thread.hpp>
#include<boost/chrono.hpp>
#include<boost/thread/mutex.hpp>

#include "AgentHealthContract.h"

#define COMBINE_DISKID_WITH_HEALTH_ISSUE_CODE(D,H,K)  K = (D + std::string("_") + H);

namespace NSHealthCollator
{
    const std::string S2Prefix = std::string("s2");
    const std::string DPPrefix = std::string("dp");
    const std::string UnderScore = std::string("_");
    const std::string ExtJson = std::string(".json");
    const std::string AgentHealth_SubDir = std::string("AgentHealth");
    const int HEALTH_HASH_LENGTH = 16;
}

class HealthCollator
{
private:
    // Mutex for synchronizing setting and resetting of Health Issues at VM and Disk Level.
    mutable boost::recursive_mutex m_hiMutex;

    //The health JSON file used for setting and resetting of Health Issues
    const std::string m_HealthJsonFile;

    //IPC lock file used for serializing acces to Health Issues file
    const std::string m_HealthJsonFileLock;

    //The .JSON file content of Health Issues.
    std::string m_strHealthIssuesAsJsonContent;
    
    //The structure which contains ths VM Level and Disk Level Health Issues in a serialized map format for easy search operations.
    SourceAgentProtectionPairHealthIssuesMap m_allHealthIssuesMap;

    //Initialization method to initially read the existing health issues into memory from JSON file upon service start
    void Init();

    //Write Health Isues as a serialized content
    bool WriteToJsonHealthFile(const std::string& strContent);

    //A retryable wrapper for the functionality to write the serialized health issues content to the file.
    bool WriteToJsonHealthFileWithRetry(const std::string& strContent);

    //Method to read the serialized content of health issues during init.
    void ReadSerializedHealthIssuesFromJsonFile();

    //Method to init the serialized content of health issues.
    bool InitJsonFile();

    //Checks if any VM Level Health Issues found or not
    bool IsHealthIssuePresent(const std::string& healthIssueCode, const HealthIssueMap_t& mapHealthIssues) const;

    /// \brief detelets the health file and its lock file
    void DeleteHealthFile();

    /// \brief
    void RestoreOldState();

public:

    static SV_LOG_LEVEL        s_logLevel;

    //Constructor which takes a JSON file to construct the Health Collator Object
    // TODO: the filepath should be hidden from consuming component of this class.
    // constructor should take only the component-id as input
    HealthCollator(const std::string& strJsonFilePath);
    
    //Destructor
    ~HealthCollator(){}

    //Single VM Level Health Issue Management
    bool SetVMHealthIssue(HealthIssue & vmHealthIssue);
    bool ResetVMHealthIssue(const std::string &vmHealthIssueCode);
    bool ResetVMHealthIssues(const std::vector<std::string>&vHealthIssueCodes);

    //Single Disk Level Health Issue Management
    bool SetDiskHealthIssue(AgentDiskLevelHealthIssue & diskHealthIssue);
    bool ResetDiskHealthIssue(const std::string &diskHealthIssueCode,
        const std::string& strDiskId);
    bool ResetDiskHealthIssues(const std::vector<std::string>&vdiskHealthIssueCodes,
        const std::string& strDiskId);

    // returns all health issues for this instance
    bool GetAllHealthIssues(SourceAgentProtectionPairHealthIssues& healthIssues);

    //Utility method to record the observation time of the Health Issue
    std::string GetCurrentObservationTime();

    //Method to check for the availabilty of health issues
    bool IsThereAnyHealthIssue() const;

    bool IsThereAnyDiskLevelHealthIssue() const;

    //Method to check if a particular health issue is found or not in the json health file
    bool IsThisHealthIssueFound(const std::string& healthIssueCode);

    //Checks if any VM Level Health Issues found or not
    bool IsVMHealthIssuePresent(const std::string& healthIssueCode) const ;

    //Checks if any Disk Level Health Issues found or not
    bool IsDiskHealthIssuePresent(const std::string& healthIssueCode, const std::string& diskId) const;
};

typedef boost::shared_ptr<HealthCollator> HealthCollatorPtr;

#endif