//---------------------------------------------------------------
//  <copyright file="azurevmrecoverymanager.h" company="Microsoft">
//      Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
//
//  Description: AzureVmRecoveryManager header file
//
//----------------------------------------------------------------

#ifndef AZUREVMRECOVERYMANAGER__H
#define AZUREVMRECOVERYMANAGER__H

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <sstream>
#include <stdexcept>
#include "localconfigurator.h"
#include "hostrecoverymanager.h"

//
// Class : AzureVmRecoveryManager
//
// Description: a derived class from HostRecoveryManager
// to handle VM recovery in Azure to Azure replication.
//
class AzureVmRecoveryManager : public HostRecoveryManager
{
public:

    //
    // checks if a recovery is required
    //
    // Returns true if the recovery is true, otherwise false.
    static bool IsRecoveryRequired()
    {
        LocalConfigurator localConfig;

        std::string rcmSettingsPath = localConfig.getRcmSettingsPath();
        if (!boost::filesystem::exists(rcmSettingsPath))
        {
            THROW_HOST_REC_EXCEPTION(
                "Could not verify if recovery is required as RCM settings file is not found."
                );
        }

        // check if the UUID of the host is same as configured
        // a recovery is required if different
        namespace bpt = boost::property_tree;
        bpt::ptree pt;
        bpt::ini_parser::read_ini(rcmSettingsPath, pt);

        std::string configuredBiosId = pt.get("rcm.BiosId", "");
        std::string systemBiosId = GetSystemUUID();

        return (!boost::iequals(configuredBiosId, systemBiosId));
    }

};

#endif // ~ AZUREVMRECOVERYMANAGER__H