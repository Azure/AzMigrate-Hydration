//
// configurevxagentproxy.cpp: used by VxAgent to get local & remote settings
//
#include <iostream>
#include <ace/Mutex.h>
#include <ace/Guard_T.h>
#include "configurevxagentproxy.h"
#include "localconfigurator.h"
#include "prismsettings.h"
#include "volumegroupsettings.h"
#include "initialsettings.h"
#include "serializecdpsnapshotrequest.h"
#include "serializevolumegroupsettings.h"
#include "serializeretentioninformation.h"
#include "xmlizecdpsnapshotrequest.h"
#include "xmlizevolumegroupsettings.h"
#include "xmlizeretentioninformation.h"
#include "apinames.h"
#include "serializer.h"
#include "inm_md5.h"

//#ifdef SV_FABRIC
#include "serializeatconfigmanagersettings.h"
#include "xmlizeatconfigmanagersettings.h"
//#endif

#include "AgentHealthContract.h"

using namespace std;

/*
Snapshot Request APIs list:
ConfigureVxAgentProxy::getSnapshotRequestFromCx
ConfigureVxAgentProxy::notifyCxOnSnapshotStatus
ConfigureVxAgentProxy::notifyCxOnSnapshotProgress
ConfigureVxAgentProxy::isSnapshotAborted
ConfigureVxAgentProxy::makeSnapshotActive
ConfigureVxAgentProxy::getSnapshotRequests
*/

/*const char UPDATE_AGENT_LOG[]               = "updateAgentLog";
// Bug #6298
const char UPDATE_REPLICATION_STATE_STATUS[]      = "updateReplicationStateStatus";
const char UPDATE_REPLICATION_CLEANUP_STATUS[]      = "updateReplicationCleanupStatus";
const char UPDATE_RESTART_RESYNC_CLEANUP_STATUS[]   = "restartReplicationUpdate";
const char RENAME_CLUSTER_GROUP[]           = "renameClusterGroup";
const char DELETE_VOLUMES_FROM_CLUSTER[]    = "deleteVolumesFromCluster";
const char DELETE_CLUSTER_NODE[]            = "deleteClusterNode";
const char ADD_VOLUMES_TO_CLUSTER[]         = "addVolumesToCluster";
const char GET_VOLUME_CHECKPOINT[]          = "getVolumeCheckpoint";
const char SET_RESYNC_PROGRESS[]            = "setResyncProgress";*/
/* Added by BSR fast sync TBC */
//const char SET_RESYNC_PROGRESS_FASTSYNC[] = "setResyncProgressFastsync" ;
/*
*  Added Following strings to fix overshoot issue using bitmap
*  By Suresh
*/
/*const char SET_RESYNC_UPDATE_PROGRESS_BYTES_FASTSYNC[]="setResyncProgressBytesFast";
const char SET_RESYNC_UPDATE_FULLSYNC_BYTES_FASTSYNC[]="setHcdProgressBytesFast";
const char SET_RESYNC_UPDATE_MATCHED_BYTES_FASTSYNC[] = "setResyncMatchedBytesFast" ;
const char SET_RESYNC_UPDATE_FULLY_UNUSED_BYTES_FASTSYNC[] = "setResyncFullyUnusedBytesFast" ;

//Changes End
const char GET_VOLUME_CHECKPOINT_FASTSYNC[] = "getVolumeCheckpointFastsync" ;
const char SET_RESYNC_TRANSITION[] = "setResyncTransitionStepOneToTwo" ; */
/* End of the change*/
/*const char GET_INITIAL_SYNC_DIRECTORIES[]   = "getInitialSyncDirectories";
const char GET_TARGET_REPLICATION_JOB_ID[]  = "getTargetReplicationJobId";
const char UPDATE_VOLUME_ATTRIBUTE[]        = "updateVolumeAttribute";
const char GET_CURRENT_VOLUME_ATTRIBUTE[]   = "getCurrentVolumeAttribute";
const char UPDATE_LOG_AVAILABLE[]           = "updateLogAvailable";
const char UPDATE_CDP_INFORMATIONV2[]       = "updateCdpInformationV2";
const char UPDATE_CDP_DISKUSAGE[]			= "updateCdpDiskUsage";
const char UPDATE_RETENTION_TAG[]           = "updateRetentionTag";
const char GET_HOST_RETENTION_WINDOW[]      = "getHostDbRetentionWindows";
const char UPDATE_RETENTION_INFO[]          = "updateRetentionDetails";
const char GET_CLEAR_DIFFS[]                = "getClearDiffs";
const char SET_TARGET_RESYNC_REQUIRED[]     = "SetTargetResyncRequired";
const char SET_SOURCE_RESYNC_REQUIRED[]     = "setSourceResyncRequired";
const char SET_PRISM_RESYNC_REQUIRED[]     = "setPrismResyncRequired";
const char SET_XS_POOL_SOURCE_RESYNC_REQUIRED[]     = "setXsPoolSourceResyncRequired";
const char PAUSE_REPLICATION_FROM_HOST[]	= "pauseReplicationFromHost";
const char RESUME_REPLICATION_FROM_HOST[]	= "resumeReplicationFromHost";
const char UPDATE_SNAPSHOT_STATUS[]         = "updateSnapshotStatus";
const char UPDATE_PENDINGDATA_ONTARGET[]    = "updatePendingDataOnTarget";

const char UPDATE_SNAPSHOT_PROGRESS[]       = "updateSnapshotProgress";
const char UPDATE_SNAPSHOT_CREATION[]		= "updateSnapshotCreation";
const char UPDATE_SNAPSHOT_DELETION[]		= "deleteCdpcliVirtualSnapshot";
const char MAKE_ACTIVE_SNAPSHOT_INSTANCE[]  = "makeActiveSnapshotInstance";

const char GET_RESYNC_START_TIMESTAMP[]		= "get_stats_resyncTimeTag";
const char SEND_RESYNC_START_TIMESTAMP[]	= "set_resync_start_time_stamp";
const char UPDATE_MIRROR_STATE[]			= "update_mirror_state";
const char UPDATE_LASTIO_TIMESTAMP_ON_ATLUN[]			= "update_lastio_timestamp_on_atlun";
const char UPDATE_VOLUMES_PENDING_CHANGES[]			= "update_source_pending_changes";
const char GET_RESYNC_END_TIMESTAMP[]		= "get_stats_resyncTimeTag";
const char SEND_RESYNC_END_TIMESTAMP[]		= "set_resync_end_time_stamp";*/

/* Added by BSR for Fast Sync TBC */
/*const char GET_RESYNC_START_TIMESTAMP_FASTSYNC[]		= "get_stats_resyncTimeTag_fastsync";
const char SEND_RESYNC_START_TIMESTAMP_FASTSYNC[]	= "set_resync_start_time_stamp_fastsync";
const char GET_RESYNC_END_TIMESTAMP_FASTSYNC[]		= "get_stats_resyncTimeTag_fastsync";
const char SEND_RESYNC_END_TIMESTAMP_FASTSYNC[]		= "set_resync_end_time_stamp_fastsync";*/

/*End of the changes*/
/*
const char SEND_END_QUASI_STATE_REQUEST[]	= "setEndQuasiStateRequest";
const char SEND_ALERT_TO_CX[]				= "updateAgentLog";
const char VSNAP_REMOUNT_VOLUMES[]			= "getVsnapDrivesMounted";
const char NOTIFY_CX_DIFFS_DRAINED[]		= "updateOutpostAgentStatus";
const char DELETE_VIRTUAL_SNAPSHOT[]		= "deleteVirtualSnapshot";
const char CDP_STOP_REPLICATION[]		    = "cdpStopReplication";
const char GET_VIRTUAL_SERVER_NAME[]		= "getVirtualServerName";
const char REGISTER_CLUSTER_INFO[]          = "registerClusterInfo";
const char REGISTER_XS_INFO[]               = "registerXsInfo";

const char GET_FABRIC_SERVICE_SETTING[]     = "getFabricServiceSetting";
const char GET_PRISM_SERVICE_SETTING[]     = "getPrismServiceSetting";
const char GET_FABRIC_SERVICE_SETTING_ON_REBOOT[]     = "getFabricServiceSettingOnReboot";
const char UPDATE_APPLIANCE_LUN_STATE[]     = "updateApplianceLunState";
const char UPDATE_PRISM_APPLIANCE_LUN_STATE[]     = "updatePrismApplianceLunState";
const char UPDATE_DEVICE_LUN_STATE[]     = "updateDeviceLunState";
const char UPDATE_BINDING_DEVICE_DISCOVERY_STATE[] = "updateBindingDeviceDiscoveryState";
const char REGISTER_INITIATORS[]           = "registerInitiators";
const char SET_LAST_RESYNC_OFFSET_DIRECTSYNC[] = "set_lastresync_directsync";
const char UPDATE_GROUP_INFO_LIST_STATE[]     = "updateGroupInfoListState";
const char SET_PAUSE_REPLICATION_STATUS[]	= "setPauseReplicationStatus";*/


/*
ConfigureVxAgentProxy 
private:
LocalConfigurator& m_localConfigurator;
ConfiguratorDeferredProcedureCall const m_dpc;
ConfiguratorTransport const& m_transport;
};
*/

// getReplicationPairs();
// getReplicationPairManager();
// getVolumeGroups();
// getVolumes();
// getSnapshotManager();
// getVxTransport();
// getRetentionManager();
// setHostInfo( hostname, ipaddress, os, agentVersion, driverVersion );
// updateAgentLog( string );


string ConfigureVxAgentProxy::getCacheDirectory() const { 
    return m_localConfigurator.getCacheDirectory();
}

string ConfigureVxAgentProxy::getHostId() const { 
    return m_localConfigurator.getHostId();
}

string ConfigureVxAgentProxy::getResourceId() const { 
    return m_localConfigurator.getResourceId();
}

string ConfigureVxAgentProxy::getSourceGroupId() const {
    return m_localConfigurator.getSourceGroupId();
}
int ConfigureVxAgentProxy::getMaxDifferentialPayload() const { 
    return m_localConfigurator.getMaxDifferentialPayload();
}

SV_UINT ConfigureVxAgentProxy::getFastSyncReadBufferSize() const {
    return m_localConfigurator.getFastSyncReadBufferSize();
}

SV_HOST_AGENT_TYPE ConfigureVxAgentProxy::getAgentType() const {
    return m_localConfigurator.getAgentType();
}

ConfigureVxTransport& ConfigureVxAgentProxy::getVxTransport() {
    return m_localConfigurator;
}

std::string ConfigureVxAgentProxy::getLogPathname() const {
    return m_localConfigurator.getLogPathname();
}

SV_LOG_LEVEL ConfigureVxAgentProxy::getLogLevel() const {
    return m_localConfigurator.getLogLevel();
}

void ConfigureVxAgentProxy::setLogLevel(SV_LOG_LEVEL logLevel) const {
    m_localConfigurator.setLogLevel(logLevel);
}

int ConfigureVxAgentProxy::getDelayBetweenAppShutdownAndTagIssue() const {
    return m_localConfigurator.getDelayBetweenAppShutdownAndTagIssue();
}

int ConfigureVxAgentProxy::getMaxWaitTimeForTagArrival() const {
    return m_localConfigurator.getMaxWaitTimeForTagArrival();
}

//Added by Ranjan bug#10404 (XenServer Registration)
int ConfigureVxAgentProxy::getMaxWaitTimeForXenRegistration() const {
    return m_localConfigurator.getMaxWaitTimeForXenRegistration();
}

int ConfigureVxAgentProxy::getMaxWaitTimeForLvActivation() const {
    return m_localConfigurator.getMaxWaitTimeForLvActivation();
}

int ConfigureVxAgentProxy::getMaxWaitTimeForDisplayVmVdiInfo() const {
    return m_localConfigurator.getMaxWaitTimeForDisplayVmVdiInfo();
}

int ConfigureVxAgentProxy::getMaxWaitTimeForCxStatusUpdate() const {
    return m_localConfigurator.getMaxWaitTimeForCxStatusUpdate();
}
//end of change

void ConfigureVxAgentProxy::setDelayBetweenAppShutdownAndTagIssue( int delayBetweenAppShutdownAndTagIssue ) const {	
    m_localConfigurator.setDelayBetweenAppShutdownAndTagIssue( delayBetweenAppShutdownAndTagIssue );
}
void ConfigureVxAgentProxy::setMaxWaitTimeForTagArrival( int noOfSecondsToWaitForTagArrival ) const {
    m_localConfigurator.setMaxWaitTimeForTagArrival( noOfSecondsToWaitForTagArrival );
}

std::string ConfigureVxAgentProxy::getDiffSourceExePathname() const {
    return m_localConfigurator.getDiffSourceExePathname();
}

std::string ConfigureVxAgentProxy::getDataProtectionExePathname() const {
    return m_localConfigurator.getDataProtectionExePathname();
}


std::string ConfigureVxAgentProxy::getDataProtectionExeV2Pathname() const
{
    return m_localConfigurator.getDataProtectionExeV2Pathname() ;
}

std::string ConfigureVxAgentProxy::getOffloadSyncPathname() const {
    return m_localConfigurator.getOffloadSyncPathname();
}

std::string ConfigureVxAgentProxy::getOffloadSyncSourceDirectory() const{
    return m_localConfigurator.getOffloadSyncSourceDirectory();
}

std::string ConfigureVxAgentProxy::getOffloadSyncCacheDirectory() const{
    return m_localConfigurator.getOffloadSyncCacheDirectory();
}

std::string ConfigureVxAgentProxy::getOffloadSyncFilenamePrefix() const{
    return m_localConfigurator.getOffloadSyncFilenamePrefix();
}

std::string ConfigureVxAgentProxy::getDiffTargetExePathname() const  {
    return m_localConfigurator.getDiffTargetExePathname();
}

std::string ConfigureVxAgentProxy::getDiffTargetSourceDirectoryPrefix() const  {
    return m_localConfigurator.getDiffTargetSourceDirectoryPrefix();
}

std::string ConfigureVxAgentProxy::getDiffTargetCacheDirectoryPrefix() const  {
    return m_localConfigurator.getDiffTargetCacheDirectoryPrefix();
}

std::string ConfigureVxAgentProxy::getDiffTargetFilenamePrefix() const  {
    return m_localConfigurator.getDiffTargetFilenamePrefix();
}

std::string ConfigureVxAgentProxy::getFastSyncExePathname() const  {
    return m_localConfigurator.getFastSyncExePathname();
}

int ConfigureVxAgentProxy::getFastSyncBlockSize() const {
    return m_localConfigurator.getFastSyncBlockSize();
}

int ConfigureVxAgentProxy::getFastSyncMaxChunkSize() const {
    return m_localConfigurator.getFastSyncMaxChunkSize();
}


int ConfigureVxAgentProxy::getFastSyncMaxChunkSizeForE2A() const {
    return m_localConfigurator.getFastSyncMaxChunkSizeForE2A();
}



bool ConfigureVxAgentProxy::getUseConfiguredHostname() const {
    return m_localConfigurator.getUseConfiguredHostname();
}

void ConfigureVxAgentProxy::setUseConfiguredHostname(bool flag) const {
    return m_localConfigurator.setUseConfiguredHostname(flag);
}

bool ConfigureVxAgentProxy::getUseConfiguredIpAddress() const {
    return m_localConfigurator.getUseConfiguredIpAddress();
}

void ConfigureVxAgentProxy::setUseConfiguredIpAddress(bool flag) const {
    return m_localConfigurator.setUseConfiguredIpAddress(flag);
}

std::string ConfigureVxAgentProxy::getConfiguredHostname() const {
    return m_localConfigurator.getConfiguredHostname();
}

void ConfigureVxAgentProxy::setConfiguredHostname(const std::string &hostName) const {
    m_localConfigurator.setConfiguredHostname(hostName);
}

std::string ConfigureVxAgentProxy::getConfiguredIpAddress() const {
    return m_localConfigurator.getConfiguredIpAddress();
}

bool ConfigureVxAgentProxy::isMobilityAgent() const {
    return m_localConfigurator.isMobilityAgent();
}

bool ConfigureVxAgentProxy::isMasterTarget() const {
    return m_localConfigurator.isMasterTarget();
}

void ConfigureVxAgentProxy::setConfiguredIpAddress(const std::string &ipAddress) const {
    return m_localConfigurator.setConfiguredIpAddress(ipAddress);
}

std::string ConfigureVxAgentProxy::getExternalIpAddress() const {
    return m_localConfigurator.getExternalIpAddress();
}

int ConfigureVxAgentProxy::getFastSyncHashCompareDataSize() const {
    return m_localConfigurator.getFastSyncHashCompareDataSize();
}

std::string ConfigureVxAgentProxy::getResyncSourceDirectoryPath() const {
    return m_localConfigurator.getResyncSourceDirectoryPath();
}

unsigned int ConfigureVxAgentProxy::getMaxFastSyncApplyThreads() const {
    return m_localConfigurator.getMaxFastSyncApplyThreads();
}

int ConfigureVxAgentProxy::getSyncBytesToApplyThreshold( std::string const& vol ) const {
    return m_localConfigurator.getSyncBytesToApplyThreshold( vol );
}

bool ConfigureVxAgentProxy::getChunkMode() const {
    return m_localConfigurator.getChunkMode();
}

bool ConfigureVxAgentProxy::getHostType() const {
    return m_localConfigurator.getHostType();
}

int ConfigureVxAgentProxy::getMaxOutpostThreads() const {
    return m_localConfigurator.getMaxOutpostThreads();
}

int ConfigureVxAgentProxy::getVolumeChunkSize() const {
    return m_localConfigurator.getVolumeChunkSize();
}

bool ConfigureVxAgentProxy::getRegisterSystemDrive() const {
    return m_localConfigurator.getRegisterSystemDrive();
}

int ConfigureVxAgentProxy::getAgentHealthCheckInterval() const {
    return m_localConfigurator.getAgentHealthCheckInterval();
}

std::string ConfigureVxAgentProxy::getHealthCollatorDirPath() const {
    return m_localConfigurator.getHealthCollatorDirPath();
}
int ConfigureVxAgentProxy::getMarsHealthCheckInterval() const {
    return m_localConfigurator.getMarsHealthCheckInterval();
}

int ConfigureVxAgentProxy::getMarsServerUnavailableCheckInterval() const {
    return m_localConfigurator.getMarsServerUnavailableCheckInterval();
}

int ConfigureVxAgentProxy::getRegisterHostInterval() const {
    return m_localConfigurator.getRegisterHostInterval();
}

int ConfigureVxAgentProxy::getTransportErrorLogInterval() const {
    return m_localConfigurator.getTransportErrorLogInterval();
}

int ConfigureVxAgentProxy::getDiskReadErrorLogInterval() const {
    return m_localConfigurator.getDiskReadErrorLogInterval();
}

int ConfigureVxAgentProxy::getDiskNotFoundErrorLogInterval() const {
    return m_localConfigurator.getDiskNotFoundErrorLogInterval();
}

int ConfigureVxAgentProxy::getMonitorHostStartDelay() const {
    return m_localConfigurator.getMonitorHostStartDelay();
}

int ConfigureVxAgentProxy::getMonitorHostInterval() const {
    return m_localConfigurator.getMonitorHostInterval();
}

std::string ConfigureVxAgentProxy::getMonitorHostCmdList() const {
    return m_localConfigurator.getMonitorHostCmdList();
}

void ConfigureVxAgentProxy::setCacheDirectory(std::string const& value) const {
    return m_localConfigurator.setCacheDirectory(value);
}

void ConfigureVxAgentProxy::setDiffTargetCacheDirectoryPrefix(std::string const& value) const {
    return m_localConfigurator.setDiffTargetCacheDirectoryPrefix(value);
}


void ConfigureVxAgentProxy::setHttp(HTTP_CONNECTION_SETTINGS s) const {
    return m_localConfigurator.setHttp(s);
}

void ConfigureVxAgentProxy::setHostName(std::string const& value) const {
    return m_localConfigurator.setHostName(value);
}

void ConfigureVxAgentProxy::setPort(int port) const {
    return m_localConfigurator.setPort(port);
}

void ConfigureVxAgentProxy::setMaxOutpostThreads(int n) const {
    return m_localConfigurator.setMaxOutpostThreads(n);
}

void ConfigureVxAgentProxy::setVolumeChunkSize(int n) const {
    return m_localConfigurator.setVolumeChunkSize(n);
}

void ConfigureVxAgentProxy::setRegisterSystemDrive(bool flag) const {
    return m_localConfigurator.setRegisterSystemDrive(flag);
}

SV_ULONG ConfigureVxAgentProxy::getResyncStaleFilesCleanupInterval() const
{
    return m_localConfigurator.getResyncStaleFilesCleanupInterval();
}

bool ConfigureVxAgentProxy::ShouldCleanupCorruptSyncFile() const
{
    return m_localConfigurator.ShouldCleanupCorruptSyncFile();
}

SV_ULONG ConfigureVxAgentProxy::getResyncUpdateInterval() const
{
    return m_localConfigurator.getResyncUpdateInterval() ;
}

SV_ULONG ConfigureVxAgentProxy::getIRMetricsReportInterval() const
{
    return m_localConfigurator.getIRMetricsReportInterval();
}

SV_ULONG ConfigureVxAgentProxy::getLogResyncProgressInterval() const
{
    return m_localConfigurator.getLogResyncProgressInterval();
}

SV_ULONG ConfigureVxAgentProxy::getResyncSlowProgressThreshold() const
{
    return m_localConfigurator.getResyncSlowProgressThreshold();
}

SV_ULONG ConfigureVxAgentProxy::getResyncNoProgressThreshold() const
{
    return m_localConfigurator.getResyncNoProgressThreshold();
}

bool ConfigureVxAgentProxy::updateAgentLog(std::string const& timestamp,std::string const& loglevel,
                                           std::string const& agentInfo,
                                           std::string const& errorString ) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_AGENT_LOG, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, timestamp,loglevel,agentInfo,errorString );
    m_transport( sr );
    return sr.UnSerialize<bool>();
}


// Bug #6298
bool ConfigureVxAgentProxy::updateReplicationStateStatus(const std::string& deviceName, VOLUME_SETTINGS::PAIR_STATE state) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_REPLICATION_STATE_STATUS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName, state );
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::updateCleanUpActionStatus(const std::string& deviceName, const std::string & cleanupstr) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_REPLICATION_CLEANUP_STATUS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName, cleanupstr );
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::updateRestartResyncCleanupStatus(const std::string& deviceName, bool& success, const std::string& error_message) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_RESTART_RESYNC_CLEANUP_STATUS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName, success, error_message );
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::setPauseReplicationStatus(const std::string& deviceName, int hosttype, const std::string & respstr) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SET_PAUSE_REPLICATION_STATUS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName, hosttype, respstr);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

void ConfigureVxAgentProxy::renameClusterGroup( std::string const& clusterName,
                                               std::string const& oldGroup, std::string const& newGroup ) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(RENAME_CLUSTER_GROUP, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, clusterName, oldGroup, newGroup );
    m_transport( sr );
}

void ConfigureVxAgentProxy::deleteVolumesFromCluster( string const& groupName,
                                                     vector<string> const& volumes ) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(DELETE_VOLUMES_FROM_CLUSTER, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, groupName, volumes );
    m_transport( sr );
}

void ConfigureVxAgentProxy::deleteClusterNode(string const& clusterName) const {
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(DELETE_CLUSTER_NODE, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, clusterName);
    m_transport( sr );
}

void ConfigureVxAgentProxy::addVolumesToCluster( std::string const& groupName,
                                                std::vector<std::string> const& volumes,
                                                std::string const& groupGuid ) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(ADD_VOLUMES_TO_CLUSTER, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, groupName, volumes, groupGuid );
    m_transport( sr );
}

JOB_ID_OFFSET ConfigureVxAgentProxy::getVolumeCheckpoint(  std::string const & drivename ) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_VOLUME_CHECKPOINT, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, drivename);
    m_transport( sr );
    return sr.UnSerialize<JOB_ID_OFFSET>();
}

int ConfigureVxAgentProxy::setResyncProgress( std::string deviceName, long long offset, long long bytes, bool matched, std::string jobId, std::string const oldName, std::string newName, std::string deleteName1, std::string deleteName2, std::string agent ) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SET_RESYNC_PROGRESS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName, offset, bytes, matched, jobId,
                 oldName, newName, deleteName1, deleteName2, agent);
    m_transport( sr );
    return sr.UnSerialize<int>();
}

/* Added by BSR Fastsync TBC*/
int ConfigureVxAgentProxy::setResyncProgressFastsync( std::string const &sourceHostId,
                                                     std::string const &sourceDeviceName, 
                                                     std::string const &destHostId,
                                                     std::string const &destDeviceName,
                                                     long long offset, 
                                                     long long bytes, 
                                                     bool matched, 
                                                     std::string jobId, 
                                                     std::string const oldName, 
                                                     std::string newName, 
                                                     std::string deleteName1, 
                                                     std::string deleteName2 ) const
{
    /* TODO: some better way of checking xmlize vs serialize has
     *       to be found out since whereever m_dpc is being used,
     *       check is happening two times */
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SET_RESYNC_PROGRESS_FASTSYNC, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceHostId, sourceDeviceName, destHostId, destDeviceName,
                 offset, bytes, matched, jobId, oldName, newName, deleteName1, deleteName2);
    m_transport( sr );
    return sr.UnSerialize<int>();
}
/*
*  Added Following functions to fix overshoot issue using bitmap
*  By Suresh
*/
int ConfigureVxAgentProxy::SetFastSyncUpdateProgressBytes(std::string const &sourceHostId,
                                                                std::string const &sourceDeviceName,
                                                                std::string const &destHostId,
                                                                std::string const &destDeviceName,
                                                                SV_ULONGLONG bytesApplied,
                                                                SV_ULONGLONG bytesMatched, 
                                                                std::string jobId) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SET_RESYNC_UPDATE_PROGRESS_BYTES_FASTSYNC, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceHostId, sourceDeviceName, destHostId,
                 destDeviceName, bytesApplied, bytesMatched, jobId); 
    m_transport( sr );
    return sr.UnSerialize<int>();
}

int ConfigureVxAgentProxy::SetFastSyncUpdateProgressBytesWithStats(std::string const &sourceHostId,
    std::string const &sourceDeviceName,
    std::string const &destHostId,
    std::string const &destDeviceName,
    const SV_ULONGLONG bytesApplied,
    const SV_ULONGLONG bytesMatched,
    const std::string &jobId,
    const std::string &stats) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SET_RESYNC_UPDATE_PROGRESS_STATS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceHostId, sourceDeviceName, destHostId,
        destDeviceName, bytesApplied, bytesMatched, jobId, stats);
    m_transport(sr);
    return sr.UnSerialize<int>();
}

int ConfigureVxAgentProxy::SetFastSyncUpdateFullSyncBytes(std::string const &sourceHostId,
                                                                std::string const &sourceDeviceName,
                                                                std::string const &destHostId,
                                                                std::string const &destDeviceName,
                                                                SV_ULONGLONG fullSyncBytesSent,
                                                                std::string jobId) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SET_RESYNC_UPDATE_FULLSYNC_BYTES_FASTSYNC, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceHostId, sourceDeviceName, destHostId,
                 destDeviceName, fullSyncBytesSent, jobId);
    m_transport( sr );
    return sr.UnSerialize<int>();
}

int ConfigureVxAgentProxy::SetFastSyncUpdateMatchedBytes( std::string const &sourceHostId,
                                                                std::string const &sourceDeviceName,
                                                                std::string const &destHostId,
                                                                std::string const &destDeviceName,
                                                                SV_ULONGLONG bytesMatched,
                                                                int during_resyncTransition,
                                                                std::string jobId ) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SET_RESYNC_UPDATE_MATCHED_BYTES_FASTSYNC, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceHostId, sourceDeviceName, destHostId,
                 destDeviceName, bytesMatched, during_resyncTransition, jobId); 
    m_transport( sr );
    return sr.UnSerialize<int>();
}


int ConfigureVxAgentProxy::SetFastSyncFullyUnusedBytes( std::string const &sourceHostId,
                                                   std::string const &sourceDeviceName,
                                                   std::string const &destHostId,
                                                   std::string const &destDeviceName,
                                                   SV_ULONGLONG bytesunused,
                                                   std::string jobId ) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SET_RESYNC_UPDATE_FULLY_UNUSED_BYTES_FASTSYNC, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceHostId, sourceDeviceName, destHostId,
                 destDeviceName, bytesunused, jobId); 
    m_transport( sr );
    return sr.UnSerialize<int>();
}



// Changes End
JOB_ID_OFFSET  ConfigureVxAgentProxy::getVolumeCheckpointFastsync( std::string const &sourceHostId,
                                                                  std::string const &sourceDeviceName, 
                                                                  std::string const &destHostId,
                                                                  std::string const &destDeviceName ) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_VOLUME_CHECKPOINT_FASTSYNC, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceHostId, sourceDeviceName, destHostId,
                 destDeviceName); 
    m_transport( sr );
    return sr.UnSerialize<JOB_ID_OFFSET>();
}

int ConfigureVxAgentProxy::setResyncTransitionStepOneToTwo( std::string const &sourceHostId,
                                                           std::string const & sourceDeviceName,
                                                           std::string const &destHostId, 
                                                           std::string const &destDeviceName,
                                                           const std::string& jobId,
                                                           std::string const &syncNoMoreFile  ) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SET_RESYNC_TRANSITION, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceHostId, sourceDeviceName, destHostId,
                 destDeviceName, jobId, syncNoMoreFile); 
    m_transport( sr );
    return sr.UnSerialize<int>();
}

ResyncTimeSettings ConfigureVxAgentProxy::getResyncStartTimeStampFastsync( const std::string & sourceHostId, 
                                                                          const std::string& sourceDeviceName,
                                                                          const std::string& destHostId,
                                                                          const std::string& destDeviceName,
                                                                          const std::string & jobId ) 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;
    const char *op = "resyncStartTagtime";

    setApiAndFirstArg(GET_RESYNC_START_TIMESTAMP_FASTSYNC, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceHostId, sourceDeviceName, destHostId,
                 destDeviceName, jobId, op); 
    m_transport( sr );
    return sr.UnSerialize<ResyncTimeSettings>();
}

int ConfigureVxAgentProxy::sendResyncStartTimeStampFastsync( const std::string & sourceHostId, 
                                                            const std::string& sourceDeviceName,
                                                            const std::string& destHostId,
                                                            const std::string& destDeviceName,
                                                            const std::string & jobId, 
                                                            const SV_ULONGLONG & ts, 
                                                            const SV_ULONG & seq ) 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SEND_RESYNC_START_TIMESTAMP_FASTSYNC, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceHostId, sourceDeviceName, destHostId,
                 destDeviceName, jobId, ts, seq);
    m_transport( sr );
    return sr.UnSerialize<int>();
}	

ResyncTimeSettings ConfigureVxAgentProxy::getResyncEndTimeStampFastsync( const std::string & sourceHostId, 
                                                                        const std::string& sourceDeviceName,
                                                                        const std::string& destHostId,
                                                                        const std::string& destDeviceName,
                                                                        const std::string & jobId )  
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;
    const char *op = "resyncEndTagtime";

    setApiAndFirstArg(GET_RESYNC_END_TIMESTAMP_FASTSYNC, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceHostId, sourceDeviceName, destHostId,
                 destDeviceName, jobId, op);
    m_transport( sr );
    return sr.UnSerialize<ResyncTimeSettings>();
}

int ConfigureVxAgentProxy::sendResyncEndTimeStampFastsync( const std::string & sourceHostId, 
                                                          const std::string& sourceDeviceName,
                                                          const std::string& destHostId,
                                                          const std::string& destDeviceName,
                                                          const std::string & jobId, 
                                                          const SV_ULONGLONG & ts, 
                                                          const SV_ULONG & seq ) 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SEND_RESYNC_END_TIMESTAMP_FASTSYNC, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceHostId, sourceDeviceName, destHostId,
                 destDeviceName, jobId, ts, seq);
    m_transport( sr );
    return sr.UnSerialize<int>();
}

/* End of the change*/


int ConfigureVxAgentProxy::setLastResyncOffsetForDirectSync(const std::string & sourceHostId,
                                                            const std::string& sourceDeviceName,
                                                            const std::string& destHostId,
                                                            const std::string& destDeviceName,
                                                            const std::string & jobId,
                                                            const SV_ULONGLONG &offset,
                                                            const SV_ULONGLONG &filesystemunusedbytes)
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SET_LAST_RESYNC_OFFSET_DIRECTSYNC, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceHostId, sourceDeviceName, destHostId,
                 destDeviceName, jobId, offset, filesystemunusedbytes);
    m_transport( sr );
    return sr.UnSerialize<int>();
}


std::string ConfigureVxAgentProxy::getInitialSyncDirectories() const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_INITIAL_SYNC_DIRECTORIES, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi);
    m_transport( sr );
    return sr.UnSerialize<std::string>();
}

std::string ConfigureVxAgentProxy::getTargetReplicationJobId( std::string deviceName ) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_TARGET_REPLICATION_JOB_ID, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName);
    m_transport( sr );
    return sr.UnSerialize<std::string>();
}

bool ConfigureVxAgentProxy::updateVolumeAttribute(NOTIFY_TYPE notifyType, const std::string &deviceName,VOLUME_STATE volumeState, const std::string &mountPoint) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_VOLUME_ATTRIBUTE, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, notifyType,volumeState,deviceName,mountPoint);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

int ConfigureVxAgentProxy::getCurrentVolumeAttribute(std::string deviceName) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_CURRENT_VOLUME_ATTRIBUTE, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName);
    m_transport( sr );
    return sr.UnSerialize<int>();
}

bool ConfigureVxAgentProxy::updateLogAvailable(string deviceName,string dateFrom, string dateTo, unsigned long long spaceOccupied, unsigned long long freeSpaceLeft,unsigned long long dateFromUtc, unsigned long long dateToUtc, string diffs) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_LOG_AVAILABLE, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName,dateFrom,dateTo,spaceOccupied,freeSpaceLeft, dateFromUtc,
                 dateToUtc, diffs);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::updateCDPInformationV2(const HostCDPInformation & cdpmap) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_CDP_INFORMATIONV2, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, cdpmap);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::updateCdpDiskUsage(const HostCDPRetentionDiskUsage& cdpRetentionDiskUsage,const HostCDPTargetDiskUsage& cdpTargetDiskUsage) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_CDP_DISKUSAGE, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, cdpRetentionDiskUsage,cdpTargetDiskUsage);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::updateRetentionTag(string deviceName,string tagTimeStamp, string appName, string userTag, string actionTag, unsigned short accuracy,std::string identifier,unsigned short verifier,std::string comment) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_RETENTION_TAG, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName, tagTimeStamp, appName, userTag, actionTag, accuracy,
                 identifier,verifier,comment );
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

HostRetentionWindow ConfigureVxAgentProxy::getHostRetentionWindow() const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_HOST_RETENTION_WINDOW, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi); 
    m_transport( sr );
    return sr.UnSerialize<HostRetentionWindow>();
}

bool ConfigureVxAgentProxy::updateRetentionInfo(const HostRetentionInformation& hRetInfo) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_RETENTION_INFO, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, hRetInfo);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::updatePendingDataInfo(const std::map<std::string,SV_ULONGLONG>& pendingDataInfo) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_PENDINGDATA_ONTARGET, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, pendingDataInfo);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::shouldThrottleResync(std::string const& deviceName, const std::string &endpointdeviceName, 
                                                 const int &grpid) const 
{
    AutoGuard lock( m_lock );
  
      
    bool bthrottleresync = false;
    bool breakout = false;
    HOST_VOLUME_GROUP_SETTINGS::volumeGroups_iterator VolumeGroupSettingsIter;

    for ( VolumeGroupSettingsIter = m_settings.hostVolumeSettings.volumeGroups.begin(); 
          (VolumeGroupSettingsIter != m_settings.hostVolumeSettings.volumeGroups.end() && !breakout) ; 
          VolumeGroupSettingsIter++ )
    {
        for(VOLUME_GROUP_SETTINGS::volumes_iterator volumes = VolumeGroupSettingsIter->volumes.begin();volumes != VolumeGroupSettingsIter->volumes.end();
            volumes++)
        {
            if (deviceName == volumes->second.deviceName)
            {
                VOLUME_SETTINGS::endpoints_iterator endPointsIter =
                    volumes->second.endpoints.begin();
                if ( (endPointsIter->deviceName == endpointdeviceName) &&
                     (grpid == VolumeGroupSettingsIter->id) )
                {
                    bthrottleresync = volumes->second.throttleSettings.IsResyncThrottled();
                    breakout = true;
                    break;
                }
            } 
        }
    }

    return bthrottleresync;
}

bool ConfigureVxAgentProxy::shouldThrottleSource(std::string const& deviceName) const 
{
    AutoGuard lock( m_lock );
    bool bthrottlesource = false;
    bool breakout = false;
    HOST_VOLUME_GROUP_SETTINGS::volumeGroups_iterator VolumeGroupSettingsIter;

    for ( VolumeGroupSettingsIter = m_settings.hostVolumeSettings.volumeGroups.begin(); 
          (VolumeGroupSettingsIter != m_settings.hostVolumeSettings.volumeGroups.end() && !breakout) ; 
          VolumeGroupSettingsIter++ )
    {
        for(VOLUME_GROUP_SETTINGS::volumes_iterator volumes = VolumeGroupSettingsIter->volumes.begin();volumes != VolumeGroupSettingsIter->volumes.end();
            volumes++)
        {
            if (deviceName == volumes->second.deviceName)
            {
                bthrottlesource = volumes->second.throttleSettings.IsSourceThrottled();
                breakout = true;
                break;
            } 
        }
    }

    return bthrottlesource;
}

bool ConfigureVxAgentProxy::shouldThrottleTarget(std::string const& deviceName) const 
{
    AutoGuard lock( m_lock );
    return m_settings.shouldThrottleTarget(deviceName);
}

bool ConfigureVxAgentProxy::setTargetResyncRequired(const std::string &deviceName, const std::string &errStr,
    const ResyncReasonStamp &resyncReasonStamp, const long errorcode) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SET_TARGET_RESYNC_REQUIRED, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName, errStr, errorcode, resyncReasonStamp);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::setSourceResyncRequired(const std::string &deviceName, const std::string &errStr,
    const ResyncReasonStamp &resyncReasonStamp) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SET_SOURCE_RESYNC_REQUIRED, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName, errStr, resyncReasonStamp);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::setPrismResyncRequired( std::string sourceLunID,std::string errStr ) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SET_PRISM_RESYNC_REQUIRED, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceLunID,errStr);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::setXsPoolSourceResyncRequired( std::string deviceName,std::string errStr ) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SET_XS_POOL_SOURCE_RESYNC_REQUIRED, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName,errStr);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::pauseReplicationFromHost( std::string deviceName,std::string errStr ) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(PAUSE_REPLICATION_FROM_HOST, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName,errStr);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::resumeReplicationFromHost( std::string deviceName,std::string errStr ) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(RESUME_REPLICATION_FROM_HOST, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName,errStr);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::canClearDifferentials( std::string deviceName ) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_CLEAR_DIFFS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceName);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}
int ConfigureVxAgentProxy::getRemoteLogLevel() const {
    return m_localConfigurator.getRemoteLogLevel();
}

void ConfigureVxAgentProxy::setRemoteLogLevel(int remoteLogLevel) const {
    m_localConfigurator.setRemoteLogLevel (remoteLogLevel);
}

std::string ConfigureVxAgentProxy::getTimeStampsOnlyTag() const
{
    return m_localConfigurator.getTimeStampsOnlyTag();
}

std::string ConfigureVxAgentProxy::getDestDir() const
{
    return m_localConfigurator.getDestDir();
}

std::string ConfigureVxAgentProxy::getDatExtension() const
{
    return m_localConfigurator.getDatExtension();
}

std::string ConfigureVxAgentProxy::getMetaDataContinuationTag() const
{
    return m_localConfigurator.getMetaDataContinuationTag();
}

std::string ConfigureVxAgentProxy::getMetaDataContinuationEndTag() const
{
    return m_localConfigurator.getMetaDataContinuationEndTag();
}

std::string ConfigureVxAgentProxy::getWriteOrderContinuationTag() const
{
    return m_localConfigurator.getWriteOrderContinuationTag();
}

std::string ConfigureVxAgentProxy::getWriteOrderContinuationEndTag() const
{
    return m_localConfigurator.getWriteOrderContinuationEndTag();
}

std::string ConfigureVxAgentProxy::getPreRemoteName() const
{
    return m_localConfigurator.getPreRemoteName();
}

std::string ConfigureVxAgentProxy::getFinalRemoteName() const
{
    return m_localConfigurator.getFinalRemoteName();
}

int ConfigureVxAgentProxy::getThrottleWaitTime()const
{
    return  m_localConfigurator.getThrottleWaitTime();
}

int ConfigureVxAgentProxy::getSentinelExitTime()const
{
    return m_localConfigurator.getSentinelExitTime();
}

int ConfigureVxAgentProxy::getS2DataWaitTime()const
{
    return  m_localConfigurator.getThrottleWaitTime();
}

int ConfigureVxAgentProxy::getWaitForDBNotify() const
{
    return m_localConfigurator.getWaitForDBNotify();
}

// Snapshot Request APIs start from here ... 
/*
SNAPSHOT_REQUESTS ConfigureVxAgentProxy::getSnapshotRequestFromCx() const {
AutoGuard lock( m_lock );
return m_snapShotRequests;
}
*/
bool ConfigureVxAgentProxy::notifyCxOnSnapshotStatus(const string &snapId, 
                                                     int timeval,const SV_ULONGLONG &VsnapId, 
                                                     const string &errMessage, int status) const 

{
    Serializer sr(m_serializeType);
    const char *api = UPDATE_SNAPSHOT_STATUS;
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_SNAPSHOT_STATUS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, snapId, timeval, VsnapId, errMessage, status);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}


bool ConfigureVxAgentProxy::notifyCxOnSnapshotProgress(const string &snapId, int percentage, int rpoint) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_SNAPSHOT_PROGRESS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, snapId, percentage, rpoint);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}
std::vector<bool> ConfigureVxAgentProxy::notifyCxOnSnapshotCreation(std::vector<VsnapPersistInfo> const & vsnapInfo) const
{
    //Bug #8807
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;
    
    setApiAndFirstArg(UPDATE_SNAPSHOT_CREATION, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, vsnapInfo);
    m_transport( sr );
    return sr.UnSerialize< std::vector<bool> >();
}
    
std::vector<bool> ConfigureVxAgentProxy::notifyCxOnSnapshotDeletion(std::vector<VsnapDeleteInfo> const & vsnapInfo) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_SNAPSHOT_DELETION, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, vsnapInfo);
    m_transport( sr );
    return sr.UnSerialize< std::vector<bool> >();
}

bool ConfigureVxAgentProxy::isSnapshotAborted(const std::string  & snapshotId) const
{
    AutoGuard lock( m_lock );

    if(m_snapShotRequests.find(snapshotId) == m_snapShotRequests.end())
        return true;
    return false;
}

int ConfigureVxAgentProxy::makeSnapshotActive(const std::string & snapshotId) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(MAKE_ACTIVE_SNAPSHOT_INSTANCE, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, snapshotId);
    m_transport( sr );
    return sr.UnSerialize<int>();
}
/*
SNAPSHOT_REQUESTS ConfigureVxAgentProxy::getSnapshotRequests(const std::string & vol, const svector_t & bookmarks) const
{
// For time being, we will fetch the latest data from cx.
getSnapshotRequestFromCx();

AutoGuard lock( m_lock );
SNAPSHOT_REQUESTS requests;

SNAPSHOT_REQUESTS::iterator iter_end = m_snapShotRequests.end();
for(SNAPSHOT_REQUESTS::iterator iter= m_snapShotRequests.begin(); iter != iter_end; ++iter)
{
std::string id = (*iter).first;
SNAPSHOT_REQUEST::Ptr ptr = (*iter).second;
FormatVolumeName(ptr -> src);
FormatVolumeName(ptr -> dest);
if(! strcmp(ptr -> src.c_str(),vol.c_str()) )	{
svector_t::iterator prefix_iter = ptr ->bookmarks.begin();
svector_t::iterator prefix_end = ptr ->bookmarks.end();

bool found = false;

if(bookmarks.empty()) {
// return only time based requests
if( ptr -> bookmarks.empty() )
found = true;
} else	{

for( ;  prefix_iter != prefix_end ; ++prefix_iter) {
string prefix = *prefix_iter;

svector_t::const_iterator bookmarks_iter = bookmarks.begin();
svector_t::const_iterator bookmarks_end = bookmarks.end();
for ( ; bookmarks_iter != bookmarks_end; ++bookmarks_iter) {
string bookmark = *bookmarks_iter;
if( (0 == strncmp(bookmark.c_str(), prefix.c_str(), prefix.length()))) {
found = true;
break;
}
}

if (found)
break;
}
}

if(found) {
SNAPSHOT_REQUEST::Ptr request(ptr);
if(ptr -> operation == SNAPSHOT_REQUEST::ROLLBACK)	{
requests.clear();
requests.insert(requests.begin(), SNAPSHOT_REQUEST_PAIR(id, request));
} else {
requests.insert(requests.end(), SNAPSHOT_REQUEST_PAIR(id, request));
}
}
}
}

return requests;
}
*/
// Snapshot Request APIs end here ... 

std::string ConfigureVxAgentProxy::getProtectedVolumes() const
{
    return m_localConfigurator.getProtectedVolumes();
}

void ConfigureVxAgentProxy::setProtectedVolumes(std::string protectedVolumes) const
{
    return m_localConfigurator.setProtectedVolumes(protectedVolumes);
}


ResyncTimeSettings ConfigureVxAgentProxy::getResyncStartTimeStamp(const std::string & volname, const std::string & jobId, const string &hostType)
{
    Serializer sr(m_serializeType);
    const char *op = "resyncStartTagtime";
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_RESYNC_START_TIMESTAMP, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi,  volname , jobId, op, hostType);
    m_transport( sr );
    return sr.UnSerialize<ResyncTimeSettings>();
}
int ConfigureVxAgentProxy::sendResyncStartTimeStamp(const std::string & volname, const std::string & jobId, const SV_ULONGLONG & ts, const SV_ULONG & seq, const string &hostType)
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SEND_RESYNC_START_TIMESTAMP, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, volname, jobId, ts, seq, hostType);
    m_transport( sr );
    return sr.UnSerialize<int>();
}

/* zero for success as of other update calls like sending resync start time stamps ... */
int ConfigureVxAgentProxy::updateMirrorState(const std::string &sourceLunID, 
                                             const PRISM_VOLUME_INFO::MIRROR_STATE mirrorStateRequested,
                                             const PRISM_VOLUME_INFO::MIRROR_ERROR errCode,
                                             const std::string &errorString)
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_MIRROR_STATE, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceLunID, mirrorStateRequested, errCode, errorString);
    m_transport( sr );
    return sr.UnSerialize<int>();
}

/* TODO: should there be volume map's device name here ? */
int ConfigureVxAgentProxy::sendLastIOTimeStampOnATLUN(const std::string &sourceVolumeName,
                                                      const SV_ULONGLONG ts)
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_LASTIO_TIMESTAMP_ON_ATLUN, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, sourceVolumeName, ts);
    m_transport( sr );
    return sr.UnSerialize<int>();
}

int ConfigureVxAgentProxy::updateVolumesPendingChanges(const VolumesStats_t &vss, const std::list<std::string>& statsNotAvailableVolumes)
{
    Serializer sr(m_serializeType);
    const char *api = UPDATE_VOLUMES_PENDING_CHANGES;
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_VOLUMES_PENDING_CHANGES, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, vss, statsNotAvailableVolumes);
    m_transport( sr );
    return sr.UnSerialize<int>();
}

ResyncTimeSettings ConfigureVxAgentProxy::getResyncEndTimeStamp(const std::string & volname, const std::string & jobId, const string &hostType)
{
    Serializer sr(m_serializeType);
    const char *op = "resyncEndTagtime";
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_RESYNC_END_TIMESTAMP, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, volname, jobId, op, hostType);
    m_transport( sr );
    return sr.UnSerialize<ResyncTimeSettings>();
}
int ConfigureVxAgentProxy::sendResyncEndTimeStamp(const std::string & volname, const std::string & jobId, const SV_ULONGLONG & ts, const SV_ULONG & seq, const string &hostType)
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SEND_RESYNC_END_TIMESTAMP, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, volname, jobId, ts, seq, hostType);
    m_transport( sr );
    return sr.UnSerialize<int>();
}

int ConfigureVxAgentProxy::sendEndQuasiStateRequest(const std::string & volname) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SEND_END_QUASI_STATE_REQUEST, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, volname);
    m_transport( sr );
    return sr.UnSerialize<int>();
}

void ConfigureVxAgentProxy::sendDebugMsgToLocalHostLog(SV_LOG_LEVEL LogLevel, const std::string& szDebugString) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SEND_DEBUG_MSG_LOCALHOST_LOG, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, LogLevel, szDebugString);
    m_transport( sr );
}

void ConfigureVxAgentProxy::ReportAgentHealthStatus(const SourceAgentProtectionPairHealthIssues&healthIssues)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    try
    {
        const char *apitocall;
        const char *firstargtoapi;

        setApiAndFirstArg(SOURCE_AGENT_PROTECTION_PAIR_HEALTH_ISSUES, m_dpc.c_str(), &apitocall, &firstargtoapi);
        SourceAgentProtectionPairHealthIssues   issues = healthIssues;
        std::string jsonSerializedAgentHealthIssues;
        std::stringstream ssErrMsg;
        try {
            jsonSerializedAgentHealthIssues = JSON::producer<SourceAgentProtectionPairHealthIssues>::convert(issues);
            //remove carriage return \r
            jsonSerializedAgentHealthIssues.erase(
                std::remove(jsonSerializedAgentHealthIssues.begin(), jsonSerializedAgentHealthIssues.end(), '\r'),
                jsonSerializedAgentHealthIssues.end());

            //remove line feed \n
            jsonSerializedAgentHealthIssues.erase(
                std::remove(jsonSerializedAgentHealthIssues.begin(), jsonSerializedAgentHealthIssues.end(), '\n'),
                jsonSerializedAgentHealthIssues.end());

            DebugPrintf(SV_LOG_DEBUG, "%s: JSON Serialized Health Issues as as string: %s\n", FUNCTION_NAME, jsonSerializedAgentHealthIssues.c_str());
        }
        catch (JSON::json_exception &je)
        {
            ssErrMsg.str("");
            ssErrMsg << "Failed to serialize agent health issues with error" << je.what();
            DebugPrintf(SV_LOG_ERROR, " %s: %s\n", FUNCTION_NAME, ssErrMsg.str().c_str());
            return;
        }

        /// veirfy serialized string is same as previous with md5 checksum
        std::string marshlledstr = marshalCxCall(apitocall, firstargtoapi, jsonSerializedAgentHealthIssues);
        DebugPrintf(SV_LOG_DEBUG, "Marshalled string of SourceAgentProtectionPairHealthIssues:\n%s\n", marshlledstr.c_str());
        unsigned char currhash[HASH_LENGTH];
        INM_MD5_CTX ctx;
        INM_MD5Init(&ctx);
        INM_MD5Update(&ctx, (unsigned char*)marshlledstr.c_str(), marshlledstr.size());
        INM_MD5Final(currhash, &ctx);

        bool shouldreport = false;
        if (comparememory(m_prevAgentHealthStatusHash, currhash, HASH_LENGTH))
        {
            shouldreport = true;
        }
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "Agent health issues checksum matched\n");
        }

        /// Repeat same issue every 1 hr
        static long long s_lastReportTime = 0;
        ACE_Time_Value currentTime = ACE_OS::gettimeofday();
        if (!s_lastReportTime || (!shouldreport && (currentTime.sec() - s_lastReportTime) > 3600))
        {
            shouldreport = true;
        }

        if (shouldreport)
        {
            if (healthIssues.HealthIssues.empty() && healthIssues.DiskLevelHealthIssues.empty())
            {
                DebugPrintf(SV_LOG_DEBUG, "No Agent health issues identified.\n");
            }
            else
            {
                DebugPrintf(SV_LOG_ALWAYS, "reporting agent health issues: %s\n", marshlledstr.c_str());
            }
            Serializer sr(m_serializeType);
            sr.Serialize(apitocall, firstargtoapi, jsonSerializedAgentHealthIssues);
            m_transport(sr);
            if (sr.UnSerialize<bool>())
            {
                inm_memcpy_s(m_prevAgentHealthStatusHash, sizeof(m_prevAgentHealthStatusHash), currhash, sizeof currhash);
                s_lastReportTime = ACE_OS::gettimeofday().sec();
                DebugPrintf(SV_LOG_DEBUG, "%s: successfuly reported agent health issues: \n%s\n", FUNCTION_NAME, jsonSerializedAgentHealthIssues.c_str());
            }
            else
            {
                DebugPrintf(SV_LOG_ERROR, "%s: Failed to send agent health issues. CS API %s failed:\n%s\n\n", FUNCTION_NAME, SOURCE_AGENT_PROTECTION_PAIR_HEALTH_ISSUES, jsonSerializedAgentHealthIssues.c_str());
            }
        }
    }
    catch (std::string const& e) {
        DebugPrintf(SV_LOG_ERROR, "\n%s: encountered exception %s.\n", FUNCTION_NAME, e.c_str());
    }
    catch (char const* e) {
        DebugPrintf(SV_LOG_ERROR, "\n%s: encountered exception %s.\n", FUNCTION_NAME, e);
    }
    catch (ContextualException const& ce) {
        DebugPrintf(SV_LOG_ERROR, "\n%s: encountered exception %s.\n", FUNCTION_NAME, ce.what());
    }
    catch (std::exception const& e) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e.what());
    }
    catch (...) {
        DebugPrintf(SV_LOG_ERROR, "\n%s: encountered exception\n", FUNCTION_NAME);
    }
}

void ConfigureVxAgentProxy::ReportMarsAgentHealthStatus(const std::list<long>& marsHealth)
{
    try
    {
        const char *apitocall;
        const char *firstargtoapi;

        setApiAndFirstArg(TARGET_AGENT_PROTECTION_PAIR_HEALTH_ISSUES, m_dpc.c_str(), &apitocall, &firstargtoapi);

        Serializer sr(m_serializeType);
        sr.Serialize(apitocall, firstargtoapi, marsHealth);
        m_transport(sr);
        if (sr.UnSerialize<bool>())
        {
            DebugPrintf(SV_LOG_DEBUG, "%s: successfuly reported MARS agent health issues\n", FUNCTION_NAME);
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "%s: Failed to send MARS agent health issues. CS API %s failed.\n", FUNCTION_NAME, TARGET_AGENT_PROTECTION_PAIR_HEALTH_ISSUES);
        }
    }
    catch (std::string const& e) {
        DebugPrintf(SV_LOG_ERROR, "\n%s: encountered exception %s.\n", FUNCTION_NAME, e.c_str());
    }
    catch (char const* e) {
        DebugPrintf(SV_LOG_ERROR, "\n%s: encountered exception %s.\n", FUNCTION_NAME, e);
    }
    catch (ContextualException const& ce) {
        DebugPrintf(SV_LOG_ERROR, "\n%s: encountered exception %s.\n", FUNCTION_NAME, ce.what());
    }
    catch (std::exception const& e) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e.what());
    }
    catch (...) {
        DebugPrintf(SV_LOG_ERROR, "\n%s: encountered exception\n", FUNCTION_NAME);
    }
}

bool ConfigureVxAgentProxy::sendAlertToCx(const std::string & timeval, const std::string & errLevel, const std::string & agentType, SV_ALERT_TYPE alertType, SV_ALERT_MODULE alertingModule, const InmAlert &alert) const
{
    /*
    vxstub_update_agent_log ( $obj,$ttime,$log_level,$agent_info,$error_msg,$module = '',$alert_type = '')
    */
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(SEND_ALERT_TO_CX, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, timeval, errLevel, agentType, alert.GetMessage(), alertingModule, alertType, alert.GetID(), alert.GetParameters());
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

std::string ConfigureVxAgentProxy::getInstallPath() const
{
    return m_localConfigurator.getInstallPath();
}
std::string ConfigureVxAgentProxy::getAgentRole() const
{
      return m_localConfigurator.getAgentRole();
}
int ConfigureVxAgentProxy::getCxUpdateInterval() const
{
    return m_localConfigurator.getCxUpdateInterval();
}

bool ConfigureVxAgentProxy::getIsCXPatched() const
{
    return m_localConfigurator.getIsCXPatched();
}

std::string ConfigureVxAgentProxy::getProfileDeviceList() const
{
    return m_localConfigurator.getProfileDeviceList();
}

int ConfigureVxAgentProxy::getVolumeRetries() const
{
    return m_localConfigurator.getVolumeRetries();
}
int ConfigureVxAgentProxy::getVolumeRetryDelay() const
{
    return m_localConfigurator.getVolumeRetryDelay();
}

int ConfigureVxAgentProxy::getEnforcerDelay() const
{
    return m_localConfigurator.getEnforcerDelay();
}

void ConfigureVxAgentProxy::setCxUpdateInterval(int interval) const
{
    return m_localConfigurator.setCxUpdateInterval(interval);
}

VsnapRemountVolumes ConfigureVxAgentProxy::getVsnapRemountVolumes() const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(VSNAP_REMOUNT_VOLUMES, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi);
    m_transport( sr );
    return sr.UnSerialize<VsnapRemountVolumes>();
}

SV_ULONG ConfigureVxAgentProxy::getMinCacheFreeDiskSpacePercent() const
{
    return m_localConfigurator.getMinCacheFreeDiskSpacePercent();
}

void ConfigureVxAgentProxy::setMinCacheFreeDiskSpacePercent(SV_ULONG percent) const
{
    return m_localConfigurator.setMinCacheFreeDiskSpacePercent(percent);
}

SV_ULONGLONG ConfigureVxAgentProxy::getMinCacheFreeDiskSpace() const
{
    return m_localConfigurator.getMinCacheFreeDiskSpace();
}

void ConfigureVxAgentProxy::setMinCacheFreeDiskSpace(SV_ULONG space) const
{
    return m_localConfigurator.setMinCacheFreeDiskSpace(space);
}

SV_ULONGLONG ConfigureVxAgentProxy::getCMMinReservedSpacePerPair() const
{
    return m_localConfigurator.getCMMinReservedSpacePerPair();
}

int ConfigureVxAgentProxy::notifyCxDiffsDrained(const string &driveName, const SV_ULONGLONG& bytesAppliedPerSecond) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(NOTIFY_CX_DIFFS_DRAINED, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, driveName, bytesAppliedPerSecond);
    m_transport( sr );
    return sr.UnSerialize<int>();
}

int ConfigureVxAgentProxy::getVirtualVolumesId() const
{
    return m_localConfigurator.getVirtualVolumesId();
}

void  ConfigureVxAgentProxy::setVirtualVolumesId(int id) const
{
    m_localConfigurator.setVirtualVolumesId(id);
}

std::string ConfigureVxAgentProxy::getVirtualVolumesPath(string key) const
{
    return m_localConfigurator.getVirtualVolumesPath(key);
}

void ConfigureVxAgentProxy::setVirtualVolumesPath(string key, string value) const
{
    return m_localConfigurator.setVirtualVolumesPath(key, value);
}

int ConfigureVxAgentProxy::getIdleWaitTime() const
{
    return m_localConfigurator.getIdleWaitTime();
}

bool ConfigureVxAgentProxy::deleteVirtualSnapshot(std::string targetVolume, unsigned long long VSnapid, int status, std::string message) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(DELETE_VIRTUAL_SNAPSHOT, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, targetVolume,VSnapid, status, message);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

unsigned long long ConfigureVxAgentProxy::getVsnapId() const
{
    return m_localConfigurator.getVsnapId();
}
void ConfigureVxAgentProxy::setVsnapId(unsigned long long snapId) const
{
    return m_localConfigurator.setVsnapId(snapId);
}
unsigned long long ConfigureVxAgentProxy::getLowLastSnapshotId() const
{
    return m_localConfigurator.getLowLastSnapshotId();
}

void ConfigureVxAgentProxy::setLowLastSnapshotId(unsigned long long snapId) const
{
    return m_localConfigurator.setLowLastSnapshotId(snapId);
}

unsigned long long ConfigureVxAgentProxy::getHighLastSnapshotId() const
{
    return m_localConfigurator.getHighLastSnapshotId();
}

void ConfigureVxAgentProxy::setHighLastSnapshotId(unsigned long long snapId) const
{
    return m_localConfigurator.setHighLastSnapshotId(snapId);
}

SV_ULONG ConfigureVxAgentProxy::getMaxMemoryUsagePerReplication() const
{
    return m_localConfigurator.getMaxMemoryUsagePerReplication();
}

SV_ULONG ConfigureVxAgentProxy::getMaxRunsPerInvocation() const
{
    return m_localConfigurator.getMaxRunsPerInvocation();
}


SV_ULONG ConfigureVxAgentProxy::getMaxInMemoryCompressedFileSize() const
{
    return m_localConfigurator.getMaxInMemoryCompressedFileSize();
}

SV_ULONG ConfigureVxAgentProxy::getMaxInMemoryUnCompressedFileSize() const
{
    return m_localConfigurator.getMaxInMemoryUnCompressedFileSize();
}

SV_ULONG ConfigureVxAgentProxy::getCompressionChunkSize() const
{
    return m_localConfigurator.getCompressionChunkSize();
}

SV_ULONG ConfigureVxAgentProxy::getCompressionBufSize() const
{
    return m_localConfigurator.getCompressionBufSize();
}

SV_ULONG ConfigureVxAgentProxy::getSequenceCount() const
{
    return m_localConfigurator.getSequenceCount();
}

SV_ULONG ConfigureVxAgentProxy::getSequenceCountInMsecs() const
{
    return m_localConfigurator.getSequenceCountInMsecs();
}

SV_ULONG ConfigureVxAgentProxy::getRetentionBufferSize() const
{
    return m_localConfigurator.getRetentionBufferSize();
}

bool ConfigureVxAgentProxy::cdpStopReplication(const std::string & volname, const std::string & cleanupaction) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(CDP_STOP_REPLICATION, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, volname, cleanupaction );
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::enforceStrictConsistencyGroups() const
{
    return m_localConfigurator.enforceStrictConsistencyGroups();
}

string ConfigureVxAgentProxy::getTestValue() const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg("getTestValue", m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi);
    m_transport( sr );
    return sr.UnSerialize<std::string>();
}

std::string ConfigureVxAgentProxy::getVolumeMountPoint(const std::string & volname) const
{
    AutoGuard lock( m_lock );
    return m_settings.hostVolumeSettings.getVolumeMountPoint(volname);
}

void ConfigureVxAgentProxy::getVolumeNameAndMountPointForAll(VolumeNameMountPointMap & volumeNameMountPointMap) const
{
    AutoGuard lock( m_lock );
    m_settings.hostVolumeSettings.getVolumeNameAndMountPointForAll(volumeNameMountPointMap);
}

void ConfigureVxAgentProxy::getVolumeNameAndFileSystemForAll(VolumeNameFileSystemMap &volumeNameFileSystemMap )const
{
    AutoGuard lock( m_lock );
    m_settings.hostVolumeSettings.getVolumeNameAndFileSystemForAll(volumeNameFileSystemMap);
}

SV_ULONGLONG ConfigureVxAgentProxy::getSourceCapacity(const std::string & volname) const
{
    AutoGuard lock( m_lock );
    return m_settings.hostVolumeSettings.getSourceCapacity(volname);
}

SV_ULONGLONG ConfigureVxAgentProxy::getSourceRawSize(const std::string & volname) const
{
    AutoGuard lock( m_lock );
    return m_settings.hostVolumeSettings.getSourceRawSize(volname);
}

std::string ConfigureVxAgentProxy::getSourceFileSystem(const std::string & volname) const
{
    AutoGuard lock( m_lock );
    return m_settings.hostVolumeSettings.getSourceFileSystem(volname);
}

std::string ConfigureVxAgentProxy::getVirtualServerName(string fieldName) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_VIRTUAL_SERVER_NAME, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, fieldName);
    m_transport( sr );
    return sr.UnSerialize<std::string>();
}

void ConfigureVxAgentProxy::registerClusterInfo(const std::string & action, const ClusterInfo & clusterInfo) const 
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(REGISTER_CLUSTER_INFO, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, action, clusterInfo);
    m_transport( sr );
}

/*void ConfigureVxAgentProxy::registerXsInfo(const std::string & action, const ClusterInfo & xsInfo, 
                                           const VMInfos_t & vmInfos, const VDIInfos_t & vdiInfos, int isMaster) const 
{
    (void) m_transport( marshalCxCall( m_dpc, REGISTER_XS_INFO, action, xsInfo, vmInfos, vdiInfos, isMaster) );
}*/

//Added by ranjan (changing from void to bool for return value of registerXsInfo)
bool ConfigureVxAgentProxy::registerXsInfo(const std::string & action, const ClusterInfo & xsInfo,
                                           const VMInfos_t & vmInfos, const VDIInfos_t & vdiInfos, int isMaster) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(REGISTER_XS_INFO, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, action, xsInfo, vmInfos, vdiInfos, isMaster);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}


//#ifdef SV_FABRIC

FabricAgentInitialSettings ConfigureVxAgentProxy::getFabricServiceSetting()const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_FABRIC_SERVICE_SETTING, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi);
    m_transport( sr );
    return sr.UnSerialize<FabricAgentInitialSettings>();
}

PrismAgentInitialSettings ConfigureVxAgentProxy::getPrismServiceSetting()const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_PRISM_SERVICE_SETTING, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi);
    m_transport( sr );
    return sr.UnSerialize<PrismAgentInitialSettings>();
}

FabricAgentInitialSettings ConfigureVxAgentProxy::getFabricServiceSettingOnReboot()const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_FABRIC_SERVICE_SETTING_ON_REBOOT, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi);
    m_transport( sr );
    return sr.UnSerialize<FabricAgentInitialSettings>();
}

bool ConfigureVxAgentProxy::updateApplianceLunState(ATLunOperations& atLunOperationState)const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_APPLIANCE_LUN_STATE, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, atLunOperationState);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::updatePrismApplianceLunState(PrismATLunOperations& atLunOperationState)const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_PRISM_APPLIANCE_LUN_STATE, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, atLunOperationState);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::updateDeviceLunState(DeviceLunOperations& deviceLunOperationState)const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_DEVICE_LUN_STATE, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, deviceLunOperationState);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

bool ConfigureVxAgentProxy::updateBindingDeviceDiscoveryState(DiscoverBindingDeviceSetting& discoverBindingDeviceSetting)const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_BINDING_DEVICE_DISCOVERY_STATE, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, discoverBindingDeviceSetting);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

void ConfigureVxAgentProxy::SanRegisterInitiators(
    std::vector<SanInitiatorSummary> const& initiators)const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(REGISTER_INITIATORS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, initiators);
    m_transport( sr );
}


bool ConfigureVxAgentProxy::getShouldS2RenameDiffs() const 
{
    return m_localConfigurator.getShouldS2RenameDiffs() ;
}

bool ConfigureVxAgentProxy::ShouldProfileDirectSync(void) const
{
    return m_localConfigurator.ShouldProfileDirectSync();
}

int ConfigureVxAgentProxy::getS2StrictMode() const 
{
    return m_localConfigurator.getS2StrictMode() ;
}

unsigned int ConfigureVxAgentProxy::getRepeatingAlertIntervalInSeconds() const 
{
    return m_localConfigurator.getRepeatingAlertIntervalInSeconds() ;
}

void ConfigureVxAgentProxy::insertRole(std::map<std::string, std::string> &m) const 
{
    m_localConfigurator.insertRole(m) ;
}

bool ConfigureVxAgentProxy::registerLabelOnDisks() const 
{
    return m_localConfigurator.registerLabelOnDisks() ;
}

bool ConfigureVxAgentProxy::compareHcd() const 
{
    return m_localConfigurator.compareHcd() ;
}

unsigned int ConfigureVxAgentProxy::DirectSyncIOBufferCount() const
{
    return m_localConfigurator.DirectSyncIOBufferCount();
}

bool ConfigureVxAgentProxy::pipelineReadWriteInDirectSync() const
{
    return m_localConfigurator.pipelineReadWriteInDirectSync();
}

SV_ULONGLONG ConfigureVxAgentProxy::getVxAlignmentSize() const
{
    return m_localConfigurator.getVxAlignmentSize();
}

unsigned int ConfigureVxAgentProxy::getSourceReadRetries() const
{
    return m_localConfigurator.getSourceReadRetries();
}

SV_UINT ConfigureVxAgentProxy::getLengthForFileSystemClustersQuery() const
{
    return m_localConfigurator.getLengthForFileSystemClustersQuery() ;
}

bool ConfigureVxAgentProxy::getZerosForSourceReadFailures() const
{
    return m_localConfigurator.getZerosForSourceReadFailures();
}


unsigned int ConfigureVxAgentProxy::getSourceReadRetriesInterval() const
{
    return m_localConfigurator.getSourceReadRetriesInterval();
}

unsigned long int ConfigureVxAgentProxy::getExpectedMaxDiffFileSize() const 
{
    return m_localConfigurator.getExpectedMaxDiffFileSize() ;
}
long ConfigureVxAgentProxy::getTransportFlushThresholdForDiff() const 
{
    return m_localConfigurator.getTransportFlushThresholdForDiff() ;
}

int ConfigureVxAgentProxy::getProfileDiffs() const 
{
    return m_localConfigurator.getProfileDiffs() ;
}

std::string ConfigureVxAgentProxy::ProfileDifferentialRate() const 
{
    return m_localConfigurator.ProfileDifferentialRate() ;
}

SV_UINT ConfigureVxAgentProxy::ProfileDifferentialRateInterval() const 
{
    return m_localConfigurator.ProfileDifferentialRateInterval() ;
}

bool ConfigureVxAgentProxy::getAccountInfo(std::map<std::string, std::string> &namevaluepairs) const 
{
    return m_localConfigurator.getAccountInfo(namevaluepairs) ;
}

unsigned int ConfigureVxAgentProxy::getWaitTimeForSrcLunsValidity() const 
{
    return m_localConfigurator.getWaitTimeForSrcLunsValidity() ;
}

unsigned int ConfigureVxAgentProxy::getPendingChangesUpdateInterval() const 
{
    return m_localConfigurator.getPendingChangesUpdateInterval() ;
}

bool ConfigureVxAgentProxy::shouldIssueScsiCmd() const 
{
    return m_localConfigurator.shouldIssueScsiCmd() ;
}

std::string ConfigureVxAgentProxy::getCxData() const 
{
    return m_localConfigurator.getCxData() ;
}

int ConfigureVxAgentProxy::getLogFileXfer() const 
{
    return m_localConfigurator.getLogFileXfer() ;
}

size_t ConfigureVxAgentProxy::getMetadataReadBufLen() const
{
    return m_localConfigurator.getMetadataReadBufLen() ;
}

unsigned long ConfigureVxAgentProxy::getMirrorResyncEventWaitTime() const 
{
    return m_localConfigurator.getMirrorResyncEventWaitTime() ;
}

int ConfigureVxAgentProxy::getEnableVolumeMonitor() const
{
    return m_localConfigurator.getEnableVolumeMonitor() ;
}

//KUMAR: TODO: below code has to be merged with getFabricServiceSettings();

list<TargetModeOperation> ConfigureVxAgentProxy::getTargetModeSettings()const{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg("getTargetModeOperationSettings", m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi);
    m_transport( sr );
    return sr.UnSerialize< list<TargetModeOperation> >();
}
void ConfigureVxAgentProxy::updateTargetModeStatus(list<TargetModeOperation> tmLunList)const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg("updateTargetModeOperationStatus", m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, tmLunList);
    m_transport( sr );
}

void ConfigureVxAgentProxy::updateTargetModeStatusOnReboot(list<TargetModeOperation> tmLunList)const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg("updateTargetModeOperationStatusOnReboot", m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, tmLunList);
    m_transport( sr );
}

std::string ConfigureVxAgentProxy::getAgentMode() const
{
    return m_localConfigurator.getAgentMode();
}

void ConfigureVxAgentProxy::setAgentMode( std::string mode ) const
{
    m_localConfigurator.setAgentMode( mode );
}

bool ConfigureVxAgentProxy::updateGroupInfoListState(AccessCtrlGroupInfo & groupInfo)const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_GROUP_INFO_LIST_STATE, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, groupInfo);
    m_transport( sr );
    return sr.UnSerialize<bool>();
}

//Added by BSR for parallelising HCD Processing 
SV_UINT ConfigureVxAgentProxy::getMaxFastSyncProcessThreads() const 
{
    return m_localConfigurator.getMaxFastSyncProcessThreads();
}

SV_UINT ConfigureVxAgentProxy::getMaxFastSyncGenerateHCDThreads() const 
{
    return m_localConfigurator.getMaxFastSyncGenerateHCDThreads() ;
}
//End of the change
//#endif

SV_UINT ConfigureVxAgentProxy::getMaxClusterProcessThreads() const 
{
    return m_localConfigurator.getMaxClusterProcessThreads();
}

SV_UINT ConfigureVxAgentProxy::getMaxGenerateClusterBitmapThreads() const 
{
    return m_localConfigurator.getMaxGenerateClusterBitmapThreads() ;
}

StorageFailover ConfigureVxAgentProxy::failoverStorage(std::string const& action, bool migration) const
{    
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg("failoverStorage", m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, action, migration);
    m_transport( sr );
    return sr.UnSerialize<StorageFailover>();

    // return unmarshal<StorageFailover> ( m_transport( marshalCxCall( m_dpc, "failoverStorage", action, migration ) ) ); 
}


bool ConfigureVxAgentProxy::getDICheck() const
{
    return m_localConfigurator.getDICheck();
}

bool ConfigureVxAgentProxy::getSVDCheck() const
{
    return m_localConfigurator.getSVDCheck() ;
}

bool ConfigureVxAgentProxy::getDirectTransfer() const
{
    return m_localConfigurator.getDirectTransfer();
}

bool ConfigureVxAgentProxy::CompareInInitialDirectSync() const
{
    return m_localConfigurator.CompareInInitialDirectSync() ;
}

bool ConfigureVxAgentProxy::IsProcessClusterPipeEnabled() const
{
    return m_localConfigurator.IsProcessClusterPipeEnabled() ;
}

SV_UINT ConfigureVxAgentProxy::getMaxHcdsAllowdAtCx() const
{
    return m_localConfigurator.getMaxHcdsAllowdAtCx() ;
}

SV_UINT ConfigureVxAgentProxy::getMaxClusterBitmapsAllowdAtCx() const
{
    return m_localConfigurator.getMaxClusterBitmapsAllowdAtCx() ;
}

SV_UINT ConfigureVxAgentProxy::getSecsToWaitForHcdSend() const
{
    return m_localConfigurator.getSecsToWaitForHcdSend() ;
}

SV_UINT ConfigureVxAgentProxy::getSecsToWaitForClusterBitmapSend() const
{
    return m_localConfigurator.getSecsToWaitForClusterBitmapSend() ;
}

bool ConfigureVxAgentProxy::getTSCheck() const
{
    return m_localConfigurator.getTSCheck() ;
}

SV_ULONG ConfigureVxAgentProxy::getDirectSyncBlockSizeInKB() const
{
    return m_localConfigurator.getDirectSyncBlockSizeInKB() ;
}


void ConfigureVxAgentProxy::registerIScsiInitiatorName(std::string const& initiatorName) const
{        
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg("registerIScsiInitiatorName", m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, initiatorName);
    m_transport( sr );
}



SV_UINT ConfigureVxAgentProxy::updateFlushAndHoldWritesPendingStatus(std::string volumename,bool status,std::string errmsg, SV_INT error_num) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_FLUSH_AND_HOLD_WRITES_PENDING, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, volumename, status, errmsg, error_num);
    m_transport( sr );
    return sr.UnSerialize<SV_UINT>();
}

SV_UINT ConfigureVxAgentProxy::updateFlushAndHoldResumePendingStatus(std::string volumename,bool status,std::string errmsg, SV_INT error_num) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(UPDATE_FLUSH_AND_HOLD_RESUME_PENDING, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, volumename, status, errmsg, error_num);
    m_transport( sr );
    return sr.UnSerialize<SV_UINT>();
}

FLUSH_AND_HOLD_REQUEST ConfigureVxAgentProxy::getFlushAndHoldRequestSettings(std::string volumename) const
{
    Serializer sr(m_serializeType);
    const char *apitocall;
    const char *firstargtoapi;

    setApiAndFirstArg(GET_FLUSH_AND_HOLD_WRITES_REQUEST_SETTINGS, m_dpc.c_str(), &apitocall, &firstargtoapi);
    sr.Serialize(apitocall, firstargtoapi, volumename);
    m_transport( sr );
    return sr.UnSerialize<FLUSH_AND_HOLD_REQUEST>();
}

std::string ConfigureVxAgentProxy::getTargetChecksumsDir() const
{
    return m_localConfigurator.getTargetChecksumsDir() ;
}

bool ConfigureVxAgentProxy::MonitorEvents () const 
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    bool bretStatus = false;
    try
    {
        Serializer sr(m_serializeType);
        const char *apitocall;
        const char *firstargtoapi;

        setApiAndFirstArg(MONITOR_EVENTS, m_dpc.c_str(), &apitocall, &firstargtoapi);
        sr.Serialize(apitocall, firstargtoapi);
        m_transport( sr );
        bretStatus = sr.UnSerialize<bool>()  ;
    }
    catch( std::string const& e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e.c_str());
    }
    catch( char const* e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e);
    }
    catch ( ContextualException const& ce ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, ce.what());
    }
    catch( std::exception const& e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e.what());
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bretStatus;
}

bool ConfigureVxAgentProxy::EnableVolumeUnprovisioningPolicy(std::list <std::string> &sourceList) const
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    bool bretStatus = false;
    try
    {
        Serializer sr(m_serializeType);
        const char *apitocall;
        const char *firstargtoapi;

        setApiAndFirstArg(ENABLE_VOLUME_UNPROVISIONING, m_dpc.c_str(), &apitocall, &firstargtoapi);
        sr.Serialize(apitocall, firstargtoapi,sourceList);
        m_transport( sr );
        bretStatus = sr.UnSerialize<bool>() ;     
    }
    catch( std::string const& e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e.c_str());
    }
    catch( char const* e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e);
    }
    catch ( ContextualException const& ce ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, ce.what());
    }
    catch( std::exception const& e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e.what());
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bretStatus;
}

/* TODO: make a class that is used commonly by
   cxproxy and configurevxagent proxy */
void ConfigureVxAgentProxy::setApiAndFirstArg(const char *apiname,
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

ESERIALIZE_TYPE ConfigureVxAgentProxy::getSerializerType() const
{
    return m_localConfigurator.getSerializerType();
}

void ConfigureVxAgentProxy::setHostId(const std::string& hostId) const
{
    m_localConfigurator.setHostId(hostId);
}

void ConfigureVxAgentProxy::setResourceId(const std::string& resourceId) const
{
    m_localConfigurator.setResourceId(resourceId);
}

void ConfigureVxAgentProxy::setSerializerType(ESERIALIZE_TYPE serializerType) const
{
    m_localConfigurator.setSerializerType(serializerType);
}

bool ConfigureVxAgentProxy::IsVsnapDriverAvailable() const {
    return m_localConfigurator.IsVsnapDriverAvailable();
}

bool ConfigureVxAgentProxy::IsVolpackDriverAvailable() const {
    return m_localConfigurator.IsVolpackDriverAvailable();
}

SV_UINT ConfigureVxAgentProxy::getSentinalStartStatus() const
{
    return m_localConfigurator.getSentinalStartStatus();
}

SV_UINT ConfigureVxAgentProxy::getDataProtectionStartStatus() const 
{
    return m_localConfigurator.getDataProtectionStartStatus();
}

SV_UINT ConfigureVxAgentProxy::getCDPManagerStartStatus() const
{
    return m_localConfigurator.getCDPManagerStartStatus();
}

SV_UINT ConfigureVxAgentProxy::getCacheManagerStartStatus() const
{
    return m_localConfigurator.getCacheManagerStartStatus();
}

bool ConfigureVxAgentProxy::canEditCatchePath() const
{
    return m_localConfigurator.canEditCatchePath();
}
bool ConfigureVxAgentProxy::BackupAgentPause(CDP_DIRECTION direction, const std::string& devicename)  const
{
    bool bretStatus ;
    try
    {
        if( m_serializeType == Xmlize )
        {
            Serializer sr(m_serializeType);
            const char *apitocall;
            const char *firstargtoapi;
            setApiAndFirstArg(BACKUPAGENT_PAUSE, m_dpc.c_str(), &apitocall, &firstargtoapi);
            sr.Serialize(apitocall, firstargtoapi, direction, devicename );
            m_transport( sr );
            bretStatus = sr.UnSerialize<bool>() ;
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
    }
    catch( std::exception const& e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e.what());
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bretStatus;
}
bool ConfigureVxAgentProxy::BackupAgentPauseTrack(CDP_DIRECTION direction, const std::string& devicename) const
{
    bool bretStatus ;
    try
    {
        if( m_serializeType == Xmlize )
        {
            Serializer sr(m_serializeType);
            const char *apitocall;
            const char *firstargtoapi;
            setApiAndFirstArg(BACKUPAGENT_PAUSE_TRACK, m_dpc.c_str(), &apitocall, &firstargtoapi);
            sr.Serialize(apitocall, firstargtoapi, direction, devicename );
            m_transport( sr );
            bretStatus = sr.UnSerialize<bool>() ;
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
    }
    catch( std::exception const& e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e.what());
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bretStatus;
}
bool ConfigureVxAgentProxy::AnyPendingEvents() const
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    bool bretStatus = false;
    try
    {
        if( m_serializeType == Xmlize )
        {
            Serializer sr(m_serializeType);
            const char *apitocall;
            const char *firstargtoapi;
            setApiAndFirstArg(PENDING_EVENTS, m_dpc.c_str(), &apitocall, &firstargtoapi);
            sr.Serialize(apitocall, firstargtoapi);
            m_transport( sr );
            bretStatus = sr.UnSerialize<bool>() ;
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
    }
    catch( std::exception const& e ) {
        DebugPrintf(SV_LOG_ERROR, "\n%s encountered exception %s.\n", FUNCTION_NAME, e.what());
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bretStatus;
}

std::string ConfigureVxAgentProxy::getMTSupportedDataPlanes() const
{
    return m_localConfigurator.getMTSupportedDataPlanes();
}

SV_ULONGLONG ConfigureVxAgentProxy::getMinAzureUploadSize() const
{
    return m_localConfigurator.getMinAzureUploadSize();
}

unsigned int ConfigureVxAgentProxy::getMinTimeGapBetweenAzureUploads() const
{
    return m_localConfigurator.getMinTimeGapBetweenAzureUploads();
}

unsigned int ConfigureVxAgentProxy::getTimeGapBetweenFileArrivalCheck() const
{
    return m_localConfigurator.getTimeGapBetweenFileArrivalCheck();
}

unsigned int ConfigureVxAgentProxy::getMaxAzureAttempts() const
{
    return m_localConfigurator.getMaxAzureAttempts();
}

unsigned int ConfigureVxAgentProxy::getAzureRetryDelayInSecs() const
{
    return m_localConfigurator.getAzureRetryDelayInSecs();
}

unsigned int ConfigureVxAgentProxy::getAzureImplType() const
{
    return m_localConfigurator.getAzureImplType();
}

int ConfigureVxAgentProxy::getMonitoringCxpsClientTimeoutInSec() const
{
    return m_localConfigurator.getMonitoringCxpsClientTimeoutInSec();
}
