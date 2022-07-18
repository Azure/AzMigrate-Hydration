#ifndef CONFIGUREVXTRANSPORT__H
#define CONFIGUREVXTRANSPORT__H


#include "transport_settings.h"

struct ConfigureVxTransport {
    // cx settings
    //CxHostAndPort getCxInfo();
    virtual HTTP_CONNECTION_SETTINGS getHttp() const = 0;
    virtual std::string getDiffSourceDirectory() const = 0;
    virtual std::string getDiffTargetDirectory() const = 0;
    virtual std::string getPrefixDiffFilename() const = 0;
    virtual std::string getResyncDirectories() const = 0;
    virtual std::string getSslClientFile() const = 0;
    virtual std::string getSslKeyPath() const = 0;
    virtual std::string getSslCertificatePath() const = 0;    
    virtual std::string getCfsLocalName() const = 0;
    virtual int getTcpSendWindowSize() const = 0;
    virtual int getTcpRecvWindowSize() const = 0;
    virtual bool get_curl_verbose() const = 0;
    virtual bool ignoreCurlPartialFileErrors() const = 0;
    virtual int getTransportMaxBufferSize() const = 0;
    virtual int getTransportConnectTimeoutSeconds() const = 0;
    virtual int getTransportResponseTimeoutSeconds() const = 0;
    virtual int getTransportWriteMode() const = 0;
    virtual bool IsHttps() const = 0;

    virtual ~ConfigureVxTransport() {}
};

#endif