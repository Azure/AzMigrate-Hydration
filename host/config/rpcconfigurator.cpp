//
// rpcconfigurator.cpp: define concrete configurator
//
#include <iostream>
#include <fstream>
#include <ace/Guard_T.h>
#include <ace/Mutex.h>
#include "rpcconfigurator.h"
#include "localconfigurator.h"
#include "logger.h"
#include "talwrapper.h"
#include "inmageex.h"
#include <ace/File_Lock.h>
#include "boost/shared_array.hpp"
#include "portablehelpersmajor.h"
#include "serializeinitialsettings.h"
#include "inmsafeint.h"
#include "inmageex.h"

using namespace std;

/*
* FUNCTION NAME : RpcConfigurator::RpcConfigurator
*
* DESCRIPTION : constructor.
*
* INPUT PARAMETERS :
*
* useCachedSettings - if set, get settings from local
* persistent store.otherwise they are fetched from CX
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : none
*
*/
RpcConfigurator::RpcConfigurator(const SerializeType_t serializetype, 
                                 ConfigSource configSource,
                                 const std::string& cachepath)

                                 : m_QuitEvent(NULL),
                                 m_configSource(configSource),
                                 m_serializeType(serializetype),
                                 m_apiWrapper(m_serializeType),
                                 m_cx( m_localConfigurator.getHostId(),
                                 m_apiWrapper,
                                 m_localConfigurator,
                                 m_lockSettings,
                                 m_settings,
                                 m_snapShotRequests ,
                                 m_serializeType ),
                                 m_lastTimeCacheUpdated(0),
                                 m_started(false),
                                 m_fileName(cachepath)
{
    // GetCurrentSettings();
    m_QuitEvent = new (nothrow) ACE_Event(1,0);
}

/*
* FUNCTION NAME : RpcConfigurator::RpcConfigurator
*
* DESCRIPTION : constructor.
*
* INPUT PARAMETERS :
*
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : none
*
*/

RpcConfigurator::RpcConfigurator( const SerializeType_t serializetype, std::string const& ipAddress, 
                                 int port, std::string const& hostId)
                                 : m_serializeType(serializetype),
                                 m_apiWrapper( ipAddress, port, m_serializeType),
                                 m_QuitEvent(NULL),
                                 m_configSource(FETCH_SETTINGS_FROM_CX),
                                 m_cx( hostId,
                                 m_apiWrapper,
                                 m_localConfigurator,
                                 m_lockSettings,
                                 m_settings,
                                 m_snapShotRequests ,
                                 m_serializeType ),
                                 m_lastTimeCacheUpdated(0),
                                 m_started(false),
                                 m_fileName("")
{
    //GetCurrentSettings();
    m_QuitEvent = new (nothrow) ACE_Event(1,0);
}
//#15949
/*
* FUNCTION NAME : RpcConfigurator::RpcConfigurator
*
* DESCRIPTION : constructor.
*
* INPUT PARAMETERS :
*
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : none
*
*/

RpcConfigurator::RpcConfigurator( const SerializeType_t serializetype, std::string const& fileName)
: m_serializeType(serializetype),
m_apiWrapper(m_serializeType),
m_QuitEvent(NULL),
m_configSource(USE_FILE_PROVIDED),
m_cx( m_localConfigurator.getHostId(),
     m_apiWrapper,
     m_localConfigurator,
     m_lockSettings,
     m_settings,
     m_snapShotRequests,
     m_serializeType),
     m_lastTimeCacheUpdated(0),
     m_started(false),
     m_fileName(fileName)
{
    //GetCurrentSettings();
    m_QuitEvent = new (nothrow) ACE_Event(1,0);
}

/*
* FUNCTION NAME : RpcConfigurator::~RpcConfigurator
*
* DESCRIPTION : destructor
*   stop the thread
*   delete quit event object
*
* INPUT PARAMETERS : none
*
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : none
*
*/
RpcConfigurator::~RpcConfigurator()
{
    Stop();
    delete m_QuitEvent;
}

/*
* FUNCTION NAME : RpcConfigurator::Start
*
* DESCRIPTION :
*   start a thread which polls for replication settings
*   at regular interval
*
*
* INPUT PARAMETERS : none
*
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : none
*
*/
void RpcConfigurator::Start()
{
    if(!m_started)
    {
        m_QuitEvent->reset();
        m_threadManager.spawn( ThreadFunc, this );
        m_started = true;
    }
}

/*
* FUNCTION NAME : RpcConfigurator::Stop
*
* DESCRIPTION :
*   stop all running configurator threads
*
*
* INPUT PARAMETERS : none
*
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : none
*
*/
void RpcConfigurator::Stop()
{
    m_threadManager.cancel_all();
    m_QuitEvent->signal();
    m_threadManager.wait();
    m_started = false;
}

/*
* FUNCTION NAME : RpcConfigurator::ThreadFunc
*
* DESCRIPTION :
*   fetch initial settings from cx/persistent store
*   every 1 min.
*
*
* INPUT PARAMETERS : none
*
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : success, if thread was run sucessfully.
*
*/
ACE_THR_FUNC_RETURN RpcConfigurator::ThreadFunc( void* arg )
{
    RpcConfigurator* p = static_cast<RpcConfigurator*>( arg );
    return p->run();
}


/*
* FUNCTION NAME : RpcConfigurator::run
*
* DESCRIPTION :
*   fetch initial settings from cx/persistent store
*   every 1 min.If the cx is down continously for
*   specified timeout period, raise a signal to
*   switch to remote cx.
*
*
* INPUT PARAMETERS : none
*
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : none
*
*/
ACE_THR_FUNC_RETURN RpcConfigurator::run()
{
    SV_ULONG timeoutInSecs = getFailoverTimeout();
    int initialSettingCallInterval = m_localConfigurator.getInitialSettingCallInterval();
    if(initialSettingCallInterval <= 0)
        initialSettingCallInterval = 60;

    while( true )
    {
        DebugPrintf(SV_LOG_DEBUG,"Configurator acquiring new settings. wait..\n");

        ACE_Time_Value waitTime;
        waitTime.set(initialSettingCallInterval,0);
        m_QuitEvent->wait(&waitTime,0);
        if( m_threadManager.testcancel( ACE_Thread::self() ) )
        {
            break;
        }

        GetCurrentSettingsAndOperate();
    }

    return 0;
}
bool RpcConfigurator::BackupAgentPause(CDP_DIRECTION direction, const std::string& devicename)
{
	return m_cx.getVxAgent().BackupAgentPause(direction, devicename) ;
}
bool RpcConfigurator::BackupAgentPauseTrack(CDP_DIRECTION direction, const std::string& devicename)
{
	 return m_cx.getVxAgent().BackupAgentPauseTrack(direction, devicename) ;
}
bool RpcConfigurator::AnyPendingEvents()
{
	return m_cx.getVxAgent().AnyPendingEvents() ;
}
void RpcConfigurator::GetCurrentSettingsAndOperate(void)
{
    DebugPrintf(SV_LOG_DEBUG,"Configurator acquiring new settings. wait..\n");
    SV_ULONG timeoutInSecs;

    try
    {
        timeoutInSecs = m_settings.timeoutInSecs;

        /* TODO: why this ? 
        SetLogLevel( m_localConfigurator.getLogLevel() );
        SetLogRemoteLogLevel( m_localConfigurator.getRemoteLogLevel() );
		*/

        GetCurrentSettings();

    }
    catch(...)
    {
    }

    // raise signal when
    // i. fetch_from_cx  or
    // ii. fetch_from_cache_as_cx_not_available

    if((m_configSource == FETCH_SETTINGS_FROM_CX)
        || (m_configSource == USE_CACHE_SETTINGS_IF_CX_NOT_AVAILABLE))
    {
        //transient failure
        ACE_Time_Value cur_time = ACE_OS::gettimeofday();

        //      Bug #9034
        //      To take care when the system date moves back (currenttime < lastfetchedtime)


        if(((SV_ULONG)(cur_time.sec() - m_lastTimeSettingsFetchedFromCx.sec()) > timeoutInSecs)
            || (cur_time.sec() < m_lastTimeSettingsFetchedFromCx.sec()))
        {
            if(remoteCxsReachable())
            {
                m_FailoverCxSignal(m_ReachableCx);
            }
        }
    }
}


void RpcConfigurator::GetCurrentSettings()
{
    getCurrentInitialSettings();
}

/*
* FUNCTION NAME :  RpcConfigurator::getCachedInitialSettings
*
* DESCRIPTION : fetch the initial settings from local persistent store.
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : InitialSettings
*
* NOTES :
*
*
* return value : none
*
*/

InitialSettings RpcConfigurator::getCachedInitialSettings(const std::string& cachePath)
{
    std::ifstream is;
    unsigned int length =0;

    std::string lockPath;


    //cannot use getLongPathName() from common module. Causes switchagent build failure
#if defined (SV_WINDOWS) && defined (SV_USES_LONGPATHS)
    // PR#10815: Long Path support
    lockPath = "\\\\?\\" + cachePath + ".lck";
#else
    lockPath = cachePath + ".lck";
#endif

    // acquire the lock
    ACE_File_Lock lock(ACE_TEXT_CHAR_TO_TCHAR(lockPath.c_str()), O_CREAT | O_RDWR, SV_LOCK_PERMISSIONS,0);
    lock.acquire_read();

    // open the local store for read operation
    is.open(cachePath.c_str(), std::ios::in | std::ios::binary);
    if (!is)
    {
        return unmarshal<InitialSettings> (""); // throw an exception
    }

    // get length of file
    is.seekg (0, ios::end);
    length = is.tellg();
    is.seekg (0, ios::beg);

    // allocate memory:
    size_t buflen;
    INM_SAFE_ARITHMETIC(buflen = InmSafeInt<unsigned int>::Type(length) + 1, INMAGE_EX(length))
    boost::shared_array<char> buffer(new char[buflen]);
    if(!buffer)
    {
        return unmarshal<InitialSettings> (""); // throw an exception
    }

    // read initialsettings as binary data
    is.read (buffer.get(),length);
    *(buffer.get() + length) = '\0';
    // close the handle
    is.close();
    lock.release();

    // for debugging, remove after unit testing
    //cout.write (buffer.get(),length);

    return unmarshal<InitialSettings> (buffer.get());
}

/*
* FUNCTION NAME :  RpcConfigurator::getInitialSettingsFromFile
*
* DESCRIPTION : fetch the initial settings from the give settings file.
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : InitialSettings
*
* NOTES :
*
*
* return value : none
*
*/

InitialSettings RpcConfigurator::getInitialSettingsFromFile()
{
    std::ifstream is;
    unsigned int length =0;

    // open the local store for read operation
    is.open(m_fileName.c_str(), std::ios::in | std::ios::binary);
    if (!is)
    {
        return unmarshal<InitialSettings> (""); // throw an exception
    }

    // get length of file
    is.seekg (0, ios::end);
    length = is.tellg();
    is.seekg (0, ios::beg);

    // allocate memory:
    size_t buflen;
    INM_SAFE_ARITHMETIC(buflen = InmSafeInt<unsigned int>::Type(length) + 1, INMAGE_EX(length))
    boost::shared_array<char> buffer(new char[buflen]);
    if(!buffer)
    {
        return unmarshal<InitialSettings> (""); // throw an exception
    }

    // read initialsettings as binary data
    is.read (buffer.get(),length);
    *(buffer.get() + length) = '\0';
    // close the handle
    is.close();

    // for debugging, remove after unit testing
    //cout.write (buffer.get(),length);

    return unmarshal<InitialSettings> (buffer.get());
}


/*
* FUNCTION NAME :  RpcConfigurator::getCurrentInitialSettings
*
* DESCRIPTION : fetch the current settings from local persistent store or cx.
*               if settings are changed, issue appropriate signals
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : none
*
*/
void RpcConfigurator::getCurrentInitialSettings()
{
    InitialSettings CurrentCxSettings;

    bool initialsettingsChanged = false;
    bool hostVolumeSettingsChanged = false;
    bool prismSettingsChanged = false;
    bool ConfigurationChange = false;
    bool cdpSettingsChanged = false;
    std::string cachePath;
    LocalConfigurator::getConfigCachePathname(cachePath);
    if( !m_fileName.empty() )
    {
        cachePath = m_fileName ;
    }
    if(m_configSource == USE_ONLY_CACHE_SETTINGS)
    {
        // Retrieve the cached settings only when the file
        // config.dat is modified (to reduce disk i/o)

        time_t FetchedCacheTime;
        CurrentCxSettings = m_settings;

        ACE_stat stat = {0};
        ACE_OS::stat(cachePath.c_str(), &stat);
        FetchedCacheTime = stat.st_mtime;

        // Read cache settings when the fils is modified

        // Bug #9034
        // To take care when the system time moves backward

        if(FetchedCacheTime != m_lastTimeCacheUpdated)
        {
            CurrentCxSettings = getCachedInitialSettings(cachePath);
            m_lastTimeCacheUpdated = FetchedCacheTime;
        }
    }
    else if(m_configSource == FETCH_SETTINGS_FROM_CX
        || m_configSource == USE_CACHE_SETTINGS_IF_CX_NOT_AVAILABLE)
    {
        try
        {
            CurrentCxSettings = m_cx.getInitialSettings();
            m_lastTimeSettingsFetchedFromCx = ACE_OS::gettimeofday();
        }
        catch (ContextualException& e)
        {
          DebugPrintf(SV_LOG_ERROR, "Caught Exception:%s \n",e.what());
          if(m_configSource == USE_CACHE_SETTINGS_IF_CX_NOT_AVAILABLE)
          {
            CurrentCxSettings = getCachedInitialSettings(cachePath);
          }
          else
          {
            DebugPrintf(SV_LOG_ERROR,
              "Failed in retrieving current InitialSettings.\n");
            throw;
          }
        }
        catch ( ... )
        {
          DebugPrintf(SV_LOG_ERROR, "Caught unknown exception\n");
          if(m_configSource == USE_CACHE_SETTINGS_IF_CX_NOT_AVAILABLE)
          {
            CurrentCxSettings = getCachedInitialSettings(cachePath);
          }
          else
          {
            DebugPrintf(SV_LOG_ERROR,
              "Failed in retrieving current InitialSettings.\n");
            throw;
          }
        }
    }
    //15949: Retrieves the initial settings from the file provided.
    else if(m_configSource == USE_FILE_PROVIDED)
    {
        try
        {
            CurrentCxSettings = getInitialSettingsFromFile();
        }
        catch ( ... )
        {
            DebugPrintf(SV_LOG_ERROR, "Failed in retrieving "
                "current InitialSettings from the given file.\n");
            throw;
        }
    }

    if (!(m_settings == CurrentCxSettings) )
    {
        initialsettingsChanged = true;
        ConfigurationChange = true;
    }
    else if( !StrictCompare(m_settings, CurrentCxSettings) )
    {
        ConfigurationChange = true;
    }

    if (!(m_settings.hostVolumeSettings == CurrentCxSettings.hostVolumeSettings) )
    {
        hostVolumeSettingsChanged = true;
    }

    if (!(m_settings.prismSettings == CurrentCxSettings.prismSettings))
    {
        prismSettingsChanged = true;
    }

	if( !(m_settings.cdpSettings == CurrentCxSettings.cdpSettings) )
    {
        cdpSettingsChanged = true;
    }

    if(ConfigurationChange)
    {
        updateSettings(CurrentCxSettings);
    }


    // We fire all the appropriate signals after our state is updated
    if(ConfigurationChange)
    {
        m_ConfigurationChangeSignal( CurrentCxSettings );
    }

    if(initialsettingsChanged)
    {
        m_InitialSettingsSignal( CurrentCxSettings );
    }

    if(hostVolumeSettingsChanged)
    {
        m_HostVolumeGroupSettingsSignal( CurrentCxSettings.hostVolumeSettings );
    }

    if (prismSettingsChanged)
    {
        m_PrismSettingsSignal( CurrentCxSettings.prismSettings );
    }

	if (CurrentCxSettings.shouldRegisterOnDemand())
	{
		m_RegisterImmediatelySignal(CurrentCxSettings.getRequestIdForOnDemandRegistration(), 
                                    CurrentCxSettings.getDisksLayoutOption());
	}

    if (!IsAgentRunningOnAzureVm())
    {
        std::string currentCsAddrForAzComponents = CurrentCxSettings.getCsAddressForAzureComponents();
        if (!currentCsAddrForAzComponents.empty())
        {
            m_CsAddressForAzureComponentsChangeSignal(m_localConfigurator, currentCsAddrForAzComponents);
        }
    }

    if(cdpSettingsChanged)
    {
        m_CdpsettingsChangeSignal();
    }
}

void RpcConfigurator::updateSettings(const InitialSettings &CurrentCxSettings)
{
    AutoGuard lock(m_lockSettings);
    m_settings = CurrentCxSettings;
}

std::string RpcConfigurator::getHostId()
{
    return m_localConfigurator.getHostId();
}

bool RpcConfigurator::gettargetvolumes(std::vector<std::string>& targetvollist)
{
    return m_settings.hostVolumeSettings.gettargetvolumes( targetvollist);
}

InitialSettings RpcConfigurator::getInitialSettings()
{
    AutoGuard lock(m_lockSettings);
    return m_settings;
}

std::list<CsJob> RpcConfigurator::getJobsToProcess()
{
    return m_cx.getJobsToProcess();
}

void RpcConfigurator::updateJobStatus(CsJob csJob)
{
    return m_cx.updateJobStatus(csJob);
}

HOST_VOLUME_GROUP_SETTINGS RpcConfigurator::getHostVolumeGroupSettings()
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings;
}

PRISM_SETTINGS RpcConfigurator::getPrismSettings()
{
    AutoGuard lock(m_lockSettings);
    return m_settings.prismSettings;
}


CDP_SETTINGS RpcConfigurator::getCdpSettings(const std::string & volname)
{
    AutoGuard lock(m_lockSettings);

	//first try to get the settings out using the volume name as key
	CDPSETTINGS_MAP::iterator iter = m_settings.cdpSettings.find(volname);
	if(iter != m_settings.cdpSettings.end())
		return iter -> second;

	// not found, walk through each entry and compare using the guid
    iter = m_settings.cdpSettings.begin();
    for( ; iter != m_settings.cdpSettings.end(); ++iter)
    {
        if(volequalitycmp()(iter -> first, volname))
            return iter -> second;
    }

    return CDP_SETTINGS();
}


string RpcConfigurator::getCdpDbName(const std::string & volname)
{
    AutoGuard lock(m_lockSettings);

	//first try to get the settings out using the volume name as key
	CDPSETTINGS_MAP::iterator iter = m_settings.cdpSettings.find(volname);
	if(iter != m_settings.cdpSettings.end())
	{
		if((iter ->second).Status() == CDP_ENABLED)
			return (iter -> second).Catalogue();
		else
			return "";
	}

	// not found, walk through each entry and compare using the guid
    iter = m_settings.cdpSettings.begin();
    for( ; iter != m_settings.cdpSettings.end(); ++iter)
    {
        if(volequalitycmp()(iter -> first, volname))
        {
            if((iter ->second).Status() == CDP_ENABLED)
                return (iter -> second).Catalogue();
            else
                return "";
        }
    }

    return "";

}

/*
* FUNCTION NAME :  RpcConfigurator::remoteCxsReachable
*
* DESCRIPTION : check if there is any viable remote cx to failover.
*
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : true if a remote cx is configured and reachable, false otherwise
*
*/
bool RpcConfigurator::remoteCxsReachable()
{
    bool rv = false;

    AutoGuard lock(m_lockSettings);
    REMOTECX_MAP::iterator iter = m_settings.remoteCxs.find("all");
    if(iter != m_settings.remoteCxs.end())
    {
        if(!(iter ->second).empty())
        {
            std::list<ViableCx>::iterator cxIter = iter->second.begin();
            for(; cxIter != iter->second.end(); ++cxIter)
            {
                if((rv = isCxReachable(cxIter->publicip, cxIter->port)) ||
                    (rv = isCxReachable(cxIter->privateip, cxIter->port)))
                    break;
            }
        }
    }
    return rv;
}

/*
* FUNCTION NAME :  RpcConfigurator::isCxReachable
*
* DESCRIPTION : Checks to see if the Cx at the specified location is reachable
*
*
* INPUT PARAMETERS : ipaddress, port
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : true if the cx is reachable, false otherwise
*
*/
bool RpcConfigurator::isCxReachable(const std::string& ip, const SV_UINT& port)
{
    bool rv = true;

    do
    {
        try
        {
            std::string     biosId("");
            GetBiosId(biosId);
            std::string dpc = marshalCxCall("::getVxAgent", m_localConfigurator.getHostId(), biosId);

			TalWrapper tal(ip,port,m_localConfigurator.IsHttps());
            std::string debug = tal(marshalCxCall(dpc,"getInitialSettings",INITIAL_SETTINGS_VERSION));
            InitialSettings settings = unmarshal<InitialSettings>( debug );

            m_ReachableCx.ip = ip;
            m_ReachableCx.port = port;
        }
        catch ( ContextualException& ce )
        {
            rv = false;
            DebugPrintf(SV_LOG_ERROR,
                "\n%s encountered exception %s.\n",
                FUNCTION_NAME, ce.what());
        }
        catch( exception const& e )
        {
            rv = false;
            DebugPrintf(SV_LOG_ERROR,
                "\n%s encountered exception %s.\n",
                FUNCTION_NAME, e.what());
        }
        catch ( ... )
        {
            rv = false;
            DebugPrintf(SV_LOG_ERROR,
                "\n%s encountered unknown exception.\n",
                FUNCTION_NAME);
        }
    } while(0);

    return rv;
}


/*
* FUNCTION NAME :  RpcConfigurator::getViableCxs
*
* DESCRIPTION : get the list of viable remote cx's to failover.
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : true if a remote cx is configured, false otherwise
*
*/
bool RpcConfigurator::getViableCxs(ViableCxs & viablecxs)
{
    AutoGuard lock(m_lockSettings);
    REMOTECX_MAP::iterator iter = m_settings.remoteCxs.find("all");
    if(iter != m_settings.remoteCxs.end())
    {
        viablecxs.assign((iter ->second).begin(), (iter ->second).end());
        return true;
    }

    return false;
}

/*
* FUNCTION NAME :  RpcConfigurator::getFailoverTimeout
*
* DESCRIPTION :
*  get the failover timeout in secs.
*
*  usage:
*  if the current cx is not accessible for timeout period,
*  a signal to switch over to remote cx will be raised by
*  configurator.
*
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : failover time out in secs
*
*/
SV_ULONG RpcConfigurator::getFailoverTimeout()
{
    AutoGuard lock(m_lockSettings);
    return m_settings.timeoutInSecs;
}

/*
* FUNCTION NAME :  RpcConfigurator::getFailoverCxSignal
*
* DESCRIPTION :
*  signal slot for notification when local cx is not
*  reachable.
*
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : sigslot to connect
*
*/
sigslot::signal1<ReachableCx>& RpcConfigurator::getFailoverCxSignal()
{
    return m_FailoverCxSignal;
}

/*
* FUNCTION NAME :  RpcConfigurator::containsRetentionFiles
*
* DESCRIPTION : check if specified volume contains retention files
*
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : true on success, otherwise false
*
*/
bool RpcConfigurator::containsRetentionFiles(const std::string & volname) const
{
    AutoGuard lock(m_lockSettings);
    bool rv = false;

    std::string s1 = volname;

    // Convert the volume name to a standard format
    FormatVolumeName(s1);
    FormatVolumeNameToGuid(s1);
    ExtractGuidFromVolumeName(s1);

    // convert to standard device name if symbolic link
    // was passed as argument

    if (IsReportingRealNameToCx())
    {
        GetDeviceNameFromSymLink(s1);
    }

    CDPSETTINGS_MAP::const_iterator iter = m_settings.cdpSettings.begin();
    CDPSETTINGS_MAP::const_iterator iterEnd = m_settings.cdpSettings.end();
    for( ; iter != iterEnd; ++iter)
    {
        std::string dbname = (iter ->second).Catalogue();
        if(dbname == "")
            continue;

        std::string mtpt,dbdevname;
        GetVolumeRootPath(dbname,mtpt);
        GetDeviceNameForMountPt(mtpt,dbdevname);

        // Convert the volume name to a standard format
        FormatVolumeName(dbdevname);
        FormatVolumeNameToGuid(dbdevname);
        ExtractGuidFromVolumeName(dbdevname);

        // convert to standard device name if symbolic link
        // was passed as argument

        if (IsReportingRealNameToCx())
        {
            GetDeviceNameFromSymLink(dbdevname);
        }

#ifdef SV_UNIX
        if(!strcmp(s1.c_str(), dbdevname.c_str()))
#else
        if(!stricmp(s1.c_str(), dbdevname.c_str()))
#endif
        {
            rv = true;
            break;
        }
    }

    return rv;
}

sigslot::signal1<HOST_VOLUME_GROUP_SETTINGS>& RpcConfigurator::getHostVolumeGroupSignal()
{
    return m_HostVolumeGroupSettingsSignal;
}

sigslot::signal1<PRISM_SETTINGS>& RpcConfigurator::getPrismSettingsSignal()
{
    return m_PrismSettingsSignal;
}

sigslot::signal2<std::string, std::string>& RpcConfigurator::getRegisterImmediatelySignal()
{
    return m_RegisterImmediatelySignal;
}

sigslot::signal1<InitialSettings>& RpcConfigurator::getInitialSettingsSignal()
{
    return m_InitialSettingsSignal;
}

sigslot::signal1<InitialSettings>& RpcConfigurator::getConfigurationChangeSignal()
{
    return m_ConfigurationChangeSignal;
}

sigslot::signal0<> &RpcConfigurator::getCdpSettingsSignal()
{
    return m_CdpsettingsChangeSignal;
}

sigslot::signal2<LocalConfigurator&, std::string> &RpcConfigurator::getCsAddressForAzureComponentsChangeSignal()
{
    return m_CsAddressForAzureComponentsChangeSignal;
}

ConfigureSnapshotManager& RpcConfigurator::getSnapshotManager()
{
    return m_cx.getSnapshotManager();
}

ConfigureVxAgent& RpcConfigurator::getVxAgent()
{
    return m_cx.getVxAgent();
}

ConfigureVxTransport& RpcConfigurator::getVxTransport()
{
    return getVxAgent().getVxTransport();
}

ConfigureVolumeManager& RpcConfigurator::getVolumeManager()
{
    return m_cx.getVolumeManager();
}

ConfigureReplicationPair& RpcConfigurator::getReplicationPair( int id )
{ throw INMAGE_EX("not impl"); }
ConfigureReplicationPairManager& RpcConfigurator::getReplicationPairManager()
{ throw INMAGE_EX("not impl"); }
ConfigureVolumeGroups& RpcConfigurator::getVolumeGroups()
{ throw INMAGE_EX("not impl"); }
ConfigureVolume& RpcConfigurator::getVolume( string deviceName )
{ throw INMAGE_EX("not impl"); }
ConfigureSnapshot& RpcConfigurator::getSnapshot( string deviceName )
{ throw INMAGE_EX("not impl"); }
ConfigureClusterManager& RpcConfigurator::getClusterManager()
{ throw INMAGE_EX("not impl"); }
ConfigureRetentionManager& RpcConfigurator::getRetentionManager()
{ throw INMAGE_EX("not impl"); }
ConfigureServiceManager& RpcConfigurator::getServiceManager()
{ throw INMAGE_EX("not impl"); }
ConfigureService& RpcConfigurator::getService( string serviceName )
{ throw INMAGE_EX("not impl"); }

HTTP_CONNECTION_SETTINGS RpcConfigurator::getHttpSettings()
{
    AutoGuard lock(m_lockSettings);
    HTTP_CONNECTION_SETTINGS s = m_localConfigurator.getHttp();
    if (IsAgentRunningOnAzureVm())
    {
        inm_strcpy_s(s.ipAddress, ARRAYSIZE(s.ipAddress), m_localConfigurator.getCsAddressForAzureComponents().c_str());
    }
    return s;
}

void RpcConfigurator::updateHttpSettings(const HTTP_CONNECTION_SETTINGS&)
{
    throw INMAGE_EX("not impl");
}

TRANSPORT_CONNECTION_SETTINGS RpcConfigurator::getTransportSettings(int volGrpId)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getTransportSettings(volGrpId);
}

TRANSPORT_CONNECTION_SETTINGS RpcConfigurator::getTransportSettings(const std::string & deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.getTransportSettings(deviceName);
}

/*
bool RpcConfigurator::shouldThrottle() const {
AutoGuard lock(m_lockSettings);
return m_settings.shouldThrottle;
}
*/

ConfigureCxAgent& RpcConfigurator::getCxAgent()
{
    return m_cx;
}

int RpcConfigurator::getVolumeAttribute(string deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getVolumeAttributeChangeSettings(deviceName);
}

string RpcConfigurator::getInitialSyncDirectory(string deviceName)
{
    AutoGuard lock(m_lockSettings);
    // Returns empty string if sync directory not available
    return m_settings.hostVolumeSettings.getSyncDirectory(deviceName);
}


/*
bool RpcConfigurator::CheckThrottle(std::string deviceName,THRESHOLD_AGENT thresholdAgent,
THROTTLE_STATUS* isThrottled)
{
AutoGuard lock(m_lockSettings);
return m_settings.getThrottleStatus(deviceName,thresholdAgent,isThrottled);
}
*/

/*
* FUNCTION NAME :  RpcConfigurator::isProtectedVolume
*
* DESCRIPTION : check if the specified volume is a protected (filtered) volume
*
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : true on success, otherwise false
*
*/
bool RpcConfigurator::isProtectedVolume(const std::string & deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.isProtectedVolume(deviceName);
}

bool RpcConfigurator::isTargetVolume(std::string deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.isTargetVolume(deviceName);
}

bool RpcConfigurator::isResyncing(string deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.isResyncing(deviceName);
}

bool RpcConfigurator::isInResyncStep1(string deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.isInResyncStep1(deviceName);
}

int RpcConfigurator::getRpoThreshold(string deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getRpoThreshold(deviceName);
}

VOLUME_SETTINGS::PAIR_STATE RpcConfigurator::getPairState(const std::string & deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getPairState(deviceName);
}
/*
* FUNCTION NAME :  RpcConfigurator::shouldOperationRun
*
* DESCRIPTION : Determine whether a cdpcli operation, process like cachemgr, dp, s2
*                                       will run or not on a specific volume
*
* INPUT PARAMETERS : std::string volume name, std::string operationName
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : false if the operation should not run otherwise true
*
*/
bool RpcConfigurator::shouldOperationRun(const std::string & driveName, const std::string & operationName)
{
    bool rv = true;
    AutoGuard lock(m_lockSettings);
    for(HOST_VOLUME_GROUP_SETTINGS::volumeGroups_iterator vgiter = m_settings.hostVolumeSettings.volumeGroups.begin();
        vgiter != m_settings.hostVolumeSettings.volumeGroups.end();++vgiter)
    {
        VOLUME_GROUP_SETTINGS vg = *vgiter;
        VOLUME_GROUP_SETTINGS::volumes_iterator ctVolumeIter =  vg.find_by_guid(driveName);
        if (ctVolumeIter == vg.volumes.end())
        {
            continue;
        }
        return ctVolumeIter->second.shouldOperationRun(operationName);
    }
    return rv;
}

SV_ULONGLONG RpcConfigurator::getDiffsPendingInCX(string deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getDiffsPendingInCX(deviceName);
}

SV_ULONGLONG RpcConfigurator::getCurrentRpo(string deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getCurrentRpo(deviceName);
}

SV_ULONGLONG RpcConfigurator::getApplyRate(string deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getApplyRate(deviceName);
}

SV_ULONGLONG RpcConfigurator::getResyncCounter(std::string deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getResyncCounter(deviceName);
}

std::string RpcConfigurator::getSourceHostIdForTargetDevice(const std::string& deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getSourceHostIdForTargetDevice(deviceName);
}

std::string RpcConfigurator::getSourceVolumeNameForTargetDevice(const std::string& deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getSourceVolumeNameForTargetDevice(deviceName);
}

OS_VAL RpcConfigurator::getSourceOSVal(const std::string &deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getSourceOSVal(deviceName);
}

bool RpcConfigurator::isSourceFullDevice(const std::string & deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.isSourceFullDevice(deviceName);
}

SV_ULONG RpcConfigurator::getOtherSiteCompartibilityNumber(string deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getOtherSiteCompartibilityNumber(deviceName);
}

SNAPSHOT_REQUESTS RpcConfigurator::getSnapshotRequests(const std::string & vol, const svector_t & bookmarks) const
{
    // This will fetch the latest data from cx.
    std::string cxVolName = vol;
    FormatVolumeNameForCxReporting(cxVolName);
    SNAPSHOT_REQUESTS snapShotRequests = m_cx.getSnapshotRequestFromCx(cxVolName);

    //AutoGuard lock( m_lock );
    SNAPSHOT_REQUESTS requests;

    SNAPSHOT_REQUESTS::iterator iter_end = snapShotRequests.end();
    for(SNAPSHOT_REQUESTS::iterator iter= snapShotRequests.begin(); iter != iter_end; ++iter)
    {
        std::string id = (*iter).first;
        SNAPSHOT_REQUEST::Ptr ptr = (*iter).second;
        FormatVolumeName(ptr -> src);
        FormatVolumeName(ptr -> dest);
#ifdef SV_UNIX
        if(! strcmp(ptr -> src.c_str(),vol.c_str()) )   

#else
        if(! stricmp(ptr -> src.c_str(),vol.c_str()) )   
#endif

        {
            svector_t::iterator prefix_iter = ptr ->bookmarks.begin();
            svector_t::iterator prefix_end = ptr ->bookmarks.end();

            bool found = false;

            if(bookmarks.empty()) {
                // return only time based requests
                if( ptr -> bookmarks.empty() )
                    found = true;
            } else      {

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
                if(ptr -> operation == SNAPSHOT_REQUEST::ROLLBACK)      {
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

//#15949 
SNAPSHOT_REQUESTS RpcConfigurator::getSnapshotRequests()
{
    // This will fetch the latest data from cx.
    return m_cx.getSnapshotRequestFromCx();
}

// This function is added to get recent settings (not cached settings)


std::map<std::string, std::string> RpcConfigurator::getReplicationPairInfo(const std::string & sourceHost)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getReplicationPairInfo(sourceHost);
}

std::map<std::string, std::string> RpcConfigurator::getSourceVolumeDeviceNames(const std::string & targetHost)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getSourceVolumeDeviceNames(targetHost);
}

SV_ULONGLONG RpcConfigurator::getResyncStartTimeStamp(const std::string &deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getResyncStartTimeStamp(deviceName);
}

SV_ULONGLONG RpcConfigurator::getResyncEndTimeStamp(const std::string &deviceName) 
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getResyncEndTimeStamp(deviceName);
}

SV_ULONG RpcConfigurator::getResyncStartTimeStampSeq(const std::string &deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getResyncStartTimeStampSeq(deviceName);
}

SV_ULONG RpcConfigurator::getResyncEndTimeStampSeq(const std::string &deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getResyncEndTimeStampSeq(deviceName);
}

TRANSPORT_PROTOCOL RpcConfigurator::getProtocol(const std::string &deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getProtocol(deviceName);
}

bool RpcConfigurator::getSecureMode(const std::string &deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getSecureMode(deviceName);
}

bool RpcConfigurator::isInQuasiState(const std::string &deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.isInQuasiState(deviceName);
}

bool RpcConfigurator::isResyncRequiredFlag(const std::string &deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.isResyncRequiredFlag(deviceName);
}

SV_ULONGLONG RpcConfigurator::getResyncRequiredTimestamp(const std::string &deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getResyncRequiredTimestamp(deviceName);
}

VOLUME_SETTINGS::RESYNCREQ_CAUSE RpcConfigurator::getResyncRequiredCause(const std::string &deviceName)
{
    AutoGuard lock(m_lockSettings);
    return m_settings.hostVolumeSettings.getResyncRequiredCause(deviceName);
}

VOLUME_SETTINGS::TARGET_DATA_PLANE RpcConfigurator::getTargetDataPlane(const std::string & deviceName)
{
	AutoGuard lock(m_lockSettings);
	return m_settings.hostVolumeSettings.getTargetDataPlane(deviceName);
}

SV_ULONGLONG RpcConfigurator::GetEndpointRawSize(const std::string & deviceName) const
{
	AutoGuard lock(m_lockSettings);
	return m_settings.hostVolumeSettings.GetEndpointRawSize(deviceName);
}

std::string RpcConfigurator::GetEndpointDeviceName(const std::string & deviceName) const
{
	AutoGuard lock(m_lockSettings);
	return m_settings.hostVolumeSettings.GetEndpointDeviceName(deviceName);
}

std::string RpcConfigurator::GetEndpointHostId(const std::string & deviceName) const
{
	AutoGuard lock(m_lockSettings);
	return m_settings.hostVolumeSettings.GetEndpointHostId(deviceName);
}

std::string RpcConfigurator::GetEndpointHostName(const std::string & deviceName) const
{
	AutoGuard lock(m_lockSettings);
	return m_settings.hostVolumeSettings.GetEndpointHostName(deviceName);
}

std::string RpcConfigurator::GetResyncJobId(const std::string & deviceName) const
{
	AutoGuard lock(m_lockSettings);
	return m_settings.hostVolumeSettings.GetResyncJobId(deviceName);
}

VolumeSummary::Devicetype RpcConfigurator::GetDeviceType(const std::string & deviceName) const
{
	AutoGuard lock(m_lockSettings);
	return (VolumeSummary::Devicetype) m_settings.hostVolumeSettings.GetDeviceType(deviceName);
}


TRANSPORT_CONNECTION_SETTINGS RpcConfigurator::getCSTransportSettings(void)
{
	AutoGuard lock(m_lockSettings);
	return m_settings.getTransportSettings();
}

VOLUME_SETTINGS::SECURE_MODE RpcConfigurator::getCSTransportSecureMode(void)
{
	AutoGuard lock(m_lockSettings);
	return m_settings.getTransportSecureMode();
}

TRANSPORT_PROTOCOL RpcConfigurator::getCSTransportProtocol(void)
{
	AutoGuard lock(m_lockSettings);
	return m_settings.getTransportProtocol();
}

ESERIALIZE_TYPE RpcConfigurator::getSerializerType() const
{
  AutoGuard lock(m_lockSettings);
  return m_localConfigurator.getSerializerType();
}

std::string RpcConfigurator::getRepositoryLocation() const
{
  AutoGuard lock(m_lockSettings);
  return m_localConfigurator.getRepositoryLocation();
}


/*End */
