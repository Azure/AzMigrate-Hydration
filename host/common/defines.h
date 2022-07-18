///
/// @file defines.h
/// Define MACROS, typedefs etc.
///
#ifndef DEFINES__H
#define DEFINES__H

#define MAXSTRINGLEN(s) ( sizeof(s)/sizeof(s[0]) - 1 )

#define LENGTH_TRANSPORT_USER 32
#define LENGTH_TRANSPORT_PASSWORD 15
#define  SV_ROOT_REGISTRY_KEY_NAME     "SOFTWARE\\SV Systems"
#define SV_FTP_HOST_VALUE_NAME         "FTPHost" 
#define  SV_TRANSPORT_VALUE_NAME       "SOFTWARE\\SV Systems\\Transport"
//#define  SV_OUTPOST_VALUE_NAME         "SOFTWARE\\SV Systems\\OutpostAgent"

#define  SV_FTP_USER_VALUE_NAME             "FTPUser"
#define  SV_FTP_PASSWORD_VALUE_NAME         "FTPKey"
#define  SV_FTP_PORT_VALUE_NAME             "FTPPort"
#define  SV_FTP_URL_VALUE_NAME              "FTPInfoURL"
#define  SV_FTP_CONNECT_TIMEOUT_VALUE_NAME  "FTPConnectTimeOut"
#define  SV_FTP_RESPONSE_TIMEOUT_VALUE_NAME "FTPResponseTimeOut"

//outpost agent run types
#define RUN_ONLY_OUTPOST_AGENT 1
#define RUN_ONLY_SNAPSHOT_AGENT 2
#define RUN_OUTPOST_SNAPSHOT_AGENT 3

// for bug#23
#define SV_SECURE_MODES_URL_VALUE_NAME             "SecureModesInfoURL"
#define SV_SRC_SECURE_VALUE_NAME                   "SrcSecure"
#define SV_DEST_SECURE_VALUE_NAME                  "DestSecure"
#define SV_SSL_KEY_PATH							   "SSLKeyPath"
#define SV_SSL_CERT_PATH						   "SSLCertificatePath"

//regisrty keys 


#endif //DEFINES__H

