#ifndef SV_CONFIG__H
#define SV_CONFIG__H

#include <string>
#include <map>
#include <vector>
#include <istream>

#include "svtypes.h"

const char ENABLE_REMOTE_LOG_OPTION_KEY[] = "EnableRemoteLogging";
const char KEY_SV_SERVER[] =  "SVServer";
const char KEY_SV_SERVER_PORT[] =  "SVServerPort";
const char KEY_AGENT_TYPE[] =  "AgentType";
const char KEY_AGENT_HOST_ID[] =  "HostId";
const char KEY_SHELL[] =  "Shell";
const char KEY_INMSYNC_EXE_PATH[] =  "InmsyncExePath";
const char KEY_CACHE_DIR_PATH[] =  "CacheDirectoryPath";
const char KEY_THROTTLE_BOOTSTRAP_URL[] =  "ThrottleBootstrapURL";
const char KEY_THROTTLE_SENTINAL_URL[] =  "ThrottleSentinalURL";
const char KEY_THROTTLE_OUTPOST_URL[] =  "ThrottleOutpostURL";
const char KEY_FILE_REPLICATION_URL[] =  "FileReplicationConfigurationURL";
const char KEY_REGISTER_URL[] =  "RegisterURL";
const char KEY_UNREGISTER_URL[] =  "UnRegisterURL";
const char KEY_UPDATE_REPLICATION_STATUS_URL[] = "UpdateFileReplicationStatusURL";
const char KEY_LOG_LEVEL[] = "LogLevel";
const char KEY_LOG_INITIALIZE_SIZE[] = "LogInitializeSize";
const char KEY_ENABLE_LOG_FILE_DELETION[] = "EnableLogFileDeletion";
const char KEY_ENABLE_DELETE_OPTIONS[] = "EnableDeleteOptions";
const char KEY_USE_CONFIGURED_IP[] = "UseConfiguredIP";
const char KEY_CONFIGURED_IP[] = "ConfiguredIP";
const char KEY_USE_CONFIGURED_HOSTNAME[] = "UseConfiguredHostname";
const char KEY_CONFIGURED_HOSTNAME[] = "ConfiguredHostname";
const char KEY_KILL_CHIDREN_SCRIPT_PATH[] = "KillChildrenScriptPath";
const char KEY_SEND_SIGNAL_EXE[] = "SendSignalExe";
const char KEY_INMSYNC_GID[] = "InmSyncGID";
const char KEY_INMSYNC_UID[] = "InmSyncUID";

const char KEY_SSHD_PORT[]	= "SSHD_PORT";
const char KEY_SSHD_PRODUCT_VER[] = "SSHD_PRODUCT_VERSION";
const char KEY_SSHD_PROTOCOL_VER[] = "SSHD_PROTOCOL_VERSION";
const char KEY_SSH_AUTHKEYS_FILE_FORMAT[] = "AUTHKEYS_FILE_FORMAT";
const char KEY_SSHD_STATUS[] = "SSHD_STATUS";
const char KEY_SSH_AUTHORIZATION_FILE_FORMAT[] = "AUTHORIZATION_FILE_FORMAT";
const char KEY_SSH_IDENTITY_FILE_FORMAT[] = "IDENTIFICATION_FILE_FORMAT";
const char KEY_SSH_CLIENT_PATH[] = "SSH_CLIENT_PATH";
const char KEY_SSH_KEYGEN_PATH[] = "SSH_KEYGEN_PATH";

const char KEY_SSH_DROP_BACK_TO_SSH1[] = "DROP_BACK_TO_SSH1";
const char KEY_SSH_SUPPORT_SSH1_ONLY[] = "SUPPORT_SSH1_ONLY";
const char KEY_SSH_SSHD_CONFIG_FILE[] = "SSHD_CONFIG_FILE";
const char KEY_SSH_ALLOWED_AUTHS[] = "ALLOWED_AUTHS";
const char KEY_SSH_CIPHERS[] = "CIPHERS";
const char KEY_SSH_MACS[] = "MACS";
const char KEY_SSH_PERMIT_TUNNEL[] = "PERMIT_TUNNEL";
const char KEY_SSH_PERMIT_ROOT_LOGIN[] = "PERMIT_ROOT_LOGIN";
const char KEY_SSH_PUBKEYAUTHENTICATION_ALLOWED[] = "PUBKEYAUTHENTICATION_ALLOWED";
const char KEY_SSH_SSHD_PID_FILE[] = "SSHD_PID_FILE";
const char KEY_SSH_PROTOCOL[] = "SSH_PROTOCOL";
const char KEY_SSH_STRICT_MODES[] = "STRICT_MODES";
const char KEY_SSH_USER_KNOWN_HOSTS_DIR[] = "USER_KNOWN_HOSTS_DIR";
const char KEY_HOME_DIR[] = "HOMEDIR";
const char KEY_USERNAME[] = "USER";
const char KEY_HTTPS[] = "Https";

struct LineParser;
class SVConfig 
{
private:
    struct ParsedLine
    {
        std::string name;
        std::string value;
        std::string sectionName;
        
        std::string spaceLeadingName;
        std::string spaceTrailingName;
        std::string spaceLeadingValue;
        std::string spaceTrailingValue;
        char quoteChar;
        std::string comment;
    };

    //std::map<std::string, std::list<int> > m_config;
    std::map<std::string, std::string> config;
    std::map<std::string, int> m_arraycount;
    std::map<std::string, bool> m_isSection;
    std::vector<ParsedLine> m_lines;
    std::string configFilePath;
    SVConfig();
    ~SVConfig();
    std::string UnparseLine( ParsedLine const& parsedLine );
    void RenameSectionIntoArrayEntry( std::string oldSectionName );
    friend struct LineParser;

    static SVConfig* pInstance;
public:
    static SVConfig* GetInstance();
    SVERROR Parse(std::string const& configFile);
    SVERROR ParseString(std::string const& configStr);
    SVERROR ParseStream( std::istream& o );
    SVERROR GetValue(std::string const& entry,std::string& value) const;
    SVERROR Write();
    SVERROR UpdateConfigParameter(std::string const& key, std::string const& value);
    int GetTotalNumberOfKeys();
    SVERROR WriteKeyValuePairToFile(std::string const& key, std::string const& value);
    SVERROR GetConfigFileName(std::string& name) const;
    void SetConfigFileName(const char* fileName);

    std::string CreateSection( std::string const& name );
    SVERROR SetValue( std::string const& sectionName, std::string const& name, std::string const& value );
    void Clear();
    int GetArraySize( std::string const& name );
};

#endif // SV_CONFIG__H

