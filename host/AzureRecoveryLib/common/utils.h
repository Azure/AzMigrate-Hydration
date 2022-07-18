/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	utils.h

Description	:   Common utility functions

History		:   29-5-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_RECOVERY_COMMON_UTILS_H
#define AZURE_RECOVERY_COMMON_UTILS_H

#include <string>
#include <list>
#include <vector>
#include <ace/Process_Manager.h>

#ifndef WIN32
typedef unsigned int  UINT;
typedef unsigned long DWORD;
const char SOURCE_OS_MOUNT_POINT_ROOT[] = "/mnt/";
#else
const char SOURCE_OS_MOUNT_POINT_ROOT[] = "C:\\";
#endif

const char MOUNT_PATH_PREFIX[] = "azure_rcvr_";

const int MAX_CHILD_PROC_WAIT_TIME = 600; // 10mins
const int MAX_RETRY_ATTEMPTS = 5;



namespace HydrationConfig
{
    const char EmptyConfig[] = "emptyconfig";
    const std::string FalseString = "false";
    const std::string TrueString = "true";
    const char EnableWindowsGAInstallation[] = "EnableWindowsGAInstallation";
    const char EnableLinuxGAInstallation[] = "EnableLinuxGAInstallation";
    const char EnableWindowsGAInstallationDR[] = "EnableWindowsGAInstallationDR";
}

namespace AzureRecovery
{
    bool GenerateUuid(std::string& uuid);

    bool FileExists(const std::string& path);

    bool DirectoryExists(const std::string& path);

    bool RenameDirectory(
        const std::string& directory_path,
        const std::string& new_directory_path);

    bool CopyDirectoryRecursively(
        const std::string& source_path,
        const std::string& dest_path);

    bool CopyXmlFile(const std::string& sourceXml, const std::string& destXml, bool bForce = true);

    bool CopyAndReadTextFile(const std::string& sourceFile,
        std::stringstream& fileData,
        const std::string& destFile = "");

    bool RenameFile(const std::string& from_path,
        const std::string& to_path);

    std::string GetCurrentDir();

    bool CreateDirIfNotExist(const std::string &dir_path);

    DWORD GenerateUniqueMntDirectory(std::string& mntDir);

    DWORD RunCommand(const std::string& cmd,
        const std::string& args,
        std::stringstream& outStream);

    DWORD RunCommand(const std::string& cmd,
        const std::string& args);

    std::string GetLastErrorMsg();

    void SetLastErrorMsg(const char* format, ...);

    void SetLastErrorMsg(const std::string& error);

    void SetRecoveryErrorCode(int errorCode);

    std::string SanitizeDiskId_Copy(const std::string& diskId);

    void SanitizeDiskId(std::string& diskId);

    bool VerifyFiles(const std::string &mountpoint,
        const std::vector<std::string> &relative_paths,
        bool verify_all = true);

    std::string GetHydrationConfigValue(
        const std::string& hydrationConfigSettings,
        const std::string& hydrationConfigKeyToCheck);

    std::string GetHigherReleaseVersion(std::string& relVerA, std::string& relVerB);
}

#endif // ~AZURE_RECOVERY_COMMON_UTILS_H
