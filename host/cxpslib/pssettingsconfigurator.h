#ifndef PS_SETTINGS_CONFIGURATOR_H
#define PS_SETTINGS_CONFIGURATOR_H

#include <boost/atomic.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/chrono.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <json_reader.h>
#include <map>

#include <inm_md5.h>
#include <securityutils.h>

class ServerOptions;

typedef boost::shared_ptr<ServerOptions> serverOptionsPtr;

namespace PSSettings
{
    static boost::posix_time::ptime ConvertDotNetDateTime(ptree& node, const std::string& keyName)
    {
        std::string timeUtcStr;
        JSON_P_KEYNAME(node, keyName, timeUtcStr);

        if (boost::iends_with(timeUtcStr, "z"))
            timeUtcStr.erase(timeUtcStr.size() - 1, 1);

        if (timeUtcStr == "0001-01-01T00:00:00")
        {
            // Unset DateTime objects starts from 01.01.0001 but the valid gregorian
            // date range starts from year 1400.
            return boost::posix_time::ptime(boost::gregorian::date(1400, 1, 1));
        }

        return boost::posix_time::from_iso_extended_string(timeUtcStr);
    }

    struct PSSettingsPairwise
    {
        /// \brief host ID
        std::string HostId;

        /// \brief disk ID
        std::string DeviceId;

        /// \brief Target host ID
        std::string TargetHostId;

        /// \brief Target disk ID
        std::string TargetDeviceId;

        /// \brief direction of protection
        std::string ProtectionDirection;

        /// \brief folder under which diff files go in from source
        /// (valid if IsSourceFolderSupported is set in PSSettings)
        std::string SourceDiffFolder;

        /// \brief folder under which diff files go in from source
        /// (if IsSourceFolderSupported is not set in PSSettings)
        /// or diff files go in from SourceDiffFolder (if
        /// IsSourceFolderSupported is set in PSSettings)
        std::string TargetDiffFolder;

        /// \brief folder under which resync files go in from source
        std::string ResyncFolder;

        /// \brief folder under which diff files that can create
        /// recovery point(s) is put by MT'
        std::string AzurePendingUploadRecoverableFolder;

        /// \brief folder under which diff files that can't create
        /// recovery point(s) is put by MT'
        std::string AzurePendingUploadNonRecoverableFolder;

        /// \brief maximum bytes of diff data allowed for the pair on PS
        long long DiffThrottleLimit;

        /// \brief maximum bytes of resync data allowed for the pair on PS
        long long ResyncThrottleLimit;

        /// \brief path of the internal file used to monitor incoming data
        /// outage of the pair
        std::string RPOFilePath;

        /// \brief folders to monitor for diffsync related health and statistics
        std::vector<std::string> DiffFoldersToMonitor;

        /// \brief folder to monitor for resync related health and statistics
        std::string ResyncFolderToMonitor;

        // TODO-SanKumar-2001:
        std::string ReplicationSessionId;

        // TODO-SanKumar-2001:
        std::string ReplicationState;

        /// \brief last IR progress updation time by source to the server
        boost::posix_time::ptime IrProgressUpdateTimeUtc;

        /// \brief internal folder used to monitor RPO of the pair
        std::string TagFolder;

        /// \brief context to be used on communication about the pair
        std::string MessageContext;

        /// \brief protection state for the pair
        int ProtectionState;

        void serialize(JSON::Adapter& adapter)
        {
            throw std::logic_error("Not implemented");
        }

        void serialize(ptree& node)
        {
            JSON_P(node, HostId);
            JSON_P(node, DeviceId);
            JSON_P(node, TargetHostId);
            JSON_P(node, TargetDeviceId);
            JSON_P(node, ProtectionDirection);
            JSON_P(node, SourceDiffFolder);
            JSON_P(node, TargetDiffFolder);
            JSON_P(node, ResyncFolder);
            JSON_P(node, AzurePendingUploadRecoverableFolder);
            JSON_P(node, AzurePendingUploadNonRecoverableFolder);
            JSON_P(node, DiffThrottleLimit);
            JSON_P(node, ResyncThrottleLimit);
            JSON_P(node, RPOFilePath);
            JSON_VP(node, DiffFoldersToMonitor);
            JSON_P(node, ResyncFolderToMonitor);
            JSON_P(node, ReplicationSessionId);
            JSON_P(node, ReplicationState);
            IrProgressUpdateTimeUtc = ConvertDotNetDateTime(node, "IrProgressUpdateTimeUtc");
            JSON_P(node, TagFolder);
            JSON_P(node, MessageContext);
            JSON_P(node, ProtectionState);

            // Always store host id and disk id in lower case for easier comparison of keys.
            boost::algorithm::to_lower(HostId);
            boost::algorithm::to_lower(DeviceId);
            boost::algorithm::to_lower(TargetHostId);
            boost::algorithm::to_lower(TargetDeviceId);
        }

        bool operator==(const PSSettingsPairwise& rhs)
        {
            const PSSettingsPairwise& lhs = *this;

            return
                lhs.HostId == rhs.HostId &&
                lhs.DeviceId == rhs.DeviceId &&
                lhs.TargetHostId == rhs.TargetHostId &&
                lhs.TargetDeviceId == rhs.TargetDeviceId &&
                lhs.ProtectionDirection == rhs.ProtectionDirection &&
                lhs.SourceDiffFolder == rhs.SourceDiffFolder &&
                lhs.TargetDiffFolder == rhs.TargetDiffFolder &&
                lhs.ResyncFolder == rhs.ResyncFolder &&
                lhs.AzurePendingUploadRecoverableFolder == rhs.AzurePendingUploadRecoverableFolder &&
                lhs.AzurePendingUploadNonRecoverableFolder == rhs.AzurePendingUploadNonRecoverableFolder &&
                lhs.DiffThrottleLimit == rhs.DiffThrottleLimit &&
                lhs.ResyncThrottleLimit == rhs.ResyncThrottleLimit &&
                lhs.RPOFilePath == rhs.RPOFilePath &&
                lhs.DiffFoldersToMonitor == rhs.DiffFoldersToMonitor &&
                lhs.ResyncFolderToMonitor == rhs.ResyncFolderToMonitor &&
                lhs.ReplicationSessionId == rhs.ReplicationSessionId &&
                lhs.ReplicationState == rhs.ReplicationState &&
                lhs.IrProgressUpdateTimeUtc == rhs.IrProgressUpdateTimeUtc &&
                lhs.TagFolder == rhs.TagFolder &&
                lhs.MessageContext == rhs.MessageContext &&
                lhs.ProtectionState == rhs.ProtectionState;
        }

        bool operator!=(const PSSettingsPairwise& rhs)
        {
            return !operator==(rhs);
        }
    };

    typedef boost::shared_ptr<PSSettingsPairwise> PSSettingsPairwisePtr;
    typedef std::map<std::string, PSSettingsPairwisePtr> PSSettingsPairwisePtrsMap;

    struct PSSettingsHostwise
    {
        /// \brief host ID
        std::string HostId;

        /// \brief Target host ID
        std::string TargetHostId;

        // TODO-SanKumar-2001:
        std::string ProtectionState;

        /// \brief folder under which all the resync and diff data
        /// will go into for the host.
        std::string LogRootFolder;

        /// \brief uri to report critical health and events for the host
        std::string CriticalChannelUri;

        /// \brief renewal time of CriticalChannelUri
        boost::posix_time::ptime CriticalChannelUriRenewalTimeUtc;

        /// \brief uri to report informational health and events
        std::string InformationalChannelUri;

        /// \brief renewal time of InformationalChannelUri
        boost::posix_time::ptime InformationalChannelUriRenewalTimeUtc;

        /// \brief context to be used, while reporting critical and
        /// informational health and events for the host
        std::string MessageContext;

        boost::shared_ptr<PSSettingsPairwisePtrsMap> PairwiseSettings;

        void serialize(JSON::Adapter& adapter)
        {
            throw std::logic_error("Not implemented");
        }

        void serialize(ptree& node)
        {
            std::string timeUtcStr;
            std::vector<PSSettingsPairwise> pairSettings;

            JSON_P(node, HostId);
            JSON_P(node, ProtectionState);
            JSON_P(node, LogRootFolder);
            JSON_P(node, CriticalChannelUri);
            CriticalChannelUriRenewalTimeUtc = ConvertDotNetDateTime(node, "CriticalChannelUriRenewalTimeUtc");
            JSON_P(node, InformationalChannelUri);
            InformationalChannelUriRenewalTimeUtc = ConvertDotNetDateTime(node, "InformationalChannelUriRenewalTimeUtc");
            JSON_P(node, MessageContext);

            JSON_VCL_KEYNAME(node, "Pairs", pairSettings);

            // Always store host id in lower case for easier comparison of keys.
            boost::algorithm::to_lower(HostId);

            PairwiseSettings = boost::make_shared<PSSettingsPairwisePtrsMap>();
            for (std::vector<PSSettingsPairwise>::iterator itr = pairSettings.begin(); itr != pairSettings.end(); itr++)
            {
                (*PairwiseSettings)[itr->DeviceId] = boost::make_shared<PSSettingsPairwise>(*itr); // std::move() candidate
                if (!itr->TargetDeviceId.empty() && !itr->TargetHostId.empty())
                {
                    TargetHostId = itr->TargetHostId;
                    (*PairwiseSettings)[itr->TargetDeviceId] = boost::make_shared<PSSettingsPairwise>(*itr);
                }
            }
        }

        bool leanEquals(const PSSettingsHostwise& rhs)
        {
            const PSSettingsHostwise& lhs = *this;

            return
                lhs.HostId == rhs.HostId &&
                lhs.ProtectionState == rhs.ProtectionState &&
                lhs.LogRootFolder == rhs.LogRootFolder &&
                lhs.CriticalChannelUri == rhs.CriticalChannelUri &&
                lhs.CriticalChannelUriRenewalTimeUtc == rhs.CriticalChannelUriRenewalTimeUtc &&
                lhs.InformationalChannelUri == rhs.InformationalChannelUri &&
                lhs.InformationalChannelUriRenewalTimeUtc == rhs.InformationalChannelUriRenewalTimeUtc &&
                lhs.MessageContext == rhs.MessageContext;
        }

        bool leanNotEquals(const PSSettingsHostwise& rhs)
        {
            return !leanEquals(rhs);
        }
    };

    typedef boost::shared_ptr<PSSettingsHostwise> PSSettingsHostwisePtr;
    typedef std::map<std::string, PSSettingsHostwisePtr> PSSettingsHostwisePtrsMap;

    struct PSProtectedMachineTelemetrySettings
    {
        /// \brief host ID
        std::string HostId;

        /// \brief folder under which the logs and telemetry of
        /// the protected machine would be put in to be
        /// uploaded to Kusto.
        std::string TelemetryFolderPath;

        void serialize(JSON::Adapter& adapter)
        {
            throw std::logic_error("Not implemented");
        }

        void serialize(ptree& node)
        {
            JSON_P(node, HostId);
            JSON_P(node, TelemetryFolderPath);
        }

        bool operator==(const PSProtectedMachineTelemetrySettings& rhs)
        {
            const PSProtectedMachineTelemetrySettings& lhs = *this;

            return
                lhs.HostId == rhs.HostId &&
                lhs.TelemetryFolderPath == rhs.TelemetryFolderPath;
        }

        bool operator!=(const PSProtectedMachineTelemetrySettings& rhs)
        {
            return !operator==(rhs);
        }
    };

    typedef boost::shared_ptr<PSProtectedMachineTelemetrySettings> PSProtMacTelSettingsPtr;
    typedef std::map<std::string, PSProtMacTelSettingsPtr> PSProtMacTelSettingsPtrsMap;

    typedef std::map<std::string, std::string> StringMap;
    typedef boost::shared_ptr<StringMap> StringMapPtr;

    struct PSSettings
    {
        /// \brief percentage of used cache volume size beyond which the incoming
        /// data must be throttled for the PS
        double CumulativeThrottleLimit;

        /// \brief does the process server type support source folder concept?
        bool IsSourceFolderSupported;

        /// \brief app insights key to upload logs and telemetry for the PS
        /// Alternate path to main path - TelemetryChannelUri
        std::string ApplicationInsightsInstrumentationKey;

        /// \brief uri to report critical health and events for the PS
        std::string CriticalChannelUri;

        /// \brief renewal time of CriticalChannelUri
        boost::posix_time::ptime CriticalChannelUriRenewalTimeUtc;

        /// \brief uri to report informational health and events for the PS
        std::string InformationalChannelUri;

        /// \brief renewal time of InformationalChannelUri
        boost::posix_time::ptime InformationalChannelUriRenewalTimeUtc;

        /// \brief uri to upload logs and telemetry of all the connected components
        /// Source(s), PS, MT', MARS
        std::string TelemetryChannelUri;

        /// \brief renewal time of <c>TelemetryChannelUri</c>
        boost::posix_time::ptime TelemetryChannelUriRenewalTimeUtc;

        /// \brief message context to use for the communication about the PS
        std::string MessageContext;

        /// \brief server provided values to override hardcoded settings in
        /// the PS components. Note that, these values don't supersede
        /// a value explicitly set in the corresponding conf file
        /// referred by the component(s).
        StringMapPtr Tunables;

        boost::shared_ptr<PSSettingsHostwisePtrsMap> HostwiseSettings;
        boost::shared_ptr<PSProtMacTelSettingsPtrsMap> TelemetrySettings;

        void serialize(JSON::Adapter& adapter)
        {
            throw std::logic_error("Not implemented");
        }

        void serialize(ptree& node)
        {
            std::string timeUtcStr;

            JSON_P(node, CumulativeThrottleLimit);
            JSON_P(node, IsSourceFolderSupported);
            JSON_P(node, ApplicationInsightsInstrumentationKey);
            JSON_P(node, CriticalChannelUri);
            CriticalChannelUriRenewalTimeUtc = ConvertDotNetDateTime(node, "CriticalChannelUriRenewalTimeUtc");
            JSON_P(node, InformationalChannelUri);
            InformationalChannelUriRenewalTimeUtc = ConvertDotNetDateTime(node, "InformationalChannelUriRenewalTimeUtc");
            JSON_P(node, TelemetryChannelUri);
            TelemetryChannelUriRenewalTimeUtc = ConvertDotNetDateTime(node, "TelemetryChannelUriRenewalTimeUtc");
            JSON_P(node, MessageContext);

            StringMap tunables;
            JSON_KV_P_KEYNAME(node, "Tunables", tunables);
            Tunables = tunables.empty()? StringMapPtr() : boost::make_shared<StringMap>(tunables); // std::move() candidate

            std::vector<PSSettingsHostwise> hostSettings;
            JSON_VCL_KEYNAME(node, "Hosts", hostSettings);
            HostwiseSettings = boost::make_shared<PSSettingsHostwisePtrsMap>();
            for (std::vector<PSSettingsHostwise>::iterator itr = hostSettings.begin(); itr != hostSettings.end(); itr++) 
            {
                (*HostwiseSettings)[itr->HostId] = boost::make_shared<PSSettingsHostwise>(*itr); // std::move() candidate
                if (!(itr->TargetHostId.empty()))
                {
                    (*HostwiseSettings)[itr->TargetHostId] = boost::make_shared<PSSettingsHostwise>(*itr);
                }
            }

            std::vector<PSProtectedMachineTelemetrySettings> telSettings;
            JSON_VCL_KEYNAME(node, "ProtectedMachineTelemetrySettings", telSettings);
            TelemetrySettings = boost::make_shared<PSProtMacTelSettingsPtrsMap>();
            for (std::vector<PSProtectedMachineTelemetrySettings>::iterator itr = telSettings.begin(); itr != telSettings.end(); itr++) 
                (*TelemetrySettings)[itr->HostId] = boost::make_shared<PSProtectedMachineTelemetrySettings>(*itr); // std::move() candidate
        }

        bool leanEquals(const PSSettings& rhs)
        {
            const PSSettings& lhs = *this;

            return
                std::abs(lhs.CumulativeThrottleLimit - rhs.CumulativeThrottleLimit) < std::numeric_limits<double>::epsilon() &&
                lhs.IsSourceFolderSupported &&
                lhs.ApplicationInsightsInstrumentationKey == rhs.ApplicationInsightsInstrumentationKey &&
                lhs.CriticalChannelUri == rhs.CriticalChannelUri &&
                lhs.CriticalChannelUriRenewalTimeUtc == rhs.CriticalChannelUriRenewalTimeUtc &&
                lhs.InformationalChannelUri == rhs.InformationalChannelUri &&
                lhs.InformationalChannelUriRenewalTimeUtc == rhs.InformationalChannelUriRenewalTimeUtc &&
                lhs.TelemetryChannelUri == rhs.TelemetryChannelUri &&
                lhs.TelemetryChannelUriRenewalTimeUtc == rhs.TelemetryChannelUriRenewalTimeUtc &&
                lhs.MessageContext == rhs.MessageContext;
        }

        bool leanNotEquals(const PSSettings& rhs)
        {
            return !leanEquals(rhs);
        }
    };

    typedef boost::shared_ptr<PSSettings> PSSettingsPtr;

    struct CacheDataHeader
    {
        std::string Version;
        std::string Checksum;
        std::string ChecksumType;

        static const char* CHECKSUM_TYPE_MD5;
        static const char* CURRENT_CACHED_DATA_VERSION;
        static const int CURRENT_CACHED_DATA_MAJOR_VERSION;
        static const int CURRENT_CACHED_DATA_MINOR_VERSION;

        void serialize(JSON::Adapter& adapter)
        {
            throw std::logic_error("Not implemented");
        }

        void serialize(ptree& node)
        {
            JSON_P(node, Version);
            JSON_P(node, Checksum);
            JSON_P(node, ChecksumType);
        }

        static CacheDataHeader BuildCacheDataHeader(const std::string& content)
        {
            CacheDataHeader toRet;

            toRet.Version = CURRENT_CACHED_DATA_VERSION;
            toRet.ChecksumType = CHECKSUM_TYPE_MD5;

            const int MD5_HASH_LENGTH = 16;
            std::vector<unsigned char> currhash(MD5_HASH_LENGTH, 0);
            INM_MD5_CTX ctx;
            INM_MD5Init(&ctx);
            INM_MD5Update(&ctx, (unsigned char*)content.c_str(), content.size());
            INM_MD5Final(&currhash[0], &ctx);

            toRet.Checksum = securitylib::base64Encode((const char*)&currhash[0], currhash.size());

            return toRet;
        }

        bool operator==(const CacheDataHeader& rhs)
        {
            CacheDataHeader lhs = *this;

            return
                lhs.Version == rhs.Version &&
                lhs.Checksum == rhs.Checksum &&
                lhs.ChecksumType == rhs.ChecksumType;
        }

        bool operator!=(const CacheDataHeader& rhs)
        {
            return !operator==(rhs);
        }

        bool IsMatchingContent(const std::string& content)
        {
            CacheDataHeader comp = BuildCacheDataHeader(content);

            // TODO-SanKumar-2002: Implement the minor and major version
            // restrictions/relaxations as per settings here too.

            return
                //this->Version == comp.Version &&
                this->Checksum == comp.Checksum &&
                this->ChecksumType == comp.ChecksumType;
        }
    };

    class PSSettingsConfigurator
    {
    public:
        void Initialize(
            const boost::filesystem::path& settingsFilePath,
            const boost::filesystem::path& settingsLckFilePath,
            serverOptionsPtr serverOptions);
        void Start();
        void Stop();

        PSSettingsPtr GetPSSettings() { return m_psSettings; }

        // Singleton pattern
        static PSSettingsConfigurator& GetInstance()
        {
            return s_instance;
        }

    private:
        // Singleton instance
        static PSSettingsConfigurator s_instance;

        PSSettingsPtr m_psSettings;
        std::string   m_settingsFileContent;

        time_t m_MTime;

        boost::atomic<bool> m_isInitialized;
        boost::unique_ptr<boost::thread> m_thread;

        boost::filesystem::path m_settingsFilePath;
        boost::filesystem::path m_settingsLckFilePath;
        serverOptionsPtr m_serverOptions;

        void Run();

        PSSettingsPtr ReadSettingsFromFile(
            bool& fileUnavailable,
            std::string& knownToFailCacheDataHeader,
            std::string& knownToFailContent);

        void ProcessNewSettings(PSSettingsPtr newSettings, bool fileUnavailable);

    public:
        typedef boost::function1<void, StringMapPtr> TunablesChangeNotificationCallback;
        typedef long long SubscriptionNum_t;

        SubscriptionNum_t SubscribeForTunablesChange(TunablesChangeNotificationCallback callback);
        void UnsubscribeForTunablesChange(SubscriptionNum_t subscriptionNumber);

    private:
        typedef std::map<SubscriptionNum_t, TunablesChangeNotificationCallback> TunablesCallbackMap;
        TunablesCallbackMap m_tunablesChangeCallbackMap;
        boost::shared_mutex m_tunablesCallbackMapMutex;

        void NotifyTunablesChange(StringMapPtr tunables);
    };

    //enum PSSettingsChangeType
    //{
    //    PSSettingsChangeType_Added,
    //    PSSettingsChangeType_Modified,
    //    PSSettingsChangeType_Removed
    //};
}

#endif // !PS_SETTINGS_CONFIGURATOR_H
