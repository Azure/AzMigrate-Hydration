#include "serializeinitialsettings.h"
#include "serializevolumegroupsettings.h"
#include "serializecdpsnapshotrequest.h"
#include "serializeatconfigmanagersettings.h"
#include "serializeswitchinitialsettings.h"
#include "xmlizeinitialsettings.h"
#include "xmlizevolumegroupsettings.h"
#include "xmlizecdpsnapshotrequest.h"
#include "xmlizeatconfigmanagersettings.h"
#include "cxproxy.h"
#include "inm_md5.h"
#include "serializer.h"
#include "apinames.h"
#include "portablehelpers.h"
#include "inmsafecapis.h"

#include <boost/shared_array.hpp>
#include <curl/curl.h>
#include <fstream>
#include <boost/filesystem/path.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#define MAX_VOLUME_SUMMARIES_LOG_SIZE  10 * 1024 * 1024

struct HttpRemoteFile
{
    std::string m_fileNamePath ;
    FILE* m_stream ;

    HttpRemoteFile()
    {
       m_stream = NULL ;
    }
    HttpRemoteFile( const std::string& localFileName )
    {
        m_fileNamePath = localFileName ;
        m_stream = NULL ;
    }
    ~HttpRemoteFile()
    {
       if( m_stream != NULL ) 
          fclose( m_stream ) ; 
    }
} ;

static bool IsPeerCertError(std::string& errString)
{
    const std::string peerCertError[] = {
        "(60) - Peer certificate cannot be authenticated with given CA certificates",
        "(60) - Peer certificate cannot be authenticated with known CA certificates" };

    if (errString.find(peerCertError[0]) != std::string::npos ||
        errString.find(peerCertError[1]) != std::string::npos)
        return true;

    return false;
}

void CxProxy::logRegisteredVolumeSummaries(VolumeSummaries_t const& vs)
{
    boost::filesystem::path logPath(m_localConfigurator.getLogPathname());
    logPath /= "mtvolreg.log";

    std::ofstream outfile;
    outfile.open(logPath.string().c_str(), std::ofstream::out | std::ofstream::app);

    if (outfile.is_open() && outfile.good())
    {
        int currPos = outfile.tellp();
        if (currPos >= MAX_VOLUME_SUMMARIES_LOG_SIZE)
        {
            outfile.close();
            outfile.open(logPath.string().c_str(), std::ofstream::out | std::ofstream::trunc);
        }
    }

    if (outfile.is_open() && outfile.good())
    {
        std::string date = boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time());
        outfile << "\n=================" << date << "============================\n";
        std::stringstream serializedVs;
        serializedVs << CxArgObj<VolumeSummaries_t>(vs);
        outfile << serializedVs.str();
        outfile << "\n================= end ============================\n";
        outfile.close();
        return;
    }

    DebugPrintf(SV_LOG_ERROR,
        "%s : Failed to log volume summaries to file %s\n",
        FUNCTION_NAME,
        logPath.string().c_str());
    return;

}

ConfigureCxAgent::RegisterHostStatus_t CxProxy::RegisterHostStaticInfo(
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
                )
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    RegisterHostStatus_t rgs = HostRegisterFailed;
    try
    {
        const char *api = REGISTER_HOST_STATIC_INFO;
        std::string biosId("");
        std::string marshlledstr = "";

        GetBiosId(biosId);

        marshlledstr = marshalCxCall(api, m_localConfigurator.getHostId(), agentVersion, driverVersion,
            hostName, ipAddress, osinfo, compatibility, agentInstallPath, osVal, zone, sysVol,
            patchHistory, allVolumes, agentMode, prod_version,
            nicinfos, hypervinfo, npwwns, e, cpuinfos, acnvpairs, memory, miscInfo, biosId);

        DebugPrintf(SV_LOG_DEBUG, "Registration Data: %s\n", marshlledstr.c_str());
		unsigned char currhash[HASH_LENGTH];
		INM_MD5_CTX ctx;
		INM_MD5Init(&ctx);
		INM_MD5Update(&ctx, (unsigned char*)marshlledstr.c_str(), marshlledstr.size());
		INM_MD5Final(currhash, &ctx);

		CompareMemory_t cms[] = {&comparememory, &alwaysreturnunequal};
		CompareMemory_t cm = cms[registerInAnyCase];

		bool shouldregister = false;
		if ((*cm)(m_prevRegHostStaticHash, currhash, HASH_LENGTH)) {
			DebugPrintf(SV_LOG_DEBUG, "register static information checksum does not match\n");
			shouldregister = true;
		}
		if (!m_dataCacher.DoesVolumesCacheExist()) {
			DebugPrintf(SV_LOG_DEBUG, "volumeinfocollector cache does not exist\n");
			shouldregister = true;
		}

        if (m_dataCacher.PutVolumesToCache(allVolumes)) {
            DebugPrintf(SV_LOG_DEBUG, "volumeinfocollector cache updated.\n");

            if (shouldregister) {
                DebugPrintf(SV_LOG_DEBUG, "registering static information\n");
                Serializer sr(m_serializeType);

                sr.Serialize(api, m_localConfigurator.getHostId(), agentVersion, driverVersion,
                    hostName, ipAddress, osinfo, compatibility, agentInstallPath, osVal, zone, sysVol,
                    patchHistory, allVolumes, agentMode, prod_version,
                    nicinfos, hypervinfo, npwwns, e, cpuinfos, acnvpairs, memory, miscInfo, biosId);
                DebugPrintf(SV_LOG_DEBUG, "Registration Data: %s\n", sr.m_SerializedRequest.c_str());

                m_transport(sr);
                rgs = sr.UnSerialize<RegisterHostStatus_t>();
                if (HostRegistered == rgs) {
                    inm_memcpy_s(m_prevRegHostStaticHash, sizeof(m_prevRegHostStaticHash), currhash, sizeof currhash);
                    
                    std::string agentRole = m_localConfigurator.getAgentRole();
                    if ( registerInAnyCase &&
                         (0 == agentRole.compare(MASTER_TARGET_ROLE)) &&
                         (osVal == SV_LINUX_OS))
                    {
                        logRegisteredVolumeSummaries(allVolumes);
                    }
                }
            }
            else {
                DebugPrintf(SV_LOG_DEBUG, "register static information checksum match\n");
                rgs = HostRegistered;
            }
        } 
        else {
            DebugPrintf(SV_LOG_ERROR, "Failed to update volumeinfocollector cache with error %s. Cannot do host registration now. It will be retried.\n", m_dataCacher.GetErrMsg().c_str());
        }
    }
    catch( std::string const& e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e.c_str());
    }
    catch( char const* e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e);
    }
    catch ( ContextualException const& ce ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, ce.what());
        
        std::string errStr = ce.what();
        if (IsPeerCertError(errStr))
            rgs = HostRegisterFailedWithPeerCertError;
    }
    catch( std::exception const& e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e.what());
    }
    catch(...) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception\n", FUNCTION_NAME);
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return rgs;
}


ConfigureCxAgent::RegisterHostStatus_t CxProxy::RegisterHostDynamicInfo(
		        std::string agentLocalTime,
		        SV_ULONGLONG sysVolCapacity,
                SV_ULONGLONG sysVolFreeSpace,
		        SV_ULONGLONG insVolCapacity,
		        SV_ULONGLONG insVolFreeSpace,
                VolumeDynamicInfos_t const& volumeDynamicInfos)
{
    RegisterHostStatus_t rgs = HostRegisterFailed;
    try
    {
		const char* api = REGISTER_HOST_DYNAMIC_INFO ;
        Serializer sr(m_serializeType);
        std::string biosId("");
        GetBiosId(biosId);

        sr.Serialize(api, m_localConfigurator.getHostId(), agentLocalTime, sysVolCapacity, sysVolFreeSpace, insVolCapacity,
            insVolFreeSpace, volumeDynamicInfos, biosId);

        m_transport( sr ) ;
        rgs = sr.UnSerialize<RegisterHostStatus_t>() ;
    }
    catch( std::string const& e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e.c_str());
    }
    catch( char const* e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e);
    }
    catch ( ContextualException const& ce ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, ce.what());

        std::string errStr = ce.what();
        if (IsPeerCertError(errStr))
            rgs = HostRegisterFailedWithPeerCertError;
    }
    catch( std::exception const& e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e.what());
    }
    catch(...) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception\n", FUNCTION_NAME);
    }

    return rgs;
}

bool CxProxy::IsHostStaticInfoRegisteredAtleastOnce(void)
{
	unsigned char zeroHash[HASH_LENGTH] = { 0 };
	return (0 != memcmp(zeroHash, m_prevRegHostStaticHash, sizeof zeroHash));
}

void CxProxy::SanRegisterHost(
        std::string agentVersion,
        std::string driverVersion,
        std::string hostName,
        std::string ipAddress,
        std::string osInfo,
        int compatibility,
        std::string agentInstallPath, 
        OS_VAL osVal,
        std::vector<SanInitiatorSummary> const& initiators, 
        std::vector<SanVolumeSummary> const& allVolumes ) 
{
    Serializer sr(m_serializeType);
    sr.Serialize(SAN_REGISTER_HOST, m_localConfigurator.getHostId(), agentVersion, driverVersion, hostName, ipAddress, osInfo, compatibility, agentInstallPath, osVal, initiators,allVolumes );
    m_transport( sr );
}
//#endif

void CxProxy::RegisterClusterInfo(
        std::string clusterName,
        std::string groupName,
        std::string groupId,
        std::string action,
        std::string newGroupName,
        std::vector<ClusterVolumeInfo> const& clusterVolumes,
        std::string fieldName,
        std::string fieldValue ) 
{
    Serializer sr(m_serializeType);
    sr.Serialize(REGISTER_CLUSTER_INFO, m_localConfigurator.getHostId(), clusterName, groupName, groupId, action, newGroupName, clusterVolumes, fieldName, fieldValue );
    m_transport( sr );
}

void CxProxy::UnregisterHost() 
{
    Serializer sr(m_serializeType);
    std::string biosId("");

    GetBiosId(biosId);

    sr.Serialize(UNREGISTER_HOST, m_localConfigurator.getHostId(), biosId);
    m_transport( sr );
}



static size_t download_function(void *buffer, size_t size, size_t nmemb, void *stream)
{
	struct HttpRemoteFile *out=(struct HttpRemoteFile *)stream;
	if(out != NULL )
	{
		if( out->m_stream == NULL ) 
		{
			out->m_stream=fopen(out->m_fileNamePath.c_str(), "wb");
		}
		if( out->m_stream != NULL )
		{
			return fwrite(buffer, size, nmemb, out->m_stream);
		}
	}
	return -1 ; 
}

InitialSettings CxProxy::getInitialSettings()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_INITIAL_SETTINGS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, INITIAL_SETTINGS_VERSION);
    m_transport( sr );
    DebugPrintf(SV_LOG_DEBUG, "EXIT %s\n", FUNCTION_NAME);
    return sr.UnSerialize<InitialSettings>();
}

std::list<CsJob> CxProxy::getJobsToProcess() {
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_JOBS_TO_PROCESS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, CSJOBS_VERSION);
    m_transport(sr);
    return sr.UnSerialize<list<CsJob> >();
}

void CxProxy::updateJobStatus(CsJob csJob) {
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_JOB_STATUS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, csJob);
    m_transport(sr);
}

void CxProxy::setApiAndFirstArg(const char *apiname,
                                const char *dpc, 
                                const char **apitocall,
                                const char **firstargtoapi) const
{
    if (PHPSerialize == m_serializeType)
    {
        *apitocall = dpc;
        *firstargtoapi = apiname;
    }
    else if (Xmlize == m_serializeType)
    {
        *apitocall = apiname;
        *firstargtoapi = dpc;
    }
}

SNAPSHOT_REQUESTS CxProxy::getSnapshotRequestFromCx(const std::string & cxVolName) const {
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_SRR_JOBS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall,firstargtoapi,SNAPSHOT_SETTINGS_VERSION, cxVolName);
    m_transport( sr );
    return typecastSnapshotRequests( sr.UnSerialize<SNAPSHOT_REQUESTS_t>() );
}	


void SwitchCxProxy::RegisterSwitchAgent(
                SwitchAgentInfo & swInfo ) 
{
    (void) m_transport( marshalCxCall( "::registerSwitchAgent", m_localConfigurator.getHostId(), swInfo ) );
}
void SwitchCxProxy::NewRegisterSwitchAgent(
        SwitchSummary & swSummary ) 
{
    (void) m_transport( marshalCxCall( "::newRegisterSwitchAgent", m_localConfigurator.getHostId(), swSummary ) );
}

void SwitchCxProxy::UnregisterSwitchAgent() 
{
   (void) m_transport( marshalCxCall( "::unregisterSwitchAgent", m_localConfigurator.getHostId()) );
}

void SwitchCxProxy::UpdateSATimestamp(
		string timeZone,
		string localTime )
{
    (void) m_transport( marshalCxCall( "::updateSaTimestamp", m_localConfigurator.getHostId(), timeZone, localTime ) );
}

SwitchInitialSettings SwitchCxProxy::getSwitchInitialSettings() {
    std::string debug = m_transport( marshalCxCall( m_dpc, "getSwitchInitialSettings" ) );
    //cout << "debug" << debug << endl;
    return unmarshal<SwitchInitialSettings>( debug );
}

SwitchInitialSettings SwitchCxProxy::getSwitchInitialSettingsOnReboot() {
    std::string debug = m_transport( marshalCxCall( m_dpc, "getSwitchInitialSettingsOnReboot" ) );
    return unmarshal<SwitchInitialSettings>( debug );
}

