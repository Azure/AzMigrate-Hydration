
///
/// \file cfsserver.cpp
///
/// \brief
///

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include "cfsserver.h"

CfsServer::ptr g_cfsServer;

boost::shared_ptr<boost::thread> g_thread;

void startCfsServer(serverOptionsPtr serverOptions,
                    boost::asio::io_service& ioService)
{
    g_cfsServer.reset(new CfsServer(serverOptions, ioService));
    g_thread.reset(new boost::thread(boost::bind(&CfsServer::run, g_cfsServer)));
}

void stopCfsServer()
{
    if (0 != g_cfsServer.get()) {
        g_cfsServer->stop();
        g_thread->join();
    }
}

void CfsServer::run()
{
    boost::thread thread(boost::bind(&boost::asio::io_service::run, &m_ioService));
    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "CFS REQUEST HANDLER WORKER THREAD STARTED: " << thread.get_id());
    CXPS_LOG_ERROR_INFO("CFS REQUEST HANDLER WORKER THREAD STARTED: " << thread.get_id());
    thread.join();
}
