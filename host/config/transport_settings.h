//
// transport_settings.h: higher level interface to
// configurator that returns settings usable by TAL
//
#ifndef TRANSPORT_SETTINGS__H
#define TRANSPORT_SETTINGS__H

#include "svtypes.h"
#include "portable.h"
#include <string>

#include "transportprotocols.h"

struct HTTP_CONNECTION_SETTINGS
{
    HTTP_CONNECTION_SETTINGS() : port( 0 ) {
        ipAddress[0] = userName[0] = password[0] = szUrl[0] = 0;
    }
    char ipAddress[ 256 ];
    char userName[ 128 ];
    char password[ 128 ];
    SV_INT port;
    char szUrl[ 2048 ];
};

struct AZURE_BLOB_CONTAINER_SETTINGS
{
    std::string sasUri;
};

struct PROCESS_SERVER_SETTINGS
{
    std::string ipAddress;
    std::string port;
    std::string logFolder;
};

struct DATA_PATH_TRANSPORT_SETTINGS {
    std::string						m_diskId;
    TRANSPORT_PROTOCOL				m_transportProtocol;
    AZURE_BLOB_CONTAINER_SETTINGS	m_AzureBlobContainerSettings;
    PROCESS_SERVER_SETTINGS			m_ProcessServerSettings;
};

struct TRANSPORT_CONNECTION_SETTINGS
{
    // Due to legacy V2A design constraint of agent to CS contracts ipAddress is reused in RCM reprotect scenario
    // to store blobContainerSasUri.
    std::string ipAddress;

    std::string port;
    std::string sslPort;
    std::string ftpPort;
    std::string user;
    std::string password;

    int connectTimeout;
    int responseTimeout;

    bool activeMode;

    TRANSPORT_CONNECTION_SETTINGS()
        : connectTimeout(0),
          responseTimeout(0),
          activeMode(false)
        {}

    bool operator==(TRANSPORT_CONNECTION_SETTINGS const& settings) const
        {
            if (ipAddress == settings.ipAddress &&
                port == settings.port &&
                sslPort == settings.sslPort &&
                ftpPort == settings.ftpPort &&
                user == settings.user &&
                password == settings.password &&
                connectTimeout == settings.connectTimeout &&
                responseTimeout == settings.responseTimeout &&
                activeMode == settings.activeMode) {
                return true;
            } else {
                return false;
            }
        }
};


#endif // TRANSPORT_SETTINGS__H
