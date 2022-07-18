//
// Copyright (c) 2005 InMage
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use, disclosure,
// or reproduction is prohibited except as permitted by express
// written license aggreement with InMage.
//
// File       : snapshotrequest.h
//
// Description: 
//

#ifndef SNAPSHOTREQUEST__H
#define SNAPSHOTREQUEST__H

#include "svtypes.h"
#include <string>
#include <iostream>
#include <vector>
#include <map>

typedef std::vector<std::string> svector_t;

#include <boost/shared_ptr.hpp>

#define SNAPSHOT_SETTINGS_VERSION 2

struct SNAPSHOT_REQUEST
{
	typedef int Operation;
	static Operation  const UNDEFINED = -1;
	static Operation const PLAIN_SNAPSHOT = 0;
	static Operation const PIT_VSNAP = 1;
	static Operation const RECOVERY_VSNAP = 2;
	static Operation const VSNAP_UNMOUNT = 3;
	static Operation const RECOVERY_SNAPSHOT = 4;
	static Operation const ROLLBACK = 5;

	typedef int ExecutionState;
	static ExecutionState const NotStarted = -1;
	static ExecutionState const Queued = 0;
	static ExecutionState const Ready = 1;
	static ExecutionState const Complete = 2;
	static ExecutionState const Failed = 3;

	// Plain Snapshot
	static ExecutionState const SnapshotStarted = 4; // 2
	static ExecutionState const SnapshotPrescriptRun = 5;
	static ExecutionState const SnapshotInProgress = 6; //3
	static ExecutionState const SnapshotPostscriptRun = 7;

	// Recovery
	static ExecutionState const RecoveryStarted = 8; // 2
	static ExecutionState const RecoveryPrescriptRun = 9;
	static ExecutionState const RecoverySnapshotPhaseInProgress = 10; //3
	static ExecutionState const RecoveryRollbackPhaseStarted = 11; //6
	static ExecutionState const RecoveryRollbackPhaseInProgress = 12; //7
	static ExecutionState const RecoveryPostscriptRun = 13;

	// rollback execution state
	static ExecutionState const RollbackStarted = 16; //6
	static ExecutionState const RollbackPrescriptRun = 17; 
	static ExecutionState const RollbackInProgress = 18; //7
	static ExecutionState const RollbackPostScriptRun = 19;

	// vsnap mount execution state
	static ExecutionState const MapGenerationStarted = 22;
	static ExecutionState const MapGenerationInProgress = 23;
	static ExecutionState const MountStarted = 24;
	static ExecutionState const MountInProgress = 25;


	typedef boost::shared_ptr<SNAPSHOT_REQUEST> Ptr;
	// vsnap unmount execution states
	

	//
	// job instance identifier
	// cx sends this as snapshotId

	std::string id;
	
	//
	// 0 - take snapshot to physical drive
	// 1 - mount a point in time virtual snapshot
	// 2 - mount a recovery based virtual snapshot
	// 3 - unmount a virtual snapshot drive
	// 4 - take a recovery snapshot on physical drive
	// 5 - rollback
	//
	Operation operation;

	//
	// This field is used as
	//   source for physical snapshot, recovery snapshot, rollback , 
	//   virtual snapshot mount operation
	//
	// unused:  during unmount virtual snapshot operation
	//
	std::string src;

	//
	// This field is used as
	//   destination for physical snapshot, recovery snapshot, virtual snapshot mount.
	//   drive to unmount for virtual snapshot unmount operation
	//
	//   should be same as src  for rollback
	std::string dest;

	//
	// This field is used as
	//   destination mount point for physical snapshot, recovery  and rollback 
	std::string destMountPoint;

	// capacity of the source volume
	SV_ULONGLONG srcVolCapacity;

	// These fields are used as
	//  pre and post scripts for all the operations
	std::string prescript;
	std::string postscript;


	// this field is used as
	//  dbpath for recovery (physical/virtual) and rollback operation
	// unused for other operation
	std::string dbpath;

	// this field is used as
	//  the rollback point for recovery (physical/virtual) and rollback operation
	std::string recoverypt;



	// this field is used as
	//   for physical and point in time virtual snapshot
	//   The snapshot will be started when we see any of these tags for the specified src
	svector_t bookmarks;


	// this field is used only for mount virtual snapshot. it specifies if we want it 
	// to be mounted read-write. by default it is read-only
	int vsnapMountOption;

	// to store COW changes for PIT and RW recovery vsnap
	std::string vsnapdatadirectory;

	// vnsap option during unmount.
	// should we delete the vsnap map on unmount
	bool deletemapflag;

	// used for point in time event based plain/virtual snapshots.
	bool eventBased;

	// if recovery (physical or virtual)/rollback is based on a tag
	// this field would contain the value
	// otherwise it would be empty
	std::string recoverytag;

    //
    // recover to the sequence point
    //
    SV_ULONGLONG sequenceId;
	bool cleanupRetention;

	SNAPSHOT_REQUEST();
	bool operator==(SNAPSHOT_REQUEST const&) const;
    SNAPSHOT_REQUEST(const SNAPSHOT_REQUEST&);
    SNAPSHOT_REQUEST& operator=(const SNAPSHOT_REQUEST&);
};
typedef std::map<std::string, SNAPSHOT_REQUEST> SNAPSHOT_REQUESTS_t;
typedef std::map<std::string, SNAPSHOT_REQUEST::Ptr> SNAPSHOT_REQUESTS;
typedef std::pair<std::string, SNAPSHOT_REQUEST::Ptr> SNAPSHOT_REQUEST_PAIR;

// message types
int const CDP_QUIT            = 0x2000;
int const CDP_ABORT           = 0x2001;

// message priority
static int const CDP_NORMAL_PRIORITY = 0x01;
static int const CDP_HIGH_PRIORITY   = 0x10;

// exit value passed to postscript

static int const CDPSUCCESS = 0;
static int const CDPPreScriptFailed = -1;
static int const CDPSrcOpenFailed = -2;
static int const CDPTgtOpenFailed = -3;
static int const CDPOUTOFMEMORY = -4;
static int const CDPReadFromSrcFailed = -5;
static int const CDPWriteToTgtFailed = -6;
static int const CDPTgtUnhideFailed = -7;
static int const CDPTgtInsufficientCapacity = -8;
static int const CDPCONNECTIONFAILURE = -9;
static int const CDPReadFromRetentionFailed = -10;
static int const CDPSeekOnTgtFailed = -11;
static int const CDPServiceQuit = -12;
static int const CDPAbortedbyUser = -13;
static int const CDPSrcVisible = -14;

struct CDPMessage
{
	typedef int Type;
	static Type  const MSG_STATECHANGE = 0x3000;
	static Type  const MSG_PROGRESSUPDATE = 0x3001;

	std::string id;
	SV_ULONG operation;
	std::string src;
	std::string dest;
	Type type;

	SNAPSHOT_REQUEST::ExecutionState executionState;
	SV_ULONG progress;
	SV_LONGLONG time_remaining_in_secs;
	std::string err;
};
SNAPSHOT_REQUESTS typecastSnapshotRequests( SNAPSHOT_REQUESTS_t snapReqs);
struct VsnapRemountVolume
{
	std::string src;
	std::string dest;
	std::string vsnapId;
	std::string recoveryPoint;
	int vsnapMountOption;
	bool deleteFlag;
	std::string dataLogPath;
	std::string retLogPath;
	//VSNAPFC - Persistence
	//In case of Linux we need to get mountpoint along
	//with devicename.
	// dest  - /dev/vs/cx?
	// destMountPoint - /mnt/vsnap1
	std::string destMountPoint;
	VsnapRemountVolume();
	bool operator==(VsnapRemountVolume const&) const;
    VsnapRemountVolume(const VsnapRemountVolume&);
    VsnapRemountVolume& operator=(const VsnapRemountVolume&);

};
typedef std::map<std::string, VsnapRemountVolume> VsnapRemountVolumes;

struct VsnapPersistInfo
{
	SV_ULONGLONG volumesize;
	SV_ULONGLONG recoverytime;
	SV_ULONGLONG snapshot_id;
	SV_UINT sectorsize;
	SV_UINT accesstype;
	SV_UINT mapfileinretention;
	SV_UINT vsnapType;
	std::string mountpoint;
	std::string devicename;
	std::string target;
	std::string tag;
	std::string datalogpath;
	std::string retentionpath;
	std::string displaytime;

	VsnapPersistInfo();
	VsnapPersistInfo(VsnapPersistInfo const& that);
	VsnapPersistInfo& operator=(VsnapPersistInfo const& that);
};

struct VsnapDeleteInfo
{
	std::string target_device;
	std::string vsnap_device;
	int status;
	std::string info;
};


#endif
