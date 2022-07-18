/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: HostInfoDefs.h

Description	: Definitions for various structures which are used for storing host info.
              It also has constant string literals defines for HostInfo xml node, property
			  and property values.

History		: 7-5-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_HOSTINFO_DEFS_AND_CONSTS_H
#define AZURE_HOSTINFO_DEFS_AND_CONSTS_H

#include <string>
#include <vector>
#include <map>
#include <set>

namespace AzureRecovery
{
	typedef long long SV_LONGLONG;

	struct disk_extent
	{
		std::string disk_id;
		SV_LONGLONG offset;
		SV_LONGLONG length;

		disk_extent()
		{
			offset = length = 0;
		}

		disk_extent(const std::string& _disk_id, SV_LONGLONG _offset, SV_LONGLONG _lenght)
		{
			disk_id = _disk_id;
			offset = _offset;
			length = _lenght;
		}

		disk_extent(disk_extent const& rhs)
		{
			disk_id = rhs.disk_id;
			offset = rhs.offset;
			length = rhs.length;
		}

		disk_extent& operator = (disk_extent const& rhs)
		{
			if (this == &rhs)
				return *this;

			disk_id = rhs.disk_id;
			offset = rhs.offset;
			length = rhs.length;

			return *this;
		}

		bool operator == (disk_extent const& rhs) const
		{
			return (
				disk_id == rhs.disk_id &&
				offset == rhs.offset &&
				length == rhs.length
				);
		}
	};

	typedef std::vector<disk_extent> disk_extents_t;
	typedef disk_extents_t::iterator disk_extents_iter_t;

	typedef struct _volume_partition_info
	{
		std::string Name;
		std::string DiskGrp;
		std::string FSType;
		std::string MountPoint;
		bool IsSystemVolume;

		_volume_partition_info()
			:IsSystemVolume(false)
		{}

		_volume_partition_info(const _volume_partition_info& rhs)
		{
			Name = rhs.Name;
			DiskGrp = rhs.DiskGrp;
			FSType = rhs.FSType;
			MountPoint = rhs.MountPoint;
			IsSystemVolume = rhs.IsSystemVolume;
		}

		_volume_partition_info& operator =(const _volume_partition_info& rhs)
		{
			if (this != &rhs)
			{
				Name = rhs.Name;
				DiskGrp = rhs.DiskGrp;
				FSType = rhs.FSType;
				MountPoint = rhs.MountPoint;
				IsSystemVolume = rhs.IsSystemVolume;
			}

			return *this;
		}

		bool operator ==(const _volume_partition_info& rhs) const
		{
			return (
				Name == rhs.Name &&
				DiskGrp == rhs.DiskGrp &&
				FSType == rhs.FSType &&
				MountPoint == rhs.MountPoint &&
				IsSystemVolume == rhs.IsSystemVolume
				);
		}

	} volume_info_t, partition_info_t;

	typedef std::vector<volume_info_t>::const_iterator partition_info_iter_t;

	typedef struct _partition_
	{
		partition_info_t info;
		std::vector<volume_info_t> logical_volumes;

		_partition_(){}

		_partition_(const _partition_& rhs)
		{
			info = rhs.info;
			logical_volumes = rhs.logical_volumes;
		}

		_partition_& operator =(const _partition_& rhs)
		{
			if (this != &rhs)
			{
				info = rhs.info;
				logical_volumes = rhs.logical_volumes;
			}

			return *this;
		}

		bool operator ==(const _partition_& rhs) const
		{
			return (
				info == rhs.info &&
				logical_volumes == rhs.logical_volumes
				);
		}
	} partition_t;

	typedef struct _disk
	{
		std::string Name;
		std::string DiskId;
		std::string DiskGrp;
		std::string FSType;
		std::string MountPoint;
		bool DynamicDisk;
		bool Bootable;

		std::vector<partition_t> partitions;

		_disk()
			:DynamicDisk(false),
			Bootable(false) { }

		_disk(const _disk& rhs)
		{
			Name = rhs.Name;
			DiskId = rhs.DiskId;
			DiskGrp = rhs.DiskGrp;
			FSType = rhs.FSType;
			MountPoint = rhs.MountPoint;
			DynamicDisk = rhs.DynamicDisk;
			Bootable = rhs.Bootable;

			partitions = rhs.partitions;
		}

		_disk& operator =(const _disk& rhs)
		{
			if (this != &rhs)
			{
				Name = rhs.Name;
				DiskId = rhs.DiskId;
				DiskGrp = rhs.DiskGrp;
				FSType = rhs.FSType;
				MountPoint = rhs.MountPoint;
				DynamicDisk = rhs.DynamicDisk;
				Bootable = rhs.Bootable;

				partitions = rhs.partitions;
			}

			return *this;
		}

		bool operator ==(const _disk& rhs) const
		{
			return (
				Name == rhs.Name &&
				DiskId == rhs.DiskId &&
				DiskGrp == rhs.DiskGrp &&
				FSType == rhs.FSType &&
				MountPoint == rhs.MountPoint &&
				DynamicDisk == rhs.DynamicDisk &&
				Bootable == rhs.Bootable &&
				partitions == rhs.partitions
				);
		}
	}disk_t;

	typedef std::vector<partition_t>::const_iterator partition_iter_t;

	typedef struct _partition_details
	{
		std::string VolType;
		std::string VgName;
		std::string LvName;
		std::string Partition;
		std::string FsType;
		std::string MountPoint;
		std::string DiskName;

		void Reset()
		{
			VolType.clear();
			VgName.clear();
			LvName.clear();
			Partition.clear();
			FsType.clear();
			MountPoint.clear();
			DiskName.clear();
		}
	} partition_details;

	namespace HostInfoXmlNode
	{
		const char ATTRIBUTE_ID[] = "Id";
		const char ATTRIBUTE_NAME[] = "Name";
		const char ATTRIBUTE_VALUE[] = "Value";

		const char PARAMETER[] = "Parameter";
		const char PARAMETER_GROUP[] = "ParameterGroup";
	}

	namespace HostInfoXmlParamName
	{
		const char COMPONENT_TYPE[] = "ComponentType";

		const char HOST_ID[] = "HostId";
		const char HOST_NAME[] = "HostName";
		const char MACHINE_TYPE[] = "ServerType";

		const char OS_TYPE[] = "OSType";
		const char OS_VERSION[] = "OSVersion";
		const char OS_VOLUME[] = "OSVolume";
		const char OS_INSTALL_PATH[] = "OSInstallPath";
		const char OS_VOLUME_DISK_EXTENTS[] = "OSVolumeDiskExtents";

		const char DISK_ID[] = "DiskId";
		const char DISK_NAME[] = "DiskName";
		const char DISK_GROUP[] = "DiskGroup";
		const char SYSTEM_DISK[] = "SystemDisk";
		const char DYNAMIC_DISK[] = "DynamicDisk";

		const char FS_TYPE[] = "FileSystemType";
		const char MOUNT_POINT[] = "MountPoint";
		const char IS_SYSTEM_VOLUME[] = "IsSystemVolume";

		const char NAME[] = "Name";
	}

	namespace HostInfoXmlParamGrpId
	{
		const char DISK_DETAILS[] = "DiskDetails";
		const char HARWARE_CONFIG[] = "HardwareConfiguration";
		const char NETWORK_CONFIG[] = "NetworkConfiguration";
		const char DISK[] = "Disk";
		const char PARTITION[] = "Partition";
		const char LOGICAL_VOLUME[] = "LogicalVolume";
	}

	namespace HostInfoXmlParamValues
	{
		const char VALUE_NO[] = "No";
		const char VALUE_YES[] = "Yes";
		const char VALUE_TRUE[] = "true";
		const char VALUE_FALSE[] = "false";
		
		const char OS_TYPE_LINUX[] = "Linux";
		const char OS_TYPE_WINDOWS[] = "Windows";
		const char SOURCE_SERVER[] = "SourceServer";
	}
}

#endif //~AZURE_HOSTINFO_DEFS_AND_CONSTS_H