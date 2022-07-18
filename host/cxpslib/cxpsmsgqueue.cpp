
///
/// \file cxpsmsgqueue.cpp
///
/// \brief
///

#if !defined(__GNUC__) || (__GNUC__ >= 4)

#include <boost/bind.hpp>

#include "cxpsmsgqueue.h"
#include "errorexception.h"
#include "scopeguard.h"
#include "cxpslogger.h"

// CxPsMsgQueue
CxPsMsgQueue::CxPsMsgQueue(std::string const& name, int maxSize)
    : m_name(name),
      m_maxSize(maxSize)
{
    // just in case server did not exit gracefully and queue was left
    boost::interprocess::message_queue::remove(m_name.c_str());

    m_queue.reset(new boost::interprocess::message_queue(boost::interprocess::create_only,
        m_name.c_str(),
        m_maxSize,
        sizeof(int)));
}

void CxPsMsgQueue::start(errorCallback_t errorCallback)
{
    try {
        bool (*mqremove)(const char*) = &boost::interprocess::message_queue::remove;
        ON_BLOCK_EXIT(boost::bind(mqremove, m_name.c_str()));

        int msg = CXPS_MSG_UNSET;
        unsigned int priority;
        std::size_t returnedSize;

        while (CXPS_MSG_QUIT != msg) {
            m_queue->receive(&msg, sizeof(msg), returnedSize, priority);
            switch (msg) {
                case CXPS_MSG_QUIT:
                    break;
                case CXPS_MSG_MONITOR_LOG_ON:
                    monitorLogOn();
                    break;
                case CXPS_MSG_MONITOR_LOG_OFF:
                    monitorLogOff();
                    break;
                case CXPS_MSG_MONITOR_LOG_LEVEL_1:
                    setMonitorLoggingLevel(MONITOR_LOG_LEVEL_1);
                    break;
                case CXPS_MSG_MONITOR_LOG_LEVEL_2:
                    setMonitorLoggingLevel(MONITOR_LOG_LEVEL_2);
                    break;
                case CXPS_MSG_MONITOR_LOG_LEVEL_3:
                    setMonitorLoggingLevel(MONITOR_LOG_LEVEL_3);
                    break;
                case CXPS_MSG_XFER_LOG_ON:
                    xferLogOn();
                    break;
                case CXPS_MSG_XFER_LOG_OFF:
                    xferLogOff();
                    break;
                case CXPS_MSG_WARNING_LOG_ON:
                    warningsOn();
                    break;
                case CXPS_MSG_WARNING_LOG_OFF:
                    warningsOff();
                    break;
                default:
                    break;
            }
        }
    } catch(boost::interprocess::interprocess_exception const& ex){
        CXPS_LOG_ERROR(CATCH_LOC << ex.what());
        // errorCallback(ex.what());
    }
}

void sendCxPsMsgQueue(const char* queueName, int msg)
{
    try {
        boost::interprocess::message_queue mq(boost::interprocess::open_only, queueName);
        mq.send(&msg, sizeof(msg), 0);
    } catch (boost::interprocess::interprocess_exception const& e){
        throw ERROR_EXCEPTION << e.what();
    }
}

void removeCxPsMsgQueue(const char* queueName)
{
    boost::interprocess::message_queue::remove(queueName);
}

#endif
