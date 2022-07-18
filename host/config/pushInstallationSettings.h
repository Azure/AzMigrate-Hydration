
#ifndef PUSH_INSTALLATION_SETTINGS_H
#define PUSH_INSTALLATION_SETTINGS_H

#include <string>
#include <list>

#include "svtypes.h"

struct PushRequestOptions;
struct PushRequests;
struct PushInstallationSettings;

enum VM_TYPE { VMWARE, PHYSICAL };
enum INSTALL_TYPE {FRESH_INSTALL, UPGRADE_INSTALL, PATCH_INSTALL};
struct PushRequestOptions
{
    std::string ip;
    bool reboot_required;
    std::string installation_path;
    std::string jobId ;
    SV_INT state ;
	std::string vmType;
	std::string vCenterUserName;
	std::string vCenterPassword;
	std::string vCenterIP;
	std::string vmName;
	std::string vmAccountId;
	std::string vcenterAccountId;
	std::string ActivityId;
	std::string ClientRequestId;
	std::string ServiceActivityId;
};

struct PushRequests
{
    int os_type;
    std::string username;
    std::string password;
    std::string domain;
    std::list<PushRequestOptions> prOptions;

	PushRequests() {}
	PushRequests(const PushRequests & rhs);
	PushRequests& operator=(const PushRequests &);
};

struct PushInstallationSettings
{
    bool ispushproxy;
    int m_maxPushInstallThreads; 
    std::list<PushRequests> piRequests;

//public:
	PushInstallationSettings() {}
	PushInstallationSettings(const PushInstallationSettings & rhs);
    bool strictCompare( PushInstallationSettings & );
    PushInstallationSettings& operator=(const PushInstallationSettings & );
    bool operator==( PushInstallationSettings & ) ;
};

struct INSTALL_TASK
{
    std::string m_preScript ;
    std::string m_url ;
    std::string m_postScript ;
	INSTALL_TYPE type_of_install;
    std::string m_commandLineOptions;
    std::list<std::string> m_logonServiceNameList;
} ;

typedef std::list<INSTALL_TASK> INSTALL_TASK_LIST ;

struct PUSH_INSTALL_JOB
{
    SV_UINT m_OsType ;
    INSTALL_TASK_LIST m_installTasks ;
} ;

struct PUSH_INSTALL_CLIENT_DOWNLOAD_PATH
{
    std::string m_PIClient_url;
} ;

struct BUILD_DOWNLOAD_PATH
{
    std::string m_Build_url;
} ;

#endif