#ifndef TAG_TELEMETRY_KEYS_H
#define TAG_TELEMETRY_KEYS_H

#include <iostream>
#include <string>
#include <sstream>
#include <map>

#define CRASH_TAG "CRASH"
#define APP_TAG "APP"
#define BASELINE_TAG "BASELINE"

namespace TagTelemetry
{
    namespace NsVacpOutputKeys
    {
        const std::string TagInsertKey = "Tag is inserted for ";
        const std::string TagGuidKey = "Generating Header for TagGuid: ";
        const std::string TagKey = "Tag: SystemLevelTag";
        const std::size_t TIMESTAMP_LEN = 8;

        const std::string BaselineTagKey = "Baseline Tag:";
        const std::string HydrationTagKey = "Hydration Tag:";
        const std::string FsTagKey = "Tag: FileSystem";
        const std::string CcTagKey = "Tag: CrashTag";
        const std::string LocalTagKey = "Issued a LOCAL TAG";
        const std::string MultivmTagKey = "Issued a DISTRIBUTED TAG on this Node.";

        const std::string TagInsertTimeKey = "TagInsertTime: ";
        const std::string IoBarrierKey = "Elapsed Time for IoBarrier is";
        const std::string AppQuiesceKey = "Elapsed Time for AppQuiesce is";
        const std::string TagCommitTimeKey = "TagCommitTime: ";

        const std::string DrainBarrierKey = "Elapsed Time for DrainBarrier is";
        const std::string PrepPhKey = "Elapsed Time for PreparePhase is";
        const std::string AppPrepPhKey = "Elapsed Time for AppPrepare is";
        const std::string TagPhKey = "Elapsed Time for InsertTagPhase is";
        const std::string QuiescePhKey = "Elapsed Time for QuiescePhase is";
        const std::string ResumePhKey = "Elapsed Time for ResumePhase is";

        const std::string VacpSpawnFailKey = "Failed to spawn the command";
        const std::string VacpAbortKey = "command terminated";

        const std::string MultVmMasterNodeKey("-mn ");
        const std::string MultVmClientNodesKey("-cn ");

        const std::string MasterNodeRoleKey("MultiVmRole:Master");
        const std::string ClientNodeRoleKey("MultiVmRole:Client");

        const std::string VacpSysTimeKey("SysTimeKey ");
        const std::string VacpStartTimeKey("VacpStartTime ");
        const std::string VacpCommand("Command Line: ");
        const std::string TagType("TagType ");
        const std::string VacpEndTimeKey("VacpEndTime ");
        const std::string VacpExitStatus("Exiting with status ");
        const std::string InternallyRescheduled("InternallyRescheduled ");

        //VSS Writer related Telemetry Keys
        const std::string VssWriterError("WriterName");
        const std::string VssWriterErrorCode("Error");
        const std::string VssWriterErrorOperation("Operation");

        //VSS Provider related Telemetry Keys
        const std::string VssProviderError(std::string("Azure Site Recovery VSS Provider"));

    }

}

#endif
