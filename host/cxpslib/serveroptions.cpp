///
///  \file serveroptions.cpp
///
///  \brief implementation for server options
///

#include <fstream>
#include <stdexcept>
#include <cctype>

#include <boost/algorithm/string/trim.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/filesystem.hpp>

#include "serveroptions.h"
#include "errorexception.h"
#include "cxpslogger.h"
#include "strutils.h"
#include "createpaths.h"
#include "fiopipe.h"
#include "defaultdirs.h"
#include "genpassphrase.h"
#include "cxps.h"
#include "crypto.h"

// Copied from PortableHelpers.h

#ifdef SV_WINDOWS
#define FUNCTION_NAME __FUNCTION__
#else
#define FUNCTION_NAME __func__
#endif /* SV_WINDOWS */

#define THROW_ON_UNKNOWN_CS_MODE(csMode) do { \
                                             if (csMode != CS_MODE_RCM && csMode != CS_MODE_LEGACY_CS) { \
                                                 std::stringstream ss; \
                                                 ss << "Unknown CS Mode : " << csMode << " - Method : " << FUNCTION_NAME << "()"; \
                                                 throw std::runtime_error(ss.str()); \
                                             } \
                                         } while (false)

int const DEFAULT_LOG_RETAIN_SIZE(1024*1024);
int const DEFAULT_LOG_SIZE(1024*1024*100);

inline void appendSlashIfNeeded(std::string& str)
{
    if ('/' != str[str.size() - 1]
        && '\\' != str[str.size() - 1]
        && '*' != str[str.size() - 1]) { // paths that end in * say exlcude path and everything under it including subdirs
        str.append("/");
    }
}

static const
    PSSettings::PSSettingsConfigurator::SubscriptionNum_t NOT_YET_SUBSCRIBED =
        std::numeric_limits<PSSettings::PSSettingsConfigurator::SubscriptionNum_t>::max();

ServerOptions::ServerOptions(std::string const& fileName, std::string const& installDir)
    : m_tunablesNotifSubscriptionNum(NOT_YET_SUBSCRIBED)
{
    PSSettings::PSSettingsConfigurator& s_configurator = PSSettings::PSSettingsConfigurator::GetInstance();

    try
    {
        if (GetCSMode() == CS_MODE_RCM) {
            idempotentRestorePSState();
        }

        boost::filesystem::path conf;
        if (fileName.empty()) {
            throw ERROR_EXCEPTION << "configuration file was not specified";
        }

        if (installDir.empty()) {
            m_defaultDir = fileName;
            m_defaultDir.remove_filename();
        } else {
            m_defaultDir = installDir;
        }
        conf = fileName;

        std::string lockFile(conf.string());
        lockFile += ".lck";
        if (!boost::filesystem::exists(lockFile)) {
            std::ofstream tmpLockFile(lockFile.c_str());
        }
        boost::interprocess::file_lock fileLock(lockFile.c_str());
        boost::interprocess::sharable_lock<boost::interprocess::file_lock> fileLockGuard(fileLock);

        std::ifstream confFile(conf.string().c_str());
        if (!confFile.good()) {
            throw ERROR_EXCEPTION << "unable to open conf file " << conf.string() << ": " << errno;
        }

        std::string line;  // assumes conf file lines are < 1024
        while (confFile.good()) {
            std::getline(confFile, line);
            addOption(line);
        }

        if (id().empty() && (GetCSMode() == CS_MODE_RCM))
        {
            setIdFromPsConfig();
        }

        PSSettings::PSSettingsPtr cachedPSSettings = s_configurator.GetPSSettings();
        if (cachedPSSettings)
        {
            m_tunablesPtr = cachedPSSettings->Tunables;
        }

        m_tunablesNotifSubscriptionNum =
            s_configurator.SubscribeForTunablesChange(boost::bind(&ServerOptions::updateTunables, this, _1));

        boost::filesystem::path requestDir = requestDefaultDir();
        CreatePaths::createPathsAsNeeded(requestDir);
        boost::filesystem::create_directory(requestDir);
        buildAllowedDirs(requestDir);
        buildExcludeDirs();
    }
    catch (...)
    {
        // Unsubscribe, if the subscription was successful
        if (m_tunablesNotifSubscriptionNum != NOT_YET_SUBSCRIBED)
        {
            s_configurator.UnsubscribeForTunablesChange(m_tunablesNotifSubscriptionNum);
        }

        throw; //rethrow
    }
}

ServerOptions::~ServerOptions()
{
    PSSettings::PSSettingsConfigurator& s_configurator = PSSettings::PSSettingsConfigurator::GetInstance();
    if (m_tunablesNotifSubscriptionNum != NOT_YET_SUBSCRIBED)
        s_configurator.UnsubscribeForTunablesChange(m_tunablesNotifSubscriptionNum);
}

void ServerOptions::setIdFromPsConfig()
{
    const boost::filesystem::path psConfFilePath(installDir() / ".." / "etc" / "ProcessServer.conf" );
    const std::string psConfParamHostId("HostId");
    try
    {
        boost::system::error_code ec;
        if (boost::filesystem::exists(psConfFilePath.string(), ec))
        {
            boost::property_tree::ptree	psconfparams;
            std::ifstream localconffile(psConfFilePath.string().c_str());
            boost::property_tree::json_parser::read_json(localconffile, psconfparams);
            std::string psHostId = psconfparams.get<std::string>(psConfParamHostId);
            if (!psHostId.empty())
            {
                boost::algorithm::trim(psHostId);
                m_options["id"] = psHostId;
                CXPS_LOG_ERROR_INFO("CS Mode RCM - HostId set from PS config file " << psConfFilePath << " as " << id());
            }
            else
            {
                CXPS_LOG_ERROR_INFO("CS Mode RCM - HostId key not found in PS config file " << psConfFilePath);
            }
        }
        else
        {
            CXPS_LOG_ERROR_INFO("CS Mode RCM - failed to get HostId from PS config file " << psConfFilePath << " with error " << ec.message());
        }
    }
    catch (const std::exception& e)
    {
        CXPS_LOG_ERROR_INFO("CS Mode RCM - failed to get HostId from PS config file " << psConfFilePath << " with exception " << e.what());
    }
    catch (...)
    {
        CXPS_LOG_ERROR_INFO("CS Mode RCM - failed to get HostId from PS config file " << psConfFilePath << " with unknown exception.");
    }

    return;
}

ServerOptions::dirs_t const& ServerOptions::allowedDirs() const
{
    return m_allowedDirs;
}

ServerOptions::dirs_t const& ServerOptions::excludeDirs() const
{
    return m_excludeDirs;
}

bool ServerOptions::getFxAllowedDirs(ServerOptions::dirs_t& dirs)
{
    boost::filesystem::directory_iterator iterEnd;
    boost::filesystem::directory_iterator iter(fxAllowedDirsPath());
    for (/* empty */; iter != iterEnd; ++iter) {
        if (boost::filesystem::is_regular_file(iter->status())) {
            std::ifstream iFile((*iter).path().string().c_str());
            if (iFile.good()) {
                std::string line;
                std::getline(iFile, line);
                std::replace(line.begin(), line.end(), '\\', '/');
                typedef boost::tokenizer<boost::char_separator<char> > tokenizer_t;
                boost::char_separator<char> sep(";");
                tokenizer_t tokens(line, sep);
                tokenizer_t::iterator iter(tokens.begin());
                tokenizer_t::iterator iterEnd(tokens.end());
                for (/* empty*/; iter != iterEnd; ++iter) {
                    std::string str(*iter);
                    appendSlashIfNeeded(str);
                    dirs.insert(str);
                }
            }
        }
    }
    return true;
}

std::string ServerOptions::id() const
{
    return getOption("id", false, std::string());
}

std::string ServerOptions::ipAddress() const
{
    return getOption("ip", false, std::string());
}

std::string ServerOptions::port() const
{
    return getOption("port", true, std::string());
}

std::string ServerOptions::sslPort() const
{
    return  getOption("ssl_port", true, std::string());
}

std::string ServerOptions::login() const
{
    return  getOption("login", true, std::string());
}

std::string ServerOptions::password() const
{
    CSMode csMode = GetCSMode();
    THROW_ON_UNKNOWN_CS_MODE(csMode);

    std::string password;

    if (csMode == CS_MODE_LEGACY_CS) {
        password = getOption("ps_password", false, std::string());
        if (password.empty()) {
            password = securitylib::getPassphrase();
        }
    } else if (csMode == CS_MODE_RCM) {
        password = getOption("encrypted_passphrase", false, std::string());
        if (!password.empty()) {
            password = securitylib::systemDecrypt(
                securitylib::base64Decode(password.c_str(), (int)password.size()));
        }
    }

    // TODO-SanKumar-1908: Cache the decrypted value to avoid repeated reads; May be applies to CS Mode as well.

    return password;
}

bool ServerOptions::isRcmPSFirstTimeConfigured() const {
    return getOption("rcm_ps_first_time_configured", false, false);
}

std::string ServerOptions::csIpAddress() const
{
    return getOption("cs_ip_address", false, std::string());
}

std::string ServerOptions::csPort() const
{
    return getOption("cs_port", false, std::string("80"));
}

std::string ServerOptions::csSslPort() const
{
    return getOption("cs_ssl_port", false, std::string("443"));
}

std::string ServerOptions::csUrl() const
{
    return getOption("cs_url", false, std::string("/ScoutAPI/CXAPI.php"));
}

std::string ServerOptions::cfsLocalName() const
{
    // note this is actually only used on unix/linux (windows always uses loop back 127.0.0.1),
    // so ok to default it to the unix/linux location.
    return getOption("cfs_local_name", false, std::string("/usr/local/InMage/transport/cfs.ud"));
}

std::string ServerOptions::keyFilePassphrase() const
{
    return getKeyFilePassphrase(getOption("key_file_passphrase", false, std::string("inmage")));
}

boost::filesystem::path ServerOptions::certificateFile() const
{
    boost::filesystem::path path = getOption("ps_cert_file", false, boost::filesystem::path());

    if (path.empty()) {
        CSMode csMode = GetCSMode();
        THROW_ON_UNKNOWN_CS_MODE(csMode);

        if (csMode == CS_MODE_LEGACY_CS) {
            path = securitylib::getCertDir();
        } else if (csMode == CS_MODE_RCM) {
            path = installDir();
            path /= "private";
        }

        path /= "ps.crt";
    }

    return path;
}

boost::filesystem::path ServerOptions::keyFile() const
{
    boost::filesystem::path path = getOption("ps_key_file", false, boost::filesystem::path());

    if (path.empty()) {
        CSMode csMode = GetCSMode();
        THROW_ON_UNKNOWN_CS_MODE(csMode);

        if (csMode == CS_MODE_LEGACY_CS) {
            path = securitylib::getPrivateDir();
        }
        else if (csMode == CS_MODE_RCM) {
            path = installDir();
            path /= "private";
        }

        path /= "ps.key";
    }

    return path;
}

boost::filesystem::path ServerOptions::diffieHillmanFile() const
{
    boost::filesystem::path path = getOption("ps_dh_file", false, boost::filesystem::path());
    if (path.empty()) {
        CSMode csMode = GetCSMode();
        THROW_ON_UNKNOWN_CS_MODE(csMode);

        if (csMode == CS_MODE_LEGACY_CS) {
            path = securitylib::getPrivateDir();
        }
        else if (csMode == CS_MODE_RCM) {
            path = installDir();
            path /= "private";
        }

        path /= "ps.dh";
    }
    return path;
}

std::string ServerOptions::fingerprint() const
{
    boost::filesystem::path path = getOption("ps_cert_fingerprint_file", false, std::string());

    if (path.empty()) {
        CSMode csMode = GetCSMode();
        THROW_ON_UNKNOWN_CS_MODE(csMode);

        if (csMode == CS_MODE_LEGACY_CS) {
            path = securitylib::getFingerprintPathForPs();
        }
        else if (csMode == CS_MODE_RCM) {
            path = installDir();
            path /= "private";
            path /= "ps.fingerprint";
        }
    }

    std::ifstream fingerprintFile(path.string().c_str());
    std::string fingerprint;
    fingerprintFile >> fingerprint;
    return fingerprint;
}

boost::filesystem::path ServerOptions::csCertFile() const
{
    boost::filesystem::path path = getOption("cs_cert_file", false, boost::filesystem::path());
    if (path.empty()) {
        path = securitylib::getCertPath(csIpAddress(), csSslPort());
    }
    return path;
}

boost::filesystem::path ServerOptions::installDir() const
{
    // NOTE: this is speical cased because the getOption for paths uses
    // this to figure out the dir to prepend when a relative path is used
    // so do not use getOption function here to avoid infinite loop if
    // the install_dir has a relative path in the configuration file
    // which is not actually allowed but could be done by accident
    boost::filesystem::path defaultValue(m_defaultDir);
    tagValue_t::const_iterator iter(m_options.find("install_dir"));
    if (m_options.end() != iter) {
        if (!(*iter).second.empty()) {
            boost::filesystem::path pathName((*iter).second);
            if (!pathName.has_root_path()) {
                throw ERROR_EXCEPTION << "install_dir must be a full path name";
            }
            return (*iter).second;
        }
    }
    return defaultValue;
}

boost::filesystem::path ServerOptions::requestDefaultDir() const
{
    return getOption("request_default_dir", false, boost::filesystem::path(installDir() /= "data"));
}

boost::filesystem::path ServerOptions::fxAllowedDirsPath() const
{
    return getOption("fx_allowed_dirs_path", false, boost::filesystem::path(DEFAULT_FX_ALLOWED_DIRS_PATH));
}

boost::filesystem::path ServerOptions::errorLogFile() const
{
    return getOption("error_log", false, boost::filesystem::path(DEFAULT_LOG_DIR DEFAULT_NAME ".err.log"));
}

boost::filesystem::path ServerOptions::xferLogFile() const
{
    return getOption("xfer_log", false, boost::filesystem::path(DEFAULT_LOG_DIR DEFAULT_NAME ".xfer.log"));
}

boost::filesystem::path ServerOptions::monitorLogFile() const
{
    return getOption("monitor_log", false, boost::filesystem::path(DEFAULT_LOG_DIR DEFAULT_NAME ".monitor.log"));
}

boost::filesystem::path ServerOptions::cfsCacheFile() const
{
    return getOption("cfs_cache_file", false, boost::filesystem::path(DEFAULT_LOG_DIR DEFAULT_NAME ".cfs.cache"));
}

ServerOptions::remapPrefixFromTo_t ServerOptions::remapFullPathPrefix() const
{
    std::string fromTo = getOption("remap_full_path_prefix", false, std::string());
    if (fromTo.empty()) {
        return remapPrefixFromTo_t(std::make_pair(std::string(), std::string()));
    }

    // have a remap setting
    std::string::size_type len(fromTo.size());

    char stopChar;

    // get <from prefix> start
    // skip over leading white space
    std::string::size_type fromStartIdx(0);
    while (fromStartIdx < len && std::isspace(fromTo[fromStartIdx])) {
        ++fromStartIdx;
    }

    // check if <from prefix> enclosed in double quotes
    if ('"' == fromTo[fromStartIdx]) {
        ++fromStartIdx;
        // skip over any leading white space after a double quote
        while (fromStartIdx < len && std::isspace(fromTo[fromStartIdx])) {
            ++fromStartIdx;
        }
        stopChar = '"';
    } else {
        stopChar = ' ';
    }

    // find <from prefix> end
    std::string::size_type fromEndIdx(fromStartIdx);
    while (fromEndIdx < len && (('"' == stopChar && '"' != fromTo[fromEndIdx]) || (' ' == stopChar && !std::isspace(fromTo[fromEndIdx])))) {
        if ('\\' == fromTo[fromEndIdx]) {
            fromTo[fromEndIdx] = '/'; // just to be safe convert backslash to slash
        }
        ++fromEndIdx;
    }

    // this is where to start looking for the <to prefix>
    std::string::size_type toStartIdx(fromEndIdx + 1);

    // if the <from prefix> was enclosed in double quotes trim
    // any trailing white space between <from prefix> and its
    // closing double quote
    if ('"' == stopChar && std::isspace(fromTo[fromEndIdx - 1])) {
        do {
            --fromEndIdx;
        } while (std::isspace(fromTo[fromEndIdx]));
        ++fromEndIdx;
    }

    // skip all white space between <from prefix> and <to prefix>
    while (toStartIdx < len && std::isspace(fromTo[toStartIdx])) {
        ++toStartIdx;
    }

    // check if <to prefix> enclosed in double quotes
    if ('"' == fromTo[toStartIdx]) {
        ++toStartIdx;
        // skip over any leading white space after a double quote
        while (toStartIdx < len && std::isspace(fromTo[toStartIdx])) {
            ++toStartIdx;
        }
        stopChar = '"';
    } else {
        stopChar = ' ';
    }

    // find <to prefix> end
    std::string::size_type toEndIdx(toStartIdx);
    while (toEndIdx < len && (('"' == stopChar && '"' != fromTo[toEndIdx]) || (' ' == stopChar && !std::isspace(fromTo[toEndIdx])))) {
        if ('\\' == fromTo[toEndIdx]) {
            fromTo[toEndIdx] = '/'; // just to be safe convert backslash to slash
        }
        ++toEndIdx;
    }

    // if the <to prefix> was enclosed in double quotes trim
    // any trailing white space between <to prefix> and its
    // closing double quote
    if ('"' == stopChar && std::isspace(fromTo[toEndIdx - 1])) {
        do {
            --toEndIdx;
        } while (std::isspace(fromTo[toEndIdx]));
        ++toEndIdx;
    }

    return remapPrefixFromTo_t(make_pair(std::string(fromTo.substr(fromStartIdx, fromEndIdx - fromStartIdx)),
                                         std::string(fromTo.substr(toStartIdx, toEndIdx - toStartIdx))));
}

int ServerOptions::maxThreads() const
{
    return (getOption("max_threads", false, 2) * boost::thread::hardware_concurrency());
}

int ServerOptions::maxBufferSizeBytes() const
{
    return getOption("max_buffer_size_bytes", false, 1024*1024);
}

int ServerOptions::sendWindowSizeBytes() const
{
    return getOption("send_window_size_bytes", false, 0);
}

int ServerOptions::receiveWindowSizeBytes() const
{
    return getOption("receive_window_size_bytes", false, 0);
}

int ServerOptions::sessionTimeoutSeconds() const
{
    return getOption("session_timeout_seconds", false, 60*5);
}

int ServerOptions::errorLogMaxSizeBytes() const
{
    return getOption("error_log_max_size_bytes", false, DEFAULT_LOG_SIZE);
}

int ServerOptions::errorLogRotateCount() const
{
    return getOption("error_log_rotate_count", false, 0);
}

int ServerOptions::errorLogRetainSizeBytes() const
{
    return getOption("error_log_retain_size_bytes", false, DEFAULT_LOG_RETAIN_SIZE);
}

int ServerOptions::xferLogMaxSizeBytes() const
{
    return getOption("xfer_log_max_size_bytes", false, DEFAULT_LOG_SIZE);
}

int ServerOptions::xferLogRotateCount() const
{
    return getOption("xfer_log_rotate_count", false, 0);
}

int ServerOptions::xferLogRetainSizeBytes() const
{
    return getOption("xfer_log_retain_size_bytes", false, DEFAULT_LOG_RETAIN_SIZE);
}

int ServerOptions::monitorLogMaxSizeBytes() const
{
    return getOption("monitor_log_max_size_bytes", false, DEFAULT_LOG_SIZE);
}

int ServerOptions::monitorLogRotateCount() const
{
    return getOption("monitor_log_rotate_count", false, 2);
}

int ServerOptions::monitorLogRetainSizeBytes() const
{
    return getOption("monitor_log_retain_size_bytes", false, DEFAULT_LOG_RETAIN_SIZE);
}

int ServerOptions::initialReadSize() const
{
    return getOption("init_read_size", false, 0);
}

int ServerOptions::monitorLogLevel() const
{
    return getOption("monitor_log_level", false, MONITOR_LOG_LEVEL_2);
}

int ServerOptions::writeMode() const
{
    return getOption("write_mode", false, (int)WRITE_MODE_NORMAL);
}

int ServerOptions::delaySessionDeleteSeconds() const
{
    return getOption("delay_session_delete_seconds", false, 30);
}

int ServerOptions::cfsGetConnectInfoIntervalSeconds() const
{
    return getOption("cfs_monitor_interval_seconds", false, 60);
}

int ServerOptions::cfsGetWorkerIntervalSeconds() const
{
    return getOption("cfs_worker_interval_seconds", false, 60);
}

int ServerOptions::cfsMonitorIntervalSeconds() const
{
    return getOption("cfs_get_connect_info_interval_seconds", false, 60);
}

int ServerOptions::cfsLocalPort() const
{
    return getOption("cfs_port", false, 9082);
}

int ServerOptions::cfsLoginRetry() const
{
    return getOption("cfs_login_retry", false, 3);
}

long ServerOptions::cnonceDurationSeconds() const
{
    return getOption("cnonce_duration_seconds", false, 1800); // 15 minutes (more if cxps clock is behind client)
}

bool ServerOptions::errorLogWarningsEnabled() const
{
    return getOption("error_log_warnings_enabled", false, false);
}

bool ServerOptions::xferLogEnabled() const
{
    return getOption("xfer_log_enabled", false, false);
}

bool ServerOptions::monitorLogEnabled() const
{
    return getOption("monitor_log_enabled", false, false);
}

bool ServerOptions::createPaths() const
{
    return getOption("create_paths", false, false);
}

bool ServerOptions::copyOnRenameLinkFailure() const
{
    return getOption("copy_on_rename_link_failure", false, false);
}

bool ServerOptions::checkForEmbeddedRequest() const
{
    return getOption("check_for_embedded_request", false, false);
}

bool ServerOptions::cfsMode() const
{
    return getOption("cfs_mode", false, false);
}

bool ServerOptions::cfsSecureLogin() const
{
    return getOption("cfs_secure_login", false, true);
}

bool ServerOptions::cfsRejectNonSecureRequests() const
{
    return getOption("cfs_reject_non_secure_requests", false, true);
}

bool ServerOptions::csUseSecure() const
{
    return getOption("cs_use_secure", false, true);
}

bool ServerOptions::delayCfsFwdConnect() const
{
    return getOption("delay_cfs_fwd_connect", false, false);
}

int ServerOptions::httpEnabled() const
{
    return getOption("http_enabled", false, 0);
}

int ServerOptions::cumulativeThrottleTimeoutInSec() const
{
    return getOption("cumulative_throttle_timeout_sec", false, 10);
}

float ServerOptions::cumulativeThrottleUsedSpaceThreshold() const
{
    return getOption("cumulative_throttle_used_space_threshold_fraction", false, 0.8F);
}

unsigned long long ServerOptions::cumulativeThrottleThresholdInBytes() const
{
    return getOption("cumulative_throttle_threshold_bytes", false, (unsigned long long)1024 * 1024 * 1024 * 50);
}

bool ServerOptions::enableSizeBasedCumulativeThrottle() const
{
    return getOption("enable_size_based_cumulative_throttle", false, false);
}

bool ServerOptions::enableCumulativeThrottle() const
{
    return getOption("enable_cumulative_throttle", false, true);
}

bool ServerOptions::enableDiffAndResyncThrottle() const
{
    return getOption("enable_diff_resync_throttle", false, true);
}

int ServerOptions::diffResyncThrottleTimeoutInSec() const
{
    return getOption("diff_resync_throttle_timeout_sec", false, 60);
}

unsigned long long ServerOptions::diffThrottleThresholdInBytes() const
{
    return getOption("diff_throttle_threshold_bytes", false, (unsigned long long)1024 * 1024 * 1024 * 8);
}

unsigned long long ServerOptions::resyncThrottleThresholdInBytes() const
{
    return getOption("resync_throttle_threshold_bytes", false, (unsigned long long)1024 * 1024 * 1024 * 8);
}

unsigned long long ServerOptions::defaultThrottleThresholdInBytes() const
{
    return getOption("default_throttle_threshold_bytes", false, (unsigned long long)1024 * 1024 * 1024);
}

int ServerOptions::diffResyncThrottleCacheExpiryIntervalInSec() const
{
    return getOption("diff_resync_throttle_cache_expiry_interval_sec", false, 60);
}

// cache for unprotected pairs is invalidated slowly as ideally we should not get any such request
int ServerOptions::nonProtectedPairPruneTimeoutInSec() const
{
    return getOption("non_protected_pair_prune_timeout_sec", false, 300);
}

boost::tribool ServerOptions::inlineCompression() const
{
    std::string inlineCompress = getOption("inline_compression", false, std::string("pair"));
    if (inlineCompress == "none") {
        return false;
    }
    if (inlineCompress == "all") {
        return boost::indeterminate;
    }
    return true;
}

unsigned long ServerOptions::recycleHandleCount() const
{
    return getOption("recycle_handle_count", false, (unsigned long)81920);
}

uint64_t ServerOptions::acceptRetryIntervalSecs() const
{
    return getOption("accept_retry_interval_secs", false, 5ull);
}

uint64_t ServerOptions::psSettingsPollIntervalSecs() const
{
    return getOption("ps_settings_poll_interval_secs", false, 60ull);
}

bool ServerOptions::psSettingsIncludesHeader() const
{
    return getOption("ps_settings_includes_header", false, true);
}

bool ServerOptions::psSettingsEnforceMajorVersionCheck() const
{
    return getOption("ps_settings_enforce_major_version_check", false, true);
}

bool ServerOptions::psSettingsEnforceMinorVersionCheck() const
{
    return getOption("ps_settings_enforce_minor_version_check", false, false);
}

bool ServerOptions::psSettingsVerifyHeader() const
{
    return getOption("ps_settings_verify_header", false, true);
}

std::string ServerOptions::getCaCertThumbprint() const
{
    return getOption("cxps_ca_cert_thumbprint", false, std::string());
}

bool ServerOptions::useCertBasedClientAuth() const
{
    return !getCaCertThumbprint().empty();
}

void ServerOptions::buildAllowedDirs(boost::filesystem::path const& requestDir)
{
    ServerOptions::remapPrefixFromTo_t fromTo = remapFullPathPrefix();

    std::string dirsOption = getOption("allowed_dirs", false, std::string());
    std::replace(dirsOption.begin(), dirsOption.end(), '\\', '/');
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer_t;
    boost::char_separator<char> sep(";");
    tokenizer_t tokens(dirsOption, sep);
    tokenizer_t::iterator iter(tokens.begin());
    tokenizer_t::iterator iterEnd(tokens.end());
    for (/* empty*/; iter != iterEnd; ++iter) {
        if (IS_EQUAL(*iter, fromTo.first) || IS_EQUAL(*iter, fromTo.second)) {
            std::string str(fromTo.first);
            appendSlashIfNeeded(str);
            m_allowedDirs.insert(str);
            str = fromTo.second;
            appendSlashIfNeeded(str);
            m_allowedDirs.insert(str);
        } else {
            std::string str(*iter);
            appendSlashIfNeeded(str);
            m_allowedDirs.insert(str);
        }
    }

        // play it safe and add default with out driver letter
        m_allowedDirs.insert("/home/svsystems/");

    if (boost::algorithm::istarts_with(fromTo.first, "/home/svsystems")) {
        std::string str(fromTo.second);
        appendSlashIfNeeded(str);
        m_allowedDirs.insert(str);
    }
    if (!requestDir.empty()) {
        std::string str(requestDir.string());
        std::replace(str.begin(), str.end(), '\\', '/');
        appendSlashIfNeeded(str);
        m_allowedDirs.insert(str);
    }
}

void ServerOptions::buildExcludeDirs()
{
    ServerOptions::remapPrefixFromTo_t fromTo = remapFullPathPrefix();
    dirs_t dirsAllowed = allowedDirs();
    dirs_t::iterator allowedIter;
    dirs_t::iterator allowedEnd(dirsAllowed.end());
    std::string dirsOption = getOption("exclude_dirs", false, std::string());
    std::replace(dirsOption.begin(), dirsOption.end(), '\\', '/');
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer_t;
    boost::char_separator<char> sep(";");
    tokenizer_t tokens(dirsOption, sep);
    tokenizer_t::iterator iter(tokens.begin());
    tokenizer_t::iterator iterEnd(tokens.end());
    for (/* empty*/; iter != iterEnd; ++iter) {
        boost::filesystem::path p(*iter);
        if (!p.has_root_path()) {
            allowedIter = dirsAllowed.begin();
            for (/* empty */; allowedIter != allowedEnd; ++allowedIter) {
                std::string dir(*allowedIter);
                appendSlashIfNeeded(dir);
                dir += *iter;
                m_excludeDirs.insert(dir);
            }
        } else {
            // full path need to check for remapping so that it can be added
            if (STARTS_WITH(*iter, fromTo.first)) {
                std::string str(*iter);
                boost::algorithm::ireplace_first(str, fromTo.first, fromTo.second);
                m_excludeDirs.insert(str);
            } else if (STARTS_WITH(*iter, fromTo.second)) {
                std::string str(*iter);
                boost::algorithm::ireplace_first(str, fromTo.second,  fromTo.first);
                m_excludeDirs.insert(str);
            }
            std::string str(*iter);
            m_excludeDirs.insert(str);
        }
    }
}

void ServerOptions::addOption(std::string line)
{
    if ('#' != line[0] && '[' != line[0]) {
        std::string data(line);
        std::string::size_type idx = data.find_first_of("=");
        if (std::string::npos != idx) {
            std::string tag(data.substr(0, idx));
            boost::algorithm::trim(tag);

            std::string value(data.substr(idx + 1));
            boost::algorithm::trim(value);
            m_options.insert(std::make_pair(tag, value));
        }
    }
}

template <typename T>
T ServerOptions::getOption(char const* option, bool required, T defaultValue) const {

    std::string value;

    if (getOption(option, value)) {
        try {
            return boost::lexical_cast<T>(value);
        } catch (...) {
            // prevent exception from being thrown, default value will be returned
        }
    }

    if (!required) {
        return defaultValue;
    }

    throw ERROR_EXCEPTION << "required option " << option << " not specfied in configuration file";
}

bool ServerOptions::getOption(char const* option, bool required, bool defaultValue) const {

    std::string value;

    if (getOption(option, value)) {
        try {
            return (value == "yes");
        } catch (...) {
            // prevent exception from being thrown, default value will be returned
        }
    }

    if (!required) {
        return defaultValue;
    }

    throw ERROR_EXCEPTION << "required option " << option << " not specfied in configuration file";
}

std::string ServerOptions::getOption(char const* option, bool required, std::string const& defaultValue) const {

    std::string value;

    if (getOption(option, value)) {
        return value;
    }

    if (!required) {
        return defaultValue;
    }

    throw ERROR_EXCEPTION << "required option " << option << " not specfied in configuration file";
}

boost::filesystem::path ServerOptions::getOption(char const* option,
                                                 bool required,
                                                 boost::filesystem::path const& defaultValue) const {

    std::string value;
    boost::filesystem::path pathName;

    if (getOption(option, value)) {
        pathName = value;
        if (!pathName.has_root_path()) {
            pathName = installDir();
            pathName /= value;
        }
    } else if (!required) {
        if (!defaultValue.empty()) {
            if (!defaultValue.has_root_path()) {
                pathName = installDir();
            }
            pathName /= defaultValue;
        }
    } else {
        throw ERROR_EXCEPTION << "required option " << option << " not specfied in configuration file";
    }

    return pathName;
}

bool ServerOptions::getOption(char const* option, std::string& value) const {

    tagValue_t::const_iterator optionsIter(m_options.find(option));

    if (m_options.end() != optionsIter && !optionsIter->second.empty()) {
        value = optionsIter->second;
        return true;
    } else {
        // Take a reference to the shared pointer, since it could be automically
        // updated by another thread.
        PSSettings::StringMapPtr tunMapPtr = m_tunablesPtr;

        if (tunMapPtr) {
            PSSettings::StringMap::const_iterator tunMapIter = tunMapPtr->find(option);
            if (tunMapPtr->end() != tunMapIter && !tunMapIter->second.empty()) {
                value = tunMapIter->second;
                return true;
            }
        }
    }

    return false;
}

void ServerOptions::updateTunables(PSSettings::StringMapPtr latestTunables)
{
    boost::atomic_exchange(&m_tunablesPtr, latestTunables);
}

std::string ServerOptions::getKeyFilePassphrase(std::string const& str) const
{
    if (!boost::algorithm::starts_with(str, "exec:")) {
        return str;
    }
    std::string::size_type idx = str.find_first_of(":");
    if (std::string::npos == idx) {
        return str;
    }
    std::string prog = boost::algorithm::trim_copy(str.substr(idx + 1));
    std::string passphrase;
    FIO::cmdArgs_t vargs;
    vargs.push_back(prog.c_str());
    vargs.push_back((char*)0);
    FIO::FioPipe fioPipe(vargs);
    char buffer[1024];
    long bytes = 0;
    bool wait = true;
    do {
        bytes = fioPipe.read(buffer, sizeof(buffer), wait);
        if (bytes > 0) {
            passphrase.append(buffer, bytes);
            wait = false;
        }
    } while (bytes > 0);
    if (!passphrase.empty()) {
        boost::algorithm::trim(passphrase);
        if ('"' == passphrase[0] && '"' == passphrase[passphrase.size() - 1]) {
            return passphrase.substr(1, passphrase.size() - 2);
        }
    }
    return passphrase;
}
