//---------------------------------------------------------------
//  <copyright file="hostrecoverymanager.cpp" company="Microsoft">
//      Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
//
//  Description: Linux implementation for HostRecoveryManager
//
//  History:    15-Aug-2016     veshivan    Created
//
//----------------------------------------------------------------

#include <string>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include "../hostrecoverymanager.h"
#include "portablehelpersmajor.h"
#include "localconfigurator.h"

#include <map>

using namespace std;


void HostRecoveryManager::ResetReplicationState()
{
	//
	// Resets the replication state on recovered VM by making following changes:
	//    1. Reset the filter driver state & clear the bitmap files if exist
	//    2. Clear the cache settings
	//

	DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

	//
	// Call drvutil to stop filtering & delete bitmap files
	//
	LocalConfigurator lConfig;
	std::string strInmDmitCmd = lConfig.getInstallPath();
	BOOST_ASSERT(!strInmDmitCmd.empty());
	boost::trim(strInmDmitCmd);
	if (!boost::ends_with(strInmDmitCmd, ACE_DIRECTORY_SEPARATOR_STR_A))
		strInmDmitCmd += ACE_DIRECTORY_SEPARATOR_STR_A;
	strInmDmitCmd += "/bin/inm_dmit";


    std::string listVolumesCmd = strInmDmitCmd + " --get_protected_volume_list";
	DebugPrintf(SV_LOG_ERROR, "Running the command: %s\n", 
		listVolumesCmd.c_str());

	int exitCode = 0;
	std::string strCmdOut, strCmdError;
	if (!RunInmCommand(listVolumesCmd, strCmdOut, strCmdError, exitCode))
	{
		DebugPrintf(SV_LOG_ERROR, "Command execution error.\n");
		DebugPrintf(SV_LOG_ERROR, "Exit code: %d\n", exitCode);
		DebugPrintf(SV_LOG_ERROR, "-----stderror----\n%s\n", 
			strCmdError.c_str());

        THROW_HOST_REC_EXCEPTION(
            "Replication state cleanup failed. Manual intervention is required."
            );
	}
	DebugPrintf(SV_LOG_ERROR, "-----stdout----\n%s\n", strCmdOut.c_str());

    std::istringstream ssOutput(strCmdOut);
    std::string device;
    while(std::getline(ssOutput, device))
    {
        boost::trim(device);
        if (!boost::starts_with(device, "/dev/"))
            continue;

        std::string stopFltCmd = strInmDmitCmd + " --op=stop_flt --src_vol=";
            stopFltCmd += device;

        DebugPrintf(SV_LOG_ERROR, "Running the command: %s\n", 
            stopFltCmd.c_str());

        if (!RunInmCommand(stopFltCmd, strCmdOut, strCmdError, exitCode))
        {
            DebugPrintf(SV_LOG_ERROR, "Command execution error.\n");
            DebugPrintf(SV_LOG_ERROR, "Exit code: %d\n", exitCode);
            DebugPrintf(SV_LOG_ERROR, "-----stderror----\n%s\n", 
                strCmdError.c_str());

            THROW_HOST_REC_EXCEPTION(
                "Replication state cleanup failed. Manual intervention is required."
                );
        }
        DebugPrintf(SV_LOG_ERROR, "-----stdout----\n%s\n", strCmdOut.c_str());
    }

	//
	// Clear chache settings files.
	//
    std::list<std::string> lstFilesToRemove;
    std::string configFilesPath;
    if (LocalConfigurator::getConfigDirname(configFilesPath))
    {
        BOOST_ASSERT(!configFilesPath.empty());
        boost::trim(configFilesPath);
        if (!boost::ends_with(configFilesPath, ACE_DIRECTORY_SEPARATOR_STR_A))
            configFilesPath += ACE_DIRECTORY_SEPARATOR_STR_A;

        std::string cleanupFileList = lConfig.getRecoveryCleanupFileList();
        boost::char_separator<char> delm(",");
        boost::tokenizer < boost::char_separator<char> > strtokens(cleanupFileList, delm);
        for (boost::tokenizer< boost::char_separator<char> > ::iterator it = strtokens.begin(); it != strtokens.end(); ++it)
        {
            /// remove leading and trailing white space if any
            std::string filename = *it;
            boost::trim(filename);
            lstFilesToRemove.push_back(configFilesPath + filename);
        }
    }
	else
	{
		THROW_HOST_REC_EXCEPTION(
			"Could not get the config directory path. Cache files cleanup won't happen"
			);
	}

	std::list<std::string>::const_iterator iterFile = lstFilesToRemove.begin();
	for (; iterFile != lstFilesToRemove.end(); iterFile++)
	{
		boost::filesystem::path cache_file(*iterFile);
		if (boost::filesystem::exists(cache_file))
		{
			DebugPrintf(SV_LOG_INFO, "Removing the file %s\n", iterFile->c_str());
			boost::filesystem::remove(cache_file);
		}
		else
		{
			DebugPrintf(SV_LOG_INFO, "File %s does not exist.\n", iterFile->c_str());
		}
	}

	DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}

void HostRecoveryManager::DisableEnableVMWareTools(bool bEnable)
{
    // TODO
}

void HostRecoveryManager::DisableEnablePlatformServices(std::map<std::string, InmServiceStartType> platformServices)
{
    // TODO
}
