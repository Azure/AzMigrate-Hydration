#ifndef SERVICE_HELPER_H
#define SERVICE_HELPER_H

#include "svtypes.h"
#include <string>

class ServiceHelper {
public:
	ServiceHelper() {};
	~ServiceHelper() {};

    static SVSTATUS UpdateCSTypeAndRestartSVAgent(const std::string &csType,
        std::string &errMsg);

    static SVSTATUS UpdateSymLinkPath(const std::string& linkPath,
        const std::string& filePath, std::string& errMsg);

private:

    static SVSTATUS StartSVAgents(int timeout, int maxRetryCount);

    static SVSTATUS StopSVAgents(int timeout, int maxRetryCount);

};

#endif
