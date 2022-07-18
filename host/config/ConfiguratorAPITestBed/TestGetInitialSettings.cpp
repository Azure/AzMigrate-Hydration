#include "Globals.h"
#include "TestGetInitialSettings.h"
#include <algorithm>
using namespace std;
#define SEPARATOR() cout << "-------------------------------------------------------------------\n";



string CDP_DIRECTION_STRING[] = { "UNSPECIFIED", "SOURCE", "TARGET" };
string SYNC_MODE_STRING[] =  { "SYNC_DIFF", "SYNC_OFFLOAD", "SYNC_FAST", "SYNC_SNAPSHOT", "SYNC_DIFF_SNAPSHOT", "SYNC_QUASIDIFF", "SYNC_FAST_TBC" , "SYNC_DIRECT"  };

void TestGetInitialSettings::
Print()
{
    system("cls");
    SEPARATOR();
    cout << "CDP_SETTINGS:" << endl << endl;
	PrintCdpsettings();
    SEPARATOR();
	cout << "HOST_VOLUME_GROUP_SETTINGS:" << endl << endl;
    PrintHostVolumeGroupSettings();
	SEPARATOR();
    cout << "PRISM_SETTINGS:" << endl << endl;
    PrintPrismSettings();
    SEPARATOR();
    cout << "TRANSPORT_CONNECTION_SETTINGS:" << endl << endl;
	PrintTransportConnectionSettings(m_settings.transportSettings);
    SEPARATOR();
	cout << "TRANSPORT_PROTOCOL: " << m_settings.transportProtocol <<  endl;
	SEPARATOR();
	cout << "SECURE_MODE: " << m_settings.transportSecureMode <<  endl;
	SEPARATOR();
	cout << "OPTIONS: " <<  endl;
	for (ConstOptionsIter_t it = m_settings.options.begin(); it != m_settings.options.end(); ++it)
	{
		cout << it->first << " --> " << it->second << endl;
	}
	SEPARATOR();
	cout << endl ;
}


void TestGetInitialSettings::
PrintTransportConnectionSettings(const TRANSPORT_CONNECTION_SETTINGS &tcs)
{
	cout<< endl
		<< "  ipAddress: " << tcs.ipAddress<< "\n"             
		<< "  port: " << tcs.port<< '\n'
		<< "  ssl port: " << tcs.sslPort<< '\n'
		<< "  ftp port: " << tcs.ftpPort<< '\n'
		<< "  user: " << tcs.user << "\n"
		<< "  password: " << tcs.password << "\n"
		<< "  activeMode: " << tcs.activeMode << "\n"
		<< "  connectTimeout: " << tcs.connectTimeout << "\n"
		<< "  responseTimeout: " << tcs.responseTimeout << "\n"             
		<< endl;
}

void TestGetInitialSettings::
PrintCdpsettings()
  {
    typedef  CDPSETTINGS_MAP::const_iterator CDPSETTINGSITER;
	typedef  std::vector<cdp_policy>::const_iterator CDPPOLICYITER;
    for(CDPSETTINGSITER iter = m_settings.cdpSettings.begin();
        iter != m_settings.cdpSettings.end();++iter)
    {
        cout << iter->first << "---------------  \n";
        CDP_SETTINGS cdpsettings  =iter->second;

        cout << "CDP_STATE: " << cdpsettings.Status() << "\n"
            << "cdp_catalogue: " << cdpsettings.Catalogue() << "\n"
          	<< "cdp_alert_threshold: " << cdpsettings.CatalogueThreshold() << "\n"
			<< "cdp_diskspace: " << cdpsettings.DiskSpace() << "\n"
			<< "cdp_onspace_shortage: " << cdpsettings.OnSpaceShortage() << "\n"
			<< "cdp_timepolicy: " << cdpsettings.TimePolicy() << "\n"
			<< "cdp_minreservedspace: " << cdpsettings.MinReservedSpace() << "\n"
			<< "CDP_POLICY : " << endl;
		for(CDPPOLICYITER piter = cdpsettings.cdp_policies.begin();
			piter != cdpsettings.cdp_policies.end();++piter)
		{
			cout << "    start: " << piter->Start()  << "\n"
				<< "    duration: " << piter->Duration()  << "\n"
				<< "    granularity: " << piter->Granularity()  << "\n"
				<< "    tagcount: " << piter->TagCount()  << "\n"
				<< "    cdp_diskspace_forthiswindow: " << piter->CdpSpaceConfigure()  << "\n"
				<< "    cdp_contentstore: " << piter->CdpContentstore()  << "\n"
				<< "    apptype: " << piter->AppType()  << "\n"
				<< "    userbookmarks:" << endl;
			for(std::vector<std::string>::const_iterator it = piter->userbookmarks.begin();
				it != piter->userbookmarks.end();++it)
			{
				cout << "      user event:" << (*it);
			}
			cout << endl;
		}
    }
  }


void TestGetInitialSettings::
PrintHostVolumeGroupSettings()
{
    typedef std::list<VOLUME_GROUP_SETTINGS>::iterator VGITER;
	typedef VOLUME_GROUP_SETTINGS::volumes_t::iterator VITER;

    for(VGITER vgiter = m_settings.hostVolumeSettings.volumeGroups.begin();
        vgiter != m_settings.hostVolumeSettings.volumeGroups.end(); ++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        cout << "Direction: " << CDP_DIRECTION_STRING[vg.direction] << "\t" << "Id: " 
            << vg.id << "\t" << "status: " << vg.status << endl << endl;;
		cout << "VOLUME_SETTINGS:" << endl << endl;
        for(VITER viter = vg.volumes.begin();viter != vg.volumes.end(); ++viter)
        {
			cout << "VOLUME:";
			cout << viter->first << endl;
            VOLUME_SETTINGS vs = viter->second;

            cout << "deviceName: " << vs.deviceName << "\n" 
				<< "mountPoint: " << vs.mountPoint << "\n"
				<< "fstype: " << vs.fstype << "\n"
                << "hostname: " << vs.hostname << "\n"
                << "hostId: " << vs.hostId << "\n"
                << "secureMode: " << vs.secureMode << "\n"
                << "sourceToCXSecureMode: " << vs.sourceToCXSecureMode << "\n"
                << "synchmode: " << SYNC_MODE_STRING[vs.syncMode] << "\n"
                <<"transportProtocol: " << vs.transportProtocol << "\n"
                << "visibility: " << vs.visibility << "\n"
                << "resyncDirectory: " << vs.resyncDirectory << "\n" 
				<< "rpoThreshold: " << vs.rpoThreshold << "\n" 
				<< "sourceOSVal: " << vs.sourceOSVal << "\n"
				<< "sourceCapacity: " << vs.sourceCapacity << "\n"
				<< "srcResyncStarttime: " << vs.srcResyncStarttime << "\n"
				<< "srcResyncEndtime: " << vs.srcResyncEndtime << "\n"
				<< "OtherSiteCompatibilityNum: " << vs.OtherSiteCompatibilityNum << "\n"
				<< "resyncRequiredFlag: " << vs.resyncRequiredFlag << "\n"
				<< "resyncRequiredCause: " << vs.resyncRequiredCause << "\n"
				<< "resyncRequiredTimestamp: " << vs.resyncRequiredTimestamp << "\n"
				<< "jobId: " << vs.jobId << "\n"
				<< "pairState: " << vs.pairState << "\n"
				<< "srcResyncStartSeq: " << vs.srcResyncStartSeq << "\n"
				<< "srcResyncEndSeq: " << vs.srcResyncEndSeq << "\n"
				<< "compressionEnable: " << vs.compressionEnable << "\n"
				<< "cleanup_action: " << vs.cleanup_action << "\n"
				<< "diffsPendingInCX: " << vs.diffsPendingInCX << "\n"
				<< "currentRPO: " << vs.currentRPO << "\n"
				<< "applyRate: " << vs.applyRate << "\n"
				<< "resyncCounter: " << vs.resyncCounter << "\n"
				<< "THROTTLE_SETTINGS :\n"
                << "throttleResync: " << vs.throttleSettings.throttleResync << "\n"
                << "throttleSource: " << vs.throttleSettings.throttleSource << "\n"
                << "throttleTarget: " << vs.throttleSettings.throttleTarget << "\n"
				<< "maintenance_action: " << vs.maintenance_action << endl
				<< "Source raw size: " << vs.sourceRawSize << endl
				<< "Source Start Offset: " << vs.srcStartOffset << endl
				<< "devicetype: " << vs.devicetype << endl
				<< "SAN_VOLUME_INFO :\n"
				<< "    physicalDeviceName: " << vs.sanVolumeInfo.physicalDeviceName << "\n"
				<< "    mirroredDeviceName: " << vs.sanVolumeInfo.mirroredDeviceName << "\n"
				<< "    virtualDevicename: " << vs.sanVolumeInfo.virtualDevicename << "\n"
				<< "    virtualName: " << vs.sanVolumeInfo.virtualName << "\n"
				<< "    physicalLunId: " << vs.sanVolumeInfo.physicalLunId << "\n";
            PrintATLUNStateRequest(vs);
            cout<< "endpoints :\n";
				int i = 1;
				std::list<VOLUME_SETTINGS>::iterator endptiter = vs.endpoints.begin();
				for(; endptiter != vs.endpoints.end(); ++endptiter)
				{
					cout << "    #SLNo. " << i++ << " :\n"
						<< "      deviceName: " << (*endptiter).deviceName << "\n" 
						<< "      mountPoint: " << (*endptiter).mountPoint << "\n"
						<< "      fstype: " << (*endptiter).fstype << "\n"
						<< "      hostname: " << (*endptiter).hostname << "\n"
						<< "      hostId: " << (*endptiter).hostId << "\n"
						<< "      pairState: " << (*endptiter).pairState << "\n"
                        << "      devicetype: " << vs.devicetype << endl;
				}
			PrintOptions(vs);

        }

		cout << '\n' << "TRANSPORT_CONNECTION_SETTINGS:";
		PrintTransportConnectionSettings(vg.transportSettings);
    }

}

void TestGetInitialSettings::
PrintPrismSettings()
{
    for (PRISM_SETTINGS_CONST_ITERATOR psiter = m_settings.prismSettings.begin(); psiter != m_settings.prismSettings.end(); psiter++)
    {
        cout << "\n\n";
        cout << "SOURCE LUN ID (Key in map): " << psiter->first << '\n';
        const PRISM_VOLUME_INFO &prismVolumeInfo = psiter->second;
        cout << "PRISM VOLUME INFO: \n";
        cout << "sourceLUNID: " << prismVolumeInfo.sourceLUNID << '\n';
        cout << "sourceLUNCapacity: " << prismVolumeInfo.sourceLUNCapacity << '\n';
        cout << "mirrorState: " << prismVolumeInfo.mirrorState << '\n';
        cout << "sourceLUNNames: " << '\n';
        std::list<std::string>::const_iterator srciter = prismVolumeInfo.sourceLUNNames.begin();
        for ( /* empty */ ; srciter != prismVolumeInfo.sourceLUNNames.end(); srciter++)
        {
            cout << *srciter << '\n';
        }
        cout << "applianceTargetLUNNumber: " << prismVolumeInfo.applianceTargetLUNNumber << '\n';
        cout << "applianceTargetLUNName: " << prismVolumeInfo.applianceTargetLUNName << '\n';
        cout << "physicalInitiatorPWWNs: " << '\n';
        std::list<std::string>::const_iterator piter = prismVolumeInfo.physicalInitiatorPWWNs.begin();
        for ( /* empty */ ; piter != prismVolumeInfo.physicalInitiatorPWWNs.end(); piter++)
        {
            cout << *piter << '\n';
        }
        cout << "applianceTargetPWWNs: " << '\n';
        std::list<std::string>::const_iterator atiter = prismVolumeInfo.applianceTargetPWWNs.begin();
        for ( /* empty */ ; atiter != prismVolumeInfo.applianceTargetPWWNs.end(); atiter++)
        {
            cout << *atiter << '\n';
        }
        cout << "sourceLUNStartOffset: " << prismVolumeInfo.sourceLUNStartOffset << '\n';
    }
}

void TestGetInitialSettings::
PrintATLUNStateRequest(const VOLUME_SETTINGS &vs)
{
    cout << "ATLUN_STATS_REQUEST :\n"
         << "    type: " << vs.atLunStatsRequest.type << "\n"
         << "    atLUNName: " << vs.atLunStatsRequest.atLUNName << "\n"
         << "    physicalInitiatorPWWNs:\n";
    std::list<std::string>::const_iterator pwwnstartiter = vs.atLunStatsRequest.physicalInitiatorPWWNs.begin();
    std::list<std::string>::const_iterator pwwnenditer = vs.atLunStatsRequest.physicalInitiatorPWWNs.end();
    for (std::list<std::string>::const_iterator iter = pwwnstartiter; iter != pwwnenditer; iter++)
    {
        cout << "        " << *iter << '\n';
    }
}

void TestGetInitialSettings::
PrintOptions(const VOLUME_SETTINGS &vs)
{
    cout << "options:\n";
         
	for (std::map<std::string, std::string>::const_iterator it = vs.options.begin(); it != vs.options.end(); it++)
	{
		cout << it->first << " --> " << it->second << '\n';
	}

}

LocalConfigurator& TestGetInitialSettings::
getLocalConfigurator()
{
    return m_localConfigurator;
}
