//
// fileconfigurator.h: defines configurator that fetches values from a config
// file.  Does not look for file changes, it assumes it is static information. 
// Thus signals will never fire.
//
#ifndef FILECONFIGURATOR__H
#define FILECONFIGURATOR__H

#include "../configurator.h"
#include "hostagenthelpers_ported.h"

class FileConfigurator : public LocalConfigurator
{
public:
	//
	// constructors/destructors
	//
	FileConfigurator( SVERROR& rc );
	virtual ~FileConfigurator();

public:
	//
	// Tal configurator interface
	//
	virtual HTTP_CONNECTION_SETTINGS getHttpSettings();
	virtual void updateHttpSettings( HTTP_CONNECTION_SETTINGS const& settings );

public:
	//
	// VX Agent configurator interface
	//
	virtual std::string getHostId();
	virtual HOST_VOLUME_GROUP_SETTINGS getHostVolumeGroupSettings();
	virtual sigslot::signal1<HOST_VOLUME_GROUP_SETTINGS>& getHostVolumeGroupSignal();
	virtual bool getResyncRequired() const;
	virtual void setResyncRequired();
	virtual SV_HOST_AGENT_TYPE getAgentType() const;

private:
	//
	// Helper methods, constants
	//
	SV_HOST_AGENT_TYPE GetAgentType() const;
	static const char RegistryKeyName[];
	static const char SENTINEL_KEY[];
	SVERROR setShouldResync( bool shouldResync );

private:
	//
	// State
	//
	SV_HOST_AGENT_PARAMS m_AgentParams;
	sigslot::signal1<HOST_VOLUME_GROUP_SETTINGS> m_HostVolumeGroupSettingsSignal;
	SV_HOST_AGENT_TYPE m_AgentType;
};

#endif // FILECONFIGURATOR__H
