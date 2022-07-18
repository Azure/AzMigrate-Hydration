//
// volumegroupsettings.cpp: define configurator interface to VOLUME_GROUP_SETTINGS
//
#include <cstdlib>
#include <string>
#include <sstream>

#include <boost/lexical_cast.hpp>

#include "volumegroupsettings.h"
#include "../common/hostagenthelpers_ported.h"
#include "logger.h"

using namespace std;

VOLUME_SETTINGS::VOLUME_SETTINGS()
{
    deviceName = "";
    mountPoint = "";
    hostname = "";
    fstype = "";
    secureMode = SECURE_MODE_NONE;
    syncMode = SYNC_OFFLOAD;
    transportProtocol = TRANSPORT_PROTOCOL_HTTP;
    visibility = 0;
    sourceCapacity = 0;
    srcResyncStarttime = 0;
    srcResyncEndtime = 0;
    OtherSiteCompatibilityNum = 0;

    //resync flag info
    resyncRequiredFlag = 0;
    resyncRequiredCause = RESYNCREQUIRED_BYSOURCE;
    resyncRequiredTimestamp = 0;
    rpoThreshold = 0;
    sourceOSVal = OS_UNKNOWN;
    compressionEnable = COMPRESS_NONE;
    pairState = UNKNOWN;
    cleanup_action = "";
    maintenance_action = "";
    srcResyncStartSeq = 0;
    srcResyncEndSeq = 0;
    resyncCounter = 0;
    sourceRawSize = 0;
    srcStartOffset = 0;
    devicetype = 3;
}

VOLUME_SETTINGS::VOLUME_SETTINGS(const VOLUME_SETTINGS& rhs):throttleSettings(rhs.throttleSettings)
{
    deviceName = rhs.deviceName;
    mountPoint = rhs.mountPoint;
    fstype = rhs.fstype;
    hostname = rhs.hostname;
    hostId = rhs.hostId;
    resyncDirectory = rhs.resyncDirectory;
    secureMode = rhs.secureMode;
    sourceToCXSecureMode = rhs.sourceToCXSecureMode;
    transportProtocol = rhs.transportProtocol;
    syncMode = rhs.syncMode;
    visibility = rhs.visibility;
    sourceCapacity = rhs.sourceCapacity;
    rpoThreshold = rhs.rpoThreshold;
    endpoints.assign(rhs.endpoints.begin(),rhs.endpoints.end());
    srcResyncStarttime = rhs.srcResyncStarttime;
    srcResyncEndtime = rhs.srcResyncEndtime;
    OtherSiteCompatibilityNum = rhs.OtherSiteCompatibilityNum;

    //resync flag info
    resyncRequiredFlag = rhs.resyncRequiredFlag;
    resyncRequiredCause = rhs.resyncRequiredCause;
    resyncRequiredTimestamp = rhs.resyncRequiredTimestamp;
    sourceOSVal = rhs.sourceOSVal;
    compressionEnable = rhs.compressionEnable;
    this->jobId = rhs.jobId ;
    sanVolumeInfo = rhs.sanVolumeInfo;
    pairState = rhs.pairState;
    cleanup_action = rhs.cleanup_action;
    diffsPendingInCX = rhs.diffsPendingInCX;
    currentRPO = rhs.currentRPO;
    applyRate = rhs.applyRate;
    maintenance_action = rhs.maintenance_action;
    srcResyncStartSeq = rhs.srcResyncStartSeq;
    srcResyncEndSeq = rhs.srcResyncEndSeq;
    resyncCounter = rhs.resyncCounter;
    sourceRawSize = rhs.sourceRawSize;
    atLunStatsRequest = rhs.atLunStatsRequest;
    srcStartOffset = rhs.srcStartOffset;
    devicetype = rhs.devicetype;
	options = rhs.options;
}

VOLUME_SETTINGS& VOLUME_SETTINGS::operator =(const VOLUME_SETTINGS& rhs)
{
    if ( this == &rhs) {
        return *this;
    }
    deviceName = rhs.deviceName;
    mountPoint = rhs.mountPoint;
    fstype = rhs.fstype;
    hostname = rhs.hostname;

    endpoints.assign(rhs.endpoints.begin(),rhs.endpoints.end());
    hostId = rhs.hostId;
    resyncDirectory = rhs.resyncDirectory;
    secureMode = rhs.secureMode;
    sourceToCXSecureMode = rhs.sourceToCXSecureMode;
    transportProtocol = rhs.transportProtocol;
    syncMode = rhs.syncMode;
    visibility = rhs.visibility;
    sourceCapacity = rhs.sourceCapacity;
    srcResyncStarttime = rhs.srcResyncStarttime;
    srcResyncEndtime = rhs.srcResyncEndtime;
    OtherSiteCompatibilityNum = rhs.OtherSiteCompatibilityNum;

    //resync flag info
    resyncRequiredFlag = rhs.resyncRequiredFlag;
    resyncRequiredCause = rhs.resyncRequiredCause;
    resyncRequiredTimestamp = rhs.resyncRequiredTimestamp;
    sourceOSVal = rhs.sourceOSVal;
    compressionEnable = rhs.compressionEnable;
    this->jobId = rhs.jobId ;
    sanVolumeInfo = rhs.sanVolumeInfo;
    pairState = rhs.pairState;
    cleanup_action = rhs.cleanup_action;
    diffsPendingInCX = rhs.diffsPendingInCX;
    currentRPO = rhs.currentRPO;
    applyRate = rhs.applyRate;
    maintenance_action = rhs.maintenance_action;
    srcResyncStartSeq = rhs.srcResyncStartSeq;
    srcResyncEndSeq = rhs.srcResyncEndSeq;
    resyncCounter = rhs.resyncCounter;
    throttleSettings = rhs.throttleSettings;
    sourceRawSize = rhs.sourceRawSize;
    atLunStatsRequest = rhs.atLunStatsRequest;
    srcStartOffset = rhs.srcStartOffset;
    devicetype = rhs.devicetype;
	options = rhs.options;
    return *this;
}

bool VOLUME_SETTINGS::strictCompare(VOLUME_SETTINGS const & target) const
{
    return(
#ifdef SV_WINDOWS
        0 == stricmp( deviceName.c_str(), target.deviceName.c_str() )
        && 0 == stricmp( mountPoint.c_str(), target.mountPoint.c_str() )
        && 0 == stricmp( fstype.c_str(), target.fstype.c_str() )
        && 0 == stricmp(hostname.c_str(), target.hostname.c_str() )
#else
        deviceName == target.deviceName
        && mountPoint == target.mountPoint
        && 0 == stricmp( fstype.c_str(), target.fstype.c_str() )
        && 0 == stricmp(hostname.c_str(), target.hostname.c_str() )
#endif
        && endpoints == target.endpoints
        && hostId == target.hostId
        && resyncDirectory == target.resyncDirectory
        && secureMode == target.secureMode
        && sourceToCXSecureMode == target.sourceToCXSecureMode
        && transportProtocol == target.transportProtocol
        && syncMode == target.syncMode
        && visibility == target.visibility
        && sourceCapacity == target.sourceCapacity
        && srcResyncStarttime == target.srcResyncStarttime
        && srcResyncEndtime == target.srcResyncEndtime
        && srcResyncStartSeq == target.srcResyncStartSeq
        && srcResyncEndSeq == target.srcResyncEndSeq
        && OtherSiteCompatibilityNum == target.OtherSiteCompatibilityNum
        && resyncRequiredFlag == target.resyncRequiredFlag
        &&  resyncRequiredCause == target.resyncRequiredCause
        && resyncRequiredTimestamp == target.resyncRequiredTimestamp
        && rpoThreshold == target.rpoThreshold
        && sourceOSVal == target.sourceOSVal
        && compressionEnable == target.compressionEnable
        && jobId == target.jobId
        && sanVolumeInfo == target.sanVolumeInfo
        && pairState == target.pairState
        && cleanup_action == target.cleanup_action
        && diffsPendingInCX == target.diffsPendingInCX
        && currentRPO == target.currentRPO
        && applyRate == target.applyRate
        && maintenance_action == target.maintenance_action
        && throttleSettings == target.throttleSettings
        && resyncCounter == target.resyncCounter
        && sourceRawSize == target.sourceRawSize
        && atLunStatsRequest == target.atLunStatsRequest
        && srcStartOffset == target.srcStartOffset 
        && devicetype == target.devicetype
		&& options == target.options);
}

bool VOLUME_SETTINGS::operator==( VOLUME_SETTINGS const& target ) const
{
	bool equals = true;

	std::stringstream msg;
	msg << "For volume settings of deviceName: " << deviceName << ", ";
#ifdef SV_WINDOWS
	if (0 != stricmp(deviceName.c_str(), target.deviceName.c_str()))
	{
		msg << "deviceName changed. lhs: " << deviceName << ", rhs: " << target.deviceName;
		equals = false;
	}

	if (equals && (0!=stricmp(mountPoint.c_str(), target.mountPoint.c_str())))
	{
        msg << "mountPoint changed. lhs: " << mountPoint << ", rhs: " << target.mountPoint;
		equals = false;
	}
#else
	if (deviceName != target.deviceName)
	{
        msg << "deviceName changed. lhs: " << deviceName << ", rhs: " << target.deviceName;
		equals = false;
	}

	if (equals && (mountPoint != target.mountPoint))
	{
        msg << "mountPoint changed. lhs: " << mountPoint << ", rhs: " << target.mountPoint;
		equals = false;
	}
#endif

    if (equals && (0!=stricmp(fstype.c_str(), target.fstype.c_str())))
	{
        msg << "fstype changed. lhs: " << fstype << ", rhs: " << target.fstype;
		equals = false;
	}

	if (equals && (0!=stricmp(hostname.c_str(), target.hostname.c_str())))
	{
        msg << "hostname changed. lhs: " << hostname << ", rhs: " << target.hostname;
		equals = false;
	}

	if (equals && !(endpoints == target.endpoints))
	{
        msg << "endpoints changed";
		equals = false;
	}

	if (equals && (hostId != target.hostId))
	{
        msg << "hostId changed. lhs: " << hostId << ", rhs: " << target.hostId;
		equals = false;
	}

	if (equals && (resyncDirectory != target.resyncDirectory))
	{
		msg << "resyncDirectory changed. lhs: " << resyncDirectory << ", rhs: " << target.resyncDirectory;
		equals = false;
	}

	if (equals && (secureMode != target.secureMode))
	{
		msg << "secureMode changed. lhs: " << secureMode << ", rhs: " << target.secureMode;
		equals = false;
	}

	if (equals && (sourceToCXSecureMode != target.sourceToCXSecureMode))
	{
		msg << "sourceToCXSecureMode changed. lhs: " << sourceToCXSecureMode << ", rhs: " << target.sourceToCXSecureMode;
		equals = false;
	}

	if (equals && (transportProtocol != target.transportProtocol))
	{
	    msg << "transportProtocol changed. lhs: " << transportProtocol << ", rhs: " << target.transportProtocol;
		equals = false;
	}

	if (equals && (syncMode != target.syncMode))
	{
		msg << "syncMode changed. lhs: " << syncMode << ", rhs: " << target.syncMode;
		equals = false;
	}

	if (equals && (visibility != target.visibility))
	{
		msg << "visibility changed. lhs: " << visibility << ", rhs: " << target.visibility;
		equals = false;
	}

	if (equals && (sourceCapacity != target.sourceCapacity))
	{
		msg << "sourceCapacity changed. lhs: " << sourceCapacity << ", rhs: " << target.sourceCapacity;
		equals = false;
	}

	if (equals && (OtherSiteCompatibilityNum != target.OtherSiteCompatibilityNum))
	{
		msg << "OtherSiteCompatibilityNum changed. lhs: " << OtherSiteCompatibilityNum << ", rhs: " << target.OtherSiteCompatibilityNum;
		equals = false;
	}

	if (equals && (resyncRequiredFlag != target.resyncRequiredFlag))
	{
		msg << "resyncRequiredFlag changed. lhs: " << resyncRequiredFlag << ", rhs: " << target.resyncRequiredFlag;
		equals = false;
	}

	if (equals && (resyncRequiredCause != target.resyncRequiredCause))
	{
		msg << "resyncRequiredCause changed. lhs: " << resyncRequiredCause << ", rhs: " << target.resyncRequiredCause;
		equals = false;
	}

	if (equals && (resyncRequiredTimestamp != target.resyncRequiredTimestamp))
	{
		msg << "resyncRequiredTimestamp changed. lhs: " << resyncRequiredTimestamp << ", rhs: " << target.resyncRequiredTimestamp;
		equals = false;
	}

	if (equals && (rpoThreshold != target.rpoThreshold))
	{
		msg << "rpoThreshold changed. lhs: " << rpoThreshold << ", rhs: " << target.rpoThreshold;
		equals = false;
	}

	if (equals && (sourceOSVal != target.sourceOSVal))
	{
		msg << "sourceOSVal changed. lhs: " << sourceOSVal << ", rhs: " << target.sourceOSVal;
		equals = false;
	}

	if (equals && (compressionEnable != target.compressionEnable))
	{
		msg << "compressionEnable changed. lhs: " << compressionEnable << ", rhs: " << target.compressionEnable;
		equals = false;
	}

	if (equals && (jobId != target.jobId))
	{
		msg << "jobId changed. lhs: " << jobId << ", rhs: " << target.jobId;
		equals = false;
	}

	if (equals && !(throttleSettings == target.throttleSettings))
	{
		msg << "throttleSettings changed";
		equals = false;
	}

	if (equals && (resyncCounter != target.resyncCounter))
	{
		msg << "resyncCounter changed. lhs: " << resyncCounter << ", rhs: " << target.resyncCounter;
		equals = false;
	}

	if (equals && !(sanVolumeInfo == target.sanVolumeInfo))
	{
		msg << "sanVolumeInfo changed";
		equals = false;
	}

	if (equals && (pairState != target.pairState))
	{
		msg << "pairState changed. lhs: " << pairState << ", rhs: " << target.pairState;
		equals = false;
	}

	if (equals && !(atLunStatsRequest == target.atLunStatsRequest))
	{
		msg << "atLunStatsRequest changed";
		equals = false;
	}

	if (equals && (srcStartOffset != target.srcStartOffset))
	{
		msg << "srcStartOffset changed. lhs: " << srcStartOffset << ", rhs: " << target.srcStartOffset;
		equals = false;
	}

	if (equals && (devicetype != target.devicetype))
	{
		msg << "devicetype changed. lhs: " << devicetype << ", rhs: " << target.devicetype;
		equals = false;
	}

	if (equals && (options != target.options))
	{
		msg << "options changed";
		equals = false;
	}

	/* currently print only when some thing has changed */
	if (equals)
		msg << "Nothing changed.";
	else
		DebugPrintf(SV_LOG_DEBUG, "%s.\n", msg.str().c_str());

	return equals;
}

/*
* FUNCTION NAME :  VOLUME_SETTINGS::shouldOperationRun
*
* DESCRIPTION : Determine whether a cdpcli operation, process like cachemgr, dp, s2
*                                      will run or not on a specific volume
*
* INPUT PARAMETERS : std::string operationName
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : false if the operation should not run otherwise true
*
*/
bool VOLUME_SETTINGS::shouldOperationRun(const std::string & operationName)
{
    bool rv = true;
    //List of opertion not to perform
    std::string opertionList;

    switch(pairState)
    {
    case VOLUME_SETTINGS::PAIR_PROGRESS:
        opertionList = "moveretention";
        break;
    case VOLUME_SETTINGS::DELETE_PENDING:
        opertionList = "recover,rollback,listevents,"
            "showsummary,vsnap,listcommonpt"
            ",validate,moveretention"
            ",cachemgr,s2,dataprotection"
			",deletestalefiles,deleteunusablepoints";
        break;
    case VOLUME_SETTINGS::PAUSE_PENDING:
        opertionList = "moveretention";
    case VOLUME_SETTINGS::PAUSE:
        if(maintenance_action.find("move_retention=yes;") != std::string::npos)
        {
            opertionList += "recover,rollback,listevents,showsummary,vsnap,listcommonpt,validate,deletestalefiles,deleteunusablepoints";
        }
        else
        {
            opertionList = "moveretention";
        }
        if(maintenance_action.find("pause_components=yes;components=all;") != std::string::npos)
        {
            opertionList += ",cachemgr,s2,dataprotection";
        }
        else
        {
            opertionList += ",dataprotection";
        }
        break;
	/*SRM:start, List of operations not allowed in each state*/
	case VOLUME_SETTINGS::FLUSH_AND_HOLD_WRITES_PENDING:
		opertionList = "moveretention,recover,rollback,vsnap,unhide_rw,unhide_ro,hide";
		break;
	case VOLUME_SETTINGS::FLUSH_AND_HOLD_WRITES:
		opertionList = "dataprotection,moveretention,recover,rollback,vsnap,unhide_rw,unhide_ro,hide";
		break;
	case VOLUME_SETTINGS::FLUSH_AND_HOLD_RESUME_PENDING:
		opertionList = "moveretention,recover,rollback,vsnap,unhide_rw,unhide_ro,hide";
		break;
	/*SRM:End*/
    default:
        opertionList = "cachemgr,s2,dataprotection,moveretention";
    };

    if(opertionList.find(operationName) != std::string::npos)
    {
        rv = false;
    }
    return rv;
}

VOLUME_SETTINGS::RESYNC_COPYMODE VOLUME_SETTINGS::GetResyncCopyMode(void) const
{
    const char *values[] = {NsVOLUME_SETTINGS::VALUE_FULL, NsVOLUME_SETTINGS::VALUE_FILESYSTEM};
    const RESYNC_COPYMODE modes[] = {RESYNC_COPYMODE_FULL, RESYNC_COPYMODE_FILESYSTEM};
    RESYNC_COPYMODE mode = RESYNC_COPYMODE_UNKNOWN;
	std::map<std::string, std::string>::const_iterator it = options.find(NsVOLUME_SETTINGS::NAME_RESYNC_COPY_MODE);

    if (it != options.end())
    {
        for (int i = 0; i < NELEMS(values); i++)
        {
            if (InmStrCmp<NoCaseCharCmp>(values[i], it->second) == 0)
            {
                mode = modes[i];
                break;
            }
        }
    }

    return mode;
}

VOLUME_SETTINGS::PROTECTION_DIRECTION VOLUME_SETTINGS::GetProtectionDirection(void) const
{
	const char *values[] = {NsVOLUME_SETTINGS::VALUE_FORWARD, NsVOLUME_SETTINGS::VALUE_REVERSE};
	const PROTECTION_DIRECTION directions[] = {PROTECTION_DIRECTION_FORWARD, PROTECTION_DIRECTION_REVERSE};
	PROTECTION_DIRECTION direction = PROTECTION_DIRECTION_UNKNOWN;
	std::map<std::string, std::string>::const_iterator it = options.find(NsVOLUME_SETTINGS::NAME_PROTECTION_DIRECTION);

	if (it != options.end())
    {
        for (int i = 0; i < NELEMS(values); i++)
        {
            if (InmStrCmp<NoCaseCharCmp>(values[i], it->second) == 0)
            {
                direction = directions[i];
                break;
            }
        }
    }

	return direction;
}

SV_ULONGLONG VOLUME_SETTINGS::GetRawSize(void) const
{
	std::map<std::string, std::string>::const_iterator it = options.find(NsVOLUME_SETTINGS::NAME_RAWSIZE);
	SV_ULONGLONG s = 0;

	if (it != options.end())
    {
		s = boost::lexical_cast<SV_ULONGLONG>( it->second );
	}

	return s;
}

SV_ULONGLONG VOLUME_SETTINGS::GetEndpointRawSize(void) const
{
	SV_ULONGLONG s = 0;

	VOLUME_SETTINGS::endpoints_constiterator endPointIter = endpoints.begin();
	if (endPointIter != endpoints.end()) 
	{
		s = endPointIter->GetRawSize();
	}

	return s;
}

std::string VOLUME_SETTINGS::GetEndpointDeviceName(void) const
{
	std::string name;

	VOLUME_SETTINGS::endpoints_constiterator endPointIter = endpoints.begin();
	if (endPointIter != endpoints.end()) 
	{
		name = endPointIter->deviceName;
	}

	return name;
}

int VOLUME_SETTINGS::GetEndpointDeviceType(void) const
{
	int type = VolumeSummary::DISK;

	VOLUME_SETTINGS::endpoints_constiterator endPointIter = endpoints.begin();
	if (endPointIter != endpoints.end())
	{
		type = endPointIter->devicetype;
	}

	return type;
}

std::string VOLUME_SETTINGS::GetEndpointHostId() const
{
	std::string name;

	VOLUME_SETTINGS::endpoints_constiterator endPointIter = endpoints.begin();
	if (endPointIter != endpoints.end())
	{
		name = endPointIter->hostId;
	}

	return name;
}

std::string VOLUME_SETTINGS::GetEndpointHostName() const
{
	std::string name;

	VOLUME_SETTINGS::endpoints_constiterator endPointIter = endpoints.begin();
	if (endPointIter != endpoints.end())
	{
		name = endPointIter->hostname;
	}

	return name;
}

VOLUME_SETTINGS::TARGET_DATA_PLANE VOLUME_SETTINGS::getTargetDataPlane(void) const
{
	const char *values[] = { NsVOLUME_SETTINGS::VALUE_UNSUPPORTED_DATA_PLANE, NsVOLUME_SETTINGS::VALUE_INMAGE_DATA_PLANE, NsVOLUME_SETTINGS::VALUE_AZURE_DATA_PLANE };
	
	// TODO: For now, setting  default as INMAGE_DATA_PLANE
	// when CS starts sending this field, set this to UNSUPPORTED_DATA_PLANE
	TARGET_DATA_PLANE targetDataPlane = INMAGE_DATA_PLANE;
	
	std::map<std::string, std::string>::const_iterator it = options.find(NsVOLUME_SETTINGS::NAME_TARGET_DATA_PLANE);

	if (it != options.end())
	{
		for (int i = 0; i < NELEMS(values); i++)
		{
			if (InmStrCmp<NoCaseCharCmp>(values[i], it->second) == 0)
			{
				targetDataPlane = (TARGET_DATA_PLANE)i;
				break;
			}
		}
	}

	return targetDataPlane;
}




VOLUME_SETTINGS::~VOLUME_SETTINGS()
{
    //      endpoints.clear();
}

VOLUME_GROUP_SETTINGS& VOLUME_GROUP_SETTINGS::operator=(const VOLUME_GROUP_SETTINGS& rhs)
{
    if ( this == &rhs )
        return *this;

    id = rhs.id;
    direction = rhs.direction;
    status = rhs.status;

    volumes = rhs.volumes;
    transportSettings = rhs.transportSettings;
    return *this;
}

VOLUME_GROUP_SETTINGS::VOLUME_GROUP_SETTINGS() : id(INVALID_GROUP_SETTINGS_ID), direction(UNSPECIFIED), status(UNPROTECTED)
{

}

VOLUME_GROUP_SETTINGS::VOLUME_GROUP_SETTINGS(const VOLUME_GROUP_SETTINGS& rhs)
{
    id = rhs.id;
    direction = rhs.direction;
    status = rhs.status;

    volumes = rhs.volumes;
    transportSettings = rhs.transportSettings;
}


bool VOLUME_GROUP_SETTINGS::operator==( VOLUME_GROUP_SETTINGS const& target ) const
{
	bool equals = true;

	std::stringstream msg;
	msg << "For volume group settings of id: " << id << ", ";
	
	if (id != target.id)
	{
		msg << "id changed, lhs: " << id << ", rhs: " << target.id;
		equals = false;
	}

	if (equals && (direction != target.direction))
	{
		msg << "direction changed, lhs: " << direction << ", rhs: " << target.direction;
		equals = false;
	}

	if (equals && (status != target.status))
	{
		msg << "status changed, lhs: " << status << ", rhs: " << target.status;
		equals = false;
	}

	if (equals && !(volumes == target.volumes))
	{
		msg << "volumes changed";
		equals = false;
	}

	if (equals && !(transportSettings == target.transportSettings))
	{
		msg << "transportSettings changed";
		equals = false;
	}

	/* currently print only when some thing has changed */
	if (equals)
		msg << "Nothing changed";
	else
		DebugPrintf(SV_LOG_DEBUG, "%s.\n", msg.str().c_str());

	return equals;
}

bool VOLUME_GROUP_SETTINGS::strictCompare(VOLUME_GROUP_SETTINGS const & target) const
{
    return( id == target.id
        && direction == target.direction
        && status == target.status
        && strictVolumesCompare(target)
        && transportSettings == target.transportSettings);
}

bool VOLUME_GROUP_SETTINGS::strictVolumesCompare(VOLUME_GROUP_SETTINGS const & volumeGroupSettings)const
{
    std::map<std::string,VOLUME_SETTINGS>::const_iterator lhsvolumeIter = volumes.begin();
    std::map<std::string,VOLUME_SETTINGS>::const_iterator  lhsvolumeEnd = volumes.end();
    std::map<std::string,VOLUME_SETTINGS>::const_iterator  rhsvolumeIter = volumeGroupSettings.volumes.begin();
    std::map<std::string,VOLUME_SETTINGS>::const_iterator rhsvolumeEnd = volumeGroupSettings.volumes.end();

    for (/* empty*/; lhsvolumeIter != lhsvolumeEnd && rhsvolumeIter != rhsvolumeEnd; ++lhsvolumeIter,++rhsvolumeIter)
    {
        if(!((*lhsvolumeIter).second.strictCompare((*rhsvolumeIter).second)))
        {
            return false;
        }
    }
    return true;
}

VOLUME_GROUP_SETTINGS::~VOLUME_GROUP_SETTINGS()
{
    volumes.clear();
}

VOLUME_GROUP_SETTINGS::volumes_iterator
VOLUME_GROUP_SETTINGS::find_by_guid(const std::string devicename)
{
    VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter = volumes.begin();
    for( ; ctVolumeIter != volumes.end(); ++ctVolumeIter)
    {
        if(volequalitycmp()(ctVolumeIter -> first, devicename))
            return ctVolumeIter;
    }

    return volumes.end();
}

VOLUME_GROUP_SETTINGS::volumes_iterator
VOLUME_GROUP_SETTINGS::find_by_name(const std::string devicename)
{
    VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter = volumes.begin();
    for( ; ctVolumeIter != volumes.end(); ++ctVolumeIter)
    {
        if(ctVolumeIter -> first == devicename)
            return ctVolumeIter;
    }

    return volumes.end();
}



HOST_VOLUME_GROUP_SETTINGS::HOST_VOLUME_GROUP_SETTINGS()
{

}
HOST_VOLUME_GROUP_SETTINGS::~HOST_VOLUME_GROUP_SETTINGS()
{
    volumeGroups.clear();
}


HOST_VOLUME_GROUP_SETTINGS::HOST_VOLUME_GROUP_SETTINGS(const HOST_VOLUME_GROUP_SETTINGS& rhs)
{
    volumeGroups.assign(rhs.volumeGroups.begin(),rhs.volumeGroups.end());
}

HOST_VOLUME_GROUP_SETTINGS& HOST_VOLUME_GROUP_SETTINGS::operator=(const HOST_VOLUME_GROUP_SETTINGS& rhs)
{
    volumeGroups.assign(rhs.volumeGroups.begin(),rhs.volumeGroups.end());
    return *this;
}

bool HOST_VOLUME_GROUP_SETTINGS::operator==( HOST_VOLUME_GROUP_SETTINGS const& target ) const
{
    return( volumeGroups == target.volumeGroups );
}

bool HOST_VOLUME_GROUP_SETTINGS::strictCompare(HOST_VOLUME_GROUP_SETTINGS const& hostVolumeGroupSettings) const
{

    std::list<VOLUME_GROUP_SETTINGS>::const_iterator lhsvolumeGroupIter = volumeGroups.begin();
    std::list<VOLUME_GROUP_SETTINGS>::const_iterator lhsvolumeGroupEnd = volumeGroups.end();

    std::list<VOLUME_GROUP_SETTINGS>::const_iterator rhsvolumeGroupIter = hostVolumeGroupSettings.volumeGroups.begin();
    std::list<VOLUME_GROUP_SETTINGS>::const_iterator rhsvolumeGroupEnd =  hostVolumeGroupSettings.volumeGroups.end();
    for (/* empty */; lhsvolumeGroupIter != lhsvolumeGroupEnd && rhsvolumeGroupIter != rhsvolumeGroupEnd; ++lhsvolumeGroupIter,++rhsvolumeGroupIter)
    {
        if(!((*lhsvolumeGroupIter).strictCompare((*rhsvolumeGroupIter))))
        {
            return false;
        }
    }
    return true;
}

int HOST_VOLUME_GROUP_SETTINGS::getVolumeAttributeChangeSettings(string driveName)
{
    // Store visibility status of all volumes in all volume groups for currrent host
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(driveName);

        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }

        return ctVolumeIter->second.visibility;
    }

    return VOLUME_DO_NOTHING ;
}

string HOST_VOLUME_GROUP_SETTINGS::getSyncDirectory(string driveName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(driveName);

        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }

        return ctVolumeIter->second.resyncDirectory;
    }
    // Return empty string if not available
    return "" ;

}

/*
* FUNCTION NAME :  HOST_VOLUME_GROUP_SETTINGS::getTransportSettings
*
* DESCRIPTION : get TRANSPORT Settings for the devicename
*
*
* INPUT PARAMETERS : device name
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*  if the device is not found, default transport settings are sent
*
* return value : transport settings.
*
*/
TRANSPORT_CONNECTION_SETTINGS HOST_VOLUME_GROUP_SETTINGS::getTransportSettings(const std::string &deviceName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(deviceName);

        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }

        return vg.transportSettings;
    }

    // Return empty values if not available
    return TRANSPORT_CONNECTION_SETTINGS() ;
}


/*
* FUNCTION NAME :  HOST_VOLUME_GROUP_SETTINGS::getTransportSettings
*
* DESCRIPTION : get TRANSPORT Settings for the volume group
*
*
* INPUT PARAMETERS : volume group identifier
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*  if the device is not found, default transport settings are sent
*
* return value : transport settings
*
*/
TRANSPORT_CONNECTION_SETTINGS HOST_VOLUME_GROUP_SETTINGS::getTransportSettings(int volGrpId)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        if(vg.id == volGrpId)
        {
            return vg.transportSettings;
        }
    }

    // Return empty value if not available
    return TRANSPORT_CONNECTION_SETTINGS() ;
}

/*
* FUNCTION NAME :  HOST_VOLUME_GROUP_SETTINGS::isProtectedVolume
*
* DESCRIPTION : check if the specified volume is a protected (filtered) volume
*
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : true on success, otherwise false
*
*/
bool HOST_VOLUME_GROUP_SETTINGS::isProtectedVolume(const std::string & deviceName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_guid(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        return vg.direction == SOURCE;
    }
    return false;
}

bool HOST_VOLUME_GROUP_SETTINGS::isTargetVolume(string driveName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_guid(driveName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        return vg.direction == TARGET;
    }
    return false;
}


bool HOST_VOLUME_GROUP_SETTINGS::isResyncing(string driveName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_guid(driveName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        return ((ctVolumeIter->second.syncMode == VOLUME_SETTINGS::SYNC_OFFLOAD) ||
            (ctVolumeIter->second.syncMode == VOLUME_SETTINGS::SYNC_FAST)||
            (ctVolumeIter->second.syncMode == VOLUME_SETTINGS::SYNC_FAST_TBC) ||
            (ctVolumeIter->second.syncMode == VOLUME_SETTINGS::SYNC_DIRECT) ||
            (ctVolumeIter->second.syncMode == VOLUME_SETTINGS::SYNC_QUASIDIFF));
    }
    return false;
}

bool HOST_VOLUME_GROUP_SETTINGS::isInResyncStep1(string driveName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(driveName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        return ((ctVolumeIter->second.syncMode == VOLUME_SETTINGS::SYNC_OFFLOAD) ||
            (ctVolumeIter->second.syncMode == VOLUME_SETTINGS::SYNC_FAST)||
            (ctVolumeIter->second.syncMode == VOLUME_SETTINGS::SYNC_FAST_TBC) ||
            (ctVolumeIter->second.syncMode == VOLUME_SETTINGS::SYNC_DIRECT));
    }
    return false;
}

int HOST_VOLUME_GROUP_SETTINGS::getRpoThreshold(string driveName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(driveName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        return ctVolumeIter->second.rpoThreshold;
    }
    return 0;
}

SV_ULONGLONG HOST_VOLUME_GROUP_SETTINGS::getDiffsPendingInCX(std::string driveName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_guid(driveName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        return ctVolumeIter->second.diffsPendingInCX;
    }
    return 0;
}

SV_ULONGLONG HOST_VOLUME_GROUP_SETTINGS::getCurrentRpo(std::string driveName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_guid(driveName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        return ctVolumeIter->second.currentRPO;
    }
    return 0;
}

SV_ULONGLONG HOST_VOLUME_GROUP_SETTINGS::getApplyRate(std::string driveName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_guid(driveName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        return ctVolumeIter->second.applyRate;
    }
    return 0;
}

SV_ULONGLONG HOST_VOLUME_GROUP_SETTINGS::getResyncCounter(const std::string& driveName)
{
    /*OOD_DESIGN1
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
    vgiter != volumeGroups.end(); ++vgiter)
    {
    VOLUME_GROUP_SETTINGS vg = *vgiter;
    VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find(driveName);
    if (ctVolumeIter == vg.volumes.end())
    {
    continue;
    }
    return ctVolumeIter->second.resyncCounter;
    }*/
    return 0;
}

std::string HOST_VOLUME_GROUP_SETTINGS::getSourceHostIdForTargetDevice(const std::string& deviceName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        if( vg.direction != TARGET )
        {
            continue;
        }
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        VOLUME_SETTINGS::endpoints_iterator endPointIter = ctVolumeIter->second.endpoints.begin();
        if(endPointIter != ctVolumeIter->second.endpoints.end())
        {
            return endPointIter->hostId;
        }
    }
    return "";
}
std::string HOST_VOLUME_GROUP_SETTINGS::getSourceVolumeNameForTargetDevice(const std::string& deviceName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        if( vg.direction != TARGET )
        {
            continue;
        }
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        VOLUME_SETTINGS::endpoints_iterator endPointIter = ctVolumeIter->second.endpoints.begin();
        if(endPointIter != ctVolumeIter->second.endpoints.end())
        {
            return endPointIter->deviceName;
        }
    }
    return "";
}

VOLUME_SETTINGS::PAIR_STATE HOST_VOLUME_GROUP_SETTINGS::getPairState(const std::string & driveName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(driveName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        return ctVolumeIter->second.pairState;
    }
    return VOLUME_SETTINGS::UNKNOWN;
}

OS_VAL HOST_VOLUME_GROUP_SETTINGS::getSourceOSVal(const std::string & driveName)
{
	//first try to get the settings using the volume name as key
	for(volumeGroups_iterator vgiter = volumeGroups.begin();
		vgiter != volumeGroups.end(); ++vgiter)
	{
		VOLUME_GROUP_SETTINGS vg = *vgiter;
		VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(driveName);
		if (ctVolumeIter == vg.volumes.end())
		{
			continue;
		}

		return ctVolumeIter->second.sourceOSVal;
	}

	// not found, compare using the guid
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_guid(driveName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        return ctVolumeIter->second.sourceOSVal;
    }
    return OS_UNKNOWN;
}

SV_ULONG HOST_VOLUME_GROUP_SETTINGS::getOtherSiteCompartibilityNumber(string driveName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_guid(driveName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        return ctVolumeIter->second.OtherSiteCompatibilityNum;
    }
    return 0;
}

bool HOST_VOLUME_GROUP_SETTINGS::isSourceFullDevice(const std::string & deviceName)
{
    bool rv = false;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        if( vg.direction != TARGET )
        {
            continue;
        }
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_guid(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        VOLUME_SETTINGS::endpoints_iterator endIter = ctVolumeIter->second.endpoints.begin();
        if ( endIter != ctVolumeIter->second.endpoints.end() )
        {
            // FULL DISK = 0
            // DISK_PARTITION = 9
            // see PR#14802 for details on why devicetype 9 is also 
            // considered full device
            if( (0 == endIter->devicetype) || (9 == endIter->devicetype))
            {
                rv = true;
            }
        }
        break;
    }
    return rv;
}

std::string HOST_VOLUME_GROUP_SETTINGS::getVolumeMountPoint(const std::string& deviceName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        return ctVolumeIter->second.mountPoint;
    }
    return "";
}

/*
* FUNCTION NAME :  HOST_VOLUME_GROUP_SETTINGS::getSourceRawSize
*
* DESCRIPTION : return the corresponding protected volume raw size
*
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : return raw size
*
*/
SV_ULONGLONG HOST_VOLUME_GROUP_SETTINGS::getSourceRawSize(const std::string & deviceName)
{
    SV_ULONGLONG rawsize = 0;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_guid(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        else
        {
            rawsize= ctVolumeIter->second.sourceRawSize;
            break;
        }
    }

    return rawsize;
}

/*
* FUNCTION NAME :  HOST_VOLUME_GROUP_SETTINGS::getSourceCapacity
*
* DESCRIPTION : return the corresponding protected volume capacity
*
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : true on success, otherwise false
*
*/
SV_ULONGLONG HOST_VOLUME_GROUP_SETTINGS::getSourceCapacity(const std::string & deviceName)
{
    SV_ULONGLONG capacity = 0;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_guid(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        else
        {
            capacity= ctVolumeIter->second.sourceCapacity;
            break;
        }
    }

    return capacity;
}

std::string HOST_VOLUME_GROUP_SETTINGS::getSourceFileSystem(const std::string & deviceName)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        if ( TARGET == vgiter->direction)
        {
            VOLUME_GROUP_SETTINGS vg = *vgiter;
            VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_guid(deviceName);
            if (ctVolumeIter == vg.volumes.end())
            {
                continue;
            }
            else
            {
                VOLUME_SETTINGS::endpoints_iterator endIter = ctVolumeIter->second.endpoints.begin();
                if ( endIter != ctVolumeIter->second.endpoints.end() )
                {
                    return endIter->fstype;
                }
            }
        }
    }
    return "";

}

std::map<std::string, std::string> HOST_VOLUME_GROUP_SETTINGS::getReplicationPairInfo(const std::string & sourceHost)
{
    std::map<std::string, std::string> replcationPairInfo;
    string tempLower1;
    string tempLower2;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        if (vg.direction != TARGET)
            continue;

        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.volumes.begin();
        for (;ctVolumeIter != vg.volumes.end(); ctVolumeIter++)
        {
            VOLUME_SETTINGS::endpoints_iterator endIter = ctVolumeIter->second.endpoints.begin();
            for (; endIter != ctVolumeIter->second.endpoints.end(); endIter++)
            {
                //string comparision are case sensitive. Making both endIter->hostname and sourceHost to lowercase
                if ((tempLower1 = ToLower(sourceHost)) == (tempLower2 =  ToLower(endIter->hostname)))
                    replcationPairInfo[endIter->deviceName] = ctVolumeIter->first;
            }
        }
    }
    return replcationPairInfo;
}

std::map<std::string, std::string> HOST_VOLUME_GROUP_SETTINGS::getSourceVolumeDeviceNames(const std::string & targetHost)
{
    std::map<std::string, std::string> replcationPairInfo;
    string tempLower1;
    string tempLower2;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        if (vg.direction != SOURCE)
            continue;

        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.volumes.begin();
        for (;ctVolumeIter != vg.volumes.end(); ctVolumeIter++)
        {
            VOLUME_SETTINGS::endpoints_iterator endIter = ctVolumeIter->second.endpoints.begin();
            for (; endIter != ctVolumeIter->second.endpoints.end(); endIter++)
            {
                //string comparision are case sensitive. Making both endIter->hostname and targetHost to lowercase
                if ((tempLower1 = ToLower(targetHost)) == (tempLower2 =  ToLower(endIter->hostname)))
                    replcationPairInfo[endIter->deviceName] = ctVolumeIter->first;
            }
        }
    }
    return replcationPairInfo;
}


void HOST_VOLUME_GROUP_SETTINGS::getVolumeNameAndMountPointForAll(VolumeNameMountPointMap &volumeNameMountPointMap )
{

    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.volumes.begin();//find(deviceName);
        for (;ctVolumeIter != vg.volumes.end();ctVolumeIter++)
        {
            volumeNameMountPointMap.insert(volumeNameMountPointMap.end(),
                VolumeNameMountPointPair(ctVolumeIter->first,ctVolumeIter->second.mountPoint));
        }

    }

}


void HOST_VOLUME_GROUP_SETTINGS::getVolumeNameAndFileSystemForAll(VolumeNameFileSystemMap &volumeNameFileSystemMap )
{

    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        if ( TARGET == vgiter->direction)
        {
            VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.volumes.begin();
            for(;ctVolumeIter != vg.volumes.end(); ctVolumeIter ++)
            {

                VOLUME_SETTINGS::endpoints_iterator endIter = ctVolumeIter->second.endpoints.begin();
                if ( endIter != ctVolumeIter->second.endpoints.end() )
                {
                    volumeNameFileSystemMap.insert(volumeNameFileSystemMap.end(),
                        VolumeNameFileSystemPair( ctVolumeIter->first,endIter->fstype));
                }
            }
        }
        else
        {
            VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.volumes.begin();
            for(;ctVolumeIter != vg.volumes.end(); ctVolumeIter ++)
            {

                VOLUME_SETTINGS::endpoints_iterator endIter = ctVolumeIter->second.endpoints.begin();
                if ( endIter != ctVolumeIter->second.endpoints.end() )
                {
                    volumeNameFileSystemMap.insert(volumeNameFileSystemMap.end(),
                        VolumeNameFileSystemPair( ctVolumeIter->first,ctVolumeIter->second.fstype));
                }
            }

        }
    }

}


THROTTLE_SETTINGS::THROTTLE_SETTINGS() :
throttleResync(false),
throttleSource(false),
throttleTarget(false)
{
}

THROTTLE_SETTINGS::THROTTLE_SETTINGS(const THROTTLE_SETTINGS& rhs ) {
    throttleResync = rhs.throttleResync;
    throttleSource = rhs.throttleSource;
    throttleTarget = rhs.throttleTarget;
}

bool THROTTLE_SETTINGS::operator==( THROTTLE_SETTINGS const& rhs ) const {
    return throttleResync == rhs.throttleResync &&
        throttleSource == rhs.throttleSource &&
        throttleTarget == rhs.throttleTarget;
}


bool THROTTLE_SETTINGS::IsResyncThrottled(void) const 
{
    return throttleResync;
}


bool THROTTLE_SETTINGS::IsSourceThrottled(void) const 
{
    return throttleSource;
}


bool THROTTLE_SETTINGS::IsTargetThrottled(void) const 
{
    return throttleTarget;
}

ATLUN_STATS_REQUEST::ATLUN_STATS_REQUEST()
{
    /* TODO: Do we need this ?
    type = ATLUN_STATS_NOREQUEST;
    */
}


ATLUN_STATS_REQUEST::~ATLUN_STATS_REQUEST()
{
    /* nothing */
}


ATLUN_STATS_REQUEST & ATLUN_STATS_REQUEST::operator=(const ATLUN_STATS_REQUEST &rhs)
{
    /* TODO: should we check &rhs to be same as this ? Not needed
    if (this == &rhs)
    {
    return *this;
    }
    */

    type = rhs.type;
    atLUNName = rhs.atLUNName;
    physicalInitiatorPWWNs = rhs.physicalInitiatorPWWNs;

    return *this;
}


bool ATLUN_STATS_REQUEST::operator==(const ATLUN_STATS_REQUEST &rhs) const
{
    return (
        (type == rhs.type) &&
        (atLUNName == rhs.atLUNName) &&
        (physicalInitiatorPWWNs == rhs.physicalInitiatorPWWNs) 
        );
}

/*SRM: start*/
FLUSH_AND_HOLD_REQUEST::FLUSH_AND_HOLD_REQUEST()
{
}
FLUSH_AND_HOLD_REQUEST::~FLUSH_AND_HOLD_REQUEST()
{
}

FLUSH_AND_HOLD_REQUEST::FLUSH_AND_HOLD_REQUEST(const FLUSH_AND_HOLD_REQUEST& rhs ) {
    m_consistency_type = rhs.m_consistency_type;
    m_timeto_pause_apply = rhs.m_timeto_pause_apply;
    m_apptype = rhs.m_apptype;
	m_bookmark = rhs.m_bookmark;
}

bool FLUSH_AND_HOLD_REQUEST::operator==( FLUSH_AND_HOLD_REQUEST const& rhs ) const {
    return m_consistency_type == rhs.m_consistency_type &&
        m_timeto_pause_apply == rhs.m_timeto_pause_apply &&
        m_apptype == rhs.m_apptype &&
		m_bookmark == rhs.m_bookmark;
}

/*SRM: end*/

bool HOST_VOLUME_GROUP_SETTINGS::gettargetvolumes(std::vector<std::string>& targetvollist)
{
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        if((*vgiter).direction == TARGET)
        {
			for(VOLUME_GROUP_SETTINGS::volumes_iterator viter = (*vgiter).volumes.begin();viter != (*vgiter).volumes.end(); ++viter)
            {
				targetvollist.push_back(viter->first);				
            }
        }
    }
	return true;
}


SV_ULONGLONG HOST_VOLUME_GROUP_SETTINGS::getResyncStartTimeStamp(const std::string &deviceName)
{
    SV_ULONGLONG resyncStartTimeStamp = 0;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        else
        {
			resyncStartTimeStamp= ctVolumeIter->second.srcResyncStarttime;
            break;
        }
    }

    return resyncStartTimeStamp;
}

SV_ULONGLONG HOST_VOLUME_GROUP_SETTINGS::getResyncEndTimeStamp(const std::string &deviceName) 
{
    SV_ULONGLONG resyncEndTimeStamp = 0;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        else
        {
			resyncEndTimeStamp = ctVolumeIter->second.srcResyncEndtime;
            break;
        }
    }

    return resyncEndTimeStamp;
}

SV_ULONG HOST_VOLUME_GROUP_SETTINGS::getResyncStartTimeStampSeq(const std::string &deviceName)
{
    SV_ULONG resyncStartSeqNum = 0;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        else
        {
			resyncStartSeqNum= ctVolumeIter->second.srcResyncStartSeq;
            break;
        }
    }

    return resyncStartSeqNum;
}

SV_ULONG HOST_VOLUME_GROUP_SETTINGS::getResyncEndTimeStampSeq(const std::string &deviceName)
{
    SV_ULONGLONG resyncEndSeq = 0;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        else
        {
			resyncEndSeq= ctVolumeIter->second.srcResyncEndSeq;
            break;
        }
    }

    return resyncEndSeq;
}

TRANSPORT_PROTOCOL HOST_VOLUME_GROUP_SETTINGS::getProtocol(const std::string &deviceName)
{
	TRANSPORT_PROTOCOL protocol = TRANSPORT_PROTOCOL_UNKNOWN;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        else
        {
			protocol= ctVolumeIter->second.transportProtocol;
            break;
        }
    }

    return protocol;
}

bool HOST_VOLUME_GROUP_SETTINGS::getSecureMode(const std::string &deviceName)
{
    bool secureMode =false;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        else
        {
			secureMode= ctVolumeIter->second.secureMode;
            break;
        }
    }

    return secureMode;
}

bool HOST_VOLUME_GROUP_SETTINGS::isInQuasiState(const std::string &deviceName)
{
    bool isInQuasiState = false;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        else
        {
			isInQuasiState = (ctVolumeIter->second.syncMode==VOLUME_SETTINGS::SYNC_QUASIDIFF);
            break;
        }
    }

    return isInQuasiState;
}

bool HOST_VOLUME_GROUP_SETTINGS::isResyncRequiredFlag(const std::string &deviceName)
{
    bool  isResyncRequired = false;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        else
        {
			isResyncRequired= ctVolumeIter->second.resyncRequiredFlag;
            break;
        }
    }

    return isResyncRequired;
}

SV_ULONGLONG HOST_VOLUME_GROUP_SETTINGS::getResyncRequiredTimestamp(const std::string &deviceName)
{
    SV_ULONGLONG resyncRequiredTimeStamp = 0;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        else
        {
			resyncRequiredTimeStamp= ctVolumeIter->second.resyncRequiredTimestamp;
            break;
        }
    }

    return resyncRequiredTimeStamp;
}

VOLUME_SETTINGS::RESYNCREQ_CAUSE HOST_VOLUME_GROUP_SETTINGS::getResyncRequiredCause(const std::string &deviceName)
{
	VOLUME_SETTINGS::RESYNCREQ_CAUSE resyncReqCause;
    for(volumeGroups_iterator vgiter = volumeGroups.begin();
        vgiter != volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_name(deviceName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        else
        {
			resyncReqCause = ctVolumeIter->second.resyncRequiredCause;
            break;
        }
    }

    return resyncReqCause;
}

VOLUME_SETTINGS::TARGET_DATA_PLANE HOST_VOLUME_GROUP_SETTINGS::getTargetDataPlane(const std::string & deviceName)
{
	VOLUME_SETTINGS::TARGET_DATA_PLANE targetDataPlane = VOLUME_SETTINGS::UNSUPPORTED_DATA_PLANE;
	for (volumeGroups_iterator vgiter = volumeGroups.begin();
		vgiter != volumeGroups.end(); ++vgiter)
	{
		VOLUME_GROUP_SETTINGS vg = *vgiter;
		VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter = vg.find_by_name(deviceName);
		if (ctVolumeIter == vg.volumes.end())
		{
			continue;
		}
		else
		{
			targetDataPlane = ctVolumeIter->second.getTargetDataPlane();
			break;
		}
	}

	return targetDataPlane;
}

SV_ULONGLONG HOST_VOLUME_GROUP_SETTINGS::GetEndpointRawSize(const std::string & deviceName) const
{
	SV_ULONGLONG rawSize;
	for (volumeGroups_constiterator vgiter = volumeGroups.begin();
		vgiter != volumeGroups.end(); ++vgiter)
	{
		VOLUME_GROUP_SETTINGS vg = *vgiter;
		VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter = vg.find_by_name(deviceName);
		if (ctVolumeIter == vg.volumes.end())
		{
			continue;
		}
		else
		{
			rawSize = ctVolumeIter->second.GetEndpointRawSize();
			break;
		}
	}

	return rawSize;
}

std::string HOST_VOLUME_GROUP_SETTINGS::GetEndpointDeviceName(const std::string & deviceName) const
{
	std::string endPointDeviceName;
	for (volumeGroups_constiterator vgiter = volumeGroups.begin();
		vgiter != volumeGroups.end(); ++vgiter)
	{
		VOLUME_GROUP_SETTINGS vg = *vgiter;
		VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter = vg.find_by_name(deviceName);
		if (ctVolumeIter == vg.volumes.end())
		{
			continue;
		}
		else
		{
			endPointDeviceName = ctVolumeIter->second.GetEndpointDeviceName();
			break;
		}
	}

	return endPointDeviceName;
}

std::string HOST_VOLUME_GROUP_SETTINGS::GetResyncJobId(const std::string & deviceName) const
{
	std::string resyncJobId;
	for (volumeGroups_constiterator vgiter = volumeGroups.begin();
		vgiter != volumeGroups.end(); ++vgiter)
	{
		VOLUME_GROUP_SETTINGS vg = *vgiter;
		VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter = vg.find_by_name(deviceName);
		if (ctVolumeIter == vg.volumes.end())
		{
			continue;
		}
		else
		{
			resyncJobId = ctVolumeIter->second.GetResyncJobId();
			break;
		}
	}

	return resyncJobId;
}

int  HOST_VOLUME_GROUP_SETTINGS::GetDeviceType(const std::string & deviceName) const
{
	int deviceType;
	for (volumeGroups_constiterator vgiter = volumeGroups.begin();
		vgiter != volumeGroups.end(); ++vgiter)
	{
		VOLUME_GROUP_SETTINGS vg = *vgiter;
		VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter = vg.find_by_name(deviceName);
		if (ctVolumeIter == vg.volumes.end())
		{
			continue;
		}
		else
		{
			deviceType = ctVolumeIter->second.GetDeviceType();
			break;
		}
	}

	return deviceType;
}

std::string HOST_VOLUME_GROUP_SETTINGS::GetEndpointHostId(const std::string & deviceName) const
{
	std::string endPointHostId;
	for (volumeGroups_constiterator vgiter = volumeGroups.begin();
		vgiter != volumeGroups.end(); ++vgiter)
	{
		VOLUME_GROUP_SETTINGS vg = *vgiter;
		VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter = vg.find_by_name(deviceName);
		if (ctVolumeIter == vg.volumes.end())
		{
			continue;
		}
		else
		{
			endPointHostId = ctVolumeIter->second.GetEndpointHostId();
			break;
		}
	}

	return endPointHostId;
}

std::string HOST_VOLUME_GROUP_SETTINGS::GetEndpointHostName(const std::string & deviceName) const
{
	std::string endPointHostName;
	for (volumeGroups_constiterator vgiter = volumeGroups.begin();
		vgiter != volumeGroups.end(); ++vgiter)
	{
		VOLUME_GROUP_SETTINGS vg = *vgiter;
		VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter = vg.find_by_name(deviceName);
		if (ctVolumeIter == vg.volumes.end())
		{
			continue;
		}
		else
		{
			endPointHostName = ctVolumeIter->second.GetEndpointHostName();
			break;
		}
	}

	return endPointHostName;
}
