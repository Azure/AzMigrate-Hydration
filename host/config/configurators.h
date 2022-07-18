#ifndef CONFIGURATORS_H
#define CONFIGURATORS_H

#include <vector>
#include <iterator>
#include <algorithm>
#include <cstdarg>

#include <ace/Thread_Manager.h>
#include <ace/Guard_T.h>
#include <ace/Mutex.h>
#include <ace/Event.h>

#include <boost/shared_ptr.hpp>

#include "configurator2.h"
#include "cxproxy.h"
#include "retentionsettings.h"
#include "cdpsnapshotrequest.h"
#include "atconfigmanagersettings.h"
#include "apiwrapper.h"
#include "prismsettings.h"
#include "serializationtype.h"
#include "sourcesystemconfigsettings.h"
#include "rpcconfigurator.h"
#include "appconfchangemediator.h"

class Configurators
{
public:
    Configurators();
    ~Configurators();
    /* format of configurators:
     * m: change mediator
     * c: configurator
     * eg: 
     * Start("mc", &changemediator, &configurator);
     */
    void Start(const char *configurators, ...);
    void Stop();

protected:
    ACE_Thread_Manager m_threadManager;
    static ACE_THR_FUNC_RETURN ThreadFunc( void* pArg );
    ACE_THR_FUNC_RETURN run();
     
private:
    void GetConfiguratorsSettings(void);
    void PerformPrePollChangeMediatorTasks(void);
    void GetChangeMediatorSettings(void);
	bool AnyPendingEvents() ;
	bool BackupAgentPause(CDP_DIRECTION direction, const std::string& devicename) ;
	bool BackupAgentPauseTrack(CDP_DIRECTION direction, const std::string& devicename);
private:
    typedef std::vector<Configurator *> Configurators_t;
    typedef Configurators_t::const_iterator ConstConfiguratorsIter_t;
    typedef Configurators_t::iterator ConfiguratorsIter_t;

    typedef std::vector<ConfigChangeMediator *> ConfigChangeMediators_t;
    typedef ConfigChangeMediators_t::const_iterator ConstConfigChangeMediatorsIter_t;
    typedef ConfigChangeMediators_t::iterator ConfigChangeMediatorsIter_t;

    boost::shared_ptr<ACE_Event> m_QuitEvent;
    bool m_started;
	bool m_fetchImmedietly ;
    Configurators_t m_configurators;
    ConfigChangeMediators_t m_configchangemediators;
};

#endif /* CONFIGURATORS_H */
