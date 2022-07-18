#include "retentionsettings.h"
#include <ace/OS.h>
#include <algorithm>



using std::max;

static bool IsRepeatingSlash( char ch1, char ch2 ) 
{ return ((ACE_DIRECTORY_SEPARATOR_CHAR_A == ch1) && (ch2 == ch1)); }

cdp_policy::cdp_policy()
{
	start = 0;
	duration = 0;
	granularity = 0;
	tagcount = 0;
	cdp_diskspace_forthiswindow = 0;
	cdp_contentstore = "";
	apptype = 0;
	userbookmarks.clear();

}

cdp_policy::cdp_policy(const cdp_policy& rhs)
{
	start = rhs.start;
	duration = rhs.duration;
	granularity = rhs.granularity;
	tagcount = rhs.tagcount;
	cdp_diskspace_forthiswindow = rhs.cdp_diskspace_forthiswindow;
	cdp_contentstore = rhs.cdp_contentstore;
	apptype = rhs.apptype;
	userbookmarks.assign(rhs.userbookmarks.begin(), rhs.userbookmarks.end());
}

cdp_policy& cdp_policy::operator =(const cdp_policy& rhs)
{
	if ( this == &rhs)
		return *this;
	start = rhs.start;
	duration = rhs.duration;
	granularity = rhs.granularity;
	tagcount = rhs.tagcount;
	cdp_diskspace_forthiswindow = rhs.cdp_diskspace_forthiswindow;
	cdp_contentstore = rhs.cdp_contentstore;
	apptype = rhs.apptype;
	userbookmarks.assign(rhs.userbookmarks.begin(), rhs.userbookmarks.end());
	return *this;
}

bool cdp_policy::operator==( cdp_policy const& rhs ) const
{
	return ( start == rhs.start 
		&& duration == rhs.duration
		&& granularity == rhs.granularity
		&& tagcount == rhs.tagcount
		&& cdp_diskspace_forthiswindow == rhs.cdp_diskspace_forthiswindow
		&& cdp_contentstore == rhs.cdp_contentstore
		&& apptype == rhs.apptype
		&& userbookmarks == rhs.userbookmarks );
}

cdp_policy::~cdp_policy()
{
	userbookmarks.clear();
}

CDP_SETTINGS::CDP_SETTINGS()
{
	cdp_status = CDP_DISABLED;
	cdp_id = "";
	cdp_version = 3;
	cdp_alert_threshold = 80;
	cdp_diskspace = 0;
	cdp_onspace_shortage = 0;
	cdp_timepolicy = 0;
	cdp_minreservedspace = 0;
	cdp_policies.clear();
}
CDP_SETTINGS::CDP_SETTINGS(const CDP_SETTINGS& rhs)
{
	cdp_status = rhs.cdp_status;
	cdp_id = rhs.cdp_id;
	cdp_version = rhs.cdp_version;
	cdp_catalogue = rhs.cdp_catalogue;
	cdp_alert_threshold = rhs.cdp_alert_threshold;
	cdp_diskspace = rhs.cdp_diskspace;
	cdp_onspace_shortage = rhs.cdp_onspace_shortage;
	cdp_timepolicy = rhs.cdp_timepolicy;
	cdp_minreservedspace = rhs.cdp_minreservedspace;
	cdp_policies.assign(rhs.cdp_policies.begin(), rhs.cdp_policies.end());
}

CDP_SETTINGS& CDP_SETTINGS::operator =(const CDP_SETTINGS& rhs)
{
	if ( this == &rhs)
		return *this;

	cdp_status = rhs.cdp_status;
	cdp_id = rhs.cdp_id;
	cdp_version = rhs.cdp_version;
	cdp_catalogue = rhs.cdp_catalogue;
	cdp_alert_threshold = rhs.cdp_alert_threshold;
	cdp_diskspace = rhs.cdp_diskspace;
	cdp_onspace_shortage = rhs.cdp_onspace_shortage;
	cdp_timepolicy = rhs.cdp_timepolicy;
	cdp_minreservedspace = rhs.cdp_minreservedspace;
	cdp_policies.assign(rhs.cdp_policies.begin(), rhs.cdp_policies.end());

	return *this;
}

bool CDP_SETTINGS::operator==( CDP_SETTINGS const& rhs ) const
{
	return ( cdp_status == rhs.cdp_status
		&& 	cdp_id == rhs.cdp_id
		&&	cdp_version == rhs.cdp_version
		&&	cdp_catalogue == rhs.cdp_catalogue
		&&	cdp_alert_threshold == rhs.cdp_alert_threshold
		&&	cdp_diskspace == rhs.cdp_diskspace
		&&	cdp_onspace_shortage == rhs.cdp_onspace_shortage
		&&	cdp_timepolicy == rhs.cdp_timepolicy
		&&  cdp_minreservedspace == rhs.cdp_minreservedspace
		&&	cdp_policies == rhs.cdp_policies );
}

SV_INT CDP_SETTINGS::Status() const 
{
	return cdp_status; 
}


void CDP_SETTINGS::Status(SV_INT status)
{
	cdp_status = status;
}


std::string CDP_SETTINGS::CatalogueDir() const
{
	char return_dirname[SV_MAX_PATH + 1];
	memset(return_dirname, 0, SV_MAX_PATH + 1);

	const char *temp1 = ACE_OS::strrchr (cdp_catalogue.c_str(), '\\');
	const char *temp2 = ACE_OS::strrchr (cdp_catalogue.c_str(), '/');

	if (temp1 == 0 && temp2 == 0)
	{
		return_dirname[0] = '.';
		return_dirname[1] = '\0';

		return return_dirname;
	}
	else
	{
		size_t len1 = 0;
		size_t len2 = 0;
		size_t len = 0;
		if(temp1 != 0)
		{
			// When the len is truncated, there are problems!  This should
			// not happen in normal circumstances
			len1 = temp1 - cdp_catalogue.c_str() + 1;
			if (len1 >= (sizeof return_dirname / sizeof (char)))
				len1 = (sizeof return_dirname / sizeof (char)) -1;
		}

		if(temp2 != 0)
		{
			// When the len is truncated, there are problems!  This should
			// not happen in normal circumstances
			len2 = temp2 - cdp_catalogue.c_str() + 1;
			if (len2 >= (sizeof return_dirname / sizeof (char)))
				len2 = (sizeof return_dirname / sizeof (char)) - 1 ;
		}
	
		len = max(len1, len2);
	

		ACE_OS::strsncpy (return_dirname, cdp_catalogue.c_str(), len);
		std::string ret_dir = return_dirname;
        if((ret_dir[0] == '\\') && (ret_dir[1] == '\\'))
		    ret_dir.erase(unique(ret_dir.begin()+1,ret_dir.end(),IsRepeatingSlash), ret_dir.end());
        else
		ret_dir.erase(unique(ret_dir.begin(),ret_dir.end(),IsRepeatingSlash), ret_dir.end());
		
		//return return_dirname;
		return ret_dir;
	}

}

std::string CDP_SETTINGS::Catalogue() const 
{
	std::string dbname = cdp_catalogue;
    if((dbname[0] == '\\') && (dbname[1] == '\\'))
	    dbname.erase(unique(dbname.begin()+1, dbname.end(), IsRepeatingSlash), dbname.end());
    else
	dbname.erase(unique(dbname.begin(), dbname.end(), IsRepeatingSlash), dbname.end());
	return dbname;
}


/*std::string CDP_SETTINGS::ContentStore() const
{
	return cdp_contentstore;
}*/

SV_ULONGLONG CDP_SETTINGS::DiskSpace() const 
{
	return cdp_diskspace; 
}

SV_ULONGLONG CDP_SETTINGS::TimePolicy() const 
{
	return cdp_timepolicy * SECONDS_IN_AN_HOUR * HUNDREDS_OF_NANOSEC_IN_SECOND; 
}

SV_UINT CDP_SETTINGS::CatalogueThreshold() const 
{
	return cdp_alert_threshold; 
}

SV_ULONGLONG CDP_SETTINGS::MinReservedSpace() const 
{
	return cdp_minreservedspace; 
}

SV_UINT CDP_SETTINGS::OnSpaceShortage() const 
{
	return cdp_onspace_shortage; 
}

