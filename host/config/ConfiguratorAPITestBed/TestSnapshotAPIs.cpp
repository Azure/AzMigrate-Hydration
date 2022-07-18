#include <iostream>
#include <string>
#include "TestSnapshotAPIs.h"
#include "rpcconfigurator.h"
#include "localconfigurator.h"

extern Configurator* TheConfigurator;

using namespace std;

TestSnapshotAPIs::TestSnapshotAPIs()
{
}


void TestSnapshotAPIs::TestGetSnapshotRequestFromCx()
{
	cout << "Snapshot Settings: " <<  endl << endl;
	try
	{
		SNAPSHOT_REQUESTS snapRequests = ((CxProxy *)&TheConfigurator->getCxAgent())->getSnapshotRequestFromCx();

		SNAPSHOT_REQUESTS::const_iterator iter;
		int i = 1;
		for ( iter = snapRequests.begin(); iter != snapRequests.end();iter++,  i++) {
			cout << "Snapshot Request [ " << i << " ]  Information:" << endl;
			cout << "Snapshot Id = " << iter->first << endl;
			PrintSnapshtoData(iter->second);
		}
	}
	catch ( ContextualException& ce )
	{
		DebugPrintf(SV_LOG_ERROR,
			"\ngetSnapshotRequests call to cx failed with exception %s.\n",ce.what());
	}
	catch ( ... )
	{
		DebugPrintf(SV_LOG_ERROR,
			"\ngetSnapshotRequests call to cx failed with unknown exception.\n");
	}
}

void TestSnapshotAPIs::PrintSnapshtoData(SNAPSHOT_REQUEST::Ptr snapreq)
{
	std::stringstream stream;

	stream << "id = " << snapreq->id << '\n';
	stream << "operation value = " << snapreq->operation << endl;
	switch (snapreq->operation)
	{
	case SNAPSHOT_REQUEST::UNDEFINED:
		stream << "operation = " << snapreq->operation << '\n';
		break;
	case SNAPSHOT_REQUEST::PLAIN_SNAPSHOT:
		stream << "operation = plain snapshot\n";
		break;
	case SNAPSHOT_REQUEST::PIT_VSNAP:
		stream << "operation = point in time vsnap\n";
		break;
	case SNAPSHOT_REQUEST::RECOVERY_VSNAP:
		stream << "operation = recovery vsnap\n";
		break;
	case SNAPSHOT_REQUEST::VSNAP_UNMOUNT:
		stream << "operation = unmount vsnap\n";
		break;
	case SNAPSHOT_REQUEST::RECOVERY_SNAPSHOT:
		stream << "operation = recovery snapshot\n";
		break;
	case SNAPSHOT_REQUEST::ROLLBACK:
		stream << "operation = rollback\n";
		break;
	}

	stream << "src = " << snapreq->src << '\n';
	stream << "dest = " << snapreq->dest <<'\n';
	stream << "srcVolCapacity = " << snapreq->srcVolCapacity << '\n';
	stream << "prescript = " << snapreq->prescript << '\n';
	stream << "postscript = " << snapreq->postscript << '\n';
	stream << "dbpath = " << snapreq->dbpath << '\n';
	stream << "recoverypt = " << snapreq->recoverypt << '\n';
	stream << "List of bookmarks:" << '\n';
	for (size_t i =0; i < snapreq->bookmarks.size(); ++i)
	{
		stream << snapreq->bookmarks[i] << '\n';
	}
	stream << '\n';
	stream << "vsnapMountOption = " << snapreq->vsnapMountOption << '\n';
	stream << "vsnapDatadirectory = " << snapreq->vsnapdatadirectory << '\n';
	stream << "deletemapflag = " << snapreq->deletemapflag << '\n';
	stream << "eventBased = " << snapreq->eventBased << '\n';
	stream << "recoveryTag = " << snapreq -> recoverytag << '\n';
	stream << "sequenceId = " << snapreq -> sequenceId << '\n';
	stream << "------------------------------------------------------------------" << endl;
	stream << endl << endl; // << endl;
	cout << stream.str();
}

void TestSnapshotAPIs::TestNotifyCxOnSnapshotStatus()
{
	string snapId = "69CF1B61-31F3-B48B-5E51-A9868B06C6EB";
	int timeval = 0;
	string errorMessage = "	Gopal Krishna";
	int status = 3;
	SV_LONGLONG llVsnapId = 0;

	bool ret1 = TheConfigurator->getVxAgent().notifyCxOnSnapshotStatus(snapId, timeval,llVsnapId,
					errorMessage, status);
	cout << "Return value from NotifyCxOnSnapshotStatus call  = " << ret1 << endl;

	
}
void TestSnapshotAPIs::TestNotifyCxOnSnapshotProgress()
{
	string snapId = "D67B1B75-7EFC-0F3C-D007-A4724294D9A4";
	int percent = 75;
	int rpoint = 0;
	bool ret2 = TheConfigurator->getVxAgent().notifyCxOnSnapshotProgress(snapId, percent, rpoint);

	cout << "Return value from TestNotifyCxOnSnapshotProgress call = " << ret2 << endl;
}
void TestSnapshotAPIs::TestMakeSnapshotActive()
{
	string snapId = "D67B1B75-7EFC-0F3C-D007-A4724294D9A4";
	int ret = TheConfigurator->getVxAgent().makeSnapshotActive(snapId);
	cout << "Return value from  TestmakeSnapshotActive call = " << ret << endl;
}
void TestSnapshotAPIs::TestIsSnapshotAborted()
{
	string snapId = "D67B1B75-7EFC-0F3C-D007-A4724294D9A4";
	bool ret = TheConfigurator->getVxAgent().isSnapshotAborted(snapId);
	cout << "Return value from  TestIsSnapshotAborted call = " << ret << endl;
}

