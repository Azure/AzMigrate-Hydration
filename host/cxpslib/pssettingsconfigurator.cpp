#include <boost/filesystem/string_file.hpp>
#include <boost/make_shared.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>

#include "Telemetry/TelemetryCommon.h"
#include "pssettingsconfigurator.h"
#include "serveroptions.h"

using namespace PSSettings;

PSSettingsConfigurator PSSettingsConfigurator::s_instance;

const char* CacheDataHeader::CHECKSUM_TYPE_MD5 = "MD5";
const char* CacheDataHeader::CURRENT_CACHED_DATA_VERSION = "1.7";
const int CacheDataHeader::CURRENT_CACHED_DATA_MAJOR_VERSION = 1;
const int CacheDataHeader::CURRENT_CACHED_DATA_MINOR_VERSION = 7;

PSSettingsPtr PSSettingsConfigurator::ReadSettingsFromFile(
    bool& fileUnavailable,
    std::string& knownToFailHeader,
    std::string& knownToFailContent)
{
    fileUnavailable = true;

    if (!boost::filesystem::exists(m_settingsLckFilePath) ||
        !boost::filesystem::exists(m_settingsFilePath))
    {
        return PSSettingsPtr();
    }

    fileUnavailable = false;

    boost::system::error_code ec;
    time_t t = boost::filesystem::last_write_time(m_settingsFilePath, ec);
    if (ec)
    {
        t = 0;
    }

    if (!t && m_MTime == t)
    {
        return PSSettingsPtr();
    }

    std::string headerStr;
    std::string settingsFileContent;

    // TODO-SanKumar-2002: This constructor throws, if the lock file is not
    // present. Instead only expect the actual setting file to present, while
    // creating this file, if not present.
    boost::interprocess::file_lock flock(m_settingsLckFilePath.string().c_str());

    {
        boost::interprocess::sharable_lock<boost::interprocess::file_lock> sharableFlock(flock);

        // TODO-SanKumar-2002: This lock is not cancellable, which could be
        // replaced with Timed sharable_lock. The problem is that the lock seems
        // to be spinning repeatedly instead of blocking. Gotta check before usage.

        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        file.open(m_settingsFilePath.string().c_str(), std::ios_base::binary);

        if (m_serverOptions->psSettingsIncludesHeader())
        {
            std::getline(file, headerStr);
        }

        std::size_t sz = static_cast<std::size_t>(boost::filesystem::file_size(m_settingsFilePath));
        sz -= file.tellg();
        settingsFileContent.resize(sz, '\0');
        file.read(&settingsFileContent[0], sz);
    }

    if (headerStr == knownToFailHeader &&
        settingsFileContent == knownToFailContent)
    {
        // By doing this precheck, we avoid repeated logging for a known
        // bad file, which will keep on failing.

        return PSSettingsPtr();

        // TODO-SanKumar-2002: Rate controlled log (say once in 10/25 times) in
        // both +ve and -ve cases.
    }

    if ((m_serverOptions->psSettingsIncludesHeader() && headerStr.empty()) ||
        (settingsFileContent.empty()))
    {
        CXPS_LOG_ERROR(AT_LOC << "Header line or content missing in the cache settings file");

        knownToFailHeader = headerStr;
        knownToFailContent = settingsFileContent;
        return PSSettingsPtr();
    }

    if ((m_serverOptions->psSettingsIncludesHeader()) &&
        (m_serverOptions->psSettingsEnforceMajorVersionCheck() ||
         m_serverOptions->psSettingsVerifyHeader()))
    {
        CacheDataHeader parsedHeader;
        try
        {
            parsedHeader =
                JSON::consumer<CacheDataHeader>::convert(headerStr, true); // std::move() candidate
        }
        catch (const std::exception & ex)
        {
            CXPS_LOG_ERROR(AT_LOC <<
                "Json serialization failed for the header in the cached settings file - " << ex.what());

            knownToFailHeader = headerStr;
            knownToFailContent = settingsFileContent;
            return PSSettingsPtr();
        }

        if (m_serverOptions->psSettingsEnforceMajorVersionCheck())
        {
            if (parsedHeader.Version.empty())
            {
                CXPS_LOG_ERROR(AT_LOC << "Empty version found in the cached settings file");

                knownToFailHeader = headerStr;
                knownToFailContent = settingsFileContent;
                return PSSettingsPtr();
            }

            std::vector<std::string> versionParts;
            boost::split(versionParts, parsedHeader.Version, boost::is_any_of("."));

            int majorVersion = boost::lexical_cast<int>(versionParts[0]);

            int minorVersion = 0;
            if (versionParts.size() != 1)
            {
                minorVersion = boost::lexical_cast<int>(versionParts[1]);
            }

            if (majorVersion != CacheDataHeader::CURRENT_CACHED_DATA_MAJOR_VERSION)
            {
                CXPS_LOG_ERROR(AT_LOC <<
                    "Major version of the cached settings file - " << majorVersion <<
                    " doesn't match the expected major version - " <<
                    CacheDataHeader::CURRENT_CACHED_DATA_MAJOR_VERSION);

                knownToFailHeader = headerStr;
                knownToFailContent = settingsFileContent;
                return PSSettingsPtr();
            }

            if (m_serverOptions->psSettingsEnforceMinorVersionCheck() &&
                minorVersion != CacheDataHeader::CURRENT_CACHED_DATA_MINOR_VERSION)
            {
                CXPS_LOG_ERROR(AT_LOC <<
                    "Minor version of the cached settings file - " << minorVersion <<
                    " doesn't match the expected minor version - " <<
                    CacheDataHeader::CURRENT_CACHED_DATA_MINOR_VERSION);

                knownToFailHeader = headerStr;
                knownToFailContent = settingsFileContent;
                return PSSettingsPtr();
            }
        }

        if (m_serverOptions->psSettingsVerifyHeader() &&
            !parsedHeader.IsMatchingContent(settingsFileContent))
        {
            CXPS_LOG_ERROR(AT_LOC <<
                "Checksum validation failed for the cached settings file - " <<
                parsedHeader.Checksum << " (" << parsedHeader.ChecksumType << ")");

            knownToFailHeader = headerStr;
            knownToFailContent = settingsFileContent;
            return PSSettingsPtr();
        }
    }

    // the contents of the file are good, so update the cached time and content
    m_MTime = t;

    if (m_settingsFileContent == settingsFileContent)
    {
        return PSSettingsPtr();
    }

    PSSettingsPtr parsedSettingsPtr;
    try
    {
        parsedSettingsPtr = boost::make_shared<PSSettings>(
            JSON::consumer<PSSettings>::convert(settingsFileContent, true)); // std::move() candidate

        m_settingsFileContent = settingsFileContent;
    }
    catch (const std::exception &ex)
    {
        CXPS_LOG_ERROR(AT_LOC <<
            "Json serialization failed for the content in the cached settings file - " << ex.what());

        knownToFailHeader = headerStr;
        knownToFailContent = settingsFileContent;
        return PSSettingsPtr();
    }

    // TODO-SanKumar-2002: By setting these values even in +ve case, we could
    // avoid unnecessary JSON parsings as well as callbacks at the caller. Adverse
    // effect would be that if any failed callbacks / other intermittent logic
    // errors wouldn't be retried.
    knownToFailHeader.clear();
    knownToFailContent.clear();
    return parsedSettingsPtr;
}

void PSSettingsConfigurator::ProcessNewSettings(PSSettingsPtr newSettings, bool fileUnavailable)
{
    if (!newSettings && !fileUnavailable)
        return; // Failure in reading the latest settings file; ignore.

    PSSettingsPtr oldSettings = boost::atomic_exchange(&m_psSettings, newSettings);

    NotifyTunablesChange(!newSettings? StringMapPtr() : newSettings->Tunables);
}

void PSSettingsConfigurator::Initialize(
    const boost::filesystem::path& settingsFilePath,
    const boost::filesystem::path& settingsLckFilePath,
    serverOptionsPtr serverOptions)
{
    m_settingsFilePath = settingsFilePath;
    m_settingsLckFilePath = settingsLckFilePath;
    m_serverOptions = serverOptions;
    m_MTime = 0;

    try
    {
        bool fileUnavailable;
        std::string ignore1, ignore2;

        PSSettingsPtr retrievedSettings = ReadSettingsFromFile(fileUnavailable, ignore1, ignore2);
        ProcessNewSettings(retrievedSettings, fileUnavailable);
    }
    GENERIC_CATCH_LOG_IGNORE("Initial load of PS Settings from file failed")

    m_isInitialized = true;
}

void PSSettingsConfigurator::Start()
{
    if (!m_isInitialized)
    {
        BOOST_ASSERT(false);
        throw std::logic_error("PSSettingsConfigurator is started without initialization");
    }

    BOOST_ASSERT(!m_thread);
    // On thread spawn failure, the exception would stop the server. So, it's
    // guaranteed that this thread would always be running, as long as the
    // server is running.
    m_thread.reset(
        new boost::thread(boost::bind(&PSSettingsConfigurator::Run, this)));
}

void PSSettingsConfigurator::Stop()
{
    // If the thread was created, signal completion and wait for it to finish.
    if (m_thread)
    {
        BOOST_ASSERT(m_isInitialized);

        m_thread->interrupt();
        m_thread->join();
        m_thread.reset();
    }
}

void PSSettingsConfigurator::Run()
{
    size_t dbgIgnoredExceptionCnt = 0;

    CXPS_LOG_ERROR_INFO("Starting PSSettingsConfigurator thread");

    boost::chrono::steady_clock::time_point lastItrStartTime;

    std::string knownToFailHeader;
    std::string knownToFailContent;

    for(;;)
    {
        lastItrStartTime = boost::chrono::steady_clock::now();

        try
        {

            bool fileUnavailable;
            PSSettingsPtr retrievedSettings =
                ReadSettingsFromFile(fileUnavailable, knownToFailHeader, knownToFailContent);
            ProcessNewSettings(retrievedSettings, fileUnavailable);
        }
        GENERIC_CATCH_LOG_ACTION("PSSettingsConfigurator run loop", dbgIgnoredExceptionCnt++)

        try
        {
            boost::chrono::steady_clock::duration durationToWait =
                boost::chrono::seconds(m_serverOptions->psSettingsPollIntervalSecs());
            durationToWait -= boost::chrono::steady_clock::now() - lastItrStartTime; // Adjust for already spent time

            if (durationToWait < boost::chrono::steady_clock::duration::zero())
                durationToWait = boost::chrono::steady_clock::duration::zero();

            boost::this_thread::sleep_for(durationToWait);
        }
        catch (boost::thread_interrupted)
        {
            CXPS_LOG_ERROR_INFO("Stopping PSSettingsConfigurator thread");
            return;
        }
    }
}

PSSettingsConfigurator::SubscriptionNum_t PSSettingsConfigurator::SubscribeForTunablesChange(TunablesChangeNotificationCallback callback)
{
    if (!callback)
    {
        throw std::logic_error("Invalid callback used for Tunables subscription");
    }

    boost::unique_lock<boost::shared_mutex> wlock(m_tunablesCallbackMapMutex);

    SubscriptionNum_t subscriptionNumber = boost::chrono::steady_clock::now().time_since_epoch().count();
    std::pair<TunablesCallbackMap::iterator, bool> result =
        m_tunablesChangeCallbackMap.insert(std::make_pair(subscriptionNumber, callback));

    if (!result.second)
    {
        BOOST_ASSERT(false);
        throw std::logic_error("Tunables subscription internal error");
    }

    return subscriptionNumber;
}

void PSSettingsConfigurator::UnsubscribeForTunablesChange(SubscriptionNum_t subscriptionNumber)
{
    boost::unique_lock<boost::shared_mutex> wlock(m_tunablesCallbackMapMutex);

    BOOST_VERIFY(m_tunablesChangeCallbackMap.erase(subscriptionNumber) == 1);
}

void PSSettingsConfigurator::NotifyTunablesChange(StringMapPtr tunables)
{
    boost::shared_lock<boost::shared_mutex> rlock(m_tunablesCallbackMapMutex);

    for (TunablesCallbackMap::iterator cbItr = m_tunablesChangeCallbackMap.begin();
        cbItr != m_tunablesChangeCallbackMap.end();
        cbItr++)
    {
        try
        {
            cbItr->second(tunables);
        }
        GENERIC_CATCH_LOG_IGNORE(<< "Notifying tunables change for subscription number : " << cbItr->first <<);
    }
}
