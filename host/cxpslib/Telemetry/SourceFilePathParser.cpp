#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include "TelemetryCommon.h"
#include "SourceFilePathParser.h"

using namespace CxpsTelemetry;

void SourceFilePathParser::ParseResyncFileName(const std::string &lowerFileName, ResyncFileMetadata &metadata)
{
    metadata.Clear();

    if (lowerFileName.empty())
    {
        BOOST_ASSERT(false);
        metadata.FileType = ResyncFileType_Invalid;
        return;
    }

    // TODO-SanKumar-1710: Make these static
    const std::string CompletedHcd("completed_hcd_");
    const std::string CompletedSync("completed_sync_");
    const std::string MapExtension(".map");
    const std::string Cluster("_cluster_");
    const std::string NoMoreFileSuffix("_missing.dat");
    const std::string MissingFileSuffix("_missing.dat");

    try
    {
        // Avoiding a regular expression match, as the format of the resync files are simpler.

        std::string::size_type ind;

        // Reverse sorting the checks in the order of frequency and the volume.
        // Once matched, move the index to the start of the jobId substring.
        if ((ind = lowerFileName.find(CompletedHcd)) != std::string::npos)
        {
            // completed_hcd_*
            // completed_hcd_<JobId>_noMore.dat
            // (pre_)completed_hcd_<JobId>_missing.dat
            // (pre_)completed_hcd_<JobId>_<HexTime>.dat

            metadata.FileType = ResyncFileType_Hcd;
            ind += CompletedHcd.length();
        }
        else if ((ind = lowerFileName.find(CompletedSync)) != std::string::npos)
        {
            // (tmp_)completed_sync_*
            // completed_sync_<JobId>_nomore.dat
            // (pre_)completed_sync_<JobId>_missing.dat
            // (pre_)completed_sync_<JobId>_<HexTime>.dat(.gz)
            // tmp_completed_sync_<JobId>_<HexTime>.dat (tmp prefix is used, when server side compression is enabled - only as new name in rename)

            metadata.FileType = ResyncFileType_Sync;
            ind += CompletedSync.length();
        }
        else if (boost::ends_with(lowerFileName, MapExtension))
        {
            // (pre_)HcdTransfer_<JobId>.map
            // (pre_)SyncsApplied_<JobId>.map
            // (pre_)HcdsMissing_<JobId>.map
            // (pre_)SyncsMissing_<JobId>.map
            // (pre_)HcdProcess_<JobId>.map
            // (pre_)ClusterTransfer_<JobId>.map

            metadata.FileType = ResyncFileType_Map;

            ind = lowerFileName.rfind('_', ind);
            if (ind == std::string::npos)
            {
                // TODO-SanKumar-1711: Should we rather assume this as a parsing error?

                // throw std::runtime_error(
                //    "Unable to find the last _ adjacent to job id in the map file name : " + lowerFileName);
                return;
            }
            ind++; // Avoid the underscore
        }
        else if ((ind = lowerFileName.find(Cluster)) != std::string::npos)
        {
            // completed_cluster_*
            // completed_cluster_<JobId>_noMore.dat
            // completed_cluster_nomore.dat
            // (pre_)completed_cluster_<JobId>_<HexTime>.dat
            // (pre_)completed_unprocessed_cluster_<JobId>.dat
            // (pre_)completed_processed_cluster_<JobId>.dat

            metadata.FileType = ResyncFileType_Cluster;
            ind += Cluster.size();
        }
        else
        {
            BOOST_ASSERT(false);
            metadata.FileType = ResyncFileType_Unknown;
            return;
        }

        // TODO-SanKumar-1711: DP at source marks a file as corrupt (prepending '_') on any error.
        // We could probably track that separately or as a counter for investigation.

        bool shouldParseTimestamp = true, shouldParseJobId = true;
        // TODO-SanKumar-1711: Add other wild card chars to this check as well.
        if (lowerFileName.rfind('*') != std::string::npos)
        {
            shouldParseJobId = false;
            shouldParseTimestamp = false;
        }
        else if (boost::ends_with(lowerFileName, MissingFileSuffix) ||
            boost::ends_with(lowerFileName, NoMoreFileSuffix) ||
            metadata.FileType == ResyncFileType_Map)
        {
            shouldParseTimestamp = false;
        }

        typedef boost::iterator_range<std::string::const_iterator> BoostStrItrRange;
        if (shouldParseJobId)
        {
            try
            {
                BoostStrItrRange subRange = boost::make_iterator_range(lowerFileName.begin() + ind, lowerFileName.end());
                BoostStrItrRange endExclJobIdRange = boost::find_token(subRange, boost::is_any_of("._"));

                metadata.JobId = std::string(subRange.begin(), endExclJobIdRange.begin());
            }
            // TODO-SanKumar-1711: Should we rather assume this as a parsing error?
            GENERIC_CATCH_LOG_ACTION("Getting Job id in ParseResyncFileName", metadata.JobId.clear(); shouldParseTimestamp = false)
        }

        if (shouldParseTimestamp)
        {
            try
            {
                ind += metadata.JobId.size() + 1; // Move ahead of JobId and an underscore.

                if (ind < lowerFileName.size())
                {
                    std::string::size_type dotInd = lowerFileName.find('.', ind);
                    // If the .dot couldn't be found, dump the substring till end, which helps us
                    // debug any issue.
                    metadata.TimeStamp = lowerFileName.substr(ind, dotInd == std::string::npos ? dotInd : dotInd - ind);
                }
            }
            // TODO-SanKumar-1711: Should we rather assume this as a parsing error?
            GENERIC_CATCH_LOG_ACTION("Getting timestamp in ParseResyncFileName", metadata.TimeStamp.clear())
        }
    }
    GENERIC_CATCH_LOG_ACTION("ParseResyncFileName", metadata.Clear(); metadata.FileType = ResyncFileType_InternalErr; BOOST_ASSERT(false))
}

void SourceFilePathParser::ParseDiffSyncFileName(const std::string &lowerFileName, DiffSyncFileMetadata &metadata)
{
    // Reference: ProtectedGroupVolumes::SetRemoteName()
    // InDskFlt always supports DiffOrder, so not expecting the older kind of
    // sequencing info ("S<ullTSFC>_<ullSeqFC>_E<ullTSLC>_ullSeqLC" ) in V2A.
    // Also, ignoring the timestamp appending performed at the source, as it's
    // only applicable for SSE (Intel).
    // in V2A RCM _ediffcompleted is added as putfile requests are to target directory

    // The passed file name is expected in lower case.
    const char *diffFileName = "^(pre_)?completed(_ediffcompleted)?_diff((_tso)|(_tag))?_p(\\d+)_(\\d+)_(\\d+)_e(\\d+)_(\\d+)_(w|m)(e|c)(\\d+)\\.dat(\\.gz)?$";
    //                           0               0                      00      0         0      0      0       0      1      1    1    1           1
    //                           1               2                      34      5         6      7      8       9      0      1    2    3           4

    metadata.Clear();

    if (lowerFileName.empty())
    {
        BOOST_ASSERT(false);
        metadata.FileType = DiffSyncFileType_Invalid;
        return;
    }

    try
    {
        // TODO-SanKumar-1708: Make the regex object static.
        boost::regex diffFileNameRegex(diffFileName);
        boost::smatch matches;

        if (!boost::regex_search(lowerFileName, matches, diffFileNameRegex))
        {
            BOOST_ASSERT(false);
            metadata.FileType = DiffSyncFileType_Unknown;
            return;
        }

        metadata.IsPre = matches[1].matched;
        metadata.IsCompressed = matches[14].matched;

        if (matches[4].matched)
        {
            metadata.FileType = DiffSyncFileType_Tso;
        }
        else if (matches[5].matched)
        {
            metadata.FileType = DiffSyncFileType_Tag;
        }
        else
        {
            // Diff file
            BOOST_ASSERT(!matches[2].matched && matches[11].matched);
            if (0 == matches[11].compare("w"))
            {
                metadata.FileType = DiffSyncFileType_WriteOrdered;
            }
            else // M
            {
                BOOST_ASSERT(0 == matches[11].compare("m"));
                metadata.FileType = DiffSyncFileType_NonWriteOrdered;
            }
        }

        metadata.PrevEndTS = boost::lexical_cast<uint64_t>(matches[6]);
        metadata.PrevEndSeq = boost::lexical_cast<uint64_t>(matches[7]);
        metadata.PrevSplitIoSeq = boost::lexical_cast<uint32_t>(matches[8]);
        metadata.CurrTS = boost::lexical_cast<uint64_t>(matches[9]);
        metadata.CurrSeq = boost::lexical_cast<uint64_t>(matches[10]);
        metadata.CurrSplitIoSeq = boost::lexical_cast<uint32_t>(matches[13]);
    }
    GENERIC_CATCH_LOG_ACTION("ParseDiffSyncFileName", metadata.Clear(); metadata.FileType = DiffSyncFileType_InternalErr; BOOST_ASSERT(false))
}

static std::string NormalizeDeviceId(const std::string &device)
{
    // In few cases (get and delete resync files), the linux devices are of
    // the format dev\sda, since the list file call returns the local win path
    // from PS.

    bool slashFoundMinOnce = false;
    bool slashFoundPrev = false;
    std::string toRetDevName;
    toRetDevName.reserve(device.size());

    for (std::string::size_type ind = 0; ind < device.length(); ind++)
    {
        if (device[ind] == '/' || device[ind] == '\\')
        {
            if (!slashFoundPrev)
            {
                // if \, it will be converted to /
                toRetDevName += '/';
            }
            // else, ignore this excess slash in the resulting device name.

            slashFoundPrev = true;
            slashFoundMinOnce = true;
        }
        else
        {
            toRetDevName += device[ind];
            slashFoundPrev = false;
        }
    }

    // Since we remove all the preceding slashes as part of the regex-based
    // parsing, prepend linux devices with a /. (/dev/sda)
    if (slashFoundMinOnce && toRetDevName[0] != '/')
        return "/" + toRetDevName;
    else
        return toRetDevName;
}

FileType SourceFilePathParser::GetCxpsFileType(
    const std::string &filePath, std::string &hostId, std::string &device, std::string &fileName)
{
    // TODO-SanKumar-1708: Make the regex static const obj

#define HOST_ID_LOWER   "([a-f]|[0-9]){8}-([a-f]|[0-9]){4}-([a-f]|[0-9]){4}-([a-f]|[0-9]){4}-([a-f]|[0-9]){12}"
//                       1                2                3                4                5
#define PS_ID_LOWER     "([a-f]|[0-9]){8}-([a-f]|[0-9]){4}-([a-f]|[0-9]){4}-([a-f]|[0-9]){4}([a-f]|[0-9]){12}"
//                                            PS ID is also a GUID like Host ID, but here a ^ hyphen (-) missing
#define DIFFS_FOLDER "diffs"
#define RESYNC_FOLDER "resync"
#define LOGS_FOLDER "logs"
#define TELEMETRY_FOLDER "telemetry"
#define CHUR_STAT_FOLDER "churstat"
#define THRP_STAT_FOLDER "thrpstat"
#define TRANSPORT_FOLDER "transport"
#define DATA_FOLDER "data"
#define TST_DATA_FOLDER "tstdata"

    // TODO-SanKumar-1711: Explain multiple and backslash
    // Regular expression needs escaping of backslash, similar to c++ source code. Thus 4 \'s but one /'s.
#define SLASH "(\\\\|/)+"
#define HOME_SVSYS SLASH "home" SLASH "svsystems"
//                   1            2
#define HOST_ID_PREFIXED_FOLDERS "(" HOST_ID_LOWER ")" SLASH "(((.+)" SLASH DIFFS_FOLDER ")|(" TELEMETRY_FOLDER ")|(" LOGS_FOLDER  "))"
//                                0                       0   001       1                   1                      1
//                                1        +5             7   890       1                   2                      3
#define RESYNC_FILES_PREFIX "(" PS_ID_LOWER ")" SLASH "c" SLASH "(" HOST_ID_LOWER ")" SLASH "(.+)" SLASH RESYNC_FOLDER
//                           0                    0         0    0                      1    1       1
//                           1       +5           7         8    9        +5            5    6       7

    hostId.clear();
    device.clear();
    fileName.clear();

    try
    {
        std::string lowerFilePath = filePath;
        boost::algorithm::to_lower(lowerFilePath);

        // TODO-SanKumar-1710: Make this static.
        boost::regex psFilesRegex(HOME_SVSYS SLASH "((" HOST_ID_PREFIXED_FOLDERS ")|(" RESYNC_FILES_PREFIX ")|(" CHUR_STAT_FOLDER ")|(" THRP_STAT_FOLDER "))" SLASH "(.+)$");
        //                                      0   00                              1                         3                      3                          3    4
        //                              +2      3   45           +13                9          +17            7                      8                          9    0

        boost::smatch matches;
        if (!boost::regex_search(lowerFilePath, matches, psFilesRegex))
        {
            // TODO-SanKumar-1711: This kind of files were added during the development of cxps tel.
            // Since the above regex was already well tested at that point, add this parsing separately.
            // Merge with it in a later release.
            // TODO-SanKumar-1710: Make this static.
            boost::regex tstDataFilesRegex("(^" TST_DATA_FOLDER ")|(" HOME_SVSYS SLASH TRANSPORT_FOLDER SLASH DATA_FOLDER SLASH TST_DATA_FOLDER ")" SLASH "(.+)$");
            //                              1                      2      +2       5                      6                 7                         8    9

            boost::smatch tstDataFileMatches;
            if (!boost::regex_search(lowerFilePath, tstDataFileMatches, tstDataFilesRegex))
            {
                BOOST_ASSERT(false);
                return FileType_Unknown;
            }

            fileName = tstDataFileMatches[9];
            return FileType_TstData;
        }

        // Files under the host folder
        if (matches[5].matched)
        {
            // Diff files of the host
            if (matches[5 + 9].matched)
            {
                hostId = matches[5 + 1];
                device = NormalizeDeviceId(matches[5 + 10]);
                fileName = matches[40];

                return FileType_DiffSync;
            }
            // Telemetry files of the host
            else if (matches[5 + 12].matched)
            {
                return FileType_Telemetry;
            }
            // Log files of the host
            else if (matches[5 + 13].matched)
            {
                return FileType_Log;
            }
            else
            {
                BOOST_ASSERT(false);
                return FileType_InternalErr;
            }
        }
        // Files under the resync folder
        else if (matches[19].matched)
        {
            // TODO-SanKumar-1711: Should we compare the PS folder name with its ID?

            hostId = matches[19 + 9];
            device = NormalizeDeviceId(matches[19 + 16]);
            fileName = matches[40];

            return FileType_Resync;
        }
        // Files under the churstat folder
        else if (matches[37].matched)
        {
            return FileType_ChurStat;
        }
        // File under the thrpstat folder
        else if (matches[38].matched)
        {
            return FileType_ThrpStat;
        }
        else
        {
            BOOST_ASSERT(FALSE);
            return FileType_InternalErr;
        }

        // TODO-SanKumar-1711: Host id, device id(only for stats) and file name
        // could be returned for the stats, logs and telemetry, if necessary.
    }
    GENERIC_CATCH_LOG_ACTION("GetCxpsFileType", hostId.clear(); device.clear(); fileName.clear(); return FileType_InternalErr);
}

FileType SourceFilePathParser::GetCxpsFileTypeInRcmMode(
    const std::string &filePath, std::string &hostId, std::string &device, std::string &fileName)
{
#define GUID_REGEX     "[{]?[0-9a-fA-F]{8}-([0-9a-fA-F]{4}-){3}[0-9a-fA-F]{12}[}]?"
//                                         1

#define DIFFSYNC_FOLDER     "diffsync"
#define IR_FOLDER           "initialreplication"

#define CACHE_FOLDERS  "(" GUID_REGEX ")" SLASH "(.+)" SLASH "(" GUID_REGEX ")" SLASH  "((" DIFFSYNC_FOLDER ")|(" IR_FOLDER ")|(" RESYNC_FOLDER "))" SLASH "(" GUID_REGEX ")" SLASH "(.+)$"
//                      0                 0      0      0     0                 0       01                     1               1                     1      1                 1      1
//                      1                 3      4      5     6                 8       90                     1               2                     3      4                 6      7

#define TELEMETRY_FOLDERS "(" GUID_REGEX ")" SLASH "((" LOGS_FOLDER ")|(" TELEMETRY_FOLDER ")|(" CHUR_STAT_FOLDER ")|(" THRP_STAT_FOLDER "))" SLASH "(.+)$"
//                         0                 0      00                 0                      0                       0                        0     1
//                         1                 3      45                 6                      7                       8                        9     0

    hostId.clear();
    device.clear();
    fileName.clear();

    FileType fileType = FileType_Invalid;

    try
    {
        std::string lowerFilePath = filePath;
        boost::algorithm::to_lower(lowerFilePath);

        boost::regex dataFilesRegex(CACHE_FOLDERS);
        boost::smatch matches;
        if (boost::regex_search(lowerFilePath, matches, dataFilesRegex))
        {
            if (matches[1].matched)
                hostId = matches[1];
            if (matches[4].matched)
                device = NormalizeDeviceId(matches[4]);
            if (matches[17].matched)
                fileName = matches[17];

            if (matches[10].matched)
            {
                fileType = FileType_DiffSync;
            }
            else if (matches[11].matched)
            {
                fileType = FileType_Resync;
            }
            else if (matches[12].matched)
            {
                    fileType = FileType_Resync;
            }

            return fileType;
        }

        boost::regex teleFilesRegex(TELEMETRY_FOLDERS);
        boost::smatch tmatches;
        if (boost::regex_search(lowerFilePath, tmatches, teleFilesRegex))
        {
            if (tmatches[1].matched)
                hostId = tmatches[1];

            if (tmatches[5].matched)
            {
                fileType = FileType_Log;
            }
            else if (tmatches[6].matched)
            {
                fileType = FileType_Telemetry;
            }
            else if (tmatches[7].matched)
            {
                fileType = FileType_ChurStat;
            }
            else if (tmatches[8].matched)
            {
                fileType = FileType_ThrpStat;
            }

            if (tmatches[10].matched)
                fileName = tmatches[10];
            return fileType;
        }
    }
    GENERIC_CATCH_LOG_ACTION("GetCxpsFileTypeInRcmMode", hostId.clear(); device.clear(); fileName.clear(); return FileType_InternalErr);
}
