#include <cstring>
#include "sourcesystemconfigsettings.h"

SystemMetaDataSettings::SystemMetaDataSettings()
{
	
}

SOURCE_METADATA_SETTINGS::SOURCE_METADATA_SETTINGS()
{
	
}

SystemMetaDataSettings & SystemMetaDataSettings::operator=(const SystemMetaDataSettings &rhs)
{
	m_repositoryPath = rhs.m_repositoryPath;
	m_metadaTataUrls = rhs.m_metadaTataUrls;
	return *this;
}

SOURCE_METADATA_SETTINGS & SOURCE_METADATA_SETTINGS::operator=(const SOURCE_METADATA_SETTINGS &rhs)
{
	m_metadataSettings = rhs.m_metadataSettings;
	return *this;	
}

bool SystemMetaDataSettings::operator==(const SystemMetaDataSettings &rhs) const
{
	bool bRet = true;
    if( strcmp ( m_repositoryPath.c_str(), rhs.m_repositoryPath.c_str() ) != 0 ) 
        return false ;
    if( m_metadaTataUrls.size() != rhs.m_metadaTataUrls.size() )
        return false ;
	//if(strcmp(m_repositoryPath.c_str(), rhs.m_repositoryPath.c_str()) == 0 && m_metadaTataUrls.size() == rhs.m_metadaTataUrls.size())
	{
		std::list<std::string>::const_iterator lhsBegIter = m_metadaTataUrls.begin();
		std::list<std::string>::const_iterator lhsEndIter = m_metadaTataUrls.end();
		std::list<std::string>::const_iterator rhsBegIter, rhsEndIter;
		while(lhsBegIter != lhsEndIter)
		{
			//Finding whether the the old settings has the url of new settings.
			rhsBegIter = rhs.m_metadaTataUrls.begin();
			rhsEndIter = rhs.m_metadaTataUrls.end();
            
			while(rhsBegIter != rhsEndIter)
			{
				if(strcmp(lhsBegIter->c_str(), rhsBegIter->c_str()) == 0)
				{
					break;
				}
				rhsBegIter++;
			}
			//If it doesn't exist, new settings are different			
			if(rhsBegIter == rhsEndIter)
			{
				//DebugPrintf(SV_LOG_DEBUG, "%s is a not found in old settings\n", lhsBegIter->c_str()) ;
				bRet = false;
				break;
			}
			lhsBegIter++;
		}
	}
	return bRet;	
}

bool SOURCE_METADATA_SETTINGS::operator==(const SOURCE_METADATA_SETTINGS &rhs) const
{
    bool bRet = true;
	if(m_metadataSettings.size() != rhs.m_metadataSettings.size())
	{
		bRet = false;
	}
	else
	{
		std::map<std::string, SystemMetaDataSettings>::const_iterator metadataSettingsBegIter;
		std::map<std::string, SystemMetaDataSettings>::const_iterator metadataSettingsEndIter;
		std::map<std::string, SystemMetaDataSettings>::const_iterator metadataSettingsTempIter;
		std::map<std::string, SystemMetaDataSettings>::const_iterator rhsMetadataSettingsEndIter;
		metadataSettingsBegIter = m_metadataSettings.begin();
		metadataSettingsEndIter = m_metadataSettings.end();
		rhsMetadataSettingsEndIter = rhs.m_metadataSettings.end();
		while(metadataSettingsBegIter != metadataSettingsEndIter)
		{
			metadataSettingsTempIter = rhs.m_metadataSettings.find(metadataSettingsBegIter->first);
			if(metadataSettingsTempIter != rhsMetadataSettingsEndIter)
			{
				if(!(metadataSettingsBegIter->second == metadataSettingsTempIter->second))
				{
					bRet = false;
					break;
				}
			}
			else
			{
				bRet = false;
				break;
			}
			metadataSettingsBegIter++;
		}
	}
	return bRet;
}
