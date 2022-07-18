#include <iostream>
#include <string>
#include "TestVolumeManager.h"
#include "configurevolumemanager.h"
#include "configurator2.h"
#include "localconfigurator.h"

using namespace std;

extern string targetdrive;

#define DebugPrintf printf

extern Configurator* TheConfigurator;


TestVolumeManager::TestVolumeManager()
{
     ///TheConfigurator->getVolumeManager().setVolumeAttributes.connect( this, &TestVolumeManager::onSetVolumeAttributes );
     //TheConfigurator->getVolumeManager().setReadWrite.connect( this, &TestVolumeManager::onSetReadWrite );
     //TheConfigurator->getVolumeManager().setVisible.connect( this, &TestVolumeManager::onSetVisible );
     //TheConfigurator->getVolumeManager().setInvisible.connect( this, &TestVolumeManager::onSetInvisible );

 }
void TestVolumeManager::onSetVolumeAttributes( std::map<std::string,int> const& deviceName )
{
    DebugPrintf( "Device %s set to read only\n" );
}

void TestVolumeManager::TestGetVolumeCheckpoint()
{
    //string jobId = TheConfigurator->getVxAgent().getTargetReplicationJobId(targetdrive);

   string drivename = "E";
    //long long offset = TheConfigurator->getVxAgent().getVolumeCheckpoint(jobId, drivename);
    JOB_ID_OFFSET data = TheConfigurator->getVxAgent().getVolumeCheckpoint(drivename);
    
	std::cout << "Response from getVolumeCheckpoint: " << data.first << "," << data.second << endl;
}

#if 0
void TestVolumeManager::onSetReadWrite( std::string const& deviceName ) {
    DebugPrintf( "Device %s set to read only\n", deviceName.c_str() );
}

void TestVolumeManager::onSetVisible( std::string const& deviceName ) {
    DebugPrintf( "Device %s set to visible\n", deviceName.c_str() );
}

void TestVolumeManager::onSetInvisible( std::string const& deviceName ) {
    DebugPrintf( "Device %s set to invisible\n", deviceName.c_str() );
}

#endif
