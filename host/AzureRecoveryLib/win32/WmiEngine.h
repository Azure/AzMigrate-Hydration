/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	WmiEngine.h

Description	:   Warpper class for iterating WMI queries results

History		:   1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_RECOVERY_WMI_ENGINE_H
#define AZURE_RECOVERY_WMI_ENGINE_H

#include "WmiConnect.h"
#include <string>
#include <map>

namespace AzureRecovery
{
	namespace WMI_PROVIDER_NAMESPACE
	{
		const char CIMV2[] = "root\\CIMV2";
		const char STORAGE[] = "root\\Microsoft\\Windows\\Storage";
	}

	namespace Win32_DiskDrive_Prop
	{
		const char Device_ID[]    = "DeviceID";
		const char SCSIPort[]     = "SCSIPort";
		const char SCSILun[]      = "SCSILogicalUnit";
		const char SCSITargetID[] = "SCSITargetID";
		const char Index[]        = "Index";
		const char InterfaceType[]= "InterfaceType";
	}

	const WCHAR PROP_RETURN_VALUE[]              = L"ReturnValue";
	const WCHAR METHOD_MSFT_DISK_ONLINE[]        = L"Online";
	const WCHAR METHOD_MSFT_DISK_SETATTRIBUTES[] = L"SetAttributes";

	//
	// Record Processor interface
	//
	class CWmiRecordProcessor
	{
	public:
		virtual bool Process(IWbemClassObject *precordobj, IWbemServices* pNamespace = NULL) = 0;
		virtual std::string GetErrMsg(void) const = 0;
		virtual ~CWmiRecordProcessor() { }
	};

	class CWmiEngine
	{
	public:
		CWmiEngine(CWmiRecordProcessor *pwrp, const std::string& wmiProvider = WMI_PROVIDER_NAMESPACE::CIMV2);
		~CWmiEngine();

		bool init();
		bool ProcessQuery(const std::string &query_string);

	private:
		bool m_init;
		WINWMIConnector m_wmiConnector;
		std::string m_wmiProvider;
		CWmiRecordProcessor *m_pWmiRecordProcessor;
	};
}
#endif // ~AZURE_RECOVERY_WMI_ENGINE_H
