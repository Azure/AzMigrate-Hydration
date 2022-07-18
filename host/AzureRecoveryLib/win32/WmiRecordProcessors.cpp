/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	WmiRecordProcessors.cpp

Description	:   WMI record processor classes.

History		:   1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/

#include <Windows.h>
#include <sstream>

#include "WmiRecordProcessors.h"
#include "../common/Trace.h"

#define WMI_SAFE_RELEASE(x)	if(x){ x->Release(); x = NULL; }

namespace AzureRecovery
{

/*
Method      : CWin32_DiskDriveWmiRecordProcessor::Process

Description : Reads the require properties from a Win32_DiskDrive wmi object and stores them into
              data members.

Parameters  : [in] precordobj:
              [in] pNamespace:

Return      : true on success, otherwise false.

*/
bool CWin32_DiskDriveWmiRecordProcessor::Process(IWbemClassObject *precordobj, IWbemServices* pNamespace)
{
	TRACE_FUNC_BEGIN;
	bool bprocessed = true;

	VARIANT vtProp;
	HRESULT hrCol;

	do 
	{
		UINT uPort = 0;
		hrCol = precordobj->Get(_bstr_t(Win32_DiskDrive_Prop::SCSIPort), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			uPort = vtProp.uintVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get Win32_DiskDrive object property: "; 
			m_error_msg += Win32_DiskDrive_Prop::SCSIPort;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		UINT uLun = 0;
		hrCol = precordobj->Get(_bstr_t(Win32_DiskDrive_Prop::SCSILun), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			uLun = vtProp.uintVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get Win32_DiskDrive object property: ";
			m_error_msg += Win32_DiskDrive_Prop::SCSILun;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		std::string device_name;
		hrCol = precordobj->Get(_bstr_t(Win32_DiskDrive_Prop::Device_ID), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			device_name = _bstr_t(vtProp.bstrVal);
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get Win32_DiskDrive object property: ";
			m_error_msg += Win32_DiskDrive_Prop::Device_ID;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		std::string interface_type;
		hrCol = precordobj->Get(_bstr_t(Win32_DiskDrive_Prop::InterfaceType), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			interface_type = _bstr_t(vtProp.bstrVal);
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get Win32_DiskDrive object property: ";
			m_error_msg += Win32_DiskDrive_Prop::InterfaceType;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		UINT uIndex = 255;
		hrCol = precordobj->Get(_bstr_t(Win32_DiskDrive_Prop::Index), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			uIndex = vtProp.uintVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get Win32_DiskDrive object property: ";
			m_error_msg += Win32_DiskDrive_Prop::Index;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		win32_disk_info disk_info;
		disk_info.DeviceId = device_name;
		disk_info.LUN = uLun;
		disk_info.SCSIPort = uPort;
		disk_info.Index = uIndex;
		disk_info.InterfaceType = interface_type;

		m_device_map[device_name] = disk_info;

	} while (false);

	TRACE_FUNC_END;
	return bprocessed;
}

std::string CWin32_DiskDriveWmiRecordProcessor::GetErrMsg(void) const
{
	return m_error_msg;
}

void CWin32_DiskDriveWmiRecordProcessor::GetDiskDriveInfo( std::map<std::string, win32_disk_info>& 
	                                                       diskDriveInfo) const
{
	diskDriveInfo.insert(m_device_map.begin(), m_device_map.end());
}

/*
Method      : CWin32_DiskDriveWmiRecordProcessor::GetLunDeviceMapForDataDisks

Description : Gets the map of Lun -> disk signature/guid mapping entries which has specified data_disks_port.

Parameters  : [out] lun_device:
              [in]  data_disks_port:

Return      : None

*/
void CWin32_DiskDriveWmiRecordProcessor::GetLunDeviceMapForDataDisks(
	std::map<UINT, std::string>& lun_device,
	UINT data_disks_port
	) const
{
	auto diskIter = m_device_map.begin();
	for (; diskIter != m_device_map.end(); diskIter++)
	{
		if (diskIter->second.SCSIPort == data_disks_port)
		{
			lun_device[diskIter->second.LUN] = diskIter->second.DeviceId;

			TRACE_INFO("Found Device %s on Port %u at Lun position %u\n",
				diskIter->second.DeviceId.c_str(),
				diskIter->second.SCSIPort,
				diskIter->second.LUN);
		}
		else
		{
			TRACE_INFO("Skipping Device %s on Port %u at Lun position %u\n",
				diskIter->second.DeviceId.c_str(),
				diskIter->second.SCSIPort,
				diskIter->second.LUN);
		}
	}
}

/*
Method      : CMSFT_DiskOnlineMethod::Process

Description : Clears readonly flag on disk and Online it. 

Parameters  : [in] precordobj:
              [in] pNamespace:

Return      : true on success, otherwise false

*/
bool CMSFT_DiskOnlineMethod::Process(IWbemClassObject *precordobj, IWbemServices* pNamespace)
{
	TRACE_FUNC_BEGIN;
	bool bprocessed = true;

	VARIANT vtProp, vtRet;
	HRESULT hr;
    
	IWbemClassObject* pClassDef = NULL;
	IWbemClassObject* pMethodSign = NULL;
	IWbemClassObject* pInParamInst = NULL;
	IWbemClassObject* pOutParams = NULL;

	do
	{
		//
		// Get Object Instance path. ObjectId is the primary key for MSFT_Disk class.
		//
		hr = precordobj->Get(_bstr_t("ObjectId"), 0, &vtProp, 0, 0);
		if (FAILED(hr))
		{
			m_error_msg = _com_error(hr).ErrorMessage();

			TRACE_ERROR("Could not retrieved ObjectId for SMFT_Disk instance. Com error %s\n",
				        m_error_msg.c_str());

			bprocessed = false;
			break;
		}

		//
		// Construct class path for the required MSFT_Disk instance
		//
		std::wstring objPath(L"MSFT_Disk.ObjectId=");
		objPath += L"\'";
		objPath += _bstr_t(vtProp);
		objPath += L"\'";
		VariantClear(&vtProp);
		
		//
		// Execute MSFT_Disk.SetAttributes to clear ReadOnly flag.
		//
		hr = pNamespace->GetObject(_bstr_t("MSFT_Disk"),
			                      0,
								  NULL,
								  &pClassDef,
								  NULL);
		if (FAILED(hr))
		{
			m_error_msg = _com_error(hr).ErrorMessage();

			TRACE_ERROR("Could not get MSFT_Disk class object definition. Com error %s\n",
				m_error_msg.c_str());

			bprocessed = false;
			break;
		}

		hr = pClassDef->GetMethod(_bstr_t(METHOD_MSFT_DISK_SETATTRIBUTES),
			                      0,
								  &pMethodSign, 
								  NULL);
		if (FAILED(hr))
		{
			m_error_msg = _com_error(hr).ErrorMessage();

			TRACE_ERROR("Could not get SMFT_Disk.SetAttributes definition method. Com error %s\n",
				m_error_msg.c_str());

			bprocessed = false;
			break;
		}

		hr = pMethodSign->SpawnInstance(0, &pInParamInst);
		if (FAILED(hr))
		{
			m_error_msg = _com_error(hr).ErrorMessage();

			TRACE_ERROR("Could not instantiate SMFT_Disk.SetAttributes method InParams. Com error %s\n",
				m_error_msg.c_str());

			bprocessed = false;
			break;
		}

		vtProp.vt = VT_BOOL;
		vtProp.boolVal = FALSE;
		hr = pInParamInst->Put(_bstr_t("IsReadOnly"), 0, &vtProp, 0);
		if (FAILED(hr))
		{
			m_error_msg = _com_error(hr).ErrorMessage();

			TRACE_ERROR("Could not set IsReadOnly Value as SMFT_Disk.SetAttributes method InParam. Com error %s\n",
				m_error_msg.c_str());

			bprocessed = false;
			break;
		}

		hr = pNamespace->ExecMethod(_bstr_t(objPath.c_str()),
			                        _bstr_t(METHOD_MSFT_DISK_SETATTRIBUTES),
			                        0,
			                        NULL,
			                        pInParamInst,
			                        &pOutParams,
			                        NULL);
		if (FAILED(hr))
		{
			m_error_msg = _com_error(hr).ErrorMessage();

			TRACE_ERROR("Could not invoke SMFT_Disk.SetAttributes method. Com error %s\n",
				m_error_msg.c_str());

			bprocessed = false;
			break;
		}

		hr = pOutParams->Get(_bstr_t(PROP_RETURN_VALUE),
			                 0,
							 &vtRet,
							 0,
							 0);
		if (FAILED(hr))
		{
			m_error_msg = _com_error(hr).ErrorMessage();

			TRACE_ERROR("Could not retrieved ReturnValue for SMFT_Disk.SetAttributes method. Com error %s\n",
				m_error_msg.c_str());

			bprocessed = false;
			break;
		}
		m_retCode = vtRet.uintVal;
		if (ERROR_SUCCESS != m_retCode)
		{
			TRACE_ERROR("Could not clear ReadOnly flag on the disk\n");
			bprocessed = false;
			break;
		}

		WMI_SAFE_RELEASE(pOutParams);

		//
		// Execute MSFT_Disk.Online method
		//
		hr = pNamespace->ExecMethod(_bstr_t(objPath.c_str()),
			_bstr_t(METHOD_MSFT_DISK_ONLINE),
			0,
			0,
			0,
			&pOutParams,
			0);
		if (FAILED(hr))
		{
			m_error_msg = _com_error(hr).ErrorMessage();

			TRACE_ERROR("Could not invoke SMFT_Disk.Online method. Com error %s\n",
				m_error_msg.c_str());

			bprocessed = false;
			break;
		}

		//
		// Get the return value
		//
		hr = pOutParams->Get(_bstr_t(PROP_RETURN_VALUE), 0, &vtRet, 0, 0);
		if (FAILED(hr))
		{
			m_error_msg = _com_error(hr).ErrorMessage();

			TRACE_ERROR("Could not retrieved ReturnValue for SMFT_Disk.Online method. Com error %s\n",
				m_error_msg.c_str());

			bprocessed = false;
			break;
		}
		m_retCode = vtRet.uintVal;
		VariantClear(&vtRet);

		if (ERROR_SUCCESS != m_retCode)
		{
			TRACE_ERROR("Could not online the disk. MSFT_Disk.Online failed with error %u\n", m_retCode);

			bprocessed = false;
		}

	} while (false);

	//
	// Cleanup
	//
	VariantClear(&vtProp);
	VariantClear(&vtRet);

	WMI_SAFE_RELEASE(pClassDef);
	WMI_SAFE_RELEASE(pMethodSign);
	WMI_SAFE_RELEASE(pInParamInst);
	WMI_SAFE_RELEASE(pOutParams);
	
	TRACE_FUNC_END;
	return bprocessed;
}

UINT32 CMSFT_DiskOnlineMethod::GetReturnCode() const
{
	return m_retCode;
}

std::string CMSFT_DiskOnlineMethod::GetErrMsg() const
{
	return m_error_msg;
}

/*
Method      : CMSFT_DiskRecordProcessor::Process

Description : Reads required properties from MSFT_Disk wmi object.

Parameters  : [in] precordobj:
              [in] pNamespace:

Return      : true on success, otherwise false

*/
bool CMSFT_DiskRecordProcessor::Process(IWbemClassObject *precordobj, IWbemServices* pNamespace)
{
	TRACE_FUNC_BEGIN;
	bool bprocessed = true;

	HRESULT hr;

	do
	{
		_msft_disk_info disk_info;

		//
		// Get Disk Number
		//
		VARIANT vtProp;
		hr = precordobj->Get(_bstr_t(MSFT_Disk_Properties::DISK_NUMBER), 0, &vtProp, 0, 0);
		if (FAILED(hr))
		{
			m_error_msg = _com_error(hr).ErrorMessage();

			TRACE_ERROR("Could not retrieve %s for SMFT_Disk instance. Com error %s\n",
				MSFT_Disk_Properties::DISK_NUMBER,
				m_error_msg.c_str());

			bprocessed = false;
		}
		else
		{
			disk_info.Number = vtProp.uintVal;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// Get PartitionStyle
		//
		hr = precordobj->Get(_bstr_t(MSFT_Disk_Properties::DISK_PARTITION_STYLE), 0, &vtProp, 0, 0);
		if (FAILED(hr))
		{
			m_error_msg = _com_error(hr).ErrorMessage();

			TRACE_ERROR("Could not retrieve %s for SMFT_Disk instance. Com error %s\n",
				MSFT_Disk_Properties::DISK_PARTITION_STYLE,
				m_error_msg.c_str());

			bprocessed = false;
		}
		else
		{
			disk_info.PartitionStyle = vtProp.uiVal;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		if (0 == disk_info.PartitionStyle)
		{
			//
			// Unkown Partition style
			//
		}
		else if (1 == disk_info.PartitionStyle)
		{
			//
			// MBR Partition style
			//
			hr = precordobj->Get(_bstr_t(MSFT_Disk_Properties::DISK_SIGNATURE), 0, &vtProp, 0, 0);
			if (FAILED(hr))
			{
				m_error_msg = _com_error(hr).ErrorMessage();

				TRACE_ERROR("Could not retrieve %s for SMFT_Disk instance. Com error %s\n",
					MSFT_Disk_Properties::DISK_SIGNATURE,
					m_error_msg.c_str());

				bprocessed = false;
			}
			else
			{
				std::stringstream val_stream;
				val_stream << vtProp.ulVal;

				disk_info.Signature_Guid = val_stream.str();
			}
			VariantClear(&vtProp); if (!bprocessed) break;
		}
		else if (2 == disk_info.PartitionStyle)
		{
			//
			// GPT Partition style
			//
			hr = precordobj->Get(_bstr_t(MSFT_Disk_Properties::DISK_GUID), 0, &vtProp, 0, 0);
			if (FAILED(hr))
			{
				m_error_msg = _com_error(hr).ErrorMessage();

				TRACE_ERROR("Could not retrieve %s for SMFT_Disk instance. Com error %s\n",
					MSFT_Disk_Properties::DISK_GUID,
					m_error_msg.c_str());

				bprocessed = false;
			}
			else
			{
				disk_info.Signature_Guid = _bstr_t(vtProp.bstrVal);
			}
			VariantClear(&vtProp); if (!bprocessed) break;
		}
	
		//
		// Get IsOffline
		//
		hr = precordobj->Get(_bstr_t(MSFT_Disk_Properties::DISK_IS_OFFLINE), 0, &vtProp, 0, 0);
		if (FAILED(hr))
		{
			m_error_msg = _com_error(hr).ErrorMessage();

			TRACE_ERROR("Could not retrieve %s for SMFT_Disk instance. Com error %s\n",
				MSFT_Disk_Properties::DISK_IS_OFFLINE,
				m_error_msg.c_str());

			bprocessed = false;
		}
		else
		{
			disk_info.IsOffline = vtProp.boolVal;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		if (disk_info.IsOffline)
		{
			//
			// Get Offline reason
			//
			hr = precordobj->Get(_bstr_t(MSFT_Disk_Properties::DISK_OFFLINE_REASON), 0, &vtProp, 0, 0);
			if (FAILED(hr))
			{
				m_error_msg = _com_error(hr).ErrorMessage();

				TRACE_ERROR("Could not retrieve %s for SMFT_Disk instance. Com error %s\n",
					MSFT_Disk_Properties::DISK_OFFLINE_REASON,
					m_error_msg.c_str());

				bprocessed = false;
			}
			else
			{
				disk_info.OfflineReason = vtProp.uiVal;
			}
			VariantClear(&vtProp); if (!bprocessed) break;
		}

		m_diskInfo[disk_info.Number] = disk_info;

	} while (false);

	TRACE_FUNC_END;
	return bprocessed;
}

std::string CMSFT_DiskRecordProcessor::GetErrMsg() const
{
	return m_error_msg;
}

void CMSFT_DiskRecordProcessor::GetDiskInfo(std::map<UINT, mdft_disk_info_t>& diskInfo)
{
	diskInfo.insert(m_diskInfo.begin(), m_diskInfo.end());
}

/*
Method      : CMSFT_PartitionRecordProcessor::Process

Description : Reads required properties from Win32_DiskPartition wmi object.

Parameters  : [in] precordobj:
              [in] pNamespace:

Return      : true on success, otherwise false

*/
bool CMSFT_PartitionRecordProcessor::Process(IWbemClassObject *precordobj, IWbemServices* pNamespace)
{
	TRACE_FUNC_BEGIN;
	bool bprocessed = true;

	VARIANT vtProp;
	HRESULT hrCol;

	do
	{
		msft_disk_partition disk_partition;

		//
		// DiskNumber
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::DiskNumber), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			disk_partition.DiskNumber = vtProp.uintVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::DiskNumber;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// PartitionNumber
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::PartitionNumber), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			disk_partition.PartitionNumber = vtProp.uintVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::PartitionNumber;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// DriveLetter
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::DriveLetter), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			if (vtProp.vt != VT_NULL && vtProp.vt != VT_EMPTY)
			{
				disk_partition.DriveLetter = vtProp.cVal;
			}
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::DriveLetter;
		}
		VariantClear(&vtProp); if (!bprocessed) break;


		//
		// AccessPaths
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::AccessPaths), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			if (vtProp.vt & VT_ARRAY && 
				vtProp.vt & VT_BSTR &&
				vtProp.parray != NULL)
			{
				BSTR *pAccessPath = NULL;
				HRESULT hr = SafeArrayAccessData(vtProp.parray, (void **)&pAccessPath);
				if (!FAILED(hr) && 
					pAccessPath != NULL &&
					vtProp.parray->rgsabound != NULL)
				{
					std::stringstream accessPaths;
					for (long index = 0; index < vtProp.parray->rgsabound[0].cElements; index++)
					{
						accessPaths << _bstr_t(pAccessPath[index]) << " ";
					}
					SafeArrayUnaccessData(vtProp.parray);

					disk_partition.AccessPaths = accessPaths.str();
				}
			}
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::AccessPaths;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// OperationalStatus
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::OperationalStatus), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			disk_partition.OperationalStatus = vtProp.uintVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::OperationalStatus;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// TransitionState
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::TransitionState), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			disk_partition.TransitionState = vtProp.uintVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::TransitionState;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// Size
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::Size), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			if (vtProp.vt == VT_BSTR)
				disk_partition.Size = _bstr_t(vtProp.bstrVal);
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::Size;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// Offset
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::Offset), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			if (vtProp.vt == VT_BSTR)
				disk_partition.Offset = _bstr_t(vtProp.bstrVal);
		}
		else
		{
			//bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::Offset;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// MbrType
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::MbrType), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			if (vtProp.vt != VT_NULL)
				disk_partition.MbrType = vtProp.uintVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::MbrType;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// GptType
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::GptType), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			if (vtProp.vt == VT_BSTR)
				disk_partition.GptType = _bstr_t(vtProp.bstrVal);
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::GptType;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// Guid
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::Guid), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			if (vtProp.vt == VT_BSTR)
				disk_partition.Guid = _bstr_t(vtProp.bstrVal);
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::Guid;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// IsReadOnly
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::IsReadOnly), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			disk_partition.IsReadOnly = vtProp.boolVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::IsReadOnly;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// IsOffline
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::IsOffline), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			disk_partition.IsOffline = vtProp.boolVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::IsOffline;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// IsSystem
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::IsSystem), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			disk_partition.IsSystem = vtProp.boolVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::IsSystem;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// IsBoot
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::IsBoot), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			disk_partition.IsBoot = vtProp.boolVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::IsBoot;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// IsActive
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::IsActive), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			disk_partition.IsActive = vtProp.boolVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::IsActive;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// IsHidden
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::IsHidden), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			disk_partition.IsHidden = vtProp.boolVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::IsHidden;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// IsShadowCopy
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::IsShadowCopy), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			disk_partition.IsShadowCopy = vtProp.boolVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::IsShadowCopy;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		//
		// NoDefaultDriveLetter
		//
		hrCol = precordobj->Get(_bstr_t(MSFT_DiskPartition_Properties::NoDefaultDriveLetter), 0, &vtProp, 0, 0);
		if (!FAILED(hrCol))
		{
			disk_partition.NoDefaultDriveLetter = vtProp.boolVal;
		}
		else
		{
			bprocessed = false;
			m_error_msg = "failed to get MSFT_Partition object property: ";
			m_error_msg += MSFT_DiskPartition_Properties::NoDefaultDriveLetter;
		}
		VariantClear(&vtProp); if (!bprocessed) break;

		m_disk_partitions.push_back(disk_partition);

	} while (false);

	TRACE_FUNC_END;
	return bprocessed;
}

std::string CMSFT_PartitionRecordProcessor::GetErrMsg(void) const
{
	return m_error_msg;
}

void CMSFT_PartitionRecordProcessor::GetDiskParitions(std::list<msft_disk_partition>& disk_partitions)
{
	disk_partitions.clear();

	disk_partitions.insert(disk_partitions.begin(),
		m_disk_partitions.begin(),
		m_disk_partitions.end());
}

void _msft_disk_partition::Print() const
{
	std::stringstream partitionDetails;

	partitionDetails 
		<< "DiskNumber: " << DiskNumber << std::endl
		<< "PartitionNumber: " << PartitionNumber << std::endl
		<< "AccessPaths: " << AccessPaths << std::endl
		<< "OperationalStatus: " << OperationalStatus << std::endl
		<< "TransitionState: " << TransitionState << std::endl
		<< "Size: " << Size << std::endl
		<< "Offset: " << Offset << std::endl
		<< "MbrType: " << MbrType << std::endl
		<< "GptType: " << GptType << std::endl
		<< "Guid: " << Guid << std::endl
		<< "IsReadOnly: " << IsReadOnly << std::endl
		<< "IsOffline: " << IsOffline << std::endl
		<< "IsSystem: " << IsSystem << std::endl
		<< "IsBoot: " << IsBoot << std::endl
		<< "IsActive: " << IsActive << std::endl
		<< "IsHidden: " << IsHidden << std::endl
		<< "IsShadowCopy: " << IsShadowCopy << std::endl
		<< "NoDefaultDriveLetter: "<< NoDefaultDriveLetter << std::endl
		<< "DriveLetter: " << (DriveLetter != '/0' ? DriveLetter : ' ');

	TRACE_INFO("\n%s\n", partitionDetails.str().c_str());
}

}
