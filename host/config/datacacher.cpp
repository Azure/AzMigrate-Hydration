#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <ace/Guard_T.h>
#include <ace/Mutex.h>
#include <ace/File_Lock.h>
#include <boost/lexical_cast.hpp>
#include "datacacher.h"
#include "localconfigurator.h"
#include "portablehelpers.h"
#include "portablehelpersmajor.h"
#include "inmdefines.h"
#include "serializevolumegroupsettings.h"
#include "logger.h"
#include "inmsafeint.h"
#include "inmageex.h"
#include "securityutils.h"
#include "scopeguard.h"
#include "setpermissions.h"

const char DataCacher::CHECKSUM_SEPARATOR = '\n';

bool DataCacher::UpdateVolumesCache(DataCacher::VolumesCache &volumesCache, std::string &err, SV_LOG_LEVEL logLevel)
{
    bool iscacheavailable = false;
    DataCacher dataCacher;
    std::stringstream ssErr;
    if (dataCacher.Init()) {
        time_t t = dataCacher.GetVolumesCacheMtime(logLevel);
        if (0 == t) {
            ssErr << "Failed to find volumes cache modified time.";
            DebugPrintf(logLevel, "%s: %s\n", FUNCTION_NAME, ssErr.str().c_str());
        }
        else if (t != volumesCache.m_MTime) {
            DebugPrintf(SV_LOG_DEBUG, "volumes cache updated. Getting latest one.\n");
            DataCacher::CACHE_STATE cs = dataCacher.GetVolumesFromCache(volumesCache);
            if (DataCacher::E_CACHE_VALID == cs) {
                DebugPrintf(SV_LOG_DEBUG, "Got latest cache.\n");
                iscacheavailable = true;
            }
            else if (DataCacher::E_CACHE_DOES_NOT_EXIST == cs) {
                ssErr << "volumes cache not existing.";
                DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, ssErr.str().c_str());
            }
            else {
                ssErr << "Failed to get volumes cache with error " << dataCacher.GetErrMsg();
                DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, ssErr.str().c_str());
            }
        }
        else {
            DebugPrintf(SV_LOG_DEBUG, "volumes cache is already latest.\n");
            iscacheavailable = true;
        }
    }
    else
    {
        ssErr << "DataCacher initialization failed.";
        DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, ssErr.str().c_str());
    }
    err = ssErr.str();
    return iscacheavailable;
}

DataCacher::DataCacher()
{
}


bool DataCacher::Init(void)
{
    std::string volumesCachePath;
    bool brval = LocalConfigurator::getVolumesCachePathname(volumesCachePath);
    if (brval)
    {
        SetVolumesCachePath(volumesCachePath);
    }
    
    return brval;
}


void DataCacher::SetVolumesCachePath(const std::string &volumesCachePath)
{
	m_VolumesCachePath = volumesCachePath;
#if defined (SV_WINDOWS) && defined (SV_USES_LONGPATHS)
	m_VolumesCacheLockPath = "\\\\?\\" + m_VolumesCachePath + INMLOCKEXT;
#else
	m_VolumesCacheLockPath = m_VolumesCachePath + INMLOCKEXT;
#endif
}

bool DataCacher::GetSCSIAddress(const VolumeSummary &vs, 
								std::string& bus, 
								std::string& port, 
								std::string& target, 
								std::string& lun)
{
	ConstAttributesIter_t	cit;
	cit = vs.attributes.find(NsVolumeAttributes::SCSI_BUS);
	if (vs.attributes.end() == cit) {
		return false;
	}

	bus = cit->second;

	cit = vs.attributes.find(NsVolumeAttributes::SCSI_PORT);
	if (vs.attributes.end() == cit){
		return false;
	}

	port = cit->second;

	cit = vs.attributes.find(NsVolumeAttributes::SCSI_TARGET_ID);
	if (vs.attributes.end() == cit){
		return false;
	}

	target = cit->second;

	cit = vs.attributes.find(NsVolumeAttributes::SCSI_LOGICAL_UNIT);
	if (vs.attributes.end() == cit){
		return false;
	}

	lun = cit->second;
	return true;
}

void DataCacher::ValidateOldVolumeState(const VolumeSummary &vs, const VolumeSummaries_t& allVolumes)
{
	std::string oldScsiBus, oldScsiLun, oldScsiPort, oldScsiTarget;
	std::string scsiBus, scsiLun, scsiPort, scsiTarget;

	if (!GetSCSIAddress(vs, oldScsiBus, oldScsiPort, oldScsiTarget, oldScsiLun)) {
		return;
	}

	VolumeSummaries_t::const_iterator volIt = allVolumes.begin();
	for (; volIt != allVolumes.end(); volIt++)
	{
        if (VolumeSummary::DISK != volIt->type) {
            continue;
        }
        ConstAttributesIter_t   cit;
        cit = volIt->attributes.find(NsVolumeAttributes::NAME_BASED_ON);
        if (volIt->attributes.end() == cit) {
            continue;
        }
        if (NsVolumeAttributes::SIGNATURE != cit->second) {
            continue;
        }
		if (!GetSCSIAddress(*volIt, scsiBus, scsiPort, scsiTarget, scsiLun)) {
			continue;
		}

		if ((0 != scsiBus.compare(oldScsiBus)) ||
			(0 != scsiTarget.compare(oldScsiTarget)) ||
			(0 != scsiPort.compare(oldScsiPort)) ||
			(0 != scsiLun.compare(oldScsiLun))) {
			continue;
		}

		if (0 != vs.name.compare(volIt->name)) {
			if (!vs.id.empty() && !volIt->id.empty())
			{
				DebugPrintf(SV_LOG_ERROR, "DiskId Changed old Id: %s Signature: %s New Id: %s Signature: %s ScsiBus: %s ScsiPort: %s ScsiTarget: %s ScsiLun: %s",
					vs.id.c_str(), vs.name.c_str(), volIt->id.c_str(), volIt->name.c_str(), scsiBus.c_str(), scsiPort.c_str(), scsiTarget.c_str(), scsiLun.c_str());
			}
			else
			{
				DebugPrintf(SV_LOG_ERROR, "DiskId Changed old Signature: %s New Signature: %s ScsiBus: %s ScsiPort: %s ScsiTarget: %s ScsiLun: %s",
					vs.name.c_str(), volIt->name.c_str(), scsiBus.c_str(), scsiPort.c_str(), scsiTarget.c_str(), scsiLun.c_str());
			}
		}
		break;
	}
}

void DataCacher::VerifyVolumeStateChanges(const VolumeSummaries_t &allVolumes)
{
	VolumesCache	oldVolumesCache;

	DataCacher::CACHE_STATE cs = GetVolumesFromCache(oldVolumesCache);
	if (cs != DataCacher::E_CACHE_VALID){
		return;
	}

	VolumeSummaries_t::iterator oldVolumeIt = oldVolumesCache.m_Vs.begin();
	for (; oldVolumeIt != oldVolumesCache.m_Vs.end(); oldVolumeIt++)
	{
		if (VolumeSummary::DISK != oldVolumeIt->type) {
			continue;
		}

		ConstAttributesIter_t	cit;
		cit = oldVolumeIt->attributes.find(NsVolumeAttributes::NAME_BASED_ON);
		if (oldVolumeIt->attributes.end() == cit) {
			continue;
		}

		if (NsVolumeAttributes::SIGNATURE != cit->second) {
			continue;
		}

		ValidateOldVolumeState(*oldVolumeIt, allVolumes);
	}
}

bool DataCacher::PutVolumesToCache(const VolumeSummaries_t &vs)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

	// Check if earlier ID has changed
#ifdef SV_WINDOWS
    VerifyVolumeStateChanges(vs);
#endif
	bool haveput = CacheVolumesAndChecksum(vs);

	// Do not leave stale cache file
	if (!haveput) {
		std::string delErrMsg;
		if (!DeleteVolumesCacheFile(delErrMsg))
			m_ErrMsg += delErrMsg;
	}

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
	return haveput;
}


/*
* volumes Cache file format:
* _______________
* |<checksum><\n>|
* |<data>        |
* |______________|
*/
bool DataCacher::CacheVolumesAndChecksum(const VolumeSummaries_t &vs)
{
	bool havecached = false;

	//Generate serialized volume summaries and its checksum
	std::stringstream serializedVs;
	serializedVs << CxArgObj<VolumeSummaries_t>(vs);
	std::string checksum = securitylib::genSha256Mac(serializedVs.str().c_str(), serializedVs.str().length());

    VolumesCache vc;
    DataCacher::CACHE_STATE cs = GetVolumesFromCache(vc);
    if (DataCacher::E_CACHE_VALID == cs) {
        if (checksum == vc.m_Checksum) {
            DebugPrintf(SV_LOG_DEBUG, "The volume cache is not updated as the checksum is matching.\n");
            return true;
        }
    }

	DebugPrintf(SV_LOG_DEBUG, "Updating volumes cache file %s with checksum: %s\n", m_VolumesCachePath.c_str(), checksum.c_str());

	//Acquire write lock
	DebugPrintf(SV_LOG_DEBUG, "Acquiring write lock %s\n", m_VolumesCacheLockPath.c_str());
	ACE_File_Lock lock(ACE_TEXT_CHAR_TO_TCHAR(m_VolumesCacheLockPath.c_str()), O_CREAT | O_RDWR, SV_LOCK_PERMISSIONS, 0);
	if (-1 == lock.acquire_write()) {
		int saveErrNo = ACE_OS::last_error();
		m_ErrMsg = std::string("Failed to take volumes cache write lock ") + m_VolumesCacheLockPath + " with error " + boost::lexical_cast<std::string>(saveErrNo) + ".";
		return false;
	}
	DebugPrintf(SV_LOG_DEBUG, "Acquired.\n");

	//Write to cache file
	FILE *fp;
	std::string mode, errMsg;
	mode = "w";
	mode += FOPEN_MODE_NOINHERIT;
	if (InmFopen(&fp, m_VolumesCachePath, mode, errMsg)) {
		DebugPrintf(SV_LOG_DEBUG, "opened cache file.\n");
		havecached = InmFprintf(fp, m_VolumesCachePath, errMsg, false, 0, "%s%c%s", checksum.c_str(), CHECKSUM_SEPARATOR, serializedVs.str().c_str());
		if (havecached)
			DebugPrintf(SV_LOG_DEBUG, "updated cache file.\n");
		else
			m_ErrMsg = std::string("Failed to update volumes cache file ") + m_VolumesCachePath + " with error " + errMsg + ".";
		//For output file io functions, fclose return value has to be checked to know all output was written correctly
		if (InmFclose(&fp, m_VolumesCachePath, errMsg))
		{
                        std::string errormsg;
                        if (!securitylib::setPermissions(m_VolumesCachePath, errormsg)) {
                            DebugPrintf(SV_LOG_ERROR, "%s\n", errormsg.c_str());
                        }
			DebugPrintf(SV_LOG_DEBUG, "closed cache file.\n");
		} else {
			havecached = false;
			m_ErrMsg += std::string("Failed to close handle of volumes cache file ") + m_VolumesCachePath + " with error " + errMsg + ".";
		}
	} else
		m_ErrMsg = std::string("Failed to open volumes cache file ") + m_VolumesCachePath + " with error " + errMsg + ".";

	return havecached;
}


DataCacher::CACHE_STATE DataCacher::GetVolumesFromCache(VolumesCache &vc)
{
	CACHE_STATE cs = VerifyCacheAndGetVolumes(vc);

	// Clear cache if it is not valid
	if ((E_CACHE_CORRUPT == cs) || (E_CACHE_READ_FAILURE == cs)) {
		std::string delErrMsg;
		if (!DeleteVolumesCacheFile(delErrMsg))
			m_ErrMsg += delErrMsg;
	}

	return cs;
}


DataCacher::CACHE_STATE DataCacher::VerifyCacheAndGetVolumes(VolumesCache &vc)
{
	//Acquire read lock
	DebugPrintf(SV_LOG_DEBUG, "Getting volumes from volumes cache %s. Acquiring read lock %s\n", m_VolumesCachePath.c_str(), m_VolumesCacheLockPath.c_str());
	ACE_File_Lock lock(ACE_TEXT_CHAR_TO_TCHAR(m_VolumesCacheLockPath.c_str()), O_CREAT | O_RDWR, SV_LOCK_PERMISSIONS, 0);
	if (-1 == lock.acquire_read()) {
		int saveErrNo = ACE_OS::last_error();
		m_ErrMsg = std::string("Failed to take volumes cache read lock ") + m_VolumesCacheLockPath + " with error " + boost::lexical_cast<std::string>(saveErrNo) + ".";
		return E_CACHE_LOCK_FAILURE;
	}
	DebugPrintf(SV_LOG_DEBUG, "Acquired.\n");

	//Open cache
	CACHE_STATE cs;
	std::string mode("r");
	mode += FOPEN_MODE_NOINHERIT;
	FILE *fp = fopen(m_VolumesCachePath.c_str(), mode.c_str());
	if (fp) {
		DebugPrintf(SV_LOG_DEBUG, "opened cache file.\n");
		ON_BLOCK_EXIT(&fclose, fp);
		vc.m_MTime = GetVolumesCacheMtime();
		cs = VerifyFileAndGetVolumes(fp, vc);
	} else {
		int saveErrNo = errno;
		cs = (ENOENT == saveErrNo) ? E_CACHE_DOES_NOT_EXIST : E_CACHE_READ_FAILURE;
		m_ErrMsg = std::string("Failed to open volumes cache file ") + m_VolumesCachePath + " with error " + boost::lexical_cast<std::string>(saveErrNo)+".";
	}

	return cs;
}


DataCacher::CACHE_STATE DataCacher::VerifyFileAndGetVolumes(FILE *fp, VolumesCache &vc)
{
	CACHE_STATE cs = E_CACHE_CORRUPT;
	
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
		m_ErrMsg = std::string("Failed to find checksum in volumes cache file ") + m_VolumesCachePath + ".";
		shouldproceed = false;
	}
	//Verify data
	if (data.empty()) {
		m_ErrMsg += std::string("Failed to find data in volumes cache file ") + m_VolumesCachePath + ".";
		shouldproceed = false;
	}

	//non empty checksum and data: calculate and compare checksum
	if (shouldproceed)
		cs = FillVolumesCacheIfDataValid(checksum, data, vc) ? E_CACHE_VALID : E_CACHE_CORRUPT;

	return cs;
}


bool DataCacher::FillVolumesCacheIfDataValid(const std::string &checksum, const std::string &data, VolumesCache &vc)
{
	bool isvalid = false;

	//Get new checksum
	std::string newChecksum = securitylib::genSha256Mac(data.c_str(), data.length());
	DebugPrintf(SV_LOG_DEBUG, "Newly generated checksum is %s\n", newChecksum.c_str());

	//Compare and parse data to get volumes
	if (newChecksum == checksum) {
		try {
			DebugPrintf(SV_LOG_DEBUG, "Checksums match.\n");
			vc.m_Vs = unmarshal<VolumeSummaries_t>(data);
			isvalid = true;
            vc.m_Checksum = checksum;
			DebugPrintf(SV_LOG_DEBUG, "Successfully parsed volumes.\n");
		} catch (std::string const& e) {
			m_ErrMsg = std::string("Failed to parse data to get volumes with error ") + e;
		} catch (char const* e) {
			m_ErrMsg = std::string("Failed to parse data to get volumes with error ") + e;
		} catch (ContextualException const& ce) {
			m_ErrMsg = std::string("Failed to parse data to get volumes with error ") + ce.what();
		} catch (std::exception const& e) {
			m_ErrMsg = std::string("Failed to parse data to get volumes with error ") + e.what();
		} catch (...) {
			m_ErrMsg = "Failed to parse data to get volumes failed";
		}
	} else
		m_ErrMsg = "Checksum in file: " + checksum + ", does not match calculated checksum: " + newChecksum;

	if (!isvalid)
		m_ErrMsg += std::string(". volumes cache file ") + m_VolumesCachePath + " is corrupt.";

	return isvalid;
}


bool DataCacher::ClearVolumesCache(void)
{
	std::string delErrMsg;
	bool havecleared = DeleteVolumesCacheFile(delErrMsg);
	if (!havecleared)
		m_ErrMsg = delErrMsg;

	return havecleared;
}

//This is also an implicit corrective action if write/read to cache fail. Hence m_ErrMsg should not be overwritten.
bool DataCacher::DeleteVolumesCacheFile(std::string &errMsg)
{
	bool havedeleted = true;

	DebugPrintf(SV_LOG_DEBUG, "Clearing cache file %s.\n", m_VolumesCachePath.c_str());
	//Dual check to avoid unnecessary lock in case lot of readers are trying to delete corrupt cache
	if (DoesVolumesCacheExist()) {
		//Acquire write lock
		DebugPrintf(SV_LOG_DEBUG, "Acquiring write lock %s\n", m_VolumesCacheLockPath.c_str());
		ACE_File_Lock lock(ACE_TEXT_CHAR_TO_TCHAR(m_VolumesCacheLockPath.c_str()), O_CREAT | O_RDWR, SV_LOCK_PERMISSIONS, 0);
		if (-1 == lock.acquire_write()) {
			int saveErrNo = ACE_OS::last_error();
			errMsg = std::string("Failed to take volumes cache write lock ") + m_VolumesCacheLockPath + " with error " + boost::lexical_cast<std::string>(saveErrNo)+".";
			return false;
		}
		DebugPrintf(SV_LOG_DEBUG, "Acquired.\n");
		//check again and clear
		if (DoesVolumesCacheExist()) {
			havedeleted = (0 == remove(m_VolumesCachePath.c_str()));
			if (havedeleted)
				DebugPrintf(SV_LOG_DEBUG, "Cleared volumes cache file %s.\n", m_VolumesCachePath.c_str());
			else {
				int saveErrNo = errno;
				errMsg = std::string("Failed to remove volumes cache file ") + m_VolumesCachePath + " with error " + boost::lexical_cast<std::string>(saveErrNo) + ".";
			}
		}
	}

	return havedeleted;
}


bool DataCacher::DoesVolumesCacheExist(void)
{
	ACE_stat st;
	bool isexisting = false;
	if (0 == sv_stat(m_VolumesCachePath.c_str(), &st)) {
		isexisting = true;
		DebugPrintf(SV_LOG_DEBUG, "volumes cache file %s exists.\n", m_VolumesCachePath.c_str());
	} else 
		DebugPrintf(SV_LOG_DEBUG, "volumes cache file %s does not exist.\n", m_VolumesCachePath.c_str());
	
	return isexisting;
}


std::string DataCacher::GetErrMsg(void)
{
	return m_ErrMsg;
}


time_t DataCacher::GetVolumesCacheMtime(SV_LOG_LEVEL logLevel)
{
	ACE_stat st;
	time_t t = 0;
	if (0 == sv_stat(m_VolumesCachePath.c_str(), &st)) {
		t = st.st_mtime;
		std::stringstream ss;
		ss << "volumes cache file " << m_VolumesCachePath << " has mtime " << t;
        DebugPrintf(SV_LOG_DEBUG, "%s.\n", ss.str().c_str());
	} else
        DebugPrintf(logLevel, "volumes cache file %s does not exist. Hence cannot find mtime.\n", m_VolumesCachePath.c_str());

	return t;
}

std::string DataCacher::GetDeviceNameForDeviceSignature(std::string m_deviceSig) 
{
    std::string err;
    DataCacher::VolumesCache    volumesCache;

    if (!DataCacher::UpdateVolumesCache(volumesCache, err))
    {
        throw INMAGE_EX(err.c_str());
    }

    ConstVolumeSummariesIter itrVS = volumesCache.m_Vs.begin();
    for (/*empty*/; itrVS != volumesCache.m_Vs.end(); ++itrVS)
    {
        ConstAttributesIter_t cit = itrVS->attributes.find(NsVolumeAttributes::NAME_BASED_ON);
        if (cit != itrVS->attributes.end()
            && ((NsVolumeAttributes::SIGNATURE == cit->second)
                || (NsVolumeAttributes::SCSIHCTL == cit->second)))
        {
            if (itrVS->type == VolumeSummary::DISK && itrVS->name.compare(m_deviceSig) == 0)
            {
                if (itrVS->attributes.find("display_name") == itrVS->attributes.end())
                {
                    DebugPrintf(SV_LOG_ERROR, "Display name is not populated");
                }
                else 
                {
                    return itrVS->attributes.at("display_name");
                }
            }
        }
    }
    DebugPrintf(SV_LOG_DEBUG, "Could not find display name for device %s\n", m_deviceSig.c_str());
    return m_deviceSig;
}
