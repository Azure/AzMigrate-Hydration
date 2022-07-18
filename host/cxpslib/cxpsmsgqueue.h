
///
/// \file cxpsmsgqueue.h
///
/// \brief
///

#ifndef CXPSMSGQUEUE_H
#define CXPSMSGQUEUE_H

#include <string>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/shared_ptr.hpp>

/// \brief cxps message queue for allowing on/off of various
/// logs without the need to restart
class CxPsMsgQueue {
public:
    /// \brief type of function to be used for thread error callback
    typedef void (*errorCallback_t)(std::string reason);

    /// \brief messages used to tell cxps to enable, disable certain
    /// logging without having to retstart the process
    enum {
        CXPS_MSG_UNSET = -1,
        CXPS_MSG_QUIT,
        CXPS_MSG_MONITOR_LOG_ON,
        CXPS_MSG_MONITOR_LOG_OFF,
        CXPS_MSG_MONITOR_LOG_LEVEL_1,
        CXPS_MSG_MONITOR_LOG_LEVEL_2,
        CXPS_MSG_MONITOR_LOG_LEVEL_3,
        CXPS_MSG_XFER_LOG_ON,
        CXPS_MSG_XFER_LOG_OFF,
        CXPS_MSG_WARNING_LOG_ON,
        CXPS_MSG_WARNING_LOG_OFF
    };

    /// \brief constructor
    explicit CxPsMsgQueue(std::string const& name,   ///< name of message queue
                                  int maxSize = 50   ///< maximum number of messages that can be in the queue
                                  );

    /// \brief create and start the message queue
    /// \param errorCallback function to call if there is an error
    void start(errorCallback_t errorCallback);

    /// \brief stop the message queue
    void stop()
        {
            postMsg(CXPS_MSG_QUIT);
        }

    /// \brief post a msg to the queue
    void postMsg(int msg) ///< one of the cxps messages to post to message queue
        {
            if (0 != m_queue.get()) {
                m_queue->send(&msg, sizeof(msg), 0);
            }
        }

protected:

private:
    std::string m_name; ///< message queue name

    int m_maxSize; ///< max size of the message queue

    boost::shared_ptr<boost::interprocess::message_queue> m_queue; ///< holds messages
};

/// \brief send message to CxPsMsgQueue
///
/// mainly for IPC method to send messages without haveing
/// to stop and start the process using the CxPsMsgQueue
void sendCxPsMsgQueue(const char* queueName, ///< name of message queue to post message
                              int msg        ///< one of the cxps messages to post to message queue
                              );

/// \brief removes the shared CxPsMsgQueue
///
/// used for cases where a process terminated in such a manner
/// that it did not properly clean up the CxPsMsgQueue
/// \param queueName name of message queue to remove
///
/// \sa cxpscli.cpp
void removeCxPsMsgQueue(const char* queueName);


#endif // CXPSMSGQUEUE_H
