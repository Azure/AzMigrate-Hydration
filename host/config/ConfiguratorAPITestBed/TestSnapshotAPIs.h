#ifndef TESTSNAPSHOTAPIS_H
#define TESTSNAPSHOTAPIS_H

#include <string>
#include <map>
#include <sstream>
#include "sigslot.h"
#include "cdpsnapshotrequest.h"

class TestSnapshotAPIs:public sigslot::has_slots<>
{
public:
    TestSnapshotAPIs();
    void TestGetSnapshotRequestFromCx();
	void PrintSnapshtoData(SNAPSHOT_REQUEST::Ptr snapreq);
	void TestNotifyCxOnSnapshotProgress();
	void TestNotifyCxOnSnapshotStatus();
	void TestMakeSnapshotActive();
	void TestIsSnapshotAborted();
};


#endif
