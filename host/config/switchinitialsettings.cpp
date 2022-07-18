 
#include "switchinitialsettings.h"

SwitchInitialSettings:: SwitchInitialSettings(const SwitchInitialSettings& rhs)
 {
	 frameRedirectOperationSet = rhs.frameRedirectOperationSet;
	 physicalLunDiscoveryInfoSet = rhs.physicalLunDiscoveryInfoSet;
	 enableSlowPathCache = rhs.enableSlowPathCache;
 }
 

SwitchInitialSettings& SwitchInitialSettings::operator=(const SwitchInitialSettings& rhs)
{
	frameRedirectOperationSet = rhs.frameRedirectOperationSet;
	physicalLunDiscoveryInfoSet = rhs.physicalLunDiscoveryInfoSet;
	enableSlowPathCache = rhs.enableSlowPathCache;
	return *this;

}

SwitchSummary:: SwitchSummary(const SwitchSummary& rhs)
{
	//guid = rhs.guid;
	switchName = rhs.switchName;
	agentVersion = rhs.agentVersion;
	//platformModel = rhs.platformModel;
	//cpIPAdressList = rhs.cpIPAdressList;
	//prefCpIPAdressList = rhs.prefCpIPAdressList;
	bpIPAdressList = rhs.bpIPAdressList;
	prefBpIPAdressList = rhs.prefBpIPAdressList;
	//fabricwwn = rhs.fabricwwn;
	//switchwwn = rhs.switchwwn;
	//physicalPortInfo =  rhs.physicalPortInfo;
	dpcCount = rhs.dpcCount;
	sasVersion = rhs.sasVersion;
        fosVersion = rhs.fosVersion;
	driverVersion = rhs.driverVersion;
	osInfo = rhs.osInfo;
	currentCompatibilityNum = rhs.currentCompatibilityNum;
	installPath = rhs.installPath;
	osVal = rhs.osVal;
	timeZone = rhs.timeZone;
	localTime = rhs.localTime;
	patchHistory = rhs.patchHistory;
	ipAddress = rhs.ipAddress;
        sysVol = rhs.sysVol;
        sysVolCapacity = rhs.sysVolCapacity;
        sysVolFreeSpace = rhs. sysVolFreeSpace;
        
 }
 bool SwitchSummary::operator==( SwitchSummary & rhs) const
{
	return (//guid == rhs.guid &&
	switchName == rhs.switchName &&
	agentVersion == rhs.agentVersion &&
	//platformModel == rhs.platformModel &&
	//cpIPAdressList == rhs.cpIPAdressList &&
	//prefCpIPAdressList == rhs.prefCpIPAdressList &&
	bpIPAdressList == rhs.bpIPAdressList &&
	prefBpIPAdressList == rhs.prefBpIPAdressList &&
	//fabricwwn == rhs.fabricwwn &&
	//switchwwn == rhs.switchwwn &&
	//physicalPortInfo == rhs.physicalPortInfo &&
	dpcCount == rhs.dpcCount &&
	sasVersion == rhs.sasVersion &&
        fosVersion == rhs.fosVersion &&
	driverVersion == rhs.driverVersion &&
	osInfo == rhs.osInfo &&
	currentCompatibilityNum == rhs.currentCompatibilityNum &&
	installPath == rhs.installPath &&
	osVal == rhs.osVal &&
	timeZone == rhs.timeZone &&
	localTime == rhs.localTime &&
	patchHistory == rhs.patchHistory &&
	ipAddress == rhs.ipAddress && 
        sysVol == rhs.sysVol &&
        sysVolCapacity == rhs.sysVolCapacity &&
        sysVolFreeSpace == rhs.sysVolFreeSpace);
}

SwitchSummary& SwitchSummary::operator=(const SwitchSummary& rhs)
{
	//guid = rhs.guid;
	switchName = rhs.switchName;
	agentVersion = rhs.agentVersion;
	//platformModel = rhs.platformModel;
	//cpIPAdressList = rhs.cpIPAdressList;
	//prefCpIPAdressList = rhs.prefCpIPAdressList;
	bpIPAdressList = rhs.bpIPAdressList;
	prefBpIPAdressList = rhs.prefBpIPAdressList;
	//fabricwwn = rhs.fabricwwn;
	//switchwwn = rhs.switchwwn;
	//physicalPortInfo =  rhs.physicalPortInfo;
	dpcCount = rhs.dpcCount;
	sasVersion = rhs.sasVersion;
        fosVersion = rhs.fosVersion;
	driverVersion = rhs.driverVersion;
	osInfo = rhs.osInfo;
	currentCompatibilityNum = rhs.currentCompatibilityNum;
	installPath = rhs.installPath;
	osVal = rhs.osVal;
	timeZone = rhs.timeZone;
	localTime = rhs.localTime;
	patchHistory = rhs.patchHistory;
	ipAddress = rhs.ipAddress;
        sysVol = rhs.sysVol;
        sysVolCapacity = rhs.sysVolCapacity;
        sysVolFreeSpace = rhs. sysVolFreeSpace;
	return *this;
}

PersistSwitchInitialSettings:: PersistSwitchInitialSettings(FrameRedirectOperationSetting &frameRedirectOperationSetting)
 {
         FrameRedirectOperationSetting::FrameRedirectOperationListIter it = frameRedirectOperationSetting.frameRedirectOperationList.begin();
         for (; it != frameRedirectOperationSetting.frameRedirectOperationList.end(); it++)
         {
               FrameRedirectOperationMap.insert(FrameRedirectOperationMapPair(it->physicalLunId, *it));       
         }
 }

/*
PersistSwitchInitialSettings:: PersistSwitchInitialSettings(std::string serializedstr)
 {
         FrameRedirectOperationMap = unmarshal<map<string, FrameRedirectOperation> >( serializedstr );
 }
*/

PersistSwitchInitialSettings:: PersistSwitchInitialSettings(const PersistSwitchInitialSettings& swSettings)
 {
	 FrameRedirectOperationMap = swSettings.FrameRedirectOperationMap;
 }

 bool PersistSwitchInitialSettings::operator==( PersistSwitchInitialSettings & rhs) const
{
	//if(FrameRedirectOperationSet == rhs.FrameRedirectOperationSet)
	return true;
	//else return false;
}

PersistSwitchInitialSettings& PersistSwitchInitialSettings::operator=(const PersistSwitchInitialSettings& rhs)
{
	FrameRedirectOperationMap = rhs.FrameRedirectOperationMap;
	return *this;

}
PersistSwitchInitialSettings::operator FrameRedirectOperationSetting ()
{
       FrameRedirectOperationSetting frameRedirectOperationSetting;
       FrameRedirectOperationMapIterator it = FrameRedirectOperationMap.begin(); 
       for (; it != FrameRedirectOperationMap.end(); it++)
       {
                 frameRedirectOperationSetting.frameRedirectOperationList.push_back(it->second);
       }
       return frameRedirectOperationSetting; 
}
void PersistSwitchInitialSettings::Add(PersistSwitchInitialSettings &pswitchsettings)
{
       FrameRedirectOperationMapIterator it = pswitchsettings.FrameRedirectOperationMap.begin(); 
       for (; it != pswitchsettings.FrameRedirectOperationMap.end(); it++)
       {
               FrameRedirectOperationMap.insert(FrameRedirectOperationMapPair(it->first, it->second));       
       }
}

EventNotifyInfoSettings:: EventNotifyInfoSettings(const EventNotifyInfoSettings& rhs)
 {
	 ITLEventNotifyInfoSet = rhs.ITLEventNotifyInfoSet;
 }
 bool EventNotifyInfoSettings::operator==( EventNotifyInfoSettings & rhs) const
{
	return true;
}

EventNotifyInfoSettings& EventNotifyInfoSettings::operator=(const EventNotifyInfoSettings& rhs)
{
	ITLEventNotifyInfoSet = rhs.ITLEventNotifyInfoSet;
	return *this;
}
