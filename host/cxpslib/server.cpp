
///
///  \file server.cpp
///
///  \brief implementation for the servers
///

#include <string>
#include <cstddef>

#include <boost/thread.hpp>

#include "server.h"

void BasicServer::start(std::string const & ipAddress, std::string const & port)
{
    boost::asio::ip::tcp::endpoint endpoint((ipAddress.length() > 0
                                             ? boost::asio::ip::address_v4(ntohl(inet_addr(ipAddress.c_str())))
                                             : boost::asio::ip::address_v4::any()),
                                            boost::lexical_cast<unsigned short>(port));
    m_acceptor.open(endpoint.protocol());
    m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    int windowSize = m_options->receiveWindowSizeBytes();
    if (windowSize > 0) {
        m_acceptor.set_option(boost::asio::socket_base::receive_buffer_size(windowSize));
    }
    windowSize = m_options->sendWindowSizeBytes();
    if (windowSize > 0) {
        m_acceptor.set_option(boost::asio::socket_base::receive_buffer_size(windowSize));
    }
    m_acceptor.bind(endpoint);
    m_acceptor.listen();
    asyncAccept();
}

void BasicServer::run(int threadCount)
{
    // Create a pool of threads to run all of the io_services.
    std::vector<threadPtr> threads;
    for (int i = 0; i < threadCount; ++i) {
        threadPtr thread(new boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService)));
        threads.push_back(thread);
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "REQUEST HANDLER WORKER THREAD STARTED: " << thread->get_id());
        CXPS_LOG_ERROR_INFO("REQUEST HANDLER WORKER THREAD STARTED: " << thread->get_id());
    }

    // Wait for all threads in the pool to exit.
    for (std::size_t i = 0; i < threads.size(); ++i) {
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "AWAITING JOIN OF REQUEST HANDLER WORKER THREAD: " << threads[i]->get_id());
        CXPS_LOG_ERROR_INFO("AWAITING JOIN OF REQUEST HANDLER WORKER THREAD: " << threads[i]->get_id());
        threads[i]->join();
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "REQUEST HANDLER WORKER THREAD JOINED");
        CXPS_LOG_ERROR_INFO("REQUEST HANDLER WORKER THREAD JOINED");
    }
}

SslServer::SslServer(serverOptionsPtr options, errorCallback_t callback, boost::asio::io_service& ioService)
    : BasicServer(options, callback, ioService)
{
    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "SSL SERVER STARTED");
    CXPS_LOG_ERROR_INFO("SSL SERVER STARTED");
    newSession();
    start(options->ipAddress(), options->sslPort());
}
