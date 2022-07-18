#ifndef RETENTIONSETTINGS_H
#define RETENTIONSETTINGS_H

#include <string>
#include <list>
#include <vector>
#include <map>
#include "svtypes.h"
#include "portable.h"


enum CDP_STATE               { CDP_DISABLED, CDP_ENABLED, CDP_PAUSED };
enum CDP_LOGTYPE             { CDP_UNDO, CDP_REDO };
enum POLICY_FAILACTION { CDP_PRUNE , CDP_STOP };

#define HUNDREDS_OF_NANOSEC_IN_SECOND 10000000LL
#define SECONDS_IN_AN_HOUR 3600
#define HUNDREDS_OF_NANOSEC_IN_MINUTE     600000000LL
#define HUNDREDS_OF_NANOSEC_IN_HOUR     36000000000LL
#define HUNDREDS_OF_NANOSEC_IN_DAY     864000000000LL     // 24 hours
#define HUNDREDS_OF_NANOSEC_IN_WEEK   6048000000000LL    //7 days
#define HUNDREDS_OF_NANOSEC_IN_MONTH 25920000000000LL //30 days
#define HUNDREDS_OF_NANOSEC_IN_YEAR 315360000000000LL //365 days

struct cdp_policy
{
	cdp_policy();
	~cdp_policy();
	bool operator==(cdp_policy const&) const;
	cdp_policy(const cdp_policy&);
	cdp_policy& operator=(const cdp_policy&);

	SV_ULONGLONG Start() const { return start; }
	void Start(SV_ULONGLONG val) { start = val; }

	SV_ULONGLONG Duration() const { return duration; }
	void Duration(SV_ULONGLONG val) { duration = val; }

	SV_UINT Granularity() const { return granularity; }
	void Granularity(SV_UINT val) { granularity = val; }

	SV_ULONGLONG CdpSpaceConfigure() const { return cdp_diskspace_forthiswindow; }
	void CdpSpaceConfigure(SV_ULONGLONG val) { cdp_diskspace_forthiswindow = val; }

	SV_UINT TagCount() const { return tagcount; }
	void TagCount(SV_UINT val) { tagcount = val; }

	std::string CdpContentstore() const { return cdp_contentstore; }
	void CdpContentstore(std::string val) { cdp_contentstore = val; }

	SV_UINT AppType() const { return apptype; }
	void AppType(SV_UINT val) { apptype = val; }

	SV_ULONGLONG			start;
	SV_ULONGLONG			duration;
	SV_UINT					granularity;
	SV_UINT					tagcount;
    SV_ULONGLONG			cdp_diskspace_forthiswindow;
	std::string				cdp_contentstore;
    SV_UINT  				apptype;
	std::vector<std::string>	userbookmarks;
};

typedef std::vector<cdp_policy> cdp_policies_t;


struct CDP_SETTINGS
{

	CDP_SETTINGS();
	CDP_SETTINGS(const CDP_SETTINGS&);
	CDP_SETTINGS& operator=(const CDP_SETTINGS&);
	bool operator==(CDP_SETTINGS const&) const;

	SV_INT Status() const;
	void Status(SV_INT status);
	std::string CatalogueDir() const;
	std::string Catalogue() const;
	std::string ContentStore() const;
	SV_ULONGLONG DiskSpace() const;
	SV_ULONGLONG TimePolicy() const;
	SV_UINT CatalogueThreshold() const;
	SV_ULONGLONG MinReservedSpace() const;
	SV_UINT OnSpaceShortage() const;

	
	unsigned int			cdp_status;
	std::string				cdp_id;
	unsigned int			cdp_version;
	std::string				cdp_catalogue;		
	unsigned int			cdp_alert_threshold;
	unsigned long long		cdp_diskspace;
	unsigned int			cdp_onspace_shortage;
	SV_ULONGLONG			cdp_timepolicy;
	SV_ULONGLONG			cdp_minreservedspace;	

	std::vector<cdp_policy>	cdp_policies;
};


typedef std::map<std::string,CDP_SETTINGS> CDPSETTINGS_MAP;

#endif // RETENTIONSETTINGS_H

