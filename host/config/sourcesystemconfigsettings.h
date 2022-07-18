#ifndef SOURCE_METADA_SETTINGS_H
#define SOURCE_METADA_SETTINGS_H

#include <string>
#include <list>
#include <map>

struct SystemMetaDataSettings
{
	std::string m_repositoryPath ;
	std::list<std::string> m_metadaTataUrls ;
	
	bool operator==(const SystemMetaDataSettings &rhs) const;
	SystemMetaDataSettings & operator=(const SystemMetaDataSettings &rhs);
	SystemMetaDataSettings();
};

struct SOURCE_METADATA_SETTINGS
{
	std::map<std::string, SystemMetaDataSettings> m_metadataSettings ;
	
	bool operator==(const SOURCE_METADATA_SETTINGS &rhs) const;
	SOURCE_METADATA_SETTINGS & operator=(const SOURCE_METADATA_SETTINGS &rhs);
	SOURCE_METADATA_SETTINGS();
};

#endif
