#ifndef SRC_AGENT_TELEMETRY_H
#define SRC_AGENT_TELEMETRY_H

#include <string>
#include <map>

#include "json_reader.h"
#include "json_writer.h"

#include "Telemetry.h"

using namespace AgentTelemetry;

namespace SrcTelemetry
{

    enum LogReason {
        LOG_PROTECTED = 1,
        LOG_ON_STOP_FLT = 1 << 1,
        LOG_ON_START_FLT = 1 << 2,
        LOG_ON_RESYNC_START = 1 << 3,
        LOG_ON_RESYNC_END = 1 << 4,
        LOG_ON_RESYNC_THROTTLE_START = 1 << 5,
        LOG_ON_RESYNC_THROTTLE_END = 1 << 6,
        LOG_ON_DIFF_THROTTLE_START = 1 << 7,
        LOG_ON_DIFF_THROTTLE_END = 1 << 8,
        LOG_ON_PAUSE_START = 1 << 9,
        LOG_ON_PAUSE_END = 1 << 10,
        LOG_ON_CLEAR_DIFFS = 1 << 11,
        LOG_ON_BASELINE_TAG = 1 << 12,
        LOG_ON_DISK_FLUSH = 1 << 13,
        LOG_ON_RESYNC_SET = 1 << 14,
        LOG_ON_PREPARE_FOR_SYNC = 1 << 15

    };

    enum PairStatus {
        PAIR_STATE_DIFF = 1,
        PAIR_STATE_RESYNC = 2,
        PAIR_STATE_PAUSED = 4,
        PAIR_STATE_DIFF_THROTTLE = 8,
        PAIR_STATE_SYNC_THROTTLE = 16
    };

    enum DiskAvailable {
        DISK_AVAILABLE = 1,
        DISK_NOT_AVAILABLE
    };

    enum FilteringStatus {
        DISK_FILTERING_STOP = 1,
        DISK_FILTERING_START
    };

    enum DriverLoadStatus {
        DISK_DRIVER_LOADED = 1,
        DISK_DRIVER_NOT_LOADED
    };

    enum DrvFlagsPosition {
        DRIVER_FLAGS_SHIFT = 0,
        PERSISTENT_REG_CREATED_SHIFT = 32,
        LAST_SHUTDOWN_MARKER_SHIFT = 34,
        SERVICE_STATE_SHIFT = 36,
        DISK_FILTER_MODE_SHIFT = 38
    };

    enum MessageTypes {
        SOURCE_TABLE_AGENT_LOG = 1 + SOURCE_TELEMETRY_BASE_MESSAGE_TYPE,
        SOURCE_TABLE_AGENT_DISK_REMOVED = 3 + SOURCE_TELEMETRY_BASE_MESSAGE_TYPE,
        SOURCE_TABLE_AGENT_DISK_CACHE_NOT_FOUND, 
        SOURCE_TABLE_DRIVER_NOT_LOADED,
        SOURCE_TABLE_DRIVER_VERSION_FAILED,
        SOURCE_TABLE_DRIVER_STATS_NOT_SUPPORTED,
        SOURCE_TABLE_MULTIPLE_DISKS_FOUND
    };

    const std::string CSHOSTID("CSHostId");
    const std::string CSIPADDR("CSIpAddr");
    const std::string PSHOSTID("PSHostId");
    const std::string PSIPADDR("PSIpAddr");
    const std::string PSPORT("PSPort");
    const std::string SRCOSVER("OsVer");
    const std::string RAMSIZE("RamSize");
    const std::string AVBPHYMEM("AvbPhyMem");
    const std::string HOSTNAME("HostName");
    const std::string HOSTIPADDR("HostIpAddr");
    const std::string MACADDR("MacAddr");
    const std::string SYSBOOTTIME("SysBootTime");
    const std::string SYSUPTIME("SysUpTime");
    const std::string SYSTIME("SysTime");
    const std::string DRVSTACK("DrvStack");
    const std::string SRCAGENTVER("SrcAgentVer");
    const std::string DRVLOADED("DrvLoaded");
    const std::string DRVVER("DrvVer");
    const std::string DRVPRODVER("DrvProdVer");
    const std::string DATAPOOLSIZEMAX("DataPoolSizeMax");
    const std::string NONPAGEDPOOLLIMIT("NonPagePoolLimit");
    const std::string LOCKDATABLKCNT("LockDataBlkCnt");
    const std::string DRVLOADTIME("DrvLoadTime");
    const std::string NUMDISKPROTBYDRV("NumDiskProtByDrv");
    const std::string TOTALNUMDISKBYDRV("TotalNumDiskByDrv");
    const std::string DRVFLAGS("DrvFlags");
    const std::string STOPALL("StopAll");
    const std::string TIMEBEFOREJUMP("TimeBeforeJump");
    const std::string TIMEJUMPED("TimeJumped");
    const std::string SYSBOOTCNT("SysBootCnt");
    const std::string DATAPOOLSIZESET("DataPoolSizSet");
    const std::string PERSTIMESTAMPONLASTBOOT("PersTimeStampOnLastBoot");
    const std::string PERSTIMESEQONLASTBOOT("PersTimeSeqOnLastBoot");
    const std::string NUMDISKBYAGENT("NumDiskByAgent");
    const std::string NUMOFDISKPROT("NumOfDiskProt");
    const std::string PROTDISKID("ProtDiskId");
    const std::string RAWSIZEINPOLICY("RawSizeInPolicy");
    const std::string PAIRSTATE("PairState");
    const std::string RESYNCSTART("ResyncStart");
    const std::string RESYNCEND("ResyncEnd");
    const std::string RESYNCSETINPOLICY("ResyncSetInPolicy");
    const std::string RESYNCTIMESTAMP("ResyncTimeStamp");
    const std::string RESYNCCAUSE("ResyncCause");
    const std::string RESYNCCOUNTER("ResyncCounter");
    const std::string DISKAVB("DiskAvb");
    const std::string DISKNAME("DiskName");
    const std::string RAWSIZEAGENT("RawSizeAgent");
    const std::string FLTSTATE("FltState");
    const std::string DEVICEDRVSIZE("DeviceDrvSize");
    const std::string DEVCONTEXTTIME("DevContextTime");
    const std::string STARTFILTKERN("StartFiltKern");
    const std::string STARTFILTUSER("StartFiltUser");
    const std::string STOPFILTKERN("StopFiltKern");
    const std::string STOPFILTUSER("StopFiltUser");
    const std::string CLEARDIFF("ClearDiff");
    const std::string CHURN("Churn");
    const std::string LASTS2START("LastS2Start");
    const std::string LASTS2STOP("LastS2Stop");
    const std::string LASTSVCSTART("LastSvStart");
    const std::string LASTSVCSTOP("LastSvStop");
    const std::string LASTDRAINDB("LastDrainDb");
    const std::string LASTCOMMITDB("LastCommitDb");
    const std::string LASTTAGREQTIME("LastTagReqTime");
    const std::string LOGREASON("LogReason");

    const std::string KERNELVERSION("KernelVersion");
    const std::string AGENTRESOURCEID("AgentResourceId");
    const std::string AGENTSOURCEGROUPID("AgentSourceGroupId");

    const std::string CUSTOMJSON("CustomJson");
    const std::string EXTENDEDPROPS("ExtendedProperties");

    class ReplicationStatus
    {
    public:
        std::map<std::string, std::string> Map;

        void serialize(JSON::Adapter& adapter)
        {
            JSON::Class root(adapter, "ReplicationStatus", false);

            JSON_T(adapter, Map);
        }
    };

}

#endif
