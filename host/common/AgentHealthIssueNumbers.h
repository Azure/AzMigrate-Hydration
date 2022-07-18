#ifndef _AGENTHEALTH_ISSUE_NUMBERS_H_
#define _AGENTHEALTH_ISSUE_NUMBERS_H_

#include <string>

namespace AgentHealthIssueCodes
{
    const std::string ObservationTime = "ObservationTime";

    namespace VMLevelHealthIssues
    {
        namespace FilterDriverNotLoaded
        {
            const std::string HealthCode = "ECH00024";
        };

        namespace LogUploadLocationNotHealthy
        {
            const std::string HealthCode = "ECH00026";
        };

        namespace VssWriterFailure
        {
            const std::string HealthCode = "ECH00031";
            const std::string WriteName = "WriterName";
            const std::string ErrorCode = "Error";
            const std::string Operation = "Operation";
        };

        namespace VssProviderMissing
        {
            const std::string HealthCode = "ECH00030";
        };

        namespace CumulativeThrottle
        {
            const std::string HealthCode = "SourceAgentCumulativeThrottleOccurred";
            const std::string ObservationTime = "ObservationTime";
        };

        namespace CrashConsistencyTagFailure
        {
            const std::string HealthCode = "CrashConsistentReplicationExceeded";
            const std::string ReplicationInterval = "CrashConsistentReplicationInterval";
        };

        namespace AppConsistencyTagFailure
        {
            const std::string HealthCode = "AppConsistentReplicationExceeded";
            const std::string ReplicationInterval = "AppConsistentReplicationInterval";
        };
    }; // VMLevelHealthIssues

    namespace DiskLevelHealthIssues
    {
        namespace PeakChurn
        {
            const std::string HealthCode = "SourceAgentPeakChurnObserved";
            const std::string ChurnRate = "ChurnRate";
            const std::string UploadPending = "DataPendingForUpload";
            const std::string ObservationTime = "ObservationTime";
        };

        namespace HighLatency
        {
            const std::string HealthCode = "SourceAgentHighLatencyObserved";
            const std::string UploadPending = "DataPendingForUpload";
            const std::string ObservationTime = "ObservationTime";
        };

        namespace DiskRemoved
        {
            const std::string HealthCode = "SourceAgentReplicatedItemRemoved";
        };

        namespace DRThrottle
        {
            const std::string HealthCode = "SourceAgentDifferentialSyncThrottleOccurred";
            const std::string ObservationTime = "ObservationTime";
        };

        namespace IRThrottle
        {
            const std::string HealthCode = "SourceAgentResyncThrottleOccurred";
            const std::string ObservationTime = "ObservationTime";
        };

        namespace SlowResyncProgressOnPremToAzure
        {
            const std::string HealthCode = "SourceAgentSlowResyncProgressOnPremToAzure";
            const std::string ObservationTime = "ObservationTime";
        };

        namespace SlowResyncProgressAzureToOnPrem
        {
            const std::string HealthCode = "SourceAgentSlowResyncProgressAzureToOnPrem";
            const std::string ObservationTime = "ObservationTime";
        };

        namespace NoResyncProgress
        {
            const std::string HealthCode = "SourceAgentNoResyncProgress";
            const std::string ObservationTime = "ObservationTime";
        };

        namespace ResyncReason
        {
            const std::string SourceInternalError = "SourceAgentInternalError";
                
            const std::string DiskResize = "SourceAgentDiskResize";

            const std::string LowMemory = "SourceAgentLowMemoryError";

            const std::string UncleanSystemShutdown = "SourceAgentUncleanSystemShutdownError";

            const std::string TargetInternalError = "TargetAgentInternalError";

            const std::string DeviceUnlockedInReadWriteMode = "TargetAgentDeviceUnlockedInReadWriteModeError";

        };

        namespace ResyncBatching
        {
            const std::string ManualResyncNotStarted = "SourceAgentManualResyncNotStarted";
            const std::string InitialResyncNotStarted = "SourceAgentInitialResyncNotStarted";
            const std::string AutoResyncNotStarted = "SourceAgentAutoResyncNotStarted";
            const std::string StartThresholdInSecs = "StartThresholdInSecs";
        };

        namespace OnPremToAzureConnectionNotHealthy
        {
            const std::string HealthCode = "OnPremToAzureConnectionNotHealthy";
            const std::string CacheStorageAccountName = "CacheStorageAccountName";
            const std::string ObservationTime = "ObservationTime";
        };

    }; // DiskLevelHealthIssues

}; // AgentHealthIssueCodes

const std::string g_IRissueCodes[] = {
    AgentHealthIssueCodes::DiskLevelHealthIssues::IRThrottle::HealthCode,
    AgentHealthIssueCodes::DiskLevelHealthIssues::SlowResyncProgressOnPremToAzure::HealthCode,
    AgentHealthIssueCodes::DiskLevelHealthIssues::SlowResyncProgressAzureToOnPrem::HealthCode,
    AgentHealthIssueCodes::DiskLevelHealthIssues::NoResyncProgress::HealthCode
};
#endif
