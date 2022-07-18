#ifndef __APINAMES__H_
#define __APINAMES__H_

const char UPDATE_AGENT_LOG[]               = "updateAgentLog";
// Bug #6298
const char UPDATE_REPLICATION_STATE_STATUS[]      = "updateReplicationStateStatus";
const char UPDATE_REPLICATION_CLEANUP_STATUS[]      = "updateReplicationCleanupStatus";
const char UPDATE_RESTART_RESYNC_CLEANUP_STATUS[]   = "restartReplicationUpdate";
const char RENAME_CLUSTER_GROUP[]           = "renameClusterGroup";
const char DELETE_VOLUMES_FROM_CLUSTER[]    = "deleteVolumesFromCluster";
const char DELETE_CLUSTER_NODE[]            = "deleteClusterNode";
const char ADD_VOLUMES_TO_CLUSTER[]         = "addVolumesToCluster";
const char GET_VOLUME_CHECKPOINT[]          = "getVolumeCheckpoint";
const char SET_RESYNC_PROGRESS[]            = "setResyncProgress";
/* Added by BSR fast sync TBC */
const char SET_RESYNC_PROGRESS_FASTSYNC[] = "setResyncProgressFastsync" ;
/*
*  Added Following strings to fix overshoot issue using bitmap
*  By Suresh
*/
const char SET_RESYNC_UPDATE_PROGRESS_BYTES_FASTSYNC[]="setResyncProgressBytesFast";
const char SET_RESYNC_UPDATE_PROGRESS_STATS[] = "resyncProgressUpdateV2";
const char SET_RESYNC_UPDATE_FULLSYNC_BYTES_FASTSYNC[]="setHcdProgressBytesFast";
const char SET_RESYNC_UPDATE_MATCHED_BYTES_FASTSYNC[] = "setResyncMatchedBytesFast" ;
const char SET_RESYNC_UPDATE_FULLY_UNUSED_BYTES_FASTSYNC[] = "setResyncFullyUnusedBytesFast" ;

//Changes End
const char GET_VOLUME_CHECKPOINT_FASTSYNC[] = "getVolumeCheckpointFastsync" ;
const char SET_RESYNC_TRANSITION[] = "setResyncTransitionStepOneToTwo" ; 
/* End of the change*/
const char GET_INITIAL_SYNC_DIRECTORIES[]   = "getInitialSyncDirectories";
const char GET_TARGET_REPLICATION_JOB_ID[]  = "getTargetReplicationJobId";
const char UPDATE_VOLUME_ATTRIBUTE[]        = "updateVolumeAttribute";
const char GET_CURRENT_VOLUME_ATTRIBUTE[]   = "getCurrentVolumeAttribute";
const char UPDATE_LOG_AVAILABLE[]           = "updateLogAvailable";
const char UPDATE_CDP_INFORMATION[]         = "updateCdpInformation";
const char UPDATE_RETENTION_TAG[]           = "updateRetentionTag";
const char GET_HOST_RETENTION_WINDOW[]      = "getHostDbRetentionWindows";
const char UPDATE_RETENTION_INFO[]          = "updateRetentionDetails";
const char GET_CLEAR_DIFFS[]                = "getClearDiffs";
const char SET_TARGET_RESYNC_REQUIRED[]     = "SetTargetResyncRequiredWithReasonCode";
const char SET_SOURCE_RESYNC_REQUIRED[]     = "setSourceResyncRequiredWithReasonCode";
const char SET_PRISM_RESYNC_REQUIRED[]     = "setPrismResyncRequired";
const char SET_XS_POOL_SOURCE_RESYNC_REQUIRED[]     = "setXsPoolSourceResyncRequired";
const char PAUSE_REPLICATION_FROM_HOST[]    = "pauseReplicationFromHost";
const char RESUME_REPLICATION_FROM_HOST[]    = "resumeReplicationFromHost";
const char UPDATE_SNAPSHOT_STATUS[]         = "updateSnapshotStatus";
const char UPDATE_PENDINGDATA_ONTARGET[]    = "updatePendingDataOnTarget";

const char UPDATE_SNAPSHOT_PROGRESS[]       = "updateSnapshotProgress";
const char UPDATE_SNAPSHOT_CREATION[]        = "updateSnapshotCreation";
const char UPDATE_SNAPSHOT_DELETION[]        = "deleteCdpcliVirtualSnapshot";
const char MAKE_ACTIVE_SNAPSHOT_INSTANCE[]  = "makeActiveSnapshotInstance";

const char GET_RESYNC_START_TIMESTAMP[]        = "get_stats_resyncTimeTag";
const char SEND_RESYNC_START_TIMESTAMP[]    = "set_resync_start_time_stamp";
const char UPDATE_MIRROR_STATE[]            = "update_mirror_state";
const char UPDATE_LASTIO_TIMESTAMP_ON_ATLUN[]            = "update_lastio_timestamp_on_atlun";
const char UPDATE_VOLUMES_PENDING_CHANGES[]         = "update_source_pending_changes";
const char GET_RESYNC_END_TIMESTAMP[]        = "get_stats_resyncTimeTag";
const char SEND_RESYNC_END_TIMESTAMP[]        = "set_resync_end_time_stamp";

/* Added by BSR for Fast Sync TBC */
const char GET_RESYNC_START_TIMESTAMP_FASTSYNC[]        = "get_stats_resyncTimeTag_fastsync";
const char SEND_RESYNC_START_TIMESTAMP_FASTSYNC[]    = "set_resync_start_time_stamp_fastsync";
const char GET_RESYNC_END_TIMESTAMP_FASTSYNC[]        = "get_stats_resyncTimeTag_fastsync";
const char SEND_RESYNC_END_TIMESTAMP_FASTSYNC[]        = "set_resync_end_time_stamp_fastsync";

/*End of the changes*/
const char SEND_END_QUASI_STATE_REQUEST[]    = "setEndQuasiStateRequest";
const char SEND_ALERT_TO_CX[]                = "updateAgentLog";
const char SEND_DEBUG_MSG_LOCALHOST_LOG[]    = "sendDebugMsgToLocalHostLog" ;
const char VSNAP_REMOUNT_VOLUMES[]            = "getVsnapDrivesMounted";
const char NOTIFY_CX_DIFFS_DRAINED[]        = "updateOutpostAgentStatus";
const char DELETE_VIRTUAL_SNAPSHOT[]        = "deleteVirtualSnapshot";
const char CDP_STOP_REPLICATION[]            = "cdpStopReplication";
const char GET_VIRTUAL_SERVER_NAME[]        = "getVirtualServerName";
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
const char SET_PAUSE_REPLICATION_STATUS[]    = "setPauseReplicationStatus";

const char GET_SNAPSHOT_COUNT[]    = "getSnapshotCount";
const char REGISTER_HOST_STATIC_INFO[]    = "::registerHostStaticInfo";
const char REGISTER_HOST_DYNAMIC_INFO[]    = "::registerHostDynamicInfo";
const char SAN_REGISTER_HOST[]    = "::sanRegisterHost";
const char UNREGISTER_HOST[]    = "::unregisterHost";
const char GET_INITIAL_SETTINGS[]    = "getInitialSettings";
const char GET_CURRENT_INITIAL_SETTINGS_V2[]    = "getCurrentInitialSettingsV2";
const char GET_SRR_JOBS[]    = "getSrrJobs";

const char GET_JOBS_TO_PROCESS[] = "getJobsToProcess";
const char UPDATE_JOB_STATUS[] = "updateJobStatus";

const char GET_APP_SETTINGS[] = "GetApplicationSettings";
const char POLICY_UPDATE[] = "UpdatePolicy";
const char ENABLE_VOLUME_UNPROVISIONING[] = "EnableVolumeUnprovisioningPolicy";
const char PENDING_EVENTS[] = "AnyPendingEvents" ; 
const char MONITOR_EVENTS[] = "MonitorEvents";
const char BACKUPAGENT_PAUSE[] = "BackupAgentPause" ;
const char BACKUPAGENT_PAUSE_TRACK[] = "BackupAgentPauseTrack" ;
const char UPDATE_CDP_INFORMATIONV2[]       = "updateCdpInformationV2";
const char UPDATE_CDP_DISKUSAGE[]			= "updateCdpDiskUsage";
const char UPDATE_FLUSH_AND_HOLD_WRITES_PENDING[] = "updateFlushAndHoldWritesPendingStatus";
const char UPDATE_FLUSH_AND_HOLD_RESUME_PENDING[] = "updateFlushAndHoldResumePendingStatus";
const char GET_FLUSH_AND_HOLD_WRITES_REQUEST_SETTINGS[] = "getFlushAndHoldWritesRequestSettings";
const char UPDATE_AGENT_HEALTH[] = "updateAgentHealth";
const char SOURCE_AGENT_PROTECTION_PAIR_HEALTH_ISSUES[] = "SourceAgentProtectionPairHealthIssues";
const char TARGET_AGENT_PROTECTION_PAIR_HEALTH_ISSUES[] = "targetAgentProtectionPairHealthIssues";

#endif /* __APINAMES__H_ */
