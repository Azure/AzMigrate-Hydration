/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: AzureRecovery.h

Description	: Declations for recovery operations entry point routines such as
              Initializing configuration Objects,
              Status update,
              Execution Log upload,
              Starting recovery steps.

History		: 1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef _AZURE_RECOVERY_H_
#define _AZURE_RECOVERY_H_

#include "Status.h"

int GlobalInit();

void GlobalUnInit();

int InitStatusConfig(const std::string& recoveryConfigFile);

int InitRecoveryConfig(const std::string& recoveryConfigFile,
    const std::string& hostinfoFile,
    const std::string& workingDir,
    const std::string& hydrationConfigSettings);

int InitMigrationConfig(const std::string& recoveryConfigFile,
    const std::string& workingDir,
    const std::string& hydrationConfigSettings);

int StartRecovery();

int StartMigration();

int StartGenConversion();

int UpdateStatusToBlob(const RecoveryUpdate& update);

int UploadExecutionLog(const std::string& logFile);

int UploadCurrentTraceLog(const std::string& curr_log);

RecoveryUpdate GetCurrentStatus();

std::string GetWorkingDir();

std::string GetHydrationConfigSettings();

bool CopyHostInfoXmlFile(const std::string& hostinfo_xmlfile,
    const std::string& workingDir);

#endif // ~_AZURE_RECOVERY_H_