//
// Copyright (c) 2008 InMage
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use, disclosure,
// or reproduction is prohibited except as permitted by express
// written license aggreement with InMage.
//
// File       : configwrapper.h
//
// Description: 
//  exception free wrapper over configurator calls
//

#ifndef _CONFIGWRAPPER_H_
#define _CONFIGWRAPPER_H_

#include <string>
#include <iostream>
#include <svtypes.h>
#include <vector>
#include <ctime>

#include "hostagenthelpers_ported.h"
#include "volumegroupsettings.h"
#include "configurator2.h"
#include "cdpsnapshotrequest.h"
#include "inmstrcmp.h"
#include "inmalert.h"
#include "portable.h"

bool makeSnapshotActive(Configurator & configurator, const std::string & snapshotId);
bool notifyCxOnSnapshotStatus(Configurator & configurator, const std::string & snapId, int timeval,const SV_ULONGLONG &VsnapId, const std::string &errMessage, int status);
bool notifyCxOnSnapshotProgress(Configurator & configurator, const std::string &snapId, int percentage, int rpoint);
bool notifyCxOnSnapshotCreation(Configurator & configurator, std::vector<VsnapPersistInfo> vsnapInfo, std::vector<bool>& results);
bool notifyCxOnSnapshotDeletion(Configurator & configurator,std::vector<VsnapDeleteInfo> vsnapInfo, std::vector<bool>& results);
bool notifyCxDiffsDrained(Configurator & configurator, const std::string &driveName, const SV_ULONGLONG& bytesAppliedPerSecond);
bool deleteVirtualSnapshot(Configurator & configurator, const std::string & targetVolume,unsigned long long vsnapid , int status, const std::string & message);
//Bug #6298
bool updateReplicationStateStatus(Configurator & configurator, const std::string& deviceName, VOLUME_SETTINGS::PAIR_STATE state);
bool updateVolumeAttribute(Configurator & configurator, NOTIFY_TYPE notifyType,const std::string & deviceName,VOLUME_STATE volumeState,const std::string & mountPoint, bool & status );
int getCurrentVolumeAttribute(Configurator & configurator, const std::string & deviceName);
bool setTargetResyncRequired(Configurator & configurator, const std::string & deviceName, bool & status, const std::string& errStr,
    const ResyncReasonStamp &resyncReasonStamp, long errorcode = RESYNC_REPORTED_BY_MT);
bool getSourceCapacity(Configurator & configurator, const std::string & deviceName, SV_ULONGLONG & capacity);
bool getSourceRawSize(Configurator & configurator, const std::string & deviceName, SV_ULONGLONG & capacity);
void setIgnoreCxErrors(bool status);

//Added for Bug#6899
bool updatePendingDataInfo(Configurator & configurator, const std::map<std::string,SV_ULONGLONG>& pendingDataInfo);

//Added for Bug#6890
bool updateCleanUpActionStatus(Configurator & configurator, const std::string& deviceName, const std::string & cleanupstr);
bool sendCacheCleanupStatus(Configurator & configurator, const std::string & devicename, bool status, const std::string & info);

bool updateRestartResyncCleanupStatus(Configurator & configurator, const std::string& deviceName, bool& success, const std::string& err_message);

bool registerClusterInfo(Configurator & configurator, const std::string & action, const ClusterInfo & clusterInfo );	
bool getResyncStartTimeStamp(Configurator & configurator, const std::string & volname, const std::string & jobId, ResyncTimeSettings& rtsettings, const std::string &hostType = "source");
bool getTargetReplicationJobId(Configurator & configurator,  std::string deviceName, std::string& jobid );
bool getResyncEndTimeStamp(Configurator & configurator, const std::string & volname, const std::string & jobId, ResyncTimeSettings& rtsettings, const std::string &hostType = "source");
bool getVolumeCheckpoint(Configurator & configurator,  std::string const & drivename, JOB_ID_OFFSET& jidoffset );
bool getVsnapRemountVolumes(Configurator & configurator, VsnapRemountVolumes& remountvols);
bool sendEndQuasiStateRequest(Configurator & configurator, const std::string & volname, int& status);
bool NotifyMaintenanceActionStatus(Configurator & configurator, const std::string & devicename, int hosttype, const std::string & respstr);
void convertToCxFormattedVsnapDeleteInfo(VsnapDeleteInfo & vsnap);
void convertToCxFormattedPersistInfo(VsnapPersistInfo & vsnap);
bool sendFlushAndHoldResumePendingStatusToCx(Configurator & configurator,std::string volumename,bool status, int error_num, std::string errmsg);
bool sendFlushAndHoldWritesPendingStatusToCx(Configurator & configurator,std::string volumename,bool status, int error_num, std::string errmsg);
bool getFlushAndHoldRequestSettings(Configurator & configurator,std::string volumename,FLUSH_AND_HOLD_REQUEST &flushAndHoldRequestSettings);
bool IsVsnapDriverAvailable();
bool IsVolpackDriverAvailable();
typedef std::vector<VOLUME_GROUP_SETTINGS *> VgsPtrs_t;
typedef VgsPtrs_t::const_iterator ConstVgsPtrVecIter_t;
typedef VgsPtrs_t::iterator VgsPtrsIter_t;

/* map from volume name to its source vgs */
typedef std::map<const char *, VgsPtrs_t, InmCStringLess> VolToVgs_t;
typedef VolToVgs_t::const_iterator ConstVolToVgsIter_t;
typedef VolToVgs_t::iterator VolToVgsIter_t;

void printVolVgs(VolToVgs_t &voltovgs);
void printVgsPtrVec(VgsPtrs_t &vgs);

class ObjectEqAttr: public std::unary_function<Object, bool>
{
    Name_t m_Name;
	Value_t m_Value;
public:
	explicit ObjectEqAttr(const Name_t &name, const Value_t &value)
		: m_Name(name), m_Value(value)
	{
	}
    bool operator()(Object &in) const
    {
		ConstAttributesIter_t it = in.m_Attributes.find(m_Name);
		return (it!=in.m_Attributes.end()) && (it->second == m_Value);
    }
};

void PrintAttributes(const Attributes_t &attributes, const SV_LOG_LEVEL LogLevel = SV_LOG_DEBUG);

std::ostream& operator<< (std::ostream& out, const Object &o);
std::ostream& operator<< (std::ostream& out, const Attributes_t &attrs);
std::ostream& operator<< (std::ostream& out, const Objects_t &objs);

HOST_VOLUME_GROUP_SETTINGS::volumeGroups_t GetEndPointVolumeSettings(HOST_VOLUME_GROUP_SETTINGS& hostVolumeGroupSettings,std::list<VOLUME_SETTINGS> endPointVolSettings);

bool cdpStopReplication(Configurator & configurator, const std::string &volname, const std::string &cleanupaction);

class LastAlertDetails
{
public:
    LastAlertDetails() : m_Time(0) { }
    std::string GetAlert(void) { return m_Alert; }
    time_t GetTime(void) { return m_Time; }
    void SetDetails(const std::string &alert, const time_t &time) { m_Alert = alert; m_Time = time; }
private:
    std::string m_Alert;
    time_t m_Time;
};
bool SendAlertToCx(SV_ALERT_TYPE AlertType, SV_ALERT_MODULE AlertingModule, const InmAlert &Alert);

#endif /* _CONFIG_WRAPPER_H_ */

