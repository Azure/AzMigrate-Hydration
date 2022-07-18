#ifndef APPLICATION_SETTINGS_H
#define APPLICATION_SETTINGS_H
#include "appglobals.h"
//#ifdef SV_WINDOWS
//#include "serializeinitialsettings.h"
//#include "xmlizeinitialsettings.h"
//#else
#include "initialsettings.h"
//#endif
#include <map>
#include <string>
#include <list>
#include <algorithm>

#include "svtypes.h"
#ifdef SV_WINDOWS
#include <clusapi.h>
#include <atlconv.h>
#include <ResApi.h>
#endif
#include "task.h"
struct VolPackAction
{
  std::string m_version ;
  VOLPACK_ACTION m_action ;
  SV_LONGLONG m_size ;
  VolPackAction()
  {
	m_version = "1.0";
	m_size = -1;
  }
} ;

struct VolPacks
{
  std::string m_version ;
  std::map<std::string, VolPackAction> m_volPackActions ; //mount pointis the key.
} ;

struct VolPackUpdate
{
    std::string m_version ;
    std::map<std::string, std::string> m_volPackMountInfo ;
    VolPackUpdate()
    {
         m_version = "1.0" ;
    }
};

struct ReadinessCheck
{
    std::string m_version ;
    ReadinessCheck()
    {
        m_version = "1.0" ;
    } 
    std::map<std::string, std::string> m_properties ;
    /*
        Property Keys
            RuleId
            RuleStatus
            RuleMessage
    */
} ;
struct ReadinessCheckInfo
{
    std::string m_version ;
    ReadinessCheckInfo()
    {
        m_version = "1.0" ;
    } 
	std::map<std::string, std::string> propertiesList;
	/*
	    Property Keys
		    policyId
		    ApplicationInstanceId    
            SrcApplicationInstanceId
	*/
	std::list<ReadinessCheck> validationsList ;
} ;
struct ReadinessCheckInfoUpdate
{
    std::string m_version ;
    ReadinessCheckInfoUpdate()
    {
        m_version = "1.0" ;
    } 
    std::list<ReadinessCheckInfo> m_readinessCheckInfos ;
} ;

struct InmServiceProperties
{
    std::string m_version ;
    InmServiceProperties()
    {
        m_version = "1.0" ;
    } 
	std::map<std::string, std::string> m_serviceProperties ;
	/*
		ServiceName
		StartupType
		ServiceStatus
		LogonUser
		Dependencies
	*/
} ;
struct NICInformation
{
    std::string m_version ;
    NICInformation()
    {
        m_version = "1.0" ;
    } 
	std::map<std::string, std::string> m_nicProperties ;
	/*
		Property Keys
			MACAddress				
	*/
	std::list<std::string> m_IpAddresses ;
} ;


struct ClusterNwResourceInfo
{
    std::string m_version ;
    ClusterNwResourceInfo()
    {
        m_version = "1.0" ;
    } 
	std::map<std::string, std::string> m_resourceProperties ;
	/* 
		Property Keys
			VirtualServerName
			OwnerNode
			ResourceCurrentState
	*/		
	std::list<std::string> m_ipsList ;
} ;
struct ClusResourceInfo
{
	std::string m_version ;
    ClusResourceInfo()
    {
        m_version = "1.0" ;
    }
	std::map<std::string, std::string> m_ResourceProps;
	/*
		Resoruce Name
		Resoruce Type
		State
		--Resource specific private properties--
	*/
	std::list<std::string> m_Dependencies;
};
struct ClusGroupInfo
{
	std::string m_version ;
    ClusGroupInfo()
    {
        m_version = "1.0" ;
    }
	std::map<std::string, std::string> m_GrpProps;
	std::list<std::string> m_prefOwners;
	/*
	  Group Name
	  Owner Node
	  State
	*/
	std::map<std::string, ClusResourceInfo> m_Resources;
};
struct ClusInformation
{
    std::string m_version ;
    ClusInformation()
    {
        m_version = "1.0" ;
    } 
	std::map<std::string, std::string> m_clusProperties ;
	/*
		Right Now properties map will be empty
		ClusterName
		ClusterId
	*/
	
	std::list<ClusterNwResourceInfo> m_nwNameInfoList ;
	std::map<std::string, ClusGroupInfo> m_ClusGroups ;
	std::list<std::string> m_nodes;
} ;
struct VSSProviderDetails
{
    std::string m_version ;
    VSSProviderDetails()
    {
        m_version = "1.0" ;
    } 
	std::map<std::string, std::string> m_providerProperties ;
	/*
			ProviderName
			ProviderGuid
			ProviderType
	*/
} ;	
struct MailBox
{
    std::string m_version ;
    MailBox()
    {
        m_version = "1.0" ;
    } 
	std::map<std::string, std::string> m_mailBoxProperties ;
	/*
		Property Keys
			MailBoxName
			OnlineStatus
			TransactionLogPath
			MailStoreType
			PublicFolderDatabaseMailServerName
			PublicFolderDatabaseStorageGroup
			PublicFolderDatabaseName
	*/
    std::map<std::string,std::string> mailstoreFiles ;
    std::list<std::string> m_copyHosts ;
} ;

//1. for exchange 2010 OwingServer key should have proper value.
//2. for other exchange versions OwningServer key is optional.
//3. if exchange 2010 and OwningServer is local host m_volumes the mail store volumes.
//4. if exchange 2010 m_copyHosts contain list of hosts on which mail store copies exist
//5. TransactionLogPath is for exchange 2010 only. as there are no are no storage groups.


struct StorageGroup 
{
    std::string m_version ;
    StorageGroup()
    {
        m_version = "1.0" ;
    } 
	std::map<std::string, std::string> m_StorageGrpProperties ;
	/* 
		Property Keys
			StorageGroupName
			TransactionLogPath
			LCRStatus
			SCRStatus
			CCRStatus
			ClusterStorageType
			OnlineStatus
	*/
	std::list<std::string> m_volumes ;		
	std::map<std::string, MailBox> m_mailBoxes ; /*key is mailbox name*/
} ;	
struct Exchange2010Data
{
    std::string m_version ;
    Exchange2010Data()
    {
        m_version = "1.0" ;
    } 
     /*
     right now the below map is empty.
    */
    std::map<std::string, std::string> dagAttributes ;
    std::map<std::string, MailBox> m_mailBoxes ; /*key is mailbox name*/
    std::list<std::string> m_dagParticipants ;
} ;

//1. for exchange 2010 ExchangeDAG is relavent.
//2. m_mailBoxes is the map of mail stores that are part of DAG.
//3. m_participants are the list of host names that are participating in DAG.
//4. if the mail store is not part of DAG all mail stores belongs to localhost. 

struct ExchangeDiscoveryInfo
{
    std::string m_version ;
    ExchangeDiscoveryInfo()
    {
        m_version = "1.0" ;
    } 
	std::map<std::string, std::string> m_policyInfo ;
	/*
		//first update will not send below entries
		applicationInstanceId  
		policyId
	*/
	std::map<std::string, std::string> m_InstanceInformation ;
	/*
		Property Keys
			ApplicationVersion
			ApplicationEdition
			ApplicationCompatibiltiNo
			InstallPath
			IsClustered
			VirtualServerName
			ServerRole //Edge Transport Role, Hub Transport Role, Mailbox Role, Client Access Role, Unified Messaging Role
			ServerType //Front End, Back End
			DistinguishedName
			ApplicationInstanceName
            DataAvailabilityGroup (optional)
	*/
	std::map<std::string, StorageGroup> m_storageGrpMap ;  
	Exchange2010Data m_exchange2010Data ;
	std::map<std::string, InmServiceProperties> m_services ;
} ;	

//1. if exchange version is 2003/2007 m_dagMap is empty.
//2. if exchange version is 2010 m_storageGrpMap is empty.
//3. if DataAvailabilityGroup is DAGNAME . if it is not part of DAG this key is not present.
//4. isCluster not is not applicable for 2k10

struct MSSQLDBMetaDataInfo
{
    std::string m_version ;
    MSSQLDBMetaDataInfo()
    {
        m_version = "1.0" ;        
    }
    std::map<std::string, std::string> m_properties ;
    /*
        Property keys
        Status
        DbType
    */
	std::list<std::string> m_dbVolumes ;
    std::list<std::string> m_LogFilesList;
} ;

struct OracleDBInstanceInfo
{
    std::map<std::string, std::string> m_dbProperties;

    /*
       Property Keys
         DBName
         DBVersion
         isClustered
         NodeName
         RecoveryLogEnabled
         DBInstallPath

    */

    std::list<std::string> m_otherClusterNodes;

    std::list<std::string> m_volumeInfo;
    
    std::list<std::string> m_filterDevices;

    std::map<std::string, std::string> m_files;

};

struct Db2DatabaseInfo
{
   std::map<std::string, std::string> m_dbProperties;
   /*
    * Property Keys
    *    DBInstance
    *    DBName
    *    DBVersion
    *    NodeName
    *    RecoveryLogEnabled
    *    DBInstallPath
    *    */

   std::list<std::string> m_volumeInfo;

   std::list<std::string> m_filterDevices;

   std::map<std::string, std::string> m_files;
};

struct OracleClusterInfo
{
    std::string m_clusterName;
    std::string m_nodeName;
    std::list<std::string> m_otherNodes;
};

struct OracleUnregisterInfo
{
    std::string m_dbName;
};

struct Db2UnregisterInfo
{
    std::string m_dbName;
};

struct ArrayDiscoveryInfo
{
    std::string m_version;
    ArrayDiscoveryInfo()
    {
        m_version = "1.0";
    }
    std::map<std::string, std::string> m_policyInfo ;
} ;

struct OracleAppDevInfo
{
    std::list<std::string> m_devNames;
};

struct OracleDiscoveryInfo
{
    std::string m_version;

    OracleDiscoveryInfo()
    {
        m_version = "1.0";
    }

    std::map<std::string, std::string> m_policyInfo ;

    std::map<std::string, OracleDBInstanceInfo> m_oracleDBInstanceInfoMap;

    std::map<std::string, OracleClusterInfo> m_oracleClusterInfoMap;

    std::map<std::string, OracleUnregisterInfo> m_oracleUnregisterInfoMap;

    std::map<std::string, OracleAppDevInfo> m_dbOracleAppDevInfo;

} ;

struct Db2DatabaseDiscoveryInfo
{
    std::string m_version;

    Db2DatabaseDiscoveryInfo()
    {
       m_version = "1.0";
    }

    std::map<std::string, std::string> m_policyInfo ;

    std::map<std::string, Db2DatabaseInfo> m_db2InstanceInfoMap;

    std::map<std::string, Db2UnregisterInfo> m_db2UnregisterInfoMap;

};

struct SQLDiscoveryInfo
{
    std::string m_version ;
    SQLDiscoveryInfo()
    {
        m_version = "1.0" ;        
    }
    std::map<std::string, std::string> m_policyInfo ;
	/*
		//first update will not send below entries
		applicationInstanceId  
		policyId
	*/
    std::map<std::string, std::string> m_InstanceInformation ;
	/*
		Property Keys
			ApplicationInstanceName
			ApplicationVersion
			ApplicationEdition
			ApplicationCompatibiltiNo
			InstallPath
			DataRoot
			IsClustered
			VirtualServerName
	*/
    std::map<std::string, MSSQLDBMetaDataInfo> m_sqlMetaDatInfo ;
    std::map<std::string, InmServiceProperties> m_services ;
    
} ;

struct ShareProperties
{
 std::map<std::string, std::string> m_properties;
 /*
  ShareName
  MountPoint(volumepathname)
  AbsolutePath
  Shares
  Security

  //For W2K8
  ShareUsers
  SecurityPwd
  ShareRemark
  ShareType
 */
};

struct FileServerInfo
{
    std::string m_version ;
    FileServerInfo()
    {
        m_version = "1.0" ;
    } 
	std::map<std::string, std::string> m_policyInfo ;
	/*
	//first update will not send below entries
	applicationInstanceId  
	policyId
	*/
	std::map<std::string, std::string> m_InstanceInformation ;
	/*
	Property Keys
	ApplicationInstanceName --> networkname. can be host name/virtual server name.
	IsClustered --> represents local shares/ cluster shares.
	Version --> registry version string.	
	*/
	std::list<std::string> m_ipAddressList; //--> assosiated to the above network name.
	std::map<std::string, ShareProperties> m_shareProperties ;// share props belogs to the above network name.
	/* keys  ShareName.  */ 
	std::list<std::string> m_volumes;
} ;
struct DiscoveryInfo
{
    std::string m_version ;
    DiscoveryInfo()
    {
        m_version = "1.0" ;
    } 
	std::map<std::string, std::string> m_properties ;
	/*
		domainName
	*/
	std::list<NICInformation> m_nwInfo ;
	std::list<VSSProviderDetails> m_vssProviders ;
	ClusInformation ClusterInformation ;
	
	std::list<ExchangeDiscoveryInfo> exchInfo ;
    std::list<SQLDiscoveryInfo> sqlInfo ;
	std::list<FileServerInfo> fileserverInfo ;
    std::list<OracleDiscoveryInfo> oraclediscoveryInfo;
    std::list<ArrayDiscoveryInfo> arrayDiscoveryInfo;
    std::list<Db2DatabaseDiscoveryInfo> db2discoveryInfo;
} ;

struct DiscoveryData
{
    std::string m_version ;
    DiscoveryData()
    {
        m_version = "1.0" ;
    } 
    std::list<std::string> m_InstancesList;
};

struct ReadinessCheckInfoData
{
    std::string m_version ;
    ReadinessCheckInfoData()
    {
        m_version = "1.0" ;
    } 
    std::list<std::string> m_InstancesList;
} ;
    
struct PrePareTargetData
{
    std::string m_version ;
    PrePareTargetData()
    {
        m_version = "1.0" ;
    } 
    std::list<std::string> m_InstancesList;
};

struct ConsistencyData
{
    std::string m_version ;
    ConsistencyData()
    {
        m_version = "1.0" ;
    }
    std::list<std::string> m_InstancesList;
};

struct UnregisterInfo
{
    std::string m_version ;
    UnregisterInfo()
    {
       m_version = "1.0" ;
    }
    std::list<std::string> m_InstancesList;
};

struct ConsistencyTagData
{
	std::map<std::string, std::string> m_tagOptions ;
	/*
	TagName
	TagType
	ApplicationType
	*/

	std::list<std::string> m_volumes ;
    std::list<std::string> m_InstancesList;
	SV_UINT timeout ;
} ;

struct TargetReadinessCheckInfo
{

    // Keys
    // SrcVolume
    // SrcCapacity
    // TgtVolume
    // TgtCapacity
    std::list<std::map<std::string, std::string> > m_volCapacities;

    // Keys
    // DeviceName
    // VolumeType
    // VendorOrigin
    // MountPoint
    // FileSystemType
    // shouldDestroy
    std::list<std::map<std::string, std::string> > m_tgtVolumeInfo;
};

struct OracleTargetReadinessCheckInfo
{
    // Keys -
    // srcVolName
    // srcVolCapacity
    // tgtVolName
    // tgtVolCapacity
    std::list<std::map<std::string, std::string> > m_volCapacities;

    // Keys -
    // DeviceName
    // VolumeType
    // VendorOrigin
    // MountPoint
    // FileSystemType
    // shouldDestroy
    std::list<std::map<std::string, std::string> > m_tgtVolumeInfo;

    // Keys -
    // dbName
    // configFileName
    // configFileContents
    std::list<std::map<std::string, std::string> > m_oracleSrcInstances;

    // Keys -
    // dbName
    // Status
    std::list<std::map<std::string, std::string> > m_oracleTgtInstances;
};

struct Db2TargetReadinessCheckInfo
{
    // Keys -
    // srcVolName
    // srcVolCapacity
    // tgtVolName
    // tgtVolCapacity
    std::list<std::map<std::string, std::string> > m_volCapacities;

    // Keys -
    // DeviceName
    // VolumeType
    // VendorOrigin
    // MountPoint
    // FileSystemType
    // shouldDestroy
    std::list<std::map<std::string, std::string> > m_tgtVolumeInfo;

    // Keys -
    // dbName
    // configFileName
    // configFileContents
    std::list<std::map<std::string, std::string> > m_db2SrcInstances;

    // Keys -
    // dbName
    // Status
    std::list<std::map<std::string, std::string> > m_db2TgtInstances;

};

struct ApplianceTargetLunCheckInfo
{
    std::string             m_applianceTargetLUNNumber;
    std::string             m_applianceTargetLUNName;
    std::list<std::string>  m_physicalInitiatorPWWNs;
    std::list<std::string>  m_applianceTargetPWWNs;
};

struct SourceReadinessCheckInfo
{
    ApplianceTargetLunCheckInfo m_atLunCheckInfo;
};

struct MSSQLTargetReadinessCheckInfo
{
	// First map Key being instance name
    // std::map<std::string,MSSQLInstanceTgtReadinessCheckInfo> m_sqlInstanceTgtReadinessCheckInfo ;
	// Nested map keys -
	// SrcVersion 
	// SrcDataRoot
	// TgtVersion 
	// TgtDataRoot
	// SrcVirtualServerName
    // TgtVirtualServerName
	// SrcHostName
	std::map<std::string, std::map<std::string, std::string> > m_sqlInstanceTgtReadinessCheckInfo ;
    // Keys -
    // srcVolName
    // srcVolCapacity
    // tgtVolName
    // tgtVolCapacity
    std::list<std::map<std::string, std::string> > m_volCapacities;
    std::list<std::string> m_tempDbVols ;
    std::list<std::string> m_InstancesList;
} ;

struct ExchangeTargetReadinessCheckInfo
{
	// Keys -
	// Version 
	// Dn 	
    // SrcVirtualServerName
    // TgtVirtualServerName
	// SrcHostName
	// isMTA
	std::map<std::string,std::string> m_properties ;
    // Keys -
    // srcVolName
    // srcVolCapacity
    // tgtVolName
    // tgtVolCapacity
    std::list<std::map<std::string, std::string> > m_volCapacities;	
	std::list<std::string> m_InstancesList;
} ;

struct FileServerTargetReadinessCheckInfo
{
	// Keys
    // SrcVolume
    // SrcCapacity
    // TgtVolume
    // TgtCapacity
    std::list<std::map<std::string, std::string> > m_volCapacities;
	std::list<std::string> m_InstancesList;
};

struct ApplicationPolicy
{
    std::string m_version ;
    ApplicationPolicy()
    {
        m_version = "1.0" ;
    } 
    std::map<std::string, std::string> m_policyProperties ;
   	/*
		properties
	        PolicyParameters
	        PolicyType    //Discovery, SourceReadinessCheck, TargetReadinessCheck, Consistency, PrepareTarget.
	        PolicyId
	        ScheduleType  //1,2 // mot using for now.
	        ScheduleInterval // in the form of seconds.
			ExcludeFrom
			ExcludeTo
            InstanceName
            ApplicationType
            SolutionType      // HOST or PRISM
            ApplicationInstanceId
			ScenarioType //Protection, Failover, Fast-Failback
			Context // HA-DR, Backup, Bulk
	*/
	//ScheduleType, ScheduleInterval, ExcludeFrom, ExcludeTo are only for discovery. for readiness check and targetreadiness check we should 
	//not consider these params.

	std::string m_policyData ; 

	bool isThisParamEqual(std::string policyPropertyKey, const std::map<std::string, std::string>& currentSettingPolicyParamsMap) const
	{
		bool bRetValue = false;
		if(m_policyProperties.find(policyPropertyKey.c_str()) != m_policyProperties.end() &&
			currentSettingPolicyParamsMap.find(policyPropertyKey.c_str()) != currentSettingPolicyParamsMap.end())
		{
			std::string tempString1;
			tempString1 = m_policyProperties.find(policyPropertyKey.c_str())->second;
			std::string tempString2;
			tempString2 = currentSettingPolicyParamsMap.find(policyPropertyKey.c_str())->second;	
			if(tempString1.compare(tempString2) == 0 )
			{
				bRetValue = true;
			}
		}
		else if(m_policyProperties.find(policyPropertyKey.c_str()) == m_policyProperties.end() &&
			currentSettingPolicyParamsMap.find(policyPropertyKey.c_str()) == currentSettingPolicyParamsMap.end())
		{
			bRetValue = true;
		}
		
		return bRetValue;
	}

	bool operator ==(const ApplicationPolicy& settings) const 
	{
		bool bRetValue = false;
		if( isThisParamEqual("PolicyType", settings.m_policyProperties) &&
			isThisParamEqual("PolicyId", settings.m_policyProperties) && 
			isThisParamEqual("ScheduleType", settings.m_policyProperties) &&
			isThisParamEqual("ScheduleInterval", settings.m_policyProperties) &&
			isThisParamEqual("ExcludeFrom", settings.m_policyProperties) &&
			isThisParamEqual("ExcludeTo", settings.m_policyProperties) &&
			isThisParamEqual("InstanceName", settings.m_policyProperties) &&
			isThisParamEqual("ApplicationType", settings.m_policyProperties))
		{
			bRetValue = true;
		}
		return bRetValue ;
	}
} ;

struct ReplicationPair
{
    std::string m_version ;
    std::map<std::string, std::string> m_properties ;
    /*
        Property Keys            
            RemoteMountPoint
            ConfiguredState
            PairState //optional if Configured State is not CONFIGURED
            ResyncRequired
    */
} ;
struct AppProtectionSettings
{
    std::string m_version ;
    std::map<std::string, std::string> m_properties ;
 /*
    Property Keys
        ApplicationType
        RemoteHostId
        ProtectionType //Failover/BackUp
        ReplicationDirection //Forward/Reverse
        ProductionServerName
        DRServerName
        SrcIPAddress
        TgtIPAddress
        ProductionVirtualServerName
        DRVirtualServerName
        TgtInstanceName
        Protected
    */    
    std::string srcDiscoveryInformation ;
    std::list<std::string> appVolumes ;
	std::list<std::string> CustomVolume ;
    std::map<std::string, ReplicationPair> Pairs ; 
    std::list<std::string> instanceList ;
 //local Mount Point as the key
} ;


struct ApplicationSettings
{
    std::string m_version ;
    ApplicationSettings()
    {
        m_version = "1.0" ;
    }
    ApplicationSettings(const ApplicationSettings& settings)
    {
        m_version = settings.m_version;
        m_policies = settings.m_policies;
		m_AppProtectionsettings = settings.m_AppProtectionsettings;
        timeoutInSecs = settings.timeoutInSecs;
        remoteCxs = settings.remoteCxs;
    }
    std::list<ApplicationPolicy> m_policies ;
    
	std::map<std::string,AppProtectionSettings> m_AppProtectionsettings ;
	//Inner map key can have like Version/Instance_<VersionName/InstanceName>
	// outer map remote host Id is the key
	//Inner map key source instance name/version
	//Inner map key can have like Version/Instance_<VersionName/InstanceName>

    SV_ULONG timeoutInSecs;
    REMOTECX_MAP remoteCxs;
	bool operator ==( ApplicationSettings const& settings) const 
    { 
		bool retVal = false ;
		if( m_version.compare(settings.m_version) == 0 &&
			m_policies.size() == settings.m_policies.size() &&
			m_AppProtectionsettings.size() == settings.m_AppProtectionsettings.size() &&
			timeoutInSecs == settings.timeoutInSecs &&
			remoteCxs.size() == settings.remoteCxs.size()  // need to add for remoteCX s and protection settings.
			)
		{
			if(m_policies.size() == settings.m_policies.size())
			{
				std::list<ApplicationPolicy>::const_iterator AppPolicyIter1 = m_policies.begin();
				while(AppPolicyIter1 != m_policies.end())
				{
					std::list<ApplicationPolicy>::const_iterator AppPolicyIter2 = std::find(settings.m_policies.begin(), settings.m_policies.end(), *AppPolicyIter1);
					if(  AppPolicyIter2 == settings.m_policies.end() )
					{
						break;
					}
					AppPolicyIter1++;
				}
				if(AppPolicyIter1 == m_policies.end())
				{
					retVal = true;
				}
			}
		}
		if( retVal )
        {
            DebugPrintf(SV_LOG_DEBUG, "The New Settings are same\n") ;
        }
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "The New Settings are Diffent\n") ;
        }
		return retVal ;
    }
} ;

struct SrcExchMailStoreInfo
{
    std::string m_version ;
    std::map<std::string, std::string> m_properties ;
    /*
        Key
            LogLocation  //only for exchange 2010
			GUID
			MountInfo			
    */
    std::map<std::string,std::string> mailStoreFiles ;
} ;
struct SrcExchStorageGrpInfo
{
    std::string m_version ;
    std::map<std::string, std::string> m_properties ;
    /*
        
        Key
            LogLocation
            Volume
    */
    std::map<std::string, SrcExchMailStoreInfo> m_mailStores; 
} ;
struct SrcExchangeDiscInfo
{
    std::string m_version ;
    std::map<std::string, std::string> m_properties ;
    /*
        Key
            Version
    */
    std::map<std::string, SrcExchStorageGrpInfo> m_storageGrpMap ;
} ;

struct SrcExchange2010DiscInfo
{
    std::string m_version ;
    std::map<std::string, std::string> m_properties ;
    /*
        Key
            Version
    */
    std::map<std::string, SrcExchMailStoreInfo> m_mailStores; 
} ;
struct RescoureInfo
{
	std::string rescourceName;
	CLUSTER_RESOURCE_STATE rescourceState;
	std::string resourceType;
	std::string groupName;
	std::string ownerNode;
};

struct clusterResourceInfo
{
	std::list<RescoureInfo> resourceListInfo;
};
struct SourceSQLDatabaseInfo
{
    std::string m_version ;
    std::list<std::string> m_files ;
} ;

struct SourceSQLInstanceInfo
{
    std::string m_version ;
    std::map<std::string, std::string> properties; 
    /*
        Version
    */
   std::map<std::string, SourceSQLDatabaseInfo> m_databases ; 
   std::list<std::string> m_volumes ;
} ;
struct SourceSQLDiscoveryInformation
{
    std::string m_version ;
    std::map<std::string, SourceSQLInstanceInfo> m_srcsqlInstanceInfo;
};

struct SrcFileServerDiscoveryInfomation
{
	std::string m_version;
	std::map<std::string,std::string> m_Properties;
	/*PropertyKey
		SrcRegistryVersion
		
	*/
	std::map<std::string,std::string> m_ShareInfoMap;
	std::map<std::string,std::string> m_SecurityInfoMap;
    std::list<std::string> m_VolumeList;
	std::map<std::string,std::string> m_ShareTypeMap;
	std::map<std::string,std::string> m_ShareUsersMap;
	std::map<std::string,std::string> m_AbsolutePathMap;
	std::map<std::string,std::string> m_SharePwdMap;
	std::map<std::string,std::string> m_ShareRemarkMap;
	std::map<std::string,std::string> m_mountPointMap;
};

struct SrcOracleDiscoveryInformation
{
    std::map<std::string, std::string> m_properties;
    /*dbInstallPath

    */
    std::map<std::string, std::string> m_srcFilesMap;
    //std::list<std::string> m_volumes;
};

struct SrcDb2DiscoveryInformation
{
    std::map<std::string, std::string> m_properties;
    std::map<std::string, std::string> m_srcFilesMap;
};

struct TargetOracleInfo
{
    std::string oracleHome;
    std::list<std::string> dbFileNames;

};

struct RecoveryPoints
{
    std::string m_version ;
    RecoveryPoints()
    {
        m_version = "1.0" ;
    }
    std::map<std::string, std::string> m_properties ;
    /*
    VolumeName,
    RecoveryPointType, //tag or time
    TagName, 
    TagType, 
    Timestamp 
    */ 
} ;
struct SrcFailoverPolicyData
{
} ;
struct FailoverPolicyData
{
    std::string m_version ;
    FailoverPolicyData()
    {
        m_version = "1.0" ;
    }
    std::list<std::string> services ;
    std::map<std::string, std::string> m_properties ;
    /* 
    Keys

    RecoveryPointType //LCCP, LCT, Custom ,Tag
    TagName
    TagType //FileServer/Application/User Defined
    */
    std::map<std::string, RecoveryPoints> m_recoveryPoints ; 
    std::list<std::string> m_InstancesList;
    std::map<std::string,std::string> m_RepointInfoMap;
} ;

struct FailoverCommand
{
    std::string m_Version ;
    std::string m_Command ;
    InmTaskState m_JobState ;
    SV_ULONG m_ExitCode ;
    std::string m_OutPut ;
    std::string m_StartTime ;
    std::string m_EndTime ;
    SV_INT m_State ;
    SV_INT m_GroupId;
    SV_INT m_StepId;
    SV_ULONGLONG m_InstanceId;
	SV_INT m_bUseUserToken;
    FailoverCommand(const std::string& command)
    {
        m_Version = "1.0" ;
        m_JobState = TASK_STATE_NOT_STARTED ;
        m_State = 0 ;
        m_ExitCode = 1 ;
        m_Command = command;
		m_bUseUserToken = 1;
    }
    FailoverCommand()
    {
        m_Version = "1.0" ;
        m_JobState = TASK_STATE_NOT_STARTED ;
        m_State = 0 ;
        m_ExitCode = 1 ;
		m_bUseUserToken = 1;
    }
} ;

struct FailoverJob
{
    std::string m_Version ;
    InmTaskState m_JobState ;
    std::list<FailoverCommand> m_FailoverCommands ;
    SV_ULONG m_ExitCode ;
    std::string m_StartTime ;
    std::string m_EndTime ;
    FailoverJob()
    {
        m_Version = "1.0" ;
        m_JobState = TASK_STATE_NOT_STARTED ;
        m_ExitCode = 1 ;
    }
};

struct FailoverJobs
{
    std::map<SV_ULONG, FailoverJob> m_FailoverJobsMap ;
} ;

struct FailoverCommandCx
{
    std::string m_Version;
    std::map<std::string,std::string> m_Properties;
    /*Property Key
      GroupId
      Command
      StartTime
      EndTime
      Log
      ExitCode
    */
};

struct ListStruct
{
	std::list<std::string> m_listValue;
};

struct VSnapDetails
{
	std::string m_version ;
	std::map<std::string, std::string> m_vsnapDetailsMap; // VsnapPath, RecoverTagName, RecoveryTime, VsnapFlag.
	VSnapDetails()
	{
		m_version = "1.0";
	}
};

struct ScriptPolicyDetails
{
    std::string m_version ;
	std::map<std::string, std::string> m_Prpertires; // ScriptCmd, CreateConfFile, SourceServerName, TargetServerName, SourceVirtualServerName, TargetVirtualServerName, VsnapFlags, VsnapDriveType.
	std::map<std::string, VSnapDetails> m_vSnapDetails; // volume as key and details as values.
	ScriptPolicyDetails()
	{
		m_version = "1.0";
	}
};

/*
Type: DISK
	 m_properties keys: 
	    //Type, DeviceNamePath(Local Device. It may be Direct/USB Attached Device), Label, MountPointNamePath, FileSystem.
			
Type: NFS_PATH
	 m_properties keys: 
	    //Type, DeviceNamePath(servername:/path), MountPointNamePath.

Type: CIFS_PATH   	
	 m_properties keys: 
	    //Type, DeviceNamePath(windows sharepath), MountPointNamePath, UserName, PassWord, DomainName.
*/

struct StorageRepositoryStr
{
	std::string m_version ;
	std::map<std::string, std::string> m_properties;
	StorageRepositoryStr()
	{
		m_version = "1.0";
	}
};

struct StorageRepositories
{
	std::string m_version ;
	std::list<StorageRepositoryStr> m_repositories;
	StorageRepositories()
	{
		m_version = "1.0";
	}
};

struct StorageReposUpdate
{
    std::string m_version ;
    std::map<std::string, std::string> m_setupRepoUpdate ; //key DeviceName, value Error/Success message.
    StorageReposUpdate()
    {
         m_version = "1.0" ;
    }
};

struct DeviceExportParams
{
    std::string m_version ;
    std::map<std::string, std::string> m_Exportparams ;
    DeviceExportParams()
    {
        m_version = "1.0" ;
    }
} ;

struct DeviceExportPolicyData
{
    std::string m_version ;
    std::map<std::string, DeviceExportParams> m_ExportDevices ;
    /*Key is the device that needs to be exported*/
    DeviceExportPolicyData()
    {
        m_version = "1.0" ;
    }
} ;

struct DeviceExportStatus
{
    std::string m_version ;
    std::map<std::string, std::string> m_statusParams ;
    DeviceExportStatus()
    {
        m_version = "1.0" ;
    }
} ;
struct ExportStatusUpdate
{
    std::string m_version ;
    std::map<std::string, DeviceExportStatus> m_ExportDeviceStatus ;
    ExportStatusUpdate()
    {
        m_version = "1.0" ;
    }
} ;
enum DeviceRole
{
    DEVICE_ROLE_BASE,
    DEVICE_ROLE_BASEPLV,
    DEVICE_ROLE_BASECLV,
    DEVICE_ROLE_PLV,
    DEVICE_ROLE_CLV
} ;

struct SetupRepositoryDetails
{
	std::string m_version ;
    std::string m_volumeGrpName ;
    std::map<std::string, DeviceRole> deviceNameRoleMap ;
	int CacheVolumeSizeInPercent ;
    int ProtectionVolumeSizeInPercent ;
} ;
struct SetupRepositoryStatus
{
    std::string m_version ;
    std::map<std::string, std::string> m_repositoryDetails ;
    SetupRepositoryStatus()
    {
        m_version = "1.0" ;
    }
} ;

struct SrcCloudVMDisksInfo
{
	std::string m_version;
	/* === VM Properties
	HostId
	HostName
	MBRFilePath
	OSName
	*/
	std::map<std::string,std::string> m_vmProps;

	/* === Source Disk-Name(\\.\PHYSICALDRIVE<n>) and Target-ID/Lun-number/Device-Name mapping === */
	std::map<std::string,std::string> m_vmSrcTgtDiskLunMap;
	SrcCloudVMDisksInfo()
	{
		m_version="1.0";
	}
};

struct SrcCloudVMsInfo
{
	std::string m_version;
	std::map<std::string,std::string> m_props;
	/*=== Host-ID and Disks information===*/
	std::map<std::string,SrcCloudVMDisksInfo> m_VmsInfo;
	SrcCloudVMsInfo()
	{
		m_version = "1.0";
	}
};

struct CloudVMsDisksVolsMap
{
	std::string m_version;
	//<VM-HostId,<Src_vol,Tgt_vol> >
	std::map<std::string,std::map<std::string,std::string> > m_volumeMapping;
	//<VM-HostId,<Src_disk_num,Tgt_disk_signture> >
	std::map<std::string,std::map<int,std::string> > m_diskSignatureMapping;

	CloudVMsDisksVolsMap()
	{
		m_version = "1.0";
	}
};

struct VMsRecoveryInfo
{
	std::string m_version;
	//For each vm the expected recovery attrributes as of now are
	//						TagName, TagType, HostId, HostName
	std::map<std::string, std::map<std::string,std::string> > m_mapVMRecoveryInfo;
	//<VM-HostId,<Src_vol,Tgt_vol> >
	std::map<std::string,std::map<std::string,std::string> > m_volumeMapping;
	//<VM-HostId,<Src_disk_num,Tgt_disk_signture> >
	std::map<std::string,std::map<int,std::string> > m_diskSignatureMapping;
	VMsRecoveryInfo()
	{
		m_version = "1.0";
	}
};

struct PlaceHolder
{
	std::string m_version;
	// ["ServerName"] = "HostName"  - Need to update placeholders
	std::map<std::string, std::string> m_Props;

	PlaceHolder()
	{
		m_version = "1.0";
	}
};

struct PrepareTargetVMInfo
{
	std::string m_version;
	// ["PrepareTargetStatus"] = "0-NotStarted | 1-Success | 2-Failed"
	// ["ErrorMessage"] = "Readable error message for prepare target failure"
	// ["ErrorCode"] = ""
	// ["LogFile"] = "EsxUtilWin.log"
	std::map<std::string, std::string> m_Props;
	//<"PlaceHolder", Placeholdermap>
	std::map<std::string, PlaceHolder> m_PlaceHolderProps;

	PrepareTargetVMInfo()
	{
		m_version = "1.0";
	}
};

struct VMsPrepareTargetUpdateInfo
{
	std::string m_version;
	// ["ErrorMessage"] = "Readable error message for prepare target failure at high level"
	// ["ErrorCode"] = ""
	std::map<std::string, std::string> m_updateProps;
	// <VM-Host-Id, PrepareTargetVMInfo>
	std::map<std::string, PrepareTargetVMInfo> m_MapProtectedVMs;
	//<"PlaceHolder", Placeholdermap>
	std::map<std::string, PlaceHolder> m_PlaceHolderProps;

	VMsPrepareTargetUpdateInfo()
	{
		m_version = "1.0";
	}

	void Add(const VMsPrepareTargetUpdateInfo& info)
	{
		m_updateProps.insert(info.m_updateProps.begin(), info.m_updateProps.end());
		m_MapProtectedVMs.insert(info.m_MapProtectedVMs.begin(), info.m_MapProtectedVMs.end());
		m_PlaceHolderProps.insert(info.m_PlaceHolderProps.begin(), info.m_PlaceHolderProps.end());
	}
};

struct RecoveredVMInfo
{
	std::string m_version;
	// ["RecoveryStatus"] = "0-NotStarted | 1-Success | 2-Failed"
	// ["ErrorMessage"] = "Readable error message for recovery failure"
	// ["ErrorCode"] = ""
	// ["LogFile"] = "EsxUtil.log"
	std::map<std::string, std::string> m_Props;
	//<"PlaceHolder", Placeholdermap>
	std::map<std::string, PlaceHolder> m_PlaceHolderProps;

	RecoveredVMInfo()
	{
		m_version = "1.0";
	}
};

struct VMsRecoveryUpdateInfo
{
	std::string m_version;
	// ["ErrorMessage"] = "Readable error message for recovery failure at high level"
	std::map<std::string, std::string> m_updateProps;
	// <VM-Host-Id, RecoveredVMInfo>
	std::map<std::string, RecoveredVMInfo> m_MapRecoveredVMs;
	//<"PlaceHolder", Placeholdermap>
	std::map<std::string, PlaceHolder> m_PlaceHolderProps;

	VMsRecoveryUpdateInfo()
	{
		m_version = "1.0";
	}

	void Add(const VMsRecoveryUpdateInfo& info)
	{
		m_updateProps.insert(info.m_updateProps.begin(), info.m_updateProps.end());
		m_MapRecoveredVMs.insert(info.m_MapRecoveredVMs.begin(), info.m_MapRecoveredVMs.end());
		m_PlaceHolderProps.insert(info.m_PlaceHolderProps.begin(), info.m_PlaceHolderProps.end());
	}
};

struct VMsCloneInfo
{
	std::string m_version;
	//For each vm the expected recovery attrributes as of now are
	//						TagName, TagType, HostId, HostName
	std::map<std::string, std::map<std::string,std::string> > m_mapVMRecoveryInfo;
	SrcCloudVMsInfo		 m_snapshotVMDisksInfo;
	CloudVMsDisksVolsMap m_protectedDisksVolsInfo;
	CloudVMsDisksVolsMap m_snapshotDisksVolsInfo;
	VMsCloneInfo()
	{
		m_version = "1.0";
	}
};
#endif
