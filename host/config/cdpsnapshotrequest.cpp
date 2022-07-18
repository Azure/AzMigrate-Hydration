//
// Copyright (c) 2005 InMage
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use, disclosure,
// or reproduction is prohibited except as permitted by express
// written license aggreement with InMage.
//
// File       : snapshotrequest.cpp
//
// Description: 
//

#include "cdpsnapshotrequest.h"
#include <cstring>

SNAPSHOT_REQUEST::SNAPSHOT_REQUEST()
{
	id.clear();
	operation = UNDEFINED;
	src.clear();
	dest.clear();
	destMountPoint.clear();
	srcVolCapacity = 0;
	prescript.clear();
	postscript.clear();
	dbpath.clear();
	recoverypt.clear();
	recoverytag.clear();
	bookmarks.clear();
	vsnapMountOption = 0;
    vsnapdatadirectory.clear();
	deletemapflag = false;
	eventBased = false;
    sequenceId = 0;
	cleanupRetention = false;
}

SNAPSHOT_REQUEST::SNAPSHOT_REQUEST(SNAPSHOT_REQUEST const& rhs)
{
	id = rhs.id;
	operation = rhs.operation;
	src = rhs.src;
	dest = rhs.dest;
	destMountPoint = rhs.destMountPoint;
	srcVolCapacity = rhs.srcVolCapacity;
	prescript = rhs.prescript;
	postscript = rhs.postscript;
	dbpath = rhs.dbpath;
	recoverypt = rhs.recoverypt;
	recoverytag = rhs.recoverytag;
	bookmarks.assign(rhs.bookmarks.begin(), rhs.bookmarks.end());
	vsnapMountOption = rhs.vsnapMountOption;
	vsnapdatadirectory = rhs.vsnapdatadirectory;
	deletemapflag = rhs.deletemapflag;
	eventBased = rhs.eventBased;
    sequenceId = rhs.sequenceId;
	cleanupRetention = rhs.cleanupRetention;
}


SNAPSHOT_REQUEST& SNAPSHOT_REQUEST::operator =(const SNAPSHOT_REQUEST& rhs)
{
    if ( this == &rhs)
        return *this;

	id = rhs.id;
	operation = rhs.operation;
	src = rhs.src;
	dest = rhs.dest;
	destMountPoint = rhs.destMountPoint;
	srcVolCapacity = rhs.srcVolCapacity;
	prescript = rhs.prescript;
	postscript = rhs.postscript;
	dbpath = rhs.dbpath;
	recoverypt = rhs.recoverypt;
	recoverytag = rhs.recoverytag;
	bookmarks.assign(rhs.bookmarks.begin(), rhs.bookmarks.end());
	vsnapMountOption = rhs.vsnapMountOption;
	vsnapdatadirectory = rhs.vsnapdatadirectory;
	deletemapflag = rhs.deletemapflag;
	eventBased = rhs.eventBased;
    sequenceId = rhs.sequenceId;
	cleanupRetention = rhs.cleanupRetention;
	return *this;
}

bool SNAPSHOT_REQUEST::operator==( SNAPSHOT_REQUEST const& rhs ) const
{
	return (   id == rhs.id
		&& operation == rhs.operation 
		&& src == rhs.src
		&& dest == rhs.dest
		&& destMountPoint == rhs.destMountPoint
		&& srcVolCapacity == rhs.srcVolCapacity
		&& prescript == rhs.prescript
		&& postscript == rhs.postscript
		&& dbpath == rhs.dbpath
		&& recoverypt == rhs.recoverypt
		&& recoverytag == rhs.recoverytag
		&& bookmarks == rhs.bookmarks
		&& vsnapMountOption == rhs.vsnapMountOption
		&& vsnapdatadirectory == rhs.vsnapdatadirectory
		&& deletemapflag == rhs.deletemapflag
		&& eventBased == rhs.eventBased
        && sequenceId == rhs.sequenceId);
}

std::ostream & operator<<(std::ostream &stream, SNAPSHOT_REQUEST &rhs)
{
	stream << "id=" << rhs.id << '\n';
	switch (rhs.operation)
	{
	case SNAPSHOT_REQUEST::UNDEFINED:
		stream << "operation=" << rhs.operation << '\n';
		break;
	case SNAPSHOT_REQUEST::PLAIN_SNAPSHOT:
		stream << "operation=plain snapshot\n";
		break;
	case SNAPSHOT_REQUEST::PIT_VSNAP:
		stream << "operation=point in time vsnap\n";
		break;
	case SNAPSHOT_REQUEST::RECOVERY_VSNAP:
		stream << "operation=recovery vsnap\n";
		break;
	case SNAPSHOT_REQUEST::VSNAP_UNMOUNT:
		stream << "operation=unmount vsnap\n";
		break;
	case SNAPSHOT_REQUEST::RECOVERY_SNAPSHOT:
		stream << "operation=recovery snapshot\n";
		break;
	case SNAPSHOT_REQUEST::ROLLBACK:
		stream << "operation=rollback\n";
		break;
	}

	stream << "src=" << rhs.src << '\n';
	stream << "dest=" << rhs.dest <<'\n';
	stream << "destMountPoint=" << rhs.destMountPoint <<'\n';
	stream << "srcVolCapacity=" << rhs.srcVolCapacity << '\n';
	stream << "prescript=" << rhs.prescript << '\n';
	stream << "postscript=" << rhs.postscript << '\n';
	stream << "dbpath=" << rhs.dbpath << '\n';
	stream << "recoverypt =" << rhs.recoverypt << '\n';
	stream << "recoverytag =" << rhs.recoverytag << '\n';
    stream << "sequence id =" << rhs.sequenceId << '\n' ;

	for (size_t i =0; i < rhs.bookmarks.size(); ++i)
	{
		stream << rhs.bookmarks[i] << '\n';
	}

	stream << "vsnapMountOption=" << rhs.vsnapMountOption << '\n';
	stream << "deletemapflag=" << rhs.deletemapflag << '\n';
	stream << "eventBased=" << rhs.eventBased << '\n';
		   
    return stream;
}
/*
Name of the method:		typecastSnapshotRequests
Purpose:				Converts from SNAPSHOT_REQUESTS_t type to SNAPSHOT_REQUESTS type
Called by:				CxProxy::getSnapshotRequestFromCx()
Arguments:				SNAPSHOT_REQUESTS_t
Return Value:			SNAPSHOT_REQUESTS
*/

SNAPSHOT_REQUESTS typecastSnapshotRequests( SNAPSHOT_REQUESTS_t snapReqs)
{
	SNAPSHOT_REQUESTS currentRequests;
	SNAPSHOT_REQUESTS_t::const_iterator snapiter = snapReqs.begin();
	for (; snapiter != snapReqs.end(); snapiter++) {
		SNAPSHOT_REQUEST::Ptr request(new SNAPSHOT_REQUEST(snapiter->second));
		currentRequests.insert(SNAPSHOT_REQUEST_PAIR(snapiter->first,request));
	}
	return currentRequests;
}

VsnapRemountVolume::VsnapRemountVolume()
{
	src.clear();
	dest.clear();
	vsnapId.clear();
	recoveryPoint.clear();
	vsnapMountOption = 0;
	deleteFlag = false;
	dataLogPath.clear();
	retLogPath.clear();
//VSNAPFC
	destMountPoint.clear();
}

VsnapRemountVolume::VsnapRemountVolume(VsnapRemountVolume const& rhs)
{
	src = rhs.src;
	dest = rhs.dest;
	vsnapId = rhs.vsnapId;
	recoveryPoint = rhs.recoveryPoint;
	vsnapMountOption = rhs.vsnapMountOption;
	deleteFlag = rhs.deleteFlag;
	dataLogPath = rhs.dataLogPath;
	retLogPath = rhs.retLogPath;
//VSNAPFC
	destMountPoint = rhs.destMountPoint;
}
VsnapRemountVolume& VsnapRemountVolume::operator =(const VsnapRemountVolume& rhs)
{
    if ( this == &rhs)
        return *this;
	src = rhs.src;
	dest = rhs.dest;
	vsnapId = rhs.vsnapId;
	recoveryPoint = rhs.recoveryPoint;
	vsnapMountOption = rhs.vsnapMountOption;
	deleteFlag = rhs.deleteFlag;
	dataLogPath = rhs.dataLogPath;
	retLogPath = rhs.retLogPath;
//VSNAPFC
	destMountPoint = rhs.destMountPoint;
	return *this;
}

bool VsnapRemountVolume::operator==( VsnapRemountVolume const& rhs ) const
{
//VSNAPFC - need destMountPoint only in case of Linux
	return (  src == rhs.src
		&& dest == rhs.dest
		&& vsnapId == rhs.vsnapId
		&& recoveryPoint == rhs.recoveryPoint
		&& vsnapMountOption == rhs.vsnapMountOption
		&& deleteFlag == rhs.deleteFlag
		&& dataLogPath == rhs.dataLogPath
		&& retLogPath == rhs.retLogPath
		&& destMountPoint == rhs.destMountPoint);
}



VsnapPersistInfo::VsnapPersistInfo()
{
	volumesize = 0;
	recoverytime = 0;
	snapshot_id = 0;
	sectorsize = 0;
	accesstype = 0;
	mapfileinretention = 0;
	vsnapType = 1;
}

VsnapPersistInfo::VsnapPersistInfo(VsnapPersistInfo const& that)
{
	volumesize = that.volumesize;
	recoverytime = that.recoverytime;
	snapshot_id = that.snapshot_id;
	sectorsize = that.sectorsize;
	accesstype = that.accesstype;
	mapfileinretention = that.mapfileinretention;
	vsnapType = that.vsnapType;
	mountpoint = that.mountpoint;
	devicename = that.devicename;
	target = that.target;
	tag = that.tag;
	datalogpath = that.datalogpath;
	retentionpath = that.retentionpath;
	displaytime = that.displaytime;
}

VsnapPersistInfo& VsnapPersistInfo::operator =(VsnapPersistInfo const& that)
{
	if(this == &that)
		return *this;

	volumesize = that.volumesize;
	recoverytime = that.recoverytime;
	snapshot_id = that.snapshot_id;
	sectorsize = that.sectorsize;
	accesstype = that.accesstype;
	mapfileinretention = that.mapfileinretention;
	vsnapType = that.vsnapType;
	mountpoint = that.mountpoint;
	devicename = that.devicename;
	target = that.target;
	tag = that.tag;
	datalogpath = that.datalogpath;
	retentionpath = that.retentionpath;
	displaytime = that.displaytime;

	return *this;
}
