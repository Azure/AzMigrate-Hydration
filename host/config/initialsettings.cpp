#include "initialsettings.h"



// Implement copy constructor for initialsettings

InitialSettings::InitialSettings(const InitialSettings& initialSettings)
{
	hostVolumeSettings = initialSettings.hostVolumeSettings;
	cdpSettings = initialSettings.cdpSettings;
    timeoutInSecs = initialSettings.timeoutInSecs;
    remoteCxs = initialSettings.remoteCxs;
    prismSettings = initialSettings.prismSettings;
	transportSettings = initialSettings.transportSettings;
	transportProtocol = initialSettings.transportProtocol;
	transportSecureMode = initialSettings.transportSecureMode;
	options = initialSettings.options;
}
	
bool InitialSettings::operator==( InitialSettings const& initialSettings) const
{
	return ((hostVolumeSettings == initialSettings.hostVolumeSettings) 
		&& (cdpSettings == initialSettings.cdpSettings) 
        && (timeoutInSecs == initialSettings.timeoutInSecs)
        && (remoteCxs == initialSettings.remoteCxs)
        && (prismSettings == initialSettings.prismSettings)
		&& (transportSettings == initialSettings.transportSettings)
		&& (transportProtocol == initialSettings.transportProtocol)
		&& (transportSecureMode == initialSettings.transportSecureMode)
		&& (options == initialSettings.options)
        );
}

bool InitialSettings::strictCompare(InitialSettings const& initialSettings) const
{
	return ((hostVolumeSettings.strictCompare(initialSettings.hostVolumeSettings)) 
		&& (cdpSettings == initialSettings.cdpSettings) 
		&& (timeoutInSecs == initialSettings.timeoutInSecs)
		&& (remoteCxs == initialSettings.remoteCxs)
        && (prismSettings == initialSettings.prismSettings)
		&& (transportSettings == initialSettings.transportSettings)
		&& (transportProtocol == initialSettings.transportProtocol)
		&& (transportSecureMode == initialSettings.transportSecureMode)
		&& (options == initialSettings.options)
		);
}

InitialSettings& InitialSettings::operator=(const InitialSettings& initialSettings)
{
    if ( this == &initialSettings)
        return *this;

	hostVolumeSettings = initialSettings.hostVolumeSettings;
	cdpSettings = initialSettings.cdpSettings;
    timeoutInSecs = initialSettings.timeoutInSecs;
    remoteCxs = initialSettings.remoteCxs;
    prismSettings = initialSettings.prismSettings;
	transportSettings = initialSettings.transportSettings;
	transportProtocol = initialSettings.transportProtocol;
	transportSecureMode = initialSettings.transportSecureMode;
	options = initialSettings.options;

	return *this;
}


TRANSPORT_CONNECTION_SETTINGS InitialSettings::getTransportSettings(const std::string & deviceName)
{
	return hostVolumeSettings.getTransportSettings(deviceName);
}

bool InitialSettings::shouldThrottleTarget(std::string const& deviceName) const
{
	bool bthrottletarget = false;
	bool breakout = false;
	HOST_VOLUME_GROUP_SETTINGS::volumeGroups_t::const_iterator VolumeGroupSettingsIter;

	for ( VolumeGroupSettingsIter = hostVolumeSettings.volumeGroups.begin(); 
		(VolumeGroupSettingsIter != hostVolumeSettings.volumeGroups.end() && !breakout) ; 
		VolumeGroupSettingsIter++ )
	{
		for(VOLUME_GROUP_SETTINGS::volumes_t::const_iterator volumes = VolumeGroupSettingsIter->volumes.begin();volumes != VolumeGroupSettingsIter->volumes.end();
			volumes++)
		{
			if (deviceName == volumes->second.deviceName)
			{
				bthrottletarget = volumes->second.throttleSettings.IsTargetThrottled();
				breakout = true;
				break;
			} 
		}
	}

	return bthrottletarget;
}


TRANSPORT_CONNECTION_SETTINGS InitialSettings::getTransportSettings(void)
{
	return transportSettings;
}


VOLUME_SETTINGS::SECURE_MODE InitialSettings::getTransportSecureMode(void)
{
	return transportSecureMode;
}


TRANSPORT_PROTOCOL InitialSettings::getTransportProtocol(void)
{
	return transportProtocol;
}

/* Register Immdediately if Request id is non-empty in initial settings options */
bool InitialSettings::shouldRegisterOnDemand(void) const
{
	return !(getRequestIdForOnDemandRegistration().empty());
}

std::string InitialSettings::getRequestIdForOnDemandRegistration(void) const
{
  ConstOptionsIter_t it = options.find(NameSpaceInitialSettings::ON_DEMAND_REGISTRATION_REQUEST_ID_KEY);
  std::string reqid;

  if (it != options.end())
  {
    reqid = it->second;
  }

  return reqid;
}

bool InitialSettings::AnyJobsAvailableForProcessing(void) const
{
	ConstOptionsIter_t it = options.find(NameSpaceInitialSettings::ANY_JOBS_TO_BE_PROCESSED_KEY);
	std::string value;

	if (it != options.end())
	{
		value = it->second;
	}

	return (strcmpi(value.c_str(), "yes") == 0);
}

std::string InitialSettings::getDisksLayoutOption(void) const
{
  std::string o;
  ConstOptionsIter_t it = options.find(NameSpaceInitialSettings::DISKS_LAYOUT_OPTION_KEY);
  if (it != options.end())
    o = it->second;
  return o;
}

std::string InitialSettings::getCsAddressForAzureComponents(void) const
{
    std::string o;
    ConstOptionsIter_t it = options.find(NameSpaceInitialSettings::CS_ADDRESS_FOR_AZURE_COMPONENTS);
    if (it != options.end())
        o = it->second;
    return o;
}

ViableCx::ViableCx() 
: port(0)
{

}

ViableCx::ViableCx(const ViableCx& rhs ) 
{
    publicip = rhs.publicip;
    privateip = rhs.privateip;
    port = rhs.port;
}

bool ViableCx::operator==( ViableCx const& rhs ) const 
{
    return ((publicip == rhs.publicip) 
        && (privateip == rhs.privateip)
        &&(port == rhs.port)); 
}

ViableCx& ViableCx::operator=(const ViableCx& rhs)
{
    if ( this == &rhs)
        return *this;

    publicip = rhs.publicip;
    privateip = rhs.privateip;
    port = rhs.port;
    return *this;
}

ReachableCx::ReachableCx()
:port(0)
{

}

ReachableCx::ReachableCx(const ReachableCx& rhs)
{
	ip = rhs.ip;
	port = rhs.port;
}

ReachableCx& ReachableCx::operator=(const ReachableCx& rhs)
{
	if(this == &rhs)
		return *this;

	ip = rhs.ip;
	port = rhs.port;
	return *this;
}

bool ReachableCx::operator==(ReachableCx const& rhs) const
{
	return (ip == rhs.ip && port == rhs.port);
}

/*
 * FUNCTION NAME : StrictCompare
 *
 * DESCRIPTION : compare the initialsetting by each field.
 *
 * INPUT PARAMETERS : two initialsettings to be compared
 *
 * NOTES :
 *
 * return value : true if same; false if different
 *
 */

bool StrictCompare(const InitialSettings & lhs, const InitialSettings &rhs)
{
    return lhs.strictCompare(rhs);
}

