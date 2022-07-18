#include "configurators.h"
#include "logger.h"
#include "portablehelpers.h"

Configurators::Configurators()
: m_started(false),
  m_QuitEvent(new ACE_Event(1, 0))
{
}


Configurators::~Configurators()
{
    Stop();
}


void Configurators::Start(const char *configurators, ...)
{
    if (m_started)
    {
        return;
    }

    ConfigChangeMediator *pm = 0;
    va_list ap;
    const char *p = 0;
    bool bshouldbreak = false;
    Configurator *pc = 0;
    va_start(ap, configurators);
    for (p = configurators; *p ; p++)
    {
        switch (*p)
        {
            case 'c':
                pc = va_arg(ap, Configurator *);
                m_configurators.push_back(pc);
                break;
            case 'm':
                /* TODO: this should be only ConfigChangeMediator */
                pm = va_arg(ap, ConfigChangeMediator *);
                m_configchangemediators.push_back(pm);
                break;
            default:
                bshouldbreak = true;
                break;
        }
        if (bshouldbreak)
        {
            DebugPrintf(SV_LOG_ERROR, "wrong arguements supplied for configurators start method\n");
            break;
        }
    }
    va_end(ap);

    m_QuitEvent->reset();
    m_threadManager.spawn( ThreadFunc, this );
    m_started = true;
}


ACE_THR_FUNC_RETURN Configurators::ThreadFunc( void* arg )
{
    Configurators* p = static_cast<Configurators*>( arg );
    return p->run();
}


ACE_THR_FUNC_RETURN Configurators::run()
{
    if (m_configurators.empty() && m_configchangemediators.empty())
    {
        return 0;
    }

    PerformPrePollChangeMediatorTasks();

    LocalConfigurator localConfigurator;
	int SettingCallInterval = localConfigurator.getSettingsCallInterval();
	if(SettingCallInterval <= 0)
		SettingCallInterval = 60;

    while( true )
    {
        ACE_Time_Value waitTime ;
        SV_UINT totalWaitTime = 0 ;
        while( SettingCallInterval >= totalWaitTime )
		{
			if( AnyPendingEvents() )
			{
				break ;
			}
			waitTime.set(30,0) ;
			m_QuitEvent->wait(&waitTime,0) ;
			totalWaitTime += 30 ;
		}
		if( m_threadManager.testcancel( ACE_Thread::self() ) )
		{
			break;
		}

		GetConfiguratorsSettings();
		GetChangeMediatorSettings();
    }

    return 0;
}
bool Configurators::BackupAgentPause(CDP_DIRECTION direction, const std::string& devicename)
{
	return true ;
}
bool Configurators::BackupAgentPauseTrack(CDP_DIRECTION direction, const std::string& devicename)
{
	return true ;
}
bool Configurators::AnyPendingEvents()
{
    Configurator *pc;
	bool pendingEvents = false ;
    for (ConfiguratorsIter_t it = m_configurators.begin(); it != m_configurators.end(); it++)
    {
        pc = *it;
        if( pc->AnyPendingEvents() )
		{
			pendingEvents = true ;
			break ;
		}
    }
	return pendingEvents ;
}

void Configurators::GetChangeMediatorSettings(void)
{
    ConfigChangeMediator *pm;
    for (ConfigChangeMediatorsIter_t it = m_configchangemediators.begin(); it != m_configchangemediators.end(); it++)
    {
        pm = *it;
        pm->GetConfigInfoAndProcess();
    }
}


void Configurators::PerformPrePollChangeMediatorTasks(void)
{
    ConfigChangeMediator *pm;
    for (ConfigChangeMediatorsIter_t it = m_configchangemediators.begin(); it != m_configchangemediators.end(); it++)
    {
        pm = *it;
        pm->LoadCachedSettings();
    }
}


void Configurators::GetConfiguratorsSettings(void)
{
    Configurator *pc;
    for (ConfiguratorsIter_t it = m_configurators.begin(); it != m_configurators.end(); it++)
    {
        pc = *it;
        pc->GetCurrentSettingsAndOperate();
    }
}


void Configurators::Stop()
{
    m_threadManager.cancel_all();
    m_QuitEvent->signal();
    m_threadManager.wait();
    m_started = false;
}
