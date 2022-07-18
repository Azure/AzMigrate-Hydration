#include "Globals.h"
#include "Localconfigurator.h"
#include "talwrapper.h"
#include "Cxproxy.h"
#include "initialsettings.h"
#include "configurator2.h"
#include "GetInitialSettings.h"

#define PRINT(x) os<< #x "=" << x << endl;
#define TRACE(s) os<< #s << endl;
#define TRACELN(s) os<< #s << endl << endl;

#define os std::cout

extern struct GBLS gbls;
extern Configurator* TheConfigurator;


string CDP_DIRECTION_STRING[] = { "UNSPECIFIED", "SOURCE", "TARGET" };
string SYNC_MODE_STRING[] =  { "SYNC_DIFF", "SYNC_OFFLOAD", "SYNC_FAST", "SYNC_SNAPSHOT", "SYNC_DIFF_SNAPSHOT" };

TestGetInitialSettings::
TestGetInitialSettings() : 
    dpc(marshalCxCall("::getVxAgent",localconf.getHostId())),
    tal( localconf.getHttp().ipAddress )
{
    initialsettings = Test_getInitialSettings();
}

InitialSettings TestGetInitialSettings::
 Test_getInitialSettings()
{
    senddata = marshalCxCall( dpc, "getInitialSettings" );
    response = tal(senddata);
    return unmarshal<InitialSettings>( response );
}


void TestGetInitialSettings::
Print()
{
    system("cls");
    TRACE(FTP_CONNECTION_SETTINGS:);
    os<< "ipAddress: " << initialsettings.ftpSettings.ipAddress << "\t"
        << "password: " << initialsettings.ftpSettings.password << "\t"
        << "port: " << initialsettings.ftpSettings.port << endl << endl;

    TRACELN(CDP_SETTINGS:);
    PrintCdpsettings();
    TRACELN(HOST_VOLUME_GROUP_SETTINGS:);
    PrintHostVolumeGroupSettings();
    PRINT(initialsettings.resyncRequired);
    PRINT(initialsettings.shouldThrottle);
  }

  
void TestGetInitialSettings::
PrintCdpsettings()
  {
    typedef  std::map<std::string,CDP_SETTINGS>::iterator CDPSETTINGSITER;
    for(CDPSETTINGSITER iter = initialsettings.cdpSettings.begin();
        iter != initialsettings.cdpSettings.end();++iter)
    {
        os<< iter->first << "  -----  \n";
        CDP_SETTINGS cdpsettings  =iter->second;

        os<< "CDP_STATE: " << cdpsettings.m_state << "\n"
            <<  "CDP_LOGTYPE: " << cdpsettings.m_type << "\n"
            << "m_metadir: " << cdpsettings.m_metadir << "\n"
            << "m_dbname: " << cdpsettings.m_dbname << "\n"
            << "m_altmetadir: " << cdpsettings.m_altmetadir << "\n"
            << "CDP_COMPRESSION_METHOD: " << cdpsettings.m_compress << "\n"
            << "CDP_DELETE_OPTION: " << cdpsettings.m_deleteoption <<  endl << endl;
    }
  }


  void TestGetInitialSettings::
  PrintHostVolumeGroupSettings()
  {
      typedef std::list<VOLUME_GROUP_SETTINGS>::iterator VGITER;
      typedef std::map<std::string,VOLUME_SETTINGS>::iterator VITER;

      for(VGITER vgiter = initialsettings.hostVolumeSettings.volumeGroups.begin();
          vgiter != initialsettings.hostVolumeSettings.volumeGroups.end(); ++vgiter)
      {
          VOLUME_GROUP_SETTINGS vg = *vgiter;
          os<< "Direction: " << CDP_DIRECTION_STRING[vg.direction] << "\t" << "Id: " 
              << vg.id << "\t" << "status: " << vg.status << endl;
          TRACELN(VOLUME_SETTINGS:);
          for(VITER viter = vg.volumes.begin();viter != vg.volumes.end(); ++viter)
         {
             TRACELN(VOLUMES:);
             os<< viter->first << endl;
             VOLUME_SETTINGS vs = viter->second;

             os<< "deviceName: " << vs.deviceName << "\n" 
                 << "hostname: " << vs.hostname << "\n"
                 << "hostId: " << vs.hostId << "\n"
                 << "secureMode: " << vs.secureMode << "\n"
                 << "sourceToCXSecureMode: " << vs.sourceToCXSecureMode << "\n"
                 << "synchmode: " << SYNC_MODE_STRING[vs.syncMode] << "\n"
                 <<"transportProtocol: " << vs.transportProtocol << "\n"
                 << "visibility: " << vs.visibility << "\n"
                 << "sourcecapacity: " << vs.sourcecapacity << "\n"
                 << "resyncDirectory: " << vs.resyncDirectory << endl << endl;
         }
      }

  }

  LocalConfigurator& TestGetInitialSettings::
  getLocalConfigurator()
  {
      return localconf;
  }

void Test_GetInitialSettings()
{
   TestGetInitialSettings tis;
   tis.Print();
   cout << typeid(TheConfigurator->getVxAgent()).name();
   //TheConfigurator->getCxAgent().RegisterHost()
}

void Test_GetVxAgent()
{
   cout << typeid(TheConfigurator->getVxAgent()).name() << endl;
   cout << "HostId: " << TheConfigurator->getVxAgent().getHostId() << endl;
   cout << "getDiffSourceExePathname: " << TheConfigurator->getVxAgent().getDiffSourceExePathname() << endl;
   cout << "getDiffTargetFilenamePrefix: " << TheConfigurator->getVxAgent().getDiffTargetFilenamePrefix() << endl;
   cout << "getDiffTargetCacheDirectoryPrefix: " << TheConfigurator->getVxAgent().getDiffTargetCacheDirectoryPrefix() << endl;
   cout << "getDiffTargetSourceDirectoryPrefix: " << TheConfigurator->getVxAgent().getDiffTargetSourceDirectoryPrefix() << endl;

   //TheConfigurator->getCxAgent().RegisterHost()
}

