#ifndef DATA__CACHER__H_
#define DATA__CACHER__H_

#include <cstdio>
#include <string>
#include <ctime>
#include "volumegroupsettings.h"


/// \brief generic data cacher to cache anything
class DataCacher
{
public:
    enum CACHE_STATE {E_CACHE_LOCK_FAILURE, E_CACHE_DOES_NOT_EXIST, E_CACHE_CORRUPT, E_CACHE_READ_FAILURE, E_CACHE_VALID};

	struct VolumesCache
	{
		time_t m_MTime;
        std::string m_Checksum;
		VolumeSummaries_t m_Vs;

		VolumesCache()
			: m_MTime(0)
		{
		}
	};

    static bool UpdateVolumesCache(DataCacher::VolumesCache &volumesCache, std::string &err, SV_LOG_LEVEL logLevel = SV_LOG_ERROR);

	DataCacher();
    bool Init(void);

	bool PutVolumesToCache(const VolumeSummaries_t &vs);
    CACHE_STATE GetVolumesFromCache(VolumesCache &vc);
    bool ClearVolumesCache(void);
	bool DoesVolumesCacheExist(void);
    void VerifyVolumeStateChanges(const VolumeSummaries_t &vs);

	/// \brief gets modified time of volumes cache
	///
	/// \return
	/// \li \c time in seconds since platform specific epoch : success
	/// \li \c 0                                             : failure
    time_t GetVolumesCacheMtime(SV_LOG_LEVEL logLevel = SV_LOG_ERROR);

	std::string GetErrMsg(void);

    static std::string GetDeviceNameForDeviceSignature(std::string m_deviceId);

private:
	void SetVolumesCachePath(const std::string &volumesCachePath);
	bool CacheVolumesAndChecksum(const VolumeSummaries_t &vs);
	CACHE_STATE VerifyCacheAndGetVolumes(VolumesCache &vc);
    CACHE_STATE VerifyFileAndGetVolumes(FILE *fp, VolumesCache &vc);
    bool FillVolumesCacheIfDataValid(const std::string &checksum, const std::string &data, VolumesCache &vc);
	bool DeleteVolumesCacheFile(std::string &errMsg);
	
	bool GetSCSIAddress(const VolumeSummary &vs, std::string& bus, std::string& port, std::string& target, std::string& lun);

	void ValidateOldVolumeState(const VolumeSummary &vs, const VolumeSummaries_t& oldVolumes);

	static const char CHECKSUM_SEPARATOR;

	std::string m_VolumesCachePath;
	std::string m_VolumesCacheLockPath;
	std::string m_ErrMsg;
};

#endif /* DATA__CACHER__H_ */
