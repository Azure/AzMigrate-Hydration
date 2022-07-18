#ifndef INM_ERROR_ALERT_DEFS_H
#define INM_ERROR_ALERT_DEFS_H

#include <sstream>
#include <boost/lexical_cast.hpp>
#include "inmerroralertimp.h"
#include "inmerroralertnumbers.h"
#include "svtypes.h"

#ifdef SV_WINDOWS
#include "VSSErrorCodes.h"
#endif

class CacheDirLowAlert : public InmErrorAlertImp
{
public:
    CacheDirLowAlert(const std::string &dir, const SV_ULONGLONG &volumecapacity,
                     const SV_ULONGLONG &available, const SV_ULONGLONG &minrequired)
    {
        Parameters_t p;
        p["DirLocation"] = dir;
        p["VolumeSize"] = boost::lexical_cast<std::string>(volumecapacity);
        p["FreeSpace"] = boost::lexical_cast<std::string>(available);
        p["MinFreeSpace"] = boost::lexical_cast<std::string>(minrequired);

        std::stringstream msg;
        msg << "Low Disk Space under Cache Directory: " << dir
          << ", Volume Capacity in bytes: " << volumecapacity
          << ", Free Disk Space in bytes: " << available
          << ", Minimum Required Free Disk Space in bytes: "  << minrequired;

        SetDetails(E_CACHE_DIR_LOW_ALERT, p, msg.str());
    }
};


class CdpLogDirCreateFailAlert : public InmErrorAlertImp
{
public:
    CdpLogDirCreateFailAlert(const std::string &dir, const std::string &error)
    {
        Parameters_t p;
        p["DirLocation"] = dir;
        p["Error"] = error;

        std::stringstream msg;
        msg << "Error during CDP Log Directory :"
          << dir << " creation."
          << " Error : " << error;

        SetDetails(E_CDP_LOG_DIR_CREATE_FAIL_ALERT, p, msg.str());
    }
};


class OutOfOrderDiffAlert : public InmErrorAlertImp
{
public:
    OutOfOrderDiffAlert(const std::string &target, const std::string &pdiff,
                        const unsigned long long &pendtime, const unsigned long long &pendtimeseq,
                        const int &pcontinuationid, const std::string &incomingdiff,
                        const SV_ULONGLONG &instarttime, const SV_ULONGLONG &instarttimeseq,
                        const SV_ULONGLONG &inendtime, const SV_ULONGLONG &inendtimeseq,
                        const SV_UINT &incontinuationid)
                   
    {
        Parameters_t p;
        p["VolumeName"] = target;
        p["PreviousDiffFileName"] = pdiff;
        p["PreviousEndTime"] = boost::lexical_cast<std::string>(pendtime);
        p["PreviousSequenceNo"] = boost::lexical_cast<std::string>(pendtimeseq);
        p["PreviousContinuationId"] = boost::lexical_cast<std::string>(pcontinuationid);
        p["IncomingDiffFileName"] = incomingdiff;
        p["IncomingStartTime"] = boost::lexical_cast<std::string>(instarttime);
        p["IncomingStartSequenceNo"] = boost::lexical_cast<std::string>(instarttimeseq);
        p["IncomingEndTime"] = boost::lexical_cast<std::string>(inendtime);
        p["IncomingEndSequenceNo"] = boost::lexical_cast<std::string>(inendtimeseq);
        p["IncomingContinuationId"] = boost::lexical_cast<std::string>(incontinuationid);

        std::stringstream msg;
        msg << "Received an out of order differential file"
          << ", Target volume : " << target
          << ", Previous Diff : " << pdiff
          << ", End Time: " << pendtime
          << ", End seq: " << pendtimeseq
          << ", continuation id: " << pcontinuationid
          << ", Incoming Diff : " << incomingdiff
          << ", Start Time: " << instarttime
          << ", Start Seq: " << instarttimeseq
          << ", End Time: " << inendtime
          << ", End seq: " << inendtimeseq
          << ", continuation id: " << incontinuationid;

        SetDetails(E_OUT_OF_ORDER_DIFF_ALERT, p, msg.str());
    }
};


class ClusSvcShutdownAlert : public InmErrorAlertImp
{
public:
    ClusSvcShutdownAlert()
    {
        SetDetails(E_CLUS_SVC_SHUTDOWN_ALERT, Parameters_t(), "Cluster service is shutdown");
    }
};


class ChangeLenZeroAlert : public InmErrorAlertImp
{
public:
    ChangeLenZeroAlert(const std::string &diff)
    {
        Parameters_t p;
        p["DiffFileName"] = diff;

        std::stringstream msg;
        msg << "Differential file : " << diff
          << " contains an IO change of length zero";

        SetDetails(E_CHANGE_LEN_ZERO_ALERT, p, msg.str());
    }
};


class ChangeBeyondSourceSizeAlert : public InmErrorAlertImp
{
public:
    ChangeBeyondSourceSizeAlert(const std::string &diff)
    {
        Parameters_t p;
        p["DiffFileName"] = diff;

        std::stringstream msg;
        msg << "Differential file : " << diff
          << " contains an IO change beyond source volume capacity";

        SetDetails(E_CHANGE_BEYOND_SOURCE_SIZE_ALERT, p, msg.str());
    }
};


class InvalidResyncFileAlert : public InmErrorAlertImp
{
public:
    InvalidResyncFileAlert(const std::string &source, const std::string &file)
    {
        Parameters_t p;
        p["DeviceName"] = source;
        p["FileName"] = file;

        std::stringstream msg;
        msg << "Recoverable error found for source volume " << source
          << ". Description: Resync file " << file << " has invalid format. "
          << "It will be prefixed with corrupt_ and resync will continue";
        
        SetDetails(E_INVALID_RESYNC_FILE_ALERT, p, msg.str());
    }
};


class FilterDriverNotFoundAlert : public InmErrorAlertImp
{
public:
    FilterDriverNotFoundAlert()
    {
        SetDetails(E_FILTER_DRIVER_NOT_FOUND_ALERT, Parameters_t(), "Vx service failed to find filter driver. It may not be loaded");
    }
};


class ResyncBitmapsInvalidAlert : public InmErrorAlertImp
{
public:
    ResyncBitmapsInvalidAlert(const std::string &target)
    {
        Parameters_t p;
        p["DeviceName"] = target;

        std::stringstream msg;
        msg << "Encountered an irrecoverable error for target " << target
          << ", since resync bitmaps are invalid. Restart resync is needed";
        
        SetDetails(E_RESYNC_BITMAPS_INVALID_ALERT, p, msg.str());
    }
};


class VolumeResizeAlert : public InmErrorAlertImp
{
public:
    VolumeResizeAlert(const std::string &sourcedevicename, const std::string &targetdevicename,
                      const SV_ULONGLONG &targetrawsize, const SV_ULONGLONG &sourcecapacity)
    {
        Parameters_t p;
        p["SrcDeviceName"] = sourcedevicename;
        p["DestDeviceName"] = targetdevicename;
        p["DestRawVolumeSize"] = boost::lexical_cast<std::string>(targetrawsize);
        p["SrcVolumeSize"] = boost::lexical_cast<std::string>(sourcecapacity);

        std::stringstream msg;
        msg << "For source device " << sourcedevicename
           << ", target device " << targetdevicename
           << ": target raw volume size " << targetrawsize
           << " is less than source capacity " << sourcecapacity << ". Protection cannot continue for this device";

        SetDetails(E_VOLUME_RESIZE_ALERT, p, msg.str());
    }
};


class VxToPSCommunicationFailAlert : public InmErrorAlertImp
{
public:
    VxToPSCommunicationFailAlert(const std::string &ip, const std::string &nonsecureport, const std::string &secureport,
                                 const std::string &protocol, const bool &issecure)
    {
        Parameters_t p;
        p["Ip"] = ip;
        p["Port"] = (issecure?secureport:nonsecureport);
        p["Protocol"] = protocol; 
        p["IsSecure"] = (issecure?"yes":"no");

        std::stringstream msg;
        msg << "Vx agent failed to send/receive to/from PS with " 
          << "ip: " << ip
          << ", port: " << (issecure?secureport:nonsecureport)
          << ", protocol: " << protocol
          << ", secure: " << (issecure?"yes":"no");

        SetDetails(E_VX_TO_PS_COMMUNICATION_FAIL_ALERT, p, msg.str());
    }
};


class VolumeReadFailureAlert : public InmErrorAlertImp
{
public:
    VolumeReadFailureAlert(const std::string &name)
    {
        Parameters_t p;
        p["DeviceName"] = name;
        SetDetails(E_VOLUME_READ_FAILURE_ALERT, p, std::string("Failed to read volume ")+name);
    }
};


class VolumeReadFailureEOFAlert : public InmErrorAlertImp
{
public:
    VolumeReadFailureEOFAlert(const std::string &name, const SV_LONGLONG &offsettoread,
                              const SV_LONGLONG &maxoffset)
    {
        Parameters_t p;
        p["DeviceName"] = name;
        p["OffsetToRead"] = boost::lexical_cast<std::string>(offsettoread);
        p["MaxOffset"] = boost::lexical_cast<std::string>(maxoffset);

        std::stringstream msg;
        msg << "Failed to read volume " << name
          << " at offset " << offsettoread
          << " which crosses max offset " << maxoffset;
        
        SetDetails(E_VOLUME_READ_FAILURE_EOF_ALERT, p, msg.str());
    }
};


class VolumeReadFailureAlertAtOffset : public InmErrorAlertImp
{
public:
    VolumeReadFailureAlertAtOffset(const std::string &name, const SV_LONGLONG &offsettoread,
                                   const SV_ULONG &bytesread, const unsigned int &bytesrequested,
                                   const SV_ULONG &errorcode)
    {
        Parameters_t p;
        p["DeviceName"] = name;
        p["OffsetToRead"] = boost::lexical_cast<std::string>(offsettoread);
        p["BytesRead"] = boost::lexical_cast<std::string>(bytesread);
        p["BytesRequested"] = boost::lexical_cast<std::string>(bytesrequested);
        p["ErrorCode"] = boost::lexical_cast<std::string>(errorcode);

        std::stringstream msg;
        msg << "Failed to read volume " << name
          << ", at offset: " << offsettoread
          << ", bytes read: " << bytesread
          << ", bytes requested: " << bytesrequested
          << ", error code: " << errorcode;
        
        SetDetails(E_VOLUME_READ_FAILURE_AT_OFFSET_ALERT, p, msg.str());
    }
};

class VolumeFSClusterQueryFailureAlert : public InmErrorAlertImp
{
public:
    VolumeFSClusterQueryFailureAlert(const std::string &name, const SV_ULONGLONG &start,
                                     const SV_UINT &count)
    {
        Parameters_t p;
        p["DeviceName"] = name;
        p["Start"] = boost::lexical_cast<std::string>(start);
        p["Count"] = boost::lexical_cast<std::string>(count);

        std::stringstream msg;
        msg << "Failed to query filesystem used blocks information at block number "
          << start << ", count requested " << count
          << ", for volume " << name;

        SetDetails(E_VOLUME_FS_CLUSTERS_QUERY_FAILURE_ALERT, p, msg.str());
    }
};


class FilterDriverReturnedNoDiffsAlert : public InmErrorAlertImp
{
public:
    FilterDriverReturnedNoDiffsAlert(const std::string &sourcedevicename)
    {
        Parameters_t p;
        p["SrcDeviceName"] = sourcedevicename;

        std::stringstream msg;
        msg << "Filter driver did not give any differentials for volume " << sourcedevicename;

        SetDetails(E_FILTER_DRIVER_GAVE_NO_DIFFS_ALERT, p, msg.str());
    }
};


class FilterDriverFailedToReturnDiffsAlert : public InmErrorAlertImp
{
public:
    FilterDriverFailedToReturnDiffsAlert(const std::string &sourcedevicename)
    {
        Parameters_t p;
        p["SrcDeviceName"] = sourcedevicename;

        std::stringstream msg;
        msg << "Filter driver failed to give any differentials for volume " << sourcedevicename;

        SetDetails(E_FILTER_DRIVER_FAILED_TO_GIVE_DIFFS_ALERT, p, msg.str());
    }
};


class VxPeriodicRegistrationFailedAlert : public InmErrorAlertImp
{
public:
    VxPeriodicRegistrationFailedAlert(const std::string &ip, const SV_INT &port, const bool &issecure)
    {
        Parameters_t p;
        p["Ip"] = ip;
        p["Port"] = boost::lexical_cast<std::string>(port);
        p["IsSecure"] = (issecure?"yes":"no");

        std::stringstream msg;
        msg << "Vx periodic registration to CS failed with "
          << "ip: " << ip
          << ", port: " << port
          << ", secure: " << (issecure?"yes":"no");

        SetDetails(E_VX_PERIODIC_REGISTRATION_FAILED_ALERT, p, msg.str());
    }

    VxPeriodicRegistrationFailedAlert(const std::string &uri, const int &error)
    {
        Parameters_t p;
        p["RcmServiceUri"] = uri;
        p["ErrorCode"] = boost::lexical_cast<std::string>(error);

        std::stringstream msg;
        msg << "Mobility service periodic registration with RCM failed. "
            << "URI: " << uri << "Error: " << error;

        SetDetails(E_VX_PERIODIC_REGISTRATION_FAILED_ALERT, p, msg.str());
    }
};


class DlmFileUploadFailedAlert : public InmErrorAlertImp
{
public:
    DlmFileUploadFailedAlert(const std::string &file, const std::string &remotedir, const std::string &ip, const SV_INT &port, const bool &issecure)
    {
        Parameters_t p;
        p["FileName"] = file;
        p["RemoteDirectory"] = remotedir;
        p["Ip"] = ip;
        p["Port"] = boost::lexical_cast<std::string>(port);
        p["IsSecure"] = (issecure?"yes":"no");

        std::stringstream msg;
        msg << "Failed to upload disks layout file " << file
          << ", to CS remote directory " << remotedir 
          << ", with ip: " << ip
          << ", port: " << port
          << ", secure: " << (issecure?"yes":"no");

        SetDetails(E_DLM_FILE_UPLOAD_FAILED_ALERT, p, msg.str());
    }
};


class TargetVolumeUnmountFailedAlert : public InmErrorAlertImp
{
public:
    TargetVolumeUnmountFailedAlert(const std::string &target)
    {
        Parameters_t p;
        p["DestDeviceName"] = target;
        SetDetails(E_TARGET_VOLUME_UNMOUNT_FAILED_ALERT, p, std::string("Failed to unmount target volume ")+target);
    }
};


class ErraticVolumesGUIDAlert : public InmErrorAlertImp
{
public:
    ErraticVolumesGUIDAlert(const std::string &volumeguids)
    {
        Parameters_t p;
        p["VolumesGUIDs"] = volumeguids;
        SetDetails(E_ERRATIC_VOLUMES_GUID_ALERT, p, std::string("Failed to find drive letters or mountpoints for volume GUIDs: ")+volumeguids);
    }
};


class CacheDirBadAlert : public InmErrorAlertImp
{
public:
    CacheDirBadAlert(const std::string &dir)
    {
        Parameters_t p;
        p["DirLocation"] = dir;

        std::stringstream msg;
        msg << "Failed to access cache directory " << dir;

        SetDetails(E_CACHE_DIR_BAD_ALERT, p, msg.str());
    }
};


class WmiServiceBadAlert : public InmErrorAlertImp
{
public:
    WmiServiceBadAlert()
    {
        SetDetails(E_WMI_SERVICE_BAD_ALERT, Parameters_t(), "Wmi service is not up or it is in bad state. It must be in healthy state for protection to work");
    }
};


class RetentionFileReadFailureAlert : public InmErrorAlertImp
{
public:
    RetentionFileReadFailureAlert(const std::string &name, const SV_OFFSET_TYPE &offset,
                                  const SV_UINT &bytesrequested, const int &errorcode)
    {
        Parameters_t p;
        p["RetentionFileName"] = name;
        p["OffsetToRead"] = boost::lexical_cast<std::string>(offset);
        p["BytesRequested"] = boost::lexical_cast<std::string>(bytesrequested);
        p["ErrorCode"] = boost::lexical_cast<std::string>(errorcode);

        std::stringstream msg;
        msg << "Failed to read retention file " << name << ", at offset " << offset << ", bytes requested " << bytesrequested << ", with error " << errorcode;

        SetDetails(E_RETENTION_FILE_READ_FAILED_ALERT, p, msg.str());
    }
};


class RetentionFileWriteFailureAlert : public InmErrorAlertImp
{
public:
    RetentionFileWriteFailureAlert(const std::string &name, const SV_OFFSET_TYPE &offset,
                                   const SV_UINT &bytesrequested, const int &errorcode)
    {
        Parameters_t p;
        p["RetentionFileName"] = name;
        p["OffsetToWrite"] = boost::lexical_cast<std::string>(offset);
        p["BytesRequested"] = boost::lexical_cast<std::string>(bytesrequested);
        p["ErrorCode"] = boost::lexical_cast<std::string>(errorcode);

        std::stringstream msg;
        msg << "Failed to write retention file " << name << ", at offset " << offset << ", bytes requested " << bytesrequested << ", with error " << errorcode;

        SetDetails(E_RETENTION_FILE_WRITE_FAILED_ALERT, p, msg.str());
    }
};


class WriteFailureAlertAtOffset : public InmErrorAlertImp
{
public:
    WriteFailureAlertAtOffset(const std::string &name, const SV_OFFSET_TYPE &offset,
                              const SV_UINT &bytesrequested, const int &errorcode)
    {
        Parameters_t p;
        p["Name"] = name;
        p["OffsetToWrite"] = boost::lexical_cast<std::string>(offset);
        p["BytesRequested"] = boost::lexical_cast<std::string>(bytesrequested);
        p["ErrorCode"] = boost::lexical_cast<std::string>(errorcode);

        std::stringstream msg;
        msg << "Failed to write to " << name << ", at offset " << offset << ", bytes requested " << bytesrequested << ", with error " << errorcode;
        
        SetDetails(E_WRITE_FAILURE_AT_OFFSET_ALERT, p, msg.str());
    }
};

class AzureAuthenticationFailureAlert : public InmErrorAlertImp
{
public:
    AzureAuthenticationFailureAlert(
		const std::string &SourceVmHostId, 
		const std::string &SourceDiskId)
    {
        Parameters_t p;
		p["SourceVmHostId"] = SourceVmHostId;
		p["SourceDiskId"] = SourceDiskId;

        std::stringstream msg;
		msg << "Message: Replication for the VM " << SourceVmHostId << " Disk:" << SourceDiskId << "is not progressing due to an authentication issue.\n"
			<< "Possible causes: Registration of the management server to the service did not complete properly.\n"
			<< "Recommended action: Register the management server to the Recovery services / Site recovery vault again,"
			<< "by running the cspsconfig utility and restart the cbengine service on the management server.\n";

		SetDetails(E_AZUREAUTHENTICATION_FAILURE_ALERT, p, msg.str());
    }
};

class AzureCancelOperationErrorAlert : public InmErrorAlertImp
{
public:
    AzureCancelOperationErrorAlert(const std::string &ErrorMsg)
    {
        SetDetails(E_AZURE_CANCEL_OPERATION_ERROR_ALERT, Parameters_t(), ErrorMsg);
    }
};

class CrashConsistencyFailureNonDataModeAlert : public InmErrorAlertImp
{
public:
    CrashConsistencyFailureNonDataModeAlert(const SV_ULONG PolicyId, const std::string &FailedNodes, const std::string &Command)
    {
        Parameters_t p;
        p["PolicyId"] = boost::lexical_cast<std::string>(PolicyId);
        p["FailedNodes"] = FailedNodes;
        p["Command"] = Command;

        std::stringstream msg;
        msg << "Crash consistency command failed as driver is not in write order mode. "
            << "Policy Id : " << PolicyId
            << "Command issued: " << Command
            << " Failed nodes: " << FailedNodes;

        SetDetails(E_CRASH_CONSISTENCY_FAILURE_NON_DATA_MODE, p, msg.str());
    }

    CrashConsistencyFailureNonDataModeAlert(const std::string &FailedNodes, const std::string &Command)
    {
        Parameters_t p;
        p["FailedNodes"] = FailedNodes;
        p["Command"] = Command;
        p["ExitCode"] = boost::lexical_cast<std::string>(VACP_E_DRIVER_IN_NON_DATA_MODE);

        std::stringstream msg;
        msg << "Crash consistency command failed. "
            << "Command issued: " << Command
            << " Failed nodes: " << FailedNodes
            << " ExitCode: " << VACP_E_DRIVER_IN_NON_DATA_MODE;

        SetDetails(E_CRASH_CONSISTENCY_FAILURE_NON_DATA_MODE, p, msg.str());
    }
};

class CrashConsistencyFailureAlert : public InmErrorAlertImp
{
public:
    CrashConsistencyFailureAlert(const SV_ULONG PolicyId, const std::string &FailedNodes, const std::string &Command)
    {
        Parameters_t p;
        p["PolicyId"] = boost::lexical_cast<std::string>(PolicyId);
        p["FailedNodes"] = FailedNodes;
        p["Command"] = Command;

        std::stringstream msg;
        msg << "Crash consistency command failed. "
            << "Policy Id : " << PolicyId
            << "Command issued: " << Command
            << " Failed nodes: " << FailedNodes;

        SetDetails(E_CRASH_CONSISTENCY_FAILURE, p, msg.str());
    }

    CrashConsistencyFailureAlert(const std::string &FailedNodes, const std::string &Command, SV_ULONG ExitCode)
    {
        Parameters_t p;
        p["FailedNodes"] = FailedNodes;
        p["Command"] = Command;
        p["ExitCode"] = boost::lexical_cast<std::string>(ExitCode);

        std::stringstream msg;
        msg << "Crash consistency command failed. "
            << "Command issued: " << Command
            << " Failed nodes: " << FailedNodes
            << " ExitCode: " << ExitCode;

        SetDetails(E_CRASH_CONSISTENCY_FAILURE, p, msg.str());
    }
};

class AppConsistencyFailureAlert : public InmErrorAlertImp
{
public:
	AppConsistencyFailureAlert()
	{
		//Do nothing
	}

    AppConsistencyFailureAlert(const SV_ULONG PolicyId, const std::string &FailedNodes, const std::string &Command)
    {
        Parameters_t p;
        p["PolicyId"] = boost::lexical_cast<std::string>(PolicyId);
        p["FailedNodes"] = FailedNodes;
        p["Command"] = Command;

        std::stringstream msg;
        msg << "Apllication consistency command failed. "
            << "Policy Id : " << PolicyId
            << "Command issued: " << Command
            << " Failed nodes: " << FailedNodes;

        SetDetails(E_APP_CONSISTENCY_FAILURE, p, msg.str());
    }

    AppConsistencyFailureAlert(const std::string &FailedNodes, const std::string &Command, SV_ULONG ExitCode)
    {
        Parameters_t p;
        p["FailedNodes"] = FailedNodes;
        p["Command"] = Command;
        p["ExitCode"] = boost::lexical_cast<std::string>(ExitCode);

        std::stringstream msg;
        msg << "Apllication consistency command failed. "
            << "Command issued: " << Command
            << " Failed nodes: " << FailedNodes
            << " ExitCode: " << ExitCode;

#ifdef SV_WINDOWS
		ErrorCodeConvertor errorCodeConvertor;
		long win32Exitcode = errorCodeConvertor.GetWin32ErrorCode(ExitCode);
		if (win32Exitcode == ASR_VSS_PROVIDER_MISSING_ERROR_CODE)
		{
			SetDetails(E_ASR_VSS_PROVIDER_MISSING, p, msg.str());
		}
		else if (win32Exitcode == ASR_VSS_SERVICE_DISABLED_ERROR_CODE)
		{
			SetDetails(E_ASR_VSS_SERVICE_DISABLED, p, msg.str());
		}
		else
		{
			SetDetails(E_APP_CONSISTENCY_FAILURE, p, msg.str());
		}
#else
		SetDetails(E_APP_CONSISTENCY_FAILURE, p, msg.str());
#endif
    }
};


class VxToAzureBlobCommunicationFailAlert : public InmErrorAlertImp
{
public:
    VxToAzureBlobCommunicationFailAlert(const std::string &blobSasUri)
    {
        std::string sanitizedBlobSasUri("sasUri");
        size_t sigpos = blobSasUri.find("&sig=");
        size_t sepos = blobSasUri.find("&se=");

        if (sigpos != std::string::npos  && sepos != std::string::npos)
        {
            sanitizedBlobSasUri = blobSasUri.substr(0, sigpos);
            sanitizedBlobSasUri += blobSasUri.substr(sepos);
        }

        Parameters_t p;
        p["StorageBlobSasUri"] = sanitizedBlobSasUri;

        std::stringstream msg;
        msg << "Vx agent failed to send data to Azure blob with "
            << "Uri: " << sanitizedBlobSasUri;

        SetDetails(E_VX_TO_AZURE_BLOB_COMMUNICATION_FAIL_ALERT, p, msg.str());
    }
};

class RebootAlert : public InmErrorAlertImp
{
public:
    RebootAlert(const std::string &systemBootTime)
    {
        Parameters_t p;
        p["SystemBootTime"] = systemBootTime;

        std::stringstream msg;
        msg << "Server has come up at "
            << systemBootTime
            << " after a reboot or shutdown";

        SetDetails(E_REBOOT_ALERT, p, msg.str());
    }
};

class AgentUpgradeAlert : public InmErrorAlertImp
{
public:
    AgentUpgradeAlert(const std::string &version)
    {
        Parameters_t p;
        p["AgentVersion"] = version;

        std::stringstream msg;
        msg << "Agent upgraded to v" << version;

        SetDetails(E_AGENT_UPGRADE_ALERT, p, msg.str());
    }
};

class LogUploadNetworkFailureAlert : public InmErrorAlertImp
{
public:
    LogUploadNetworkFailureAlert(const std::string &timestamp,
        const std::string &device,
        const std::string &value,
        const std::string &count,
        const std::string &failedNodes)
    {
        Parameters_t p;
        p["FailedNodes"] = failedNodes;

        std::stringstream msg;
        msg << "Log upload failed with network error.";
        msg << " Timestamp : " << timestamp;
        msg << " Device : " << device;
        msg << " Last Error : " << value;
        msg << " Error Count : " << count << ".";

        SetDetails(E_LOG_UPLOAD_NETWORK_FAILURE_ALERT, p, msg.str());
    }
};

class AgentPeakChurnAlert : public InmErrorAlertImp
{
public:
    AgentPeakChurnAlert(const std::string &timestamp,
        const std::string &device,
        const std::string &value,
        const std::string &count,
        const std::string &failedNodes)
    {
        Parameters_t p;
        p["FailedNodes"] = failedNodes;

        std::stringstream msg;
        msg << "Disk churn more than supported peak churn observed.";
        msg << " Timestamp : " << timestamp;
        msg << " Device : " << device;
        msg << " Churn : " << value << " bytes";
        msg << " Accumulated Churn : " << count << " bytes.";

        SetDetails(E_DISK_CHURN_PEAK_ALERT, p, msg.str());
    }
};

class LogUploadNetworkLatencyAlert : public InmErrorAlertImp
{
public:
    LogUploadNetworkLatencyAlert(const std::string &timestamp,
        const std::string &device,
        const std::string &value,
        const std::string &failedNodes)
    {
        Parameters_t p;
        p["FailedNodes"] = failedNodes;

        std::stringstream msg;
        msg << "High log upload network latencies observed.";
        msg << " Timestamp : " << timestamp;
        msg << " Device : " << device;
        msg << " ChurnThroughputDiff : " << value << "bytes";

        SetDetails(E_LOG_UPLOAD_NETWORK_HIGH_LATENCY_ALERT, p, msg.str());
    }
};

class DiskResizeAlert : public InmErrorAlertImp
{
public:
    DiskResizeAlert(const std::string &device)
    {
        Parameters_t p;
        p["Device"] = device;

        std::stringstream msg;
        msg << "A resize is detected for device " << device;

        SetDetails(E_DISK_RESIZE_ALERT, p, msg.str());
    }
};

class TimeJumpForwardAlert : public InmErrorAlertImp
{
public:
    TimeJumpForwardAlert(const std::string &timeJumpedAt,
        const std::string &value)
    {
        Parameters_t p;
        p["TimeJumpedAt"] = timeJumpedAt;
        p["TimeJump"] = value;

        std::stringstream msg;
        msg << "System time jump forward of " << value 
            << " seconds detected at " << timeJumpedAt;

        SetDetails(E_TIME_JUMPED_FORWARD_ALERT, p, msg.str());
    }
};

class TimeJumpBackwardAlert : public InmErrorAlertImp
{
public:
    TimeJumpBackwardAlert(const std::string &timeJumpedAt,
        const std::string &value)
    {
        Parameters_t p;
        p["TimeJumpedAt"] = timeJumpedAt;
        p["TimeJump"] = value;

        std::stringstream msg;
        msg << "System time jump backward of " << value
            << " seconds detected at " << timeJumpedAt;

        SetDetails(E_TIME_JUMPED_BACKWARD_ALERT, p, msg.str());
    }
};

#endif /* INM_ERROR_ALERT_DEFS_H */
