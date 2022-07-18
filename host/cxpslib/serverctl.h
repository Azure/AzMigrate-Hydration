
///
///  \file serverctl.h
///
///  \brief proto types for starting and stopping server threads
///

#ifndef SERVERCTL_H
#define SERVERCTL_H

#include <string>

/// \brief name of the cx process server message queue
#define CX_PROCESS_SERVER_MSG_QUEUE "cxps_msg_queue"

/// \brief type of function to be used for thread error callback
typedef void (*errorCallback_t)(std::string reason);

/// \brief start the http and https servers in their own threads
void startServers(std::string const& confFile,                   ///< configuration file
                  errorCallback_t,                               ///< function to call if there are errors starting server
                  std::string const& installDir = std::string()  ///< cxps install dir
                  );

/// \brief stop the servers
void stopServers();

/// \brief get recycle handle count
unsigned long getRecycleHandleCount(std::string const& confFile, std::string const& installDir);

#endif // SERVERCTL_H
