/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	WmiRecordProcessors.h

Description	:   WMI record processor classes.

History		:   1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_RECOVERY_WMI_RECORD_PROCESSORS_H
#define AZURE_RECOVERY_WMI_RECORD_PROCESSORS_H

#include "WmiEngine.h"

namespace AzureRecovery
{
    typedef struct _win32_disk_info
    {
        UINT LUN;
        UINT SCSIPort;
        UINT TargetId;
        UINT Index;
        std::string DeviceId;
        std::string InterfaceType;

        _win32_disk_info()
        {
            SCSIPort = 0;
            LUN = 0;
            TargetId = 0;
            Index = 255; //0xFF indicates invalid value for the Index property of Win32_DiskDrive
        }
    } win32_disk_info;

    class CWin32_DiskDriveWmiRecordProcessor : public CWmiRecordProcessor
    {
    public:
        bool Process(IWbemClassObject *precordobj, IWbemServices* pNamespace = NULL);
        virtual std::string GetErrMsg(void) const;

        void GetLunDeviceMapForDataDisks(std::map<UINT, std::string>& lun_device,
            UINT data_disks_port) const;

        void GetDiskDriveInfo(std::map<std::string, win32_disk_info>& diskDriveInfo) const;

    private:
        std::map<std::string, win32_disk_info> m_device_map;
        std::string m_error_msg;
    };

    typedef struct _msft_disk_info
    {
        UINT Number;
        UINT16 PartitionStyle;
        std::string Signature_Guid;
        bool IsOffline;
        UINT16 OfflineReason;

        _msft_disk_info()
        {
            Number = 0;
            PartitionStyle = 0;
            OfflineReason = 0;
            IsOffline = false;
        }
    }mdft_disk_info_t;

    enum Disk_Offline_Reason
    {
        Disk_Offline_Reason_Policy = 1,
        Disk_Offline_Reason_RedundantPath,
        Disk_Offline_Reason_Snapshot,
        Disk_Offline_Reason_Collision,
        Disk_Offline_Reason_ResourceExhaustion,
        Disk_Offline_Reason_CriticalWriteFailures,
        Disk_Offline_Reason_DataIntegrityScanRequired
    };

    class CMSFT_DiskRecordProcessor : public CWmiRecordProcessor
    {
    public:
        bool Process(IWbemClassObject *precordobj, IWbemServices* pNamespace = NULL);
        virtual std::string GetErrMsg(void) const;

        void GetDiskInfo(std::map<UINT, mdft_disk_info_t>& diskInfo);

    private:
        std::map<UINT, mdft_disk_info_t> m_diskInfo;
        std::string m_error_msg;
    };

    namespace MSFT_Disk_Properties
    {
        const char DISK_NUMBER[] = "Number";
        const char DISK_PARTITION_STYLE[] = "PartitionStyle";
        const char DISK_SIGNATURE[] = "Signature";
        const char DISK_GUID[] = "Guid";
        const char DISK_IS_OFFLINE[] = "IsOffline";
        const char DISK_OFFLINE_REASON[] = "OfflineReason";
    }

    class CMSFT_DiskOnlineMethod : public CWmiRecordProcessor
    {
    public:
        bool Process(IWbemClassObject *precordobj, IWbemServices* pNamespace = NULL);
        virtual std::string GetErrMsg(void) const;

        UINT32 GetReturnCode() const;

    private:
        UINT32 m_retCode;
        std::string m_error_msg;
    };

    typedef struct _msft_disk_partition
    {
        UINT32  DiskNumber;
        UINT32  PartitionNumber;
        CHAR  DriveLetter;
        std::string  AccessPaths;
        UINT16  OperationalStatus;
        UINT16  TransitionState;
        std::string  Size;
        std::string  Offset;
        UINT16  MbrType;
        std::string  GptType;
        std::string  Guid;
        bool IsReadOnly;
        bool IsOffline;
        bool IsSystem;
        bool IsBoot;
        bool IsActive;
        bool IsHidden;
        bool IsShadowCopy;
        bool NoDefaultDriveLetter;

        _msft_disk_partition()
        {
            DiskNumber = 0;
            PartitionNumber = 0;
            DriveLetter = 0;
            OperationalStatus = 0;
            TransitionState = 0;
            MbrType = 0;
            IsReadOnly = false;
            IsOffline = false;
            IsSystem = false;
            IsBoot = false;
            IsActive = false;
            IsHidden = false;
            IsShadowCopy = false;
            NoDefaultDriveLetter = false;
        }

        void Print() const;
    }msft_disk_partition;


    namespace MSFT_DiskPartition_Properties
    {
        const char  DiskNumber[] = "DiskNumber";
        const char  PartitionNumber[] = "PartitionNumber";
        const char  DriveLetter[] = "DriveLetter";
        const char  AccessPaths[] = "AccessPaths";
        const char  OperationalStatus[] = "OperationalStatus";
        const char  TransitionState[] = "TransitionState";
        const char  Size[] = "Size";
        const char  Offset[] = "Offset";
        const char  MbrType[] = "MbrType";
        const char  GptType[] = "GptType";
        const char  Guid[] = "Guid";
        const char  IsReadOnly[] = "IsReadOnly";
        const char  IsOffline[] = "IsOffline";
        const char  IsSystem[] = "IsSystem";
        const char  IsBoot[] = "IsBoot";
        const char  IsActive[] = "IsActive";
        const char  IsHidden[] = "IsHidden";
        const char  IsShadowCopy[] = "IsShadowCopy";
        const char  NoDefaultDriveLetter[] = "NoDefaultDriveLetter";
    }

    class CMSFT_PartitionRecordProcessor : public CWmiRecordProcessor
    {
    public:
        bool Process(IWbemClassObject *precordobj, IWbemServices* pNamespace = NULL);
        virtual std::string GetErrMsg(void) const;

        void GetDiskParitions(std::list<msft_disk_partition>& disk_partitions);

    private:
        std::list<msft_disk_partition> m_disk_partitions;
        std::string m_error_msg;
    };
}

#endif // ~AZURE_RECOVERY_WMI_RECORD_PROCESSORS_H
