#ifndef VOLUME_RECOVERY_HELPERS_SETTINGS_H
#define VOLUME_RECOVERY_HELPERS_SETTINGS_H

#include <iostream>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <boost/lexical_cast.hpp>
#include "svtypes.h"
struct CPUInfo
{
    std::string cpuId ;
    std::string architecture ;
    std::string manufacturer ;
    SV_UINT number_of_cores ;
    SV_UINT number_of_logical_processors ;
    std::string name ;
} ;
struct OSInfo
{
    std::string build ;
    std::string caption ;
    std::string major_version ;
    std::string minor_version ;
    std::string name ;
} ;
struct HostInfo
{
	std::string HostName ;
	std::string HostIP ;
	std::string HostAgentGUID ;
	std::string SystemGUID ;
	std::string RepoPath ;
    std::string RecoveryFeaturePolicy ;
    std::string AgentVersion ;
	std::string SystemDir ;
    std::list<CPUInfo> CPUInfoList ;
    OSInfo osInfo ;
    bool operator==(HostInfo const& rhs)
    {
        return (HostName == rhs.HostName
            && HostIP == rhs.HostIP
            && HostAgentGUID == rhs.HostAgentGUID
            && SystemGUID == rhs.SystemGUID
            && RecoveryFeaturePolicy == rhs.RecoveryFeaturePolicy 
			&& RepoPath == rhs.RepoPath );
    }

    bool operator!=(HostInfo const& rhs)
    {
        return !operator==(rhs);
    }    
};
typedef std::string SystemGUID_t;
typedef std::map<SystemGUID_t,HostInfo> HostInfoMap;
//key:SystemGUID //systemId

enum RECOVERY_ACCURACY { UNKNOWN, ACCURATE, APPROXIMATE, NOT_GUARANTEED } ;

struct RecoveryTimeRange
{
	std::string StartTime ;
	std::string EndTime ;
	std::string StartTimeUTC ;
	std::string EndTimeUTC ;
	std::string Accuracy ;
};
struct CommonRecoveryTimeRange
{
	std::string StartTime ;
	std::string EndTime ;
} ;

struct RestorePointInfo
{
	std::string RestorePoint;
	std::string RestorePointGUID;
	std::string RestorePointTimestamp;
	std::string Accuracy ;
	std::string ApplicationType;
};
typedef std::string Rangeno_t;
typedef std::map<Rangeno_t, RecoveryTimeRange> RecoveryRangeMap ;
//Key : range1,2...

typedef std::map<std::string, RestorePointInfo> RestorePointMap ;
//Key : Tag Name

struct VolumeRestorePoints
{
	std::string VolumeName ;
	RecoveryRangeMap recoveryRanges ;
	RestorePointMap restorePoints ;
};
typedef std::string VolName_t;
typedef std::map<VolName_t, VolumeRestorePoints> VolumeRestorePointsMap ;
//Key : Volume Name

struct VolumeErrorDetails
{
	std::string VolumeName ;
	SV_ULONG ReturnCode ;
	std::string ErrorMessage ;
};
typedef std::map<VolName_t, VolumeErrorDetails> VolumeErrors ;
//Key : Volume Name

struct VolumeRecoveryDetails
{
	std::string restorePoint;
	std::string recoveryTarget ;//orginal source volume
	std::string recoverySrc;//orginal target volume
	std::string retentionPath;//need to check
	std::string recoverySrcCapacity;//Actaul source volume capacity
};
typedef std::map<VolName_t, VolumeRecoveryDetails> VolumeRecoveryDetailsMap ;
//Key : Volume Name (protection source volume)

enum SNAPSHOT_TYPE { PHYSICAL, VIRTUAL } ;
struct VolumeSnapshotDetails
{
	SNAPSHOT_TYPE type ;
	SV_ULONGLONG restorePoint;
	std::string mountPointDirectory ;
	std::string retentionPath;
};
typedef std::map<VolName_t, VolumeSnapshotDetails> VolumeSnapshotDetailsMap ;
//Key : Volume Name(protection source volume)



//for GetExportedRespositoryDetailsByAgentGUID
struct CifsShareInfo
{
	std::string ShareName;
	std::string TargetIP;
    std::string Status;
    std::string ReturnCode;
    std::string ErrorMessage;
};
struct NFSShareInfo
{
	std::string RepositoryPath;
	std::string TargetIP;
    std::string Status;
    std::string ReturnCode;
    std::string ErrorMessage;
};
//commented ISCSI related for now
//struct ISCSIShareInfo
//{
//	std::string	ISCSITargetAlias;
//	std::string	ISCSITargetIP;
//	std::string	ISCSITargetPortNumber;
//	std::string	Status;
//	std::string	ReturnCode;
//	std::string	ErrorMessage;
//};

typedef std::string RepoName_t;
//typedef std::map<RepoName_t,ISCSIShareInfo> ISCSIShareMap_t;
typedef std::map<RepoName_t,CifsShareInfo> CIFSShareMap_t;
typedef std::map<RepoName_t,NFSShareInfo> NFSShareMap_t;

struct ExportedRepoDetails
{
	//ISCSIShareMap_t ISCSI;
	CIFSShareMap_t  CIFS;
	NFSShareMap_t NFS;
};

//ExportRepositoryOnCIFS
struct RepoCIFSExport
{
	std::string RepositoryName;
	//if agentguid is not mentioned repository will exported for all machines
	//std::string AgentGUID;
	std::string ShareName;
	std::string UserName;
	std::string Password;
	std::string Domain;
	std::string IPsAllowed;
};
typedef	std::map<std::string,RepoCIFSExport> RepoCIFSExportMap_t;
//key:RepositoryName	

//ExportRepositoryOnNFS
struct RepoNFSExport
{
	std::string RepositoryName;
	std::string UserName;
	std::string Password;
	std::string Domain;
	std::string IPsAllowed;
};
typedef	std::map<std::string,RepoNFSExport> RepoNFSExportMap_t;
//key:RepositoryName

//ListVolumesWithProtectionDetailsByAgentGUID
struct VolumeInformation
{
    std::string VolumeName;
    std::string VolumeMountPoint;
    std::string VolumeUsedAs;
    std::string VolumeType;
	std::string Capacity;
    std::string VolumeLabel ;
    std::string RawCapacity ;
	std::string FileSystemType ;
};

struct VolProtectionDetails
{
    std::string RepositoryName;
    std::string	RepositoryPath;
    std::string	TargetVolume;
    std::string PairState;
	std::string RetentionDataPath ;
};
       
typedef std::map<Rangeno_t, RecoveryTimeRange> RecoveryRangeMap_t ;

typedef std::string TagGUID_t ;
typedef std::map<TagGUID_t, RestorePointInfo> RestorePointMap_t ;		
        
struct ProtectionInfo
{
    VolProtectionDetails ProtectionDetails;
    RecoveryRangeMap_t RecoveryRanges ;
    RestorePointMap_t RestorePoints ;
};
typedef std::string TargetGUID_t;
typedef std::map<TargetGUID_t,ProtectionInfo> VolumeRestorePointsMap_t;
    
struct VolumeProtectionRecoveryInfo
{
    VolumeInformation VolumeDetails;
    VolumeRestorePointsMap_t VolumeRestorePoints;
};
typedef std::string  VolumeGUID_t ;
typedef std::map<VolumeGUID_t, VolumeProtectionRecoveryInfo> VolumeProtectionRecoveryInfo_t;
                
typedef std::map< Rangeno_t, CommonRecoveryTimeRange> CommonRecoveryRangeMap_t ;
typedef std::map< TagGUID_t, RestorePointInfo> CommonRestorePointMap_t ;	
        
struct CommonRecoveryData
{
    CommonRecoveryRangeMap_t CommonRecoveryRanges ;
    CommonRestorePointMap_t CommonRestorePoints ;
};
        
typedef std::map<TargetGUID_t,CommonRecoveryData> CommonRestorePointsMap_t;
        
struct ListVolumesWithProtectionDetails
{
    VolumeProtectionRecoveryInfo_t VolumeLevelProtectionInfo;
    CommonRestorePointsMap_t CommonRestorePointInfo;		
};

struct PauseResumeDetails
{
	std::list<std::string> VolumeList ;
	std::string AgentGUID ;
	bool PauseDirectly ;
	std::string RecoveryMode ;
};

struct AvailableDriveLetters
{
	std::list<std::string> availableDrives ;
} ;

#endif // VOLUME_RECOVERY_HELPERS_SETTINGS_H