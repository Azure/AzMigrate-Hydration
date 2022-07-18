///
///  \file protecteddevicedetails.cpp
///
///  \brief manages the device name mapping to the SCSI-ID and device name for Azure to Azure
///  replication scenarios
///     class VxProtectedDeviceDetail: mapping of Disk ID to SCSI-ID and Disk ID to Device Name 
///     class VxProtectedDeviceDetailCache: manages the in-memory cache of the device details
///     class VxProtectedDeviceDetailPeristentStore: Adds, removes and deletes th device details to persistant file
///
///

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <ace/Guard_T.h>
#include <ace/Mutex.h>
#include <ace/File_Lock.h>
#include <boost/lexical_cast.hpp>
#include "protecteddevicedetails.h"
#include "localconfigurator.h"
#include "portablehelpers.h"
#include "portablehelpersmajor.h"
#include "inmdefines.h"
#include "logger.h"
#include "inmsafeint.h"
#include "inmageex.h"
#include "securityutils.h"
#include "scopeguard.h"
#include "errorexception.h"
#include <boost/algorithm/string.hpp>

#include "setpermissions.h"

const char VxProtectedDeviceDetailPeristentStore::CHECKSUM_SEPARATOR = '\n';

VxProtectedDeviceDetailPeristentStore::VxProtectedDeviceDetailPeristentStore()
{
}


bool VxProtectedDeviceDetailPeristentStore::Init(void)
{
    std::string cachePath;
    bool brval = LocalConfigurator::getVxProtectedDeviceDetailCachePathname(cachePath);
    if (brval)
    {
        SetCachePath(cachePath);
    }

    return brval;
}


void VxProtectedDeviceDetailPeristentStore::SetCachePath(const std::string &cachePath)
{
    m_cachePath = cachePath;
#if defined (SV_WINDOWS) && defined (SV_USES_LONGPATHS)
    m_cacheLockPath = "\\\\?\\" + m_cachePath + INMLOCKEXT;
#else
    m_cacheLockPath = m_cachePath + INMLOCKEXT;
#endif
}

/// RefreshCache should be called only with the m_detailCacheLock held.
VxProtectedDeviceDetailPeristentStore::MAP_CACHE_STATE VxProtectedDeviceDetailPeristentStore::RefreshCache()
{
    VxProtectedDeviceDetailPeristentStore::MAP_CACHE_STATE cs = E_MAP_CACHE_DOES_NOT_EXIST;

    time_t t = GetMapCacheMtime();
    if (0 == t)
    {
        m_errMsg = "Failed to find device name map file modified time.";
        return cs;
    }

    if (t != m_detailCache.m_mTime)
    {
        cs = ReadFromPersistence(m_detailCache);
    }

    return cs;
}

bool VxProtectedDeviceDetailPeristentStore::AddMap(const VxProtectedDeviceDetail &devNameMap)
{
    boost::unique_lock<boost::mutex> lock(m_detailCacheLock);

    VxProtectedDeviceDetailPeristentStore::MAP_CACHE_STATE cs = E_MAP_CACHE_DOES_NOT_EXIST;

    cs = RefreshCache();

    if (cs != E_MAP_CACHE_VALID && cs != E_MAP_CACHE_DOES_NOT_EXIST)
        return false;

    m_detailCache.m_details.push_back(devNameMap);

    if (Persist(m_detailCache.m_details))
    {
        cs = ReadFromPersistence(m_detailCache);
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, 
            "%s adding to device name map failed.\n",
            FUNCTION_NAME);
        cs = E_MAP_CACHE_WRITE_FAILURE;
    }

    return (cs == E_MAP_CACHE_VALID);

}

bool VxProtectedDeviceDetailPeristentStore::GetMap(const std::string &id, VxProtectedDeviceDetail &devNameMap)
{
    boost::unique_lock<boost::mutex> lock(m_detailCacheLock);
    VxProtectedDeviceDetailPeristentStore::MAP_CACHE_STATE cs = E_MAP_CACHE_DOES_NOT_EXIST;
    cs = RefreshCache();

    if (cs != E_MAP_CACHE_VALID)
        return false;

    VxProtectedDeviceDetailIter_t iter = m_detailCache.m_details.begin();

    while (iter != m_detailCache.m_details.end())
    {
        if (boost::iequals(iter->m_id,id))
        {
            devNameMap = *iter;
            return true;
        }
        iter++;
    }

    return false;
}

bool VxProtectedDeviceDetailPeristentStore::RemoveMap(const VxProtectedDeviceDetail &devNameMap)
{
    boost::unique_lock<boost::mutex> lock(m_detailCacheLock);
    VxProtectedDeviceDetailPeristentStore::MAP_CACHE_STATE cs = E_MAP_CACHE_DOES_NOT_EXIST;
    cs = RefreshCache();

    if (cs != E_MAP_CACHE_VALID)
        return false;

    VxProtectedDeviceDetailIter_t iter = m_detailCache.m_details.begin();
    while (iter != m_detailCache.m_details.end())
    {
        if (iter->m_id == devNameMap.m_id)
            iter = m_detailCache.m_details.erase(iter);
        else
            iter++;
    }

    if (Persist(m_detailCache.m_details))
    {
        cs = ReadFromPersistence(m_detailCache);
    }
    else
        cs = E_MAP_CACHE_WRITE_FAILURE;

    return (cs == E_MAP_CACHE_VALID);
}

bool VxProtectedDeviceDetailPeristentStore::GetCurrentDetails(VxProtectedDeviceDetails_t &details)
{
    boost::unique_lock<boost::mutex> lock(m_detailCacheLock);
    VxProtectedDeviceDetailPeristentStore::MAP_CACHE_STATE cs = E_MAP_CACHE_DOES_NOT_EXIST;
    cs = RefreshCache();

    if (cs == E_MAP_CACHE_DOES_NOT_EXIST)
        return true;

    if (cs != E_MAP_CACHE_VALID)
        return false;

    details = m_detailCache.m_details;
    return true;
}

bool VxProtectedDeviceDetailPeristentStore::Persist(const VxProtectedDeviceDetails_t &details)
{
    // Check if earlier ID has changed
    bool haveput = CacheMapsAndChecksum(details);

    // Do not leave stale cache file
    if (!haveput) {
        std::string delErrMsg;
        if (!DeleteCacheFile(delErrMsg))
            m_errMsg += delErrMsg;
    }

    std::string errormsg;
    if (!securitylib::setPermissions(m_cachePath, errormsg)) {
        DebugPrintf(SV_LOG_ERROR, "%s\n", errormsg.c_str());
    }

    return haveput;
}


/*
* Cache file format:
* _______________
* |<checksum><\n>|
* |<data>        |
* |______________|
*/
bool VxProtectedDeviceDetailPeristentStore::CacheMapsAndChecksum(const VxProtectedDeviceDetails_t &details)
{
    bool havecached = false;
    VxProtectedDeviceDetailCache deviceDetailCache;
    deviceDetailCache.m_details = details;

    std::string serializedDetailss;
    try {
        serializedDetailss = JSON::producer<VxProtectedDeviceDetailCache>::convert(deviceDetailCache);
    }
    catch (JSON::json_exception& jsone)
    {
        m_errMsg = "serializing map caught json exception. ";
        m_errMsg += jsone.what();
        return havecached;
    }
    catch (...) {
        m_errMsg = "serializing map caught unknown exception.";
        return havecached;
    }

    std::string checksum = securitylib::genSha256Mac(serializedDetailss.c_str(), serializedDetailss.length());

    DebugPrintf(SV_LOG_DEBUG, "Updating cache file %s with checksum: %s\n", m_cachePath.c_str(), checksum.c_str());

    //Acquire write lock
    DebugPrintf(SV_LOG_DEBUG, "Acquiring write lock %s\n", m_cacheLockPath.c_str());

    ACE_File_Lock lock(ACE_TEXT_CHAR_TO_TCHAR(m_cacheLockPath.c_str()), O_CREAT | O_RDWR, SV_LOCK_PERMISSIONS, 0);
    if (-1 == lock.acquire_write()) {
        int saveErrNo = ACE_OS::last_error();
        m_errMsg = std::string("Failed to take dev details cache write lock ") + m_cacheLockPath + " with error " + boost::lexical_cast<std::string>(saveErrNo) + ".";
        return false;
    }

    DebugPrintf(SV_LOG_DEBUG, "Acquired.\n");

    //Write to cache file
    FILE *fp;
    std::string mode, errMsg;
    mode = "w";
    mode += FOPEN_MODE_NOINHERIT;
    if (InmFopen(&fp, m_cachePath, mode, errMsg)) {
        DebugPrintf(SV_LOG_DEBUG, "opened cache file.\n");
        havecached = InmFprintf(fp, m_cachePath, errMsg, false, 0, "%s%c%s", checksum.c_str(), CHECKSUM_SEPARATOR, serializedDetailss.c_str());
        if (havecached)
            DebugPrintf(SV_LOG_DEBUG, "updated cache file.\n");
        else
            m_errMsg = std::string("Failed to update dev details cache file ") + m_cachePath + " with error " + errMsg + ".";
        //For output file io functions, fclose return value has to be checked to know all output was written correctly
        if (InmFclose(&fp, m_cachePath, errMsg))
            DebugPrintf(SV_LOG_DEBUG, "closed cache file.\n");
        else {
            havecached = false;
            m_errMsg += std::string("Failed to close handle of dev details cache file ") + m_cachePath + " with error " + errMsg + ".";
        }
    } else
        m_errMsg = std::string("Failed to open dev details cache file ") + m_cachePath + " with error " + errMsg + ".";

    return havecached;
}

VxProtectedDeviceDetailPeristentStore::MAP_CACHE_STATE VxProtectedDeviceDetailPeristentStore::ReadFromPersistence(VxProtectedDeviceDetailCache &dc)
{
    MAP_CACHE_STATE cs = VerifyAndGetCache(dc);

    // Clear cache if it is not valid
    if ((E_MAP_CACHE_CORRUPT == cs) || (E_MAP_CACHE_READ_FAILURE == cs)) {
        std::string delErrMsg;
        if (!DeleteCacheFile(delErrMsg))
            m_errMsg += delErrMsg;
    }

    return cs;
}

VxProtectedDeviceDetailPeristentStore::MAP_CACHE_STATE VxProtectedDeviceDetailPeristentStore::VerifyAndGetCache(VxProtectedDeviceDetailCache &dc)
{
    //Acquire read lock
    DebugPrintf(SV_LOG_DEBUG, "Getting dev details from cache %s. Acquiring read lock %s\n", m_cachePath.c_str(), m_cacheLockPath.c_str());
    ACE_File_Lock lock(ACE_TEXT_CHAR_TO_TCHAR(m_cacheLockPath.c_str()), O_CREAT | O_RDWR, SV_LOCK_PERMISSIONS, 0);
    if (-1 == lock.acquire_read()) {
        int saveErrNo = ACE_OS::last_error();
        m_errMsg = std::string("Failed to take dev details cache read lock ") + m_cacheLockPath + " with error " + boost::lexical_cast<std::string>(saveErrNo) + ".";
        return E_MAP_CACHE_LOCK_FAILURE;
    }
    DebugPrintf(SV_LOG_DEBUG, "Acquired.\n");

    //Open cache
    MAP_CACHE_STATE cs;
    std::string mode("r");
    mode += FOPEN_MODE_NOINHERIT;
    FILE *fp = fopen(m_cachePath.c_str(), mode.c_str());
    if (fp) {
        DebugPrintf(SV_LOG_DEBUG, "opened cache file.\n");
        ON_BLOCK_EXIT(&fclose, fp);
        dc.m_mTime = GetMapCacheMtime();
        cs = VerifyFileAndGetCache(fp, dc);
    } else {
        int saveErrNo = errno;
        cs = (ENOENT == saveErrNo) ? E_MAP_CACHE_DOES_NOT_EXIST : E_MAP_CACHE_READ_FAILURE;
        m_errMsg = std::string("Failed to open cache file ") + m_cachePath + " with error " + boost::lexical_cast<std::string>(saveErrNo)+".";
    }

    return cs;
}


VxProtectedDeviceDetailPeristentStore::MAP_CACHE_STATE VxProtectedDeviceDetailPeristentStore::VerifyFileAndGetCache(FILE *fp, VxProtectedDeviceDetailCache &dc)
{
    MAP_CACHE_STATE cs = E_MAP_CACHE_CORRUPT;
    
    //Get checksum
    int i;
    char c;
    std::string checksum;
    while ((i = getc(fp)) != EOF) {
        c = static_cast<char>(i);
        if (CHECKSUM_SEPARATOR == c)
            break;
        checksum += c;
    }
    DebugPrintf(SV_LOG_DEBUG, "Checksum found is %s\n", checksum.c_str());

    //Get data
    std::string data;
    while ((i = getc(fp)) != EOF) {
        c = static_cast<char>(i);
        data += c;
    }

    bool shouldproceed = true;
    //Verify checksum
    if (checksum.empty()) {
        m_errMsg = std::string("Failed to find checksum in dev details cache file ") + m_cachePath + ".";
        shouldproceed = false;
    }
    //Verify data
    if (data.empty()) {
        m_errMsg += std::string("Failed to find data in dev details cache file ") + m_cachePath + ".";
        shouldproceed = false;
    }

    //non empty checksum and data: calculate and compare checksum
    if (shouldproceed)
        cs = FillCacheIfDataValid(checksum, data, dc) ? E_MAP_CACHE_VALID : E_MAP_CACHE_CORRUPT;

    return cs;
}


bool VxProtectedDeviceDetailPeristentStore::FillCacheIfDataValid(const std::string &checksum, const std::string &data, VxProtectedDeviceDetailCache &dc)
{
    bool isvalid = false;

    //Get new checksum
    std::string newChecksum = securitylib::genSha256Mac(data.c_str(), data.length());
    DebugPrintf(SV_LOG_DEBUG, "Newly generated checksum is %s\n", newChecksum.c_str());

    //Compare and parse data to get volumes
    if (newChecksum == checksum) {
        try {
            DebugPrintf(SV_LOG_DEBUG, "Checksums match.\n");
            dc = JSON::consumer<VxProtectedDeviceDetailCache>::convert(data);
            isvalid = true;
            dc.m_checksum = checksum;
            DebugPrintf(SV_LOG_DEBUG, "Successfully parsed dev details.\n");
        } catch (std::string const& e) {
            m_errMsg = std::string("Failed to parse data to get dev details with error ") + e;
        } catch (char const* e) {
            m_errMsg = std::string("Failed to parse data to get dev details with error ") + e;
        } catch (ContextualException const& ce) {
            m_errMsg = std::string("Failed to parse data to get dev details with error ") + ce.what();
        } catch (std::exception const& e) {
            m_errMsg = std::string("Failed to parse data to get dev details with error ") + e.what();
        } catch (...) {
            m_errMsg = "Failed to parse data to get dev details.";
        }
    } else
        m_errMsg = "Checksum in file: " + checksum + ", does not match calculated checksum: " + newChecksum;

    if (!isvalid)
        m_errMsg += std::string(". dev details cache file ") + m_cachePath + " is corrupt.";

    return isvalid;
}

bool VxProtectedDeviceDetailPeristentStore::ClearMap(void)
{
    std::string delErrMsg;
    bool havecleared = DeleteCacheFile(delErrMsg);
    if (!havecleared)
        m_errMsg = delErrMsg;

    return havecleared;
}

//This is also an implicit corrective action if write/read to cache fail. Hence m_errMsg should not be overwritten.
bool VxProtectedDeviceDetailPeristentStore::DeleteCacheFile(std::string &errMsg)
{
    bool havedeleted = true;

    DebugPrintf(SV_LOG_DEBUG, "Clearing cache file %s.\n", m_cachePath.c_str());
    //Dual check to avoid unnecessary lock in case lot of readers are trying to delete corrupt cache
    if (MapExists()) {
        //Acquire write lock
        DebugPrintf(SV_LOG_DEBUG, "Acquiring write lock %s\n", m_cacheLockPath.c_str());
        ACE_File_Lock lock(ACE_TEXT_CHAR_TO_TCHAR(m_cacheLockPath.c_str()), O_CREAT | O_RDWR, SV_LOCK_PERMISSIONS, 0);
        if (-1 == lock.acquire_write()) {
            int saveErrNo = ACE_OS::last_error();
            errMsg = std::string("Failed to take dev details cache write lock ") + m_cacheLockPath + " with error " + boost::lexical_cast<std::string>(saveErrNo)+".";
            return false;
        }
        DebugPrintf(SV_LOG_DEBUG, "Acquired.\n");
        //check again and clear
        if (MapExists()) {
            havedeleted = (0 == remove(m_cachePath.c_str()));
            if (havedeleted)
                DebugPrintf(SV_LOG_DEBUG, "Cleared dev details cache file %s.\n", m_cachePath.c_str());
            else {
                int saveErrNo = errno;
                errMsg = std::string("Failed to remove dev details cache file ") + m_cachePath + " with error " + boost::lexical_cast<std::string>(saveErrNo) + ".";
            }
        }
    }

    return havedeleted;
}


bool VxProtectedDeviceDetailPeristentStore::MapExists(void)
{
    ACE_stat st;
    bool isexisting = false;
    if (0 == sv_stat(m_cachePath.c_str(), &st)) {
        isexisting = true;
        DebugPrintf(SV_LOG_DEBUG, "dev details cache file %s exists.\n", m_cachePath.c_str());
    } else 
        DebugPrintf(SV_LOG_DEBUG, "dev details cache file %s does not exist.\n", m_cachePath.c_str());
    
    return isexisting;
}


std::string VxProtectedDeviceDetailPeristentStore::GetErrorMessage(void)
{
    return m_errMsg;
}


time_t VxProtectedDeviceDetailPeristentStore::GetMapCacheMtime(void)
{
    ACE_stat st;
    time_t t = 0;
    if (0 == sv_stat(m_cachePath.c_str(), &st)) {
        t = st.st_mtime;
        std::stringstream ss;
        ss << "dev details cache file " << m_cachePath << " has mtime " << t;
        DebugPrintf(SV_LOG_DEBUG, "%s.\n", ss.str().c_str());
    } else
        DebugPrintf(SV_LOG_DEBUG, "dev details cache file %s does not exist. Hence cannot find mtime.\n", m_cachePath.c_str());

    return t;
}

bool VxProtectedDeviceDetailPeristentStore::BackupCurrentDetails(void)
{
    bool havecopied = true;

    DebugPrintf(SV_LOG_DEBUG, "backing up cache file %s.\n", m_cachePath.c_str());
    //Dual check to avoid unnecessary lock in case lot of readers are trying to delete corrupt cache
    if (MapExists()) {
        //Acquire write lock
        DebugPrintf(SV_LOG_DEBUG, "Acquiring write lock %s\n", m_cacheLockPath.c_str());
        ACE_File_Lock lock(ACE_TEXT_CHAR_TO_TCHAR(m_cacheLockPath.c_str()), O_CREAT | O_RDWR, SV_LOCK_PERMISSIONS, 0);
        if (-1 == lock.acquire_write()) {
            int saveErrNo = ACE_OS::last_error();
            m_errMsg = std::string("Failed to take dev details cache write lock ") + m_cacheLockPath + " with error " + boost::lexical_cast<std::string>(saveErrNo)+".";
            return false;
        }
        DebugPrintf(SV_LOG_DEBUG, "Acquired.\n");
        //check again and clear
        if (MapExists()) {

            try {
                boost::filesystem::path backupFilePath(m_cachePath);
                backupFilePath.replace_extension(boost::posix_time::to_iso_string(boost::posix_time::microsec_clock::local_time()) + ".dat");
                boost::filesystem::copy_file(m_cachePath, backupFilePath.string());
            }
            catch (const boost::filesystem::filesystem_error& exp)
            {
                havecopied = false;
                m_errMsg = std::string("Failed to take backup of dev details cache file ") + m_cachePath + " with error " + exp.what();
            }

            if (havecopied)
                DebugPrintf(SV_LOG_DEBUG, "backed up dev details cache file %s.\n", m_cachePath.c_str());
        }
    }

    return havecopied;
}

void VxProtectedDeviceDetailPeristentStore::Dump(void)
{
    boost::unique_lock<boost::mutex> lock(m_detailCacheLock);

    VxProtectedDeviceDetailIter_t iter = m_detailCache.m_details.begin();

    while (iter != m_detailCache.m_details.end())
    {
        DebugPrintf(SV_LOG_ALWAYS,
            "%s : %s %s %s\n",
            FUNCTION_NAME,
            iter->m_id.c_str(),
            iter->m_scsiId.c_str(),
            iter->m_deviceName.c_str());
        iter++;
    }

}
