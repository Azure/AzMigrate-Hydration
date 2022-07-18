/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: RecoveryConfig.cpp

Description	: AzureRecoveryConfig class is implementation.

History		: 7-5-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include "RecoveryConfig.h"
#include "../common/Trace.h"
#include "../common/AzureRecoveryException.h"
#include "../resthelper/HttpUtil.h"

#include <boost/foreach.hpp>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

namespace AzureRecovery
{

// Singleton AzureRecoveryConfig object
AzureRecoveryConfig AzureRecoveryConfig::s_config_obj;

/*
Method      : AzureRecoveryConfig::AzureRecoveryConfig
              AzureRecoveryConfig::~AzureRecoveryConfig

Description : AzureRecoveryConfig constructor & descrtuctor

Parameters  : None

Return Code : None

*/
AzureRecoveryConfig::AzureRecoveryConfig()
:m_bInitialized(false)
{
	m_logLevel = LogLevelAlways;

	m_scsiHost = DEFAULT_SCSI_HOST_NUM;

	m_sysMountPoint = DEFAULT_SYS_MOUNT_POINT;

	m_bTestFailover = false;

	m_bEnableRDP = false;

	m_bIsUefi = false;

	m_activePartitionStartingOffset = 0;
}

AzureRecoveryConfig::~AzureRecoveryConfig()
{
}

/*
Method      : AzureRecoveryConfig::SetDiskMap

Description : Reads the DiskMap section from parsed recovery config tree structure
              and sets them to disks map data member.

Parameters  : [in] tree: parsed recovery config tree structure

Return Code : None

*/
void AzureRecoveryConfig::SetDiskMap(const conf_tree& tree)
{
	TRACE_FUNC_BEGIN;

	TRACE("Parsing [DiskMap] section:\n");
	conf_tree disk_tree = tree.get_child(RecoveryConfigKey::DISK_MAP_SECTION);
	for (conf_tree::const_iterator iterDisk = disk_tree.begin();
		iterDisk != disk_tree.end();
		iterDisk++)
	{
		TRACE("%s = %s\n", iterDisk->first.c_str(), iterDisk->second.data().c_str());
		m_disks[iterDisk->first] = boost::lexical_cast<int>(iterDisk->second.data());
	}

	TRACE_FUNC_END;
}

/*
Method      : AzureRecoveryConfig::SetStatusBlobUri

Description : Reads the blob-sas key from parsed data and sets to the its data member.

Parameters  : [in] tree: parsed recovery config tree structure

Return Code : None

An exception will be thrown if blob-sas is empty.
*/
void AzureRecoveryConfig::SetStatusBlobUri(const conf_tree& tree)
{
	TRACE_FUNC_BEGIN;

	m_blob_sas = tree.get<std::string>(RecoveryConfigKey::STATUS_BLOB_URI);
	if (m_blob_sas.empty())
		THROW_CONFIG_EXCEPTION("Blob SAS should not be empty");

	AzureStorageRest::Uri uri(m_blob_sas);
	TRACE("%s\n", uri.GetResourceUri().c_str());

	TRACE_FUNC_END;
}

/*
Method      : AzureRecoveryConfig::SetSCSIHost

Description : Reads the ScsiHost key from parsed data and sets to the its data member.

Parameters  : [in] tree: parsed recovery config tree structure

Return Code : None

An exception will be thrown if bad value present for ScsiHost key.
*/
void AzureRecoveryConfig::SetSCSIHost(const conf_tree& tree)
{
	TRACE_FUNC_BEGIN;

	try
	{
		std::string strScsiHost = tree.get<std::string>(RecoveryConfigKey::SCSI_HOST);
		if (!strScsiHost.empty())
			m_scsiHost = boost::lexical_cast<int>(strScsiHost);
	}
	catch (const boost::property_tree::ptree_error& exp)
	{
		TRACE_WARNING("Read error for key %s. Exception: %s\n",
			RecoveryConfigKey::SCSI_HOST,
			exp.what());
		//
		// Its an optional key, not considering it as failure
		//
	}
	catch (boost::bad_lexical_cast& cast_exp)
	{
		TRACE_ERROR("Bad value in %s key\n", RecoveryConfigKey::SCSI_HOST);

		THROW_CONFIG_EXCEPTION("Bad ScsiHost value in recovery config file");
	}

	TRACE("%d\n", m_scsiHost);

	TRACE_FUNC_END;
}

/*
Method      : AzureRecoveryConfig::SetLogLevel

Description : Reads the LogLevel key from parsed data and sets to the its data member.

Parameters  : [in] tree: parsed recovery config tree structure

Return Code : None

An exception will be thrown if bad value present for Loglevel key.
*/
void AzureRecoveryConfig::SetLogLevel(const conf_tree& tree)
{
	TRACE_FUNC_BEGIN;

	try
	{
		std::string strLoglevel = tree.get<std::string>(RecoveryConfigKey::LOG_LEVEL);
		if (!strLoglevel.empty())
		{
			int iLogLevel = boost::lexical_cast<int>(strLoglevel);
			
			m_logLevel =  (LogLevel)(iLogLevel > LogLevelTrace ? LogLevelTrace : iLogLevel);
		}
	}
	catch (const boost::property_tree::ptree_error& exp)
	{
		TRACE_WARNING("Read error for key %s. Exception: %s\n",
			RecoveryConfigKey::LOG_LEVEL,
			exp.what());
		//
		// Its an optional key, not considering it as failure
		//
	}
	catch (boost::bad_lexical_cast& cast_exp)
	{
		TRACE_ERROR("Bad value in %s key\n", RecoveryConfigKey::LOG_LEVEL);

		THROW_CONFIG_EXCEPTION("Bad LogLevel value in recovery config file");
	}

	TRACE("%d\n", m_logLevel);

	TRACE_FUNC_END;
}

/*
Method      : AzureRecoveryConfig::SetBootDevice

Description : Opens the recovery config file and parses its content.

Parameters  : [in] tree: parsed recovery config tree structure

Return Code : None

*/
void AzureRecoveryConfig::SetSysMountPoint(const conf_tree& tree)
{
	TRACE_FUNC_BEGIN;

	try
	{
		m_sysMountPoint = tree.get<std::string>(RecoveryConfigKey::SYS_MOUNT_POINT);
	}
	catch (const boost::property_tree::ptree_error& exp)
	{
		TRACE_WARNING("Read error for key %s. Exception: %s\n",
			          RecoveryConfigKey::SYS_MOUNT_POINT,
					  exp.what() );

		//
		// Its an optional key, not considering it as failure
		//
	}

	TRACE("%s\n", m_sysMountPoint.c_str());

	TRACE_FUNC_END;
}

/*
Method      : AzureRecoveryConfig::SetNewHostId

Description : Opens the recovery config file and parses its content.

Parameters  : [in] tree: parsed recovery config tree structure

Return Code : None

*/
void AzureRecoveryConfig::SetNewHostId(const conf_tree& tree)
{
	TRACE_FUNC_BEGIN;

	m_newHostId = tree.get<std::string>(RecoveryConfigKey::NEW_HOST_ID);
	if (m_newHostId.empty())
		THROW_CONFIG_EXCEPTION("HostId value should not be empty in recovery config file");

	TRACE("%s\n", m_newHostId.c_str());

	TRACE_FUNC_END;
}

/*
Method      : AzureRecoveryConfig::SetTestFailover

Description : Opens the recovery config file and parses its content.

Parameters  : [in] tree: parsed recovery config tree structure

Return Code : None

*/
void AzureRecoveryConfig::SetTestFailover(const conf_tree& tree)
{
	TRACE_FUNC_BEGIN;

	try
	{
		std::string testFailoverKeyVal = tree.get<std::string>(RecoveryConfigKey::TEST_FAILOVER);
		m_bTestFailover = boost::iequals(testFailoverKeyVal, "true");
	}
	catch (const boost::property_tree::ptree_error& exp)
	{
		TRACE_WARNING("Read error for key %s. Exception: %s\n",
			RecoveryConfigKey::TEST_FAILOVER,
			exp.what());

		//
		// Its an optional key, not considering it as failure
		//
	}

	TRACE("%s\n", m_bTestFailover ? "test failover" : "failover");

	TRACE_FUNC_END;
}

/*
Method      : AzureRecoveryConfig::SetEnableRDP

Description : Opens the recovery config file and parses its content.

Parameters  : [in] tree: parsed recovery config tree structure

Return Code : None

*/
void AzureRecoveryConfig::SetEnableRDP(const conf_tree& tree)
{
	TRACE_FUNC_BEGIN;

	try
	{
		std::string enableRDPKeyVal = tree.get<std::string>(RecoveryConfigKey::ENABLE_RDP);
		m_bEnableRDP = boost::iequals(enableRDPKeyVal, "true");
	}
	catch (const boost::property_tree::ptree_error& exp)
	{
		TRACE_WARNING("Read error for key %s. Exception: %s\n",
			RecoveryConfigKey::ENABLE_RDP,
			exp.what());

		//
		// Its an optional key, not considering it as failure
		//
	}

	TRACE("EnableRDP : %s\n", m_bEnableRDP ? "true" : "false");

	TRACE_FUNC_END;
}

/*
Method      : AzureRecoveryConfig::SetIsUEFI

Description : Opens the recovery config file and parses its content.

Parameters  : [in] tree: parsed recovery config tree structure

Return Code : None

*/
void AzureRecoveryConfig::SetIsUEFI(const conf_tree& tree)
{
	TRACE_FUNC_BEGIN;

	try
	{
		std::string isUefiKeyVal = tree.get<std::string>(RecoveryConfigKey::IS_UEFI);
		m_bIsUefi = boost::iequals(isUefiKeyVal, "true");
	}
	catch (const boost::property_tree::ptree_error& exp)
	{
		TRACE_WARNING("Read error for key %s. Exception: %s\n",
			RecoveryConfigKey::ENABLE_RDP,
			exp.what());

		//
		// Its an optional key, not considering it as failure
		//
	}

	TRACE("IsUEFI : %s\n", m_bIsUefi ? "true" : "false");

	TRACE_FUNC_END;
}

/*
Method      : AzureRecoveryConfig::SetActivePartitionStartingOffset

Description : Reads the ActivePartitionStartingOffset key from 
              parsed data and sets to the its data member.

Parameters  : [in] tree: parsed recovery config tree structure

Return Code : None

An exception will be thrown if bad value present for 
ActivePartitionStartingOffset key and IsUEFI is true.
*/
void AzureRecoveryConfig::SetActivePartitionStartingOffset(const conf_tree& tree)
{
	TRACE_FUNC_BEGIN;

	std::string strValue;
	try
	{
		strValue = tree.get<std::string>(RecoveryConfigKey::ACTIVE_PARTITION_STARTING_OFFSET);
		if (!strValue.empty())
		{
			m_activePartitionStartingOffset = boost::lexical_cast<long long>(strValue);
		}
	}
	catch (const boost::property_tree::ptree_error& exp)
	{
		TRACE_WARNING("Read error for key %s. Exception: %s\n",
			RecoveryConfigKey::ACTIVE_PARTITION_STARTING_OFFSET,
			exp.what());
		//
		// this value is needed only in UEFI case.
		//
		if (m_bIsUefi)
			THROW_CONFIG_EXCEPTION("Could not read ActivePartitionOffset value from recovery config file.");
	}
	catch (boost::bad_lexical_cast& cast_exp)
	{
		TRACE_ERROR("Bad value in %s key\n", RecoveryConfigKey::ACTIVE_PARTITION_STARTING_OFFSET);
		
		if(m_bIsUefi)
			THROW_CONFIG_EXCEPTION("Bad ActivePartitionOffset value in recovery config file");
	}

	TRACE("Active Partition Number: %s\n", strValue.c_str());

	TRACE_FUNC_END;
}

/*
Method      : AzureRecoveryConfig::SetDiskSignature

Description : Reads the LogLevel key from parsed data and sets to the its data member.

Parameters  : [in] tree: parsed recovery config tree structure

Return Code : None

An exception will be thrown if bad value present for Loglevel key.
*/
void AzureRecoveryConfig::SetDiskSignature(const conf_tree& tree)
{
	TRACE_FUNC_BEGIN;

	try
	{
		m_diskSignature = tree.get<std::string>(RecoveryConfigKey::DISK_SIGNATURE);
		boost::trim(m_diskSignature);
	}
	catch (const boost::property_tree::ptree_error& exp)
	{
		TRACE_WARNING("Read error for key %s. Exception: %s\n",
			RecoveryConfigKey::DISK_SIGNATURE,
			exp.what());
	}

	TRACE("DiskSignature: %s\n", m_diskSignature.c_str());

	if (m_bIsUefi && m_diskSignature.empty())
		THROW_CONFIG_EXCEPTION("DiskSignature value is empty or invalid in recovery config file");

	TRACE_FUNC_END;
}

/*
Method      : AzureRecoveryConfig::SetFilesToDetectSystemPartitions.

Description : Reads the SetFilesToDetectSystemPartitions key from parsed data
              and set to its data member.

Parameters  : [in] tree: parsed recovery config tree structure.

Return Code : None
*/
void AzureRecoveryConfig::SetFilesToDetectSystemPartitions(
	const conf_tree& tree)
{
	TRACE_FUNC_BEGIN;

	try
	{
		m_filesToDetectSystemPartitions = tree.get<std::string>(
			RecoveryConfigKey::FILES_TO_DETECT_SYS_PARTITION);

		boost::trim(m_filesToDetectSystemPartitions);
	}
	catch (const boost::property_tree::ptree_error& exp)
	{
		TRACE_WARNING("Read error for key %s. Exception: %s\n",
			RecoveryConfigKey::FILES_TO_DETECT_SYS_PARTITION,
			exp.what());
	}

	TRACE_INFO("FilesToDetectSystemPartitions: %s\n",
		m_filesToDetectSystemPartitions.c_str());

	TRACE_FUNC_END;
}

/*
Method      : AzureRecoveryConfig::Parse

Description : Opens the recovery config file and parses its content.

Parameters  : [in] recovery_conf_file: recovery config file path

Return Code : None

An exception will be thrown on parsing failure.
*/
void AzureRecoveryConfig::Parse(const std::string& recovry_conf_file)
{
	TRACE_FUNC_BEGIN;

	BOOST_ASSERT(!recovry_conf_file.empty());
	boost::property_tree::ptree conf_ptree;
	boost::property_tree::read_ini(recovry_conf_file, conf_ptree);

	SetStatusBlobUri(conf_ptree);

	SetDiskMap(conf_ptree);

	SetSCSIHost(conf_ptree);

	SetSysMountPoint(conf_ptree);

	SetLogLevel(conf_ptree);

	SetNewHostId(conf_ptree);

	SetTestFailover(conf_ptree);

	SetEnableRDP(conf_ptree);

	SetIsUEFI(conf_ptree);

	SetActivePartitionStartingOffset(conf_ptree);

	SetDiskSignature(conf_ptree);

	SetFilesToDetectSystemPartitions(conf_ptree);

	m_bInitialized = true;

	TRACE_FUNC_END;
}

/*
Method      : AzureRecoveryConfig::Init

Description : Static method to initialize the singleton object

Parameters  : [in] recovery_conf_file: recovery config file path

Return Code : None, 

An exception will be thrown on parsing failure or empty file name.

*/
void AzureRecoveryConfig::Init(const std::string& recovery_conf_file)
{
	TRACE_FUNC_BEGIN;

	if (s_config_obj.m_bInitialized)
	{
		TRACE_WARNING("Recovery Config object is already initialized\n");
		return;
	}

	if (recovery_conf_file.empty())
	{
		THROW_CONFIG_EXCEPTION("Recovrey config file name should not be empty");
	}

	s_config_obj.Parse(recovery_conf_file);

	TRACE_FUNC_END;
}

/*
Method      : AzureRecoveryConfig::Instance

Description : static method to access the singleton obj instance

Parameters  : None

Return Code : Singleton object reference

*/
const AzureRecoveryConfig& AzureRecoveryConfig::Instance()
{
	if (!s_config_obj.m_bInitialized)
	{
		THROW_CONFIG_EXCEPTION("Recovery Config Obj is not Initiazlized");
	}

	return s_config_obj;
}

/*
Method      : AzureRecoveryConfig::GetStatusBlobUri

Description : Get method for blob-sas parsed from recovery config file

Parameters  : None

Return Code : blob sas uri string

*/
std::string AzureRecoveryConfig::GetStatusBlobUri() const
{
	BOOST_ASSERT(m_bInitialized);

	return m_blob_sas;
}

/*
Method      : AzureRecoveryConfig::GetDiskMap

Description : Fills the disk-lun mapping information parsed from recovery config file
              to the out-param disk_map.

Parameters  : [out] disk_map : Filled with SourceDisk->LUN mapping data

Return Code : None

*/
void AzureRecoveryConfig::GetDiskMap(disk_lun_map& disk_map) const
{
	BOOST_ASSERT(m_bInitialized);

	disk_map.insert(m_disks.begin(), m_disks.end());
}

/*
Method      : AzureRecoveryConfig::GetScsiHostNum

Description : Returns the scsi host number on which the data disks appear on azure hydration-vm.
              On windows 3 is the default value. On Linux its 5. If 'ScsiHost' is set on 
			  recoveryconfig.conf file then that value will be returned.

Parameters  : None

Return Code : Data disks SCSI Host number

*/
int AzureRecoveryConfig::GetScsiHostNum() const
{
	BOOST_ASSERT(m_bInitialized);

	return m_scsiHost;
}


/*
Method      : AzureRecoveryConfig::GetLogLevel

Description : Returns the log level for trace messages. Default value is 3 i.e Info

Parameters  : None

Return Code : Log level for trace

*/
LogLevel AzureRecoveryConfig::GetLogLevel() const
{
	BOOST_ASSERT(m_bInitialized);

	return m_logLevel;
}

/*
Method      : AzureRecoveryConfig::GetSysMountPoint

Description : Returns the boot device identify specified in recovryconfig.conf file.
              If this value is present then the lookup will happen with this identity.

Parameters  : None

Return Code : One of system mount point on linux, OS install drive letter on windows.

*/
std::string AzureRecoveryConfig::GetSysMountPoint() const
{
	BOOST_ASSERT(m_bInitialized);

	return m_sysMountPoint;
}

/*
Method      : AzureRecoveryConfig::GetNewHostId

Description : Returns the new host-id for recovering vm specified in recovryconfig.conf file.

Parameters  : None

Return Code : uuid in string format

*/
std::string AzureRecoveryConfig::GetNewHostId() const
{
	BOOST_ASSERT(m_bInitialized);

	return m_newHostId;
}

/*
Method      : AzureRecoveryConfig::IsTestFailover

Description : 

Parameters  : None

Return Code : true if its a TestFailover, otherwise false.

*/
bool AzureRecoveryConfig::IsTestFailover() const
{
	BOOST_ASSERT(m_bInitialized);

	return m_bTestFailover;
}

/*
Method      : AzureRecoveryConfig::EnableRDP

Description :

Parameters  : None

Return Code : true if user has selected Enable RDP option, otherwise false.

*/
bool AzureRecoveryConfig::EnableRDP() const
{
	BOOST_ASSERT(m_bInitialized);

	return m_bEnableRDP;
}

/*
Method      : AzureRecoveryConfig::EnableRDP

Description :

Parameters  : None

Return Code : true if os disk was UEFI, otherwise false.

*/
bool AzureRecoveryConfig::IsUEFI() const
{
	BOOST_ASSERT(m_bInitialized);

	return m_bIsUefi;
}

/*
Method      : AzureRecoveryConfig::GetActivePartitionStartingOffset

Description :

Parameters  : None

Return Code : Active partition StartingOffset, 0 on non-UEFI case.

*/
long long AzureRecoveryConfig::GetActivePartitionStartingOffset() const
{
	BOOST_ASSERT(m_bInitialized);

	return m_activePartitionStartingOffset;
}

/*
Method      : AzureRecoveryConfig::GetDiskSignature

Description :

Parameters  : None

Return Code : disk signature of the converted UEFI Disk, empty string on non-UEFI case.

*/
std::string AzureRecoveryConfig::GetDiskSignature() const
{
	BOOST_ASSERT(m_bInitialized);

	return m_diskSignature;
}

/*
Method      : AzureRecoveryConfig::GetFilesToDetectSystemPartitions

Description :

Parameters  : None

Return Code : Files to detect system partitions setting.

*/
std::string AzureRecoveryConfig::GetFilesToDetectSystemPartitions() const
{
	BOOST_ASSERT(m_bInitialized);

	return m_filesToDetectSystemPartitions;
}

/*
Method      : AzureRecoveryConfig::PrintConfig

Description : Prints the parsed confgiration to console and log files

Parameters  : None

Return Code : None

*/
void AzureRecoveryConfig::PrintConfig() const
{
	TRACE_FUNC_BEGIN;

	TRACE_INFO("Blob Sas Uri: %s\n", Instance().GetStatusBlobUri().c_str());
	TRACE_INFO("[DiskMap]\n");

	disk_lun_map disks;
	Instance().GetDiskMap(disks);
	for (disk_lun_cons_iter iter = disks.begin();
		iter != disks.end(); iter++)
		TRACE_INFO("%s = %d\n", iter->first.c_str(), iter->second);

	TRACE_FUNC_END;
}

}
