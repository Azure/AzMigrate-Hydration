
///
/// \file cfsserver.h
///
/// \brief
///

#ifndef CFSSERVER_H
#define CFSSERVER_H

#include "server.h"
#include "cfssession.h"
#include "client.h"
#include "cxpslogger.h"

/// \brief for CFS server connections needed for linux
class CfsServer {
public:
    typedef boost::shared_ptr<CfsServer> ptr;

    explicit CfsServer(serverOptionsPtr options,
                       boost::asio::io_service& ioService)
        : m_ioService(ioService),
          m_acceptor(ioService),
          m_serverOptions(options)
        {
            CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "CFS SERVER STARTED");
            CXPS_LOG_ERROR_INFO("CFS SERVER STARTED");
            boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4(ntohl(inet_addr("127.0.0.1"))), 9082);
            m_acceptor.open(endpoint.protocol());
            m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            m_acceptor.bind(endpoint);
            m_acceptor.listen();
            newSession();
            asyncAccept();
        }

    ~CfsServer()
        {
        }

    void run();

    void stop()
        {
            logStop();
            m_ioService.stop();
        }

protected:
    /// \brief log server stopped
    void logStop()
        {
            CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "CFS SERVER STOPPED");
            CXPS_LOG_ERROR_INFO("CFS SERVER STOPPED");
        }

    boost::asio::ip::tcp::socket& lowestLayerSocket()
        {
            return m_session->socket();
        }

    void asyncAccept() {
        m_acceptor.async_accept(lowestLayerSocket(),
                                boost::bind(&CfsServer::handleAsyncAccept,
                                            this,
                                            boost::asio::placeholders::error));
    }

    /// \brief handle the results of the asyncAccept
    void handleAsyncAccept(const boost::system::error_code& error) ///< result of async accept
        {
            try {
                if (!error) {
                    sessionStart();
                    newSession();
                    asyncAccept();
                } else {
                    CXPS_LOG_ERROR(AT_LOC << "error: " << error);
                }
            } catch (std::exception const& e) {
                CXPS_LOG_ERROR(CATCH_LOC << e.what());
            } catch (...) {
                CXPS_LOG_ERROR(CATCH_LOC << "unknonw exception");
            }
        }


    /// \brief start session processing
    void sessionStart()
        {
            m_session->start();
        }

    /// \brief create a new session
    void newSession()
        {
            m_session.reset(new CfsSession(m_ioService, m_serverOptions));
        }

private:
    boost::asio::io_service& m_ioService;

    boost::asio::ip::tcp::acceptor m_acceptor;

    CfsSession::ptr m_session;

    serverOptionsPtr m_serverOptions;
};

void startCfsServer(serverOptionsPtr serverOptions, boost::asio::io_service& ioService);
void stopCfsServer();

#endif // CFSSERVER_H
