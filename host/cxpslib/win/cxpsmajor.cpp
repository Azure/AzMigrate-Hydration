#include <boost/atomic.hpp>
#include <boost/lexical_cast.hpp>

#include "cxps.h"
#include "cxpslogger.h"

#define OpenRcmPSRegKey(hkey) RegOpenKeyExW(\
                                    HKEY_LOCAL_MACHINE,\
                                    L"SOFTWARE\\Microsoft\\Azure Site Recovery Process Server",\
                                    0,\
                                    KEY_WOW64_64KEY | KEY_QUERY_VALUE,\
                                    &hkey)

static boost::once_flag s_psInstallRegReadOnce = BOOST_ONCE_INIT;
static CSMode s_csMode = CS_MODE_UNKNOWN;
static PSInstallationInfo s_rcmPS_InstallationInfo;

static std::wstring ReadStringValue(HKEY hkey, const std::wstring &name, bool ignoreIfNotFound = false)
{
    std::vector<WCHAR> stringValueBuff;
    DWORD valueSize = 0;

    auto error = RegQueryValueExW(hkey, name.c_str(), NULL, NULL, NULL, &valueSize);

    if (ignoreIfNotFound && error == ERROR_FILE_NOT_FOUND)
        return std::wstring();

    if (error != ERROR_SUCCESS)
    {
        boost::system::system_error err(error, boost::system::system_category());
        auto errString = err.what();

        CXPS_LOG_ERROR(
            THROW_LOC << "Failed to read string value from registry key " << hkey
            << " under name " << name << " - (" << error << ") " << errString);

        throw err;
    }

    stringValueBuff.assign(valueSize / sizeof(WCHAR), L'\0');

    error = RegQueryValueExW(hkey, name.c_str(), NULL, NULL, reinterpret_cast<LPBYTE>(stringValueBuff.data()), &valueSize);

    if (error != ERROR_SUCCESS)
    {
        boost::system::system_error err(error, boost::system::system_category());
        auto errString = err.what();

        CXPS_LOG_ERROR(
            THROW_LOC << "Failed to read string value from registry key " << hkey
            << " under name " << name << " - (" << error << ") " << errString);

        throw err;
    }

    // The bytes returned may (or) may not contain NULL in the end, based on if
    // it was provided, when the value was set. The following handling avoids
    // including the NULL character in either case on creating the string object.
    auto valueStrLen = wcsnlen_s(stringValueBuff.data(), valueSize / sizeof(WCHAR));

    return std::wstring(stringValueBuff.data(), valueStrLen);
}

static std::wstring ReadExpandableStringValue(HKEY hkey, const std::wstring &name, bool ignoreIfNotFound = false)
{
    std::vector<WCHAR> expStringValueBuff;

    std::wstring stringValue = ReadStringValue(hkey, name, ignoreIfNotFound);

    DWORD size = ExpandEnvironmentStringsW(stringValue.c_str(), NULL, 0);

    if (size == 0)
    {
        boost::system::system_error err(GetLastError(), boost::system::system_category());
        auto errString = err.what();

        CXPS_LOG_ERROR(
            THROW_LOC << "Failed to expand string value " << stringValue
            << " from registry key " << hkey
            << " under name " << name << " - (" << err.code() << ") " << errString);

        throw err;
    }

    expStringValueBuff.assign(size, L'\0');

    size = ExpandEnvironmentStringsW(stringValue.c_str(), expStringValueBuff.data(), size);

    if (size == 0)
    {
        boost::system::system_error err(GetLastError(), boost::system::system_category());
        auto errString = err.what();

        CXPS_LOG_ERROR(
            THROW_LOC << "Failed to expand string value " << stringValue
            << " from registry key " << hkey
            << " under name " << name << " - (" << err.code() << ") " << errString);

        throw err;
    }

    return std::wstring(expStringValueBuff.data(), size - 1); // Excluding the NULL character
}

static LSTATUS RegCloseKeyWrapper(HKEY key)
{
    // Simplest fix to use boost::bind on __stdcall
    return RegCloseKey(key);
}

static void ReadPSInstallationRegistryKey()
{
    HKEY hkey;
    std::wstring csModeStr;
    s_csMode = CS_MODE_UNKNOWN;

    DWORD error = OpenRcmPSRegKey(hkey);

    if (error == ERROR_FILE_NOT_FOUND)
    {
        CXPS_LOG_ERROR(AT_LOC << "PS installation registry key is not found");
        return;
    }

    if (error != ERROR_SUCCESS)
    {
        boost::system::system_error err(GetLastError(), boost::system::system_category());
        CXPS_LOG_ERROR(AT_LOC << "Failed to open PS installation registry key - (" << err.code() << ") " << err.what());
        return;
    }

    SCOPE_GUARD regKeyGuard = MAKE_SCOPE_GUARD(boost::bind(RegCloseKeyWrapper, hkey));

    try
    {
        csModeStr = ReadStringValue(hkey, L"CS Mode");
    }
    catch(...)
    {
        CXPS_LOG_ERROR(CATCH_LOC << "Failed to read CSMode from PS installation registry key. Marking as Unknown.");
        return;
    }

    if (_wcsicmp(csModeStr.c_str(), CSMode_String::LegacyCS.c_str()) == 0)
    {
        s_csMode = CS_MODE_LEGACY_CS;
        return;
    }

    if (_wcsicmp(csModeStr.c_str(), CSMode_String::Rcm.c_str()) != 0)
    {
        CXPS_LOG_ERROR(AT_LOC << "CS mode " << csModeStr << " is not RCM as expected. Marking as Unknown.");
        return;
    }

    s_csMode = CS_MODE_RCM;

    try
    {
        // TODO-SanKumar-1908: extra handling for long path?

        s_rcmPS_InstallationInfo.m_installLocation = ReadExpandableStringValue(hkey, L"Install Location");
        s_rcmPS_InstallationInfo.m_logFolderPath = ReadExpandableStringValue(hkey, L"Log Folder Path", true);
        s_rcmPS_InstallationInfo.m_configuratorPath = ReadExpandableStringValue(hkey, L"Configurator Path");
        s_rcmPS_InstallationInfo.m_telemetryFolderPath = ReadExpandableStringValue(hkey, L"Telemetry Folder Path", true);
        s_rcmPS_InstallationInfo.m_reqDefFolderPath = ReadExpandableStringValue(hkey, L"Request Default Folder Path", true);
        s_rcmPS_InstallationInfo.m_settingsPath = ReadExpandableStringValue(hkey, L"Settings Path");
        s_rcmPS_InstallationInfo.m_idempotencyLckFilePath = ReadExpandableStringValue(hkey, L"Idempotency Lock File Path");
    }
    catch (...)
    {
        // If there are any failure in retrieving the PS installation info from
        // inside the registry key, ignore the failure and continue. Not failing,
        // since this information is used just for logging purposes in cxps.

        CXPS_LOG_WARNING(CATCH_LOC << "Failed to read RCM PS installation information from Registry key.");
    }
}

CSMode GetCSMode()
{
    boost::call_once(ReadPSInstallationRegistryKey, s_psInstallRegReadOnce);

    return s_csMode;
}

const PSInstallationInfo& GetRcmPSInstallationInfo()
{
    CSMode csMode = GetCSMode();

    if (csMode != CS_MODE_RCM)
    {
        std::stringstream ss;
        ss << "Unexpected call, as CS mode is " << csMode;

        CXPS_LOG_ERROR(THROW_LOC << ss.str());
        throw std::runtime_error(ss.str());
    }

    return s_rcmPS_InstallationInfo;
}
