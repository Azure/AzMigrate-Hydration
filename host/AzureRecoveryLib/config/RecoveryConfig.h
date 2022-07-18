/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: RecoveryConfig.h

Description	: AzureRecoveryConfig class is implemented as a singleton. On Init() call
              the signleton object will be initialized by loading recovery configuration
			  information from Recovery config file to its data member. It will offers 
			  interface to access the parsed recovery information.

History		: 7-5-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_RECOVERY_CONFIG_INI_H
#define AZURE_RECOVERY_CONFIG_INI_H
#include <string>
#include <map>

#include <boost/property_tree/ini_parser.hpp>

#include "../common/Trace.h"

#ifdef WIN32

#define DEFAULT_WORKING_DIR		"C:\\AzureRecovery"
#define DEFAULT_SCSI_HOST_NUM	3
#define DEFAULT_SYS_MOUNT_POINT "C:\\"

#else //LINUX

#define DEFAULT_WORKING_DIR		"/usr/local/AzureRecovery"
#define DEFAULT_SCSI_HOST_NUM	5
#define DEFAULT_SYS_MOUNT_POINT "/"

#endif

#define DEFAULT_LOG_LEVEL		3

namespace AzureRecovery
{

	typedef std::map<std::string, int> disk_lun_map;
	typedef disk_lun_map::const_iterator disk_lun_cons_iter;

	typedef boost::property_tree::ptree conf_tree;

	namespace RecoveryConfigKey
	{
		const char STATUS_BLOB_URI[]  = "PreRecoveryExecutionBlobSasUri";

		const char DISK_MAP_SECTION[] = "DiskMap";

		const char SYS_MOUNT_POINT[]  = "SysMountPoint";

		const char SCSI_HOST[]        = "ScsiHost";

		const char LOG_LEVEL[]        = "LogLevel";

		const char TEST_FAILOVER[]    = "TestFailover";

		const char NEW_HOST_ID[]      = "HostId";

		const char ENABLE_RDP[]       = "EnableRDP";

		const char IS_UEFI[] = "IsUEFI";

		const char ACTIVE_PARTITION_STARTING_OFFSET[] = "ActivePartitionStartingOffset";

		const char DISK_SIGNATURE[] = "DiskSignature";

		const char FILES_TO_DETECT_SYS_PARTITION[] = "FilesToDetectSystemPartitions";

		//
		// To parse new keys from recovery config file, define the key/section name here and
		// add Set & Get methods to access the key/section in AzureRecoveryConfig class.
		//
	}

	class AzureRecoveryConfig
	{
		AzureRecoveryConfig();
		~AzureRecoveryConfig();

		void Parse(const std::string& recovery_conf_file);

		static AzureRecoveryConfig s_config_obj;

		void SetStatusBlobUri(const conf_tree& tree);

		void SetDiskMap(const conf_tree& tree);

		void SetSCSIHost(const conf_tree& tree);
	
		void SetSysMountPoint(const conf_tree& tree);
		
		void SetLogLevel(const conf_tree& tree);

		void SetTestFailover(const conf_tree& tree);

		void SetEnableRDP(const conf_tree& tree);

		void SetNewHostId(const conf_tree& tree);

		void SetIsUEFI(const conf_tree& tree);

		void SetActivePartitionStartingOffset(const conf_tree& tree);

		void SetDiskSignature(const conf_tree& tree);

		void SetFilesToDetectSystemPartitions(const conf_tree& tree);

	public:
		static void Init(const std::string& recovery_conf_file);

		static const AzureRecoveryConfig& Instance();

		std::string GetStatusBlobUri() const;

		void GetDiskMap(disk_lun_map& disk_map) const;

		int GetScsiHostNum() const;

		LogLevel GetLogLevel() const;

		std::string GetSysMountPoint() const;

		std::string GetNewHostId() const;

		bool IsTestFailover() const;

		bool EnableRDP() const;

		bool IsUEFI() const;

		long long GetActivePartitionStartingOffset() const;

		std::string GetDiskSignature() const;

		std::string GetFilesToDetectSystemPartitions() const;

		void PrintConfig() const;

	private:

		bool m_bInitialized;

		std::string m_blob_sas;

		disk_lun_map m_disks;

		int m_scsiHost;

		LogLevel m_logLevel;

		std::string m_sysMountPoint;

		std::string m_newHostId;

		bool m_bTestFailover;

		bool m_bEnableRDP;

		bool m_bIsUefi;

		long long m_activePartitionStartingOffset;

		std::string m_diskSignature;

		std::string m_filesToDetectSystemPartitions;
	};
}
#endif
