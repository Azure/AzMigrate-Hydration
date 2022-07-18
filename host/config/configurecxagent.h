
#ifndef CONFIGURECXAGENT__H
#define CONFIGURECXAGENT__H

#include <string>
#include <vector>
#include "volumegroupsettings.h"

#define HASH_LENGTH 16

namespace NSRegisterHostStaticInfo
{
	const char * const StrRegisterHostStatus[] = {"registered", "update failure", "update failed with peer cert error"};

    const char DISKS_LAYOUT_STATUS_KEY[] = "disks_layout_file_transfer_status";
	const char DISKS_LAYOUT_STATUS_SEP = ':';

	/* here follows status values for disks_layout_file_transfer_status */
	const char SUCCESS[] = "success";
	const char FAILURE[] = "failure";
};

struct ConfigureCxAgent {
    //boost::range<> getVxAgents();

    typedef enum eRegisterHostStatus
    {
        HostRegistered = 0,
        HostRegisterFailed,
        HostRegisterFailedWithPeerCertError

    } RegisterHostStatus_t;

    virtual RegisterHostStatus_t RegisterHostStaticInfo(
		const bool &registerInAnyCase,
        std::string agentVersion,
        std::string driverVersion,
        std::string hostName,
        std::string ipAddress,
		Object const & osinfo,
		int compatibility,
		std::string agentInstallPath,
        OS_VAL osVal,
		std::string zone,
		std::string sysVol,
		std::string patchHistory,
        VolumeSummaries_t const& allVolumes,
		std::string agentMode,
		std::string prod_version , 
        NicInfos_t const& nicinfos,
        HypervisorInfo_t const& hypervinfo,
        NwwnPwwns_t const &npwwns,
        ENDIANNESS e,
		CpuInfos_t const & cpuinfos,
        std::map<std::string, std::string> const& acnvpairs,
		unsigned long long const & memory,
        const Options_t & miscInfo
        ) = 0;

    virtual RegisterHostStatus_t RegisterHostDynamicInfo( 
		std::string agentLocalTime,
		SV_ULONGLONG sysVolCapacity,
        SV_ULONGLONG sysVolFreeSpace,
		SV_ULONGLONG insVolCapacity,
		SV_ULONGLONG insVolFreeSpace,
        VolumeDynamicInfos_t const& volumeDynamicInfos
        ) = 0;

    virtual std::string GetRegisterHostStatusStr(const RegisterHostStatus_t rgs) = 0;

	/// \brief returns true if host static info registered atleast once
	virtual bool IsHostStaticInfoRegisteredAtleastOnce(void) = 0;

//#ifdef SV_FABRIC	
		virtual void SanRegisterHost(
	    std::string agentVersion,
	    std::string driverVersion,
	    std::string hostName,
	    std::string ipAddress,
	    std::string osInfo,
	    int compatibility,
	    std::string agentInstallPath, 
	    OS_VAL osVal,
            std::vector<SanInitiatorSummary> const& initiators,
	    std::vector<SanVolumeSummary> const& allVolumes) = 0;
//#endif
    //VolumeSummaries_t const& allVolumes ) = 0;


    
    virtual void UnregisterHost() = 0;
};

#endif // CONFIGURECXAGENT__H

