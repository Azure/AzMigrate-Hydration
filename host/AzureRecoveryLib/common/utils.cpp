/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	utils.cpp

Description	:   Common utility functions

History		:   29-5-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include "utils.h"
#include "Trace.h"
#include "Process.h"
#include "inmsafecapis.h"
#include "../Status.h"
#include <ace/OS.h>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace AzureRecovery
{

    std::string GetLastErrorMsg()
    {
        return RecoveryStatus::Instance().GetLastErrorMessge();
    }

    void SetLastErrorMsg(const std::string& errorMsg)
    {
        RecoveryStatus::Instance().SetLastErrorMessage(errorMsg);
    }

    void SetLastErrorMsg(const char* format, ...)
    {
        ACE_ASSERT(NULL != format);

        if (0 == ACE_OS::strlen(format))
            return;

        va_list args;
        va_start(args, format);
        std::stringstream msg;

        std::vector<char> frmtMsgBuff(FORMAT_MSG_BUFF_SIZE, 0);
        VSNPRINTF(&frmtMsgBuff[0], frmtMsgBuff.size(), frmtMsgBuff.size() - 1, format, args);

        msg << &frmtMsgBuff[0];

        va_end(args);

        SetLastErrorMsg(msg.str());
    }

    void SetRecoveryErrorCode(int errorCode)
    {
        TRACE_INFO("Setting global error code: %d\n", errorCode);

        RecoveryStatus::Instance().SetStatusErrorCode(errorCode);
    }

    /*
    Method      : GenerateUuid

    Description : Generates a random uuid

    Parameters  : [out] uuid: filled with generated uuid in string format, otherwise empty

    Return      : returns true on success, otherwise false.

    */
    bool GenerateUuid(std::string& uuid)
    {
        TRACE_FUNC_BEGIN;

        bool bGenerated = true;

        try
        {
            boost::uuids::random_generator uuid_gen;

            boost::uuids::uuid new_uuid = uuid_gen();

            std::stringstream suuid;
            suuid << new_uuid;

            uuid = suuid.str();
        }
        catch (const std::exception& e)
        {
            TRACE_ERROR("Could not generate UUID. Exception: %s\n", e.what());
            bGenerated = false;
        }
        catch (...)
        {
            TRACE_ERROR("Could not generate UUID. Unknown exception\n");
            bGenerated = false;
        }

        TRACE_FUNC_END;
        return bGenerated;
    }

    /*
    Method: GetHydrationConfigValue

    Description : Get the config setting provided to hydration tool.

    Parameters : [in] hydrationConfigSettings: The config setting provided to the tool
                 [in] hydrationConfigKeyToCheck: The key for which value is to be checked.

    Return : Value of the key if Key is found, otherwise false.

     */
    std::string GetHydrationConfigValue(
        const std::string& hydrationConfigSettings,
        const std::string& hydrationConfigKeyToCheck)
    {
        TRACE_FUNC_BEGIN;

        if (boost::iequals(hydrationConfigSettings, HydrationConfig::EmptyConfig))
        {
            return HydrationConfig::FalseString;
        }

        TRACE_INFO("Hydration Config Settings are: %s\n", hydrationConfigSettings.c_str());

        std::vector<std::string> tokens;
        boost::split(tokens, hydrationConfigSettings, boost::is_any_of(";"), boost::token_compress_on);
        std::vector<std::string>::iterator tokenIter = tokens.begin();
        for (/*empty*/; tokenIter != tokens.end(); tokenIter++)
        {
            size_t pos = tokenIter->find_last_of(':');
            std::string configKey = tokenIter->substr(0, pos);
            std::string configValue = tokenIter->substr(pos + 1);
            if (boost::iequals(hydrationConfigKeyToCheck, configKey))
            {
                return configValue;
            }
        }

        TRACE_FUNC_END;
        return HydrationConfig::FalseString;
    }

    /// <summary>
    /// Returns the greater of the two release versions by comparing upto 5 minor releases.
    /// </summary>
    /// <param name="releaseAlpha">First release to compare.</param>
    /// <param name="releaseBeta">Second release to compare.</param>
    /// <returns></returns>
    std::string GetHigherReleaseVersion(std::string& relVerA, std::string& relVerB)
    {
        TRACE_FUNC_BEGIN;
        unsigned int releaseVerA[5], releaseVerB[5];

        //Fill 0 by default at all positions.
        std::fill_n(releaseVerA, 5, 0);
        std::fill_n(releaseVerB, 5, 0);

        (void)sscanf(relVerA.c_str(), "%u.%u.%u.%u.%u", &releaseVerA[0], &releaseVerA[1], &releaseVerA[2], &releaseVerA[3], &releaseVerA[4]);
        (void)sscanf(relVerB.c_str(), "%u.%u.%u.%u.%u", &releaseVerB[0], &releaseVerB[1], &releaseVerB[2], &releaseVerB[3], &releaseVerB[4]);
        
        for (int relItr = 0; relItr < 5; relItr++)
        {
            /* Example Set - 
              85.1.123 > 85.1.22
              85.1.23 < 90.1.22
              85.2.23 > 85.1.23
              85.1.0.78 > 85.1
              90.2.0.0.0 = 90.2
            */

            if (releaseVerA[relItr] > releaseVerB[relItr])
            {
                TRACE_FUNC_END;
                return relVerA;
            }
            else if (releaseVerA[relItr] < releaseVerB[relItr])
            {
                TRACE_FUNC_END;
                return relVerB;
            }
        }

        // Both release to be equal if can't be differentiated till 5 minor versions.
        TRACE_FUNC_END;
        return relVerA;
    }

    /*
    Method      : DirectoryExists

    Description : Verifies whether the directory_path is a valid directory.

    Parameters  : [in] directory_path: complete path of a directory.

    Return      : true if the directory_path is valid and is a directory, otherwise false.
    */
    bool DirectoryExists(const std::string& directory_path)
    {
        TRACE_FUNC_BEGIN;
        bool bExists = false;
        BOOST_ASSERT(!directory_path.empty());

        TRACE_INFO("Checking for directory - %s\n", directory_path.c_str());

        try
        {
            boost::filesystem::path fpath(directory_path);

            bExists = boost::filesystem::exists(fpath) && boost::filesystem::is_directory(fpath);
            if (bExists)
            {
                TRACE_INFO("%s Directory exists.\n", directory_path.c_str());
            }
        }
        catch (const std::exception& ex)
        {
            TRACE_ERROR("Checking for directory \"%s\" failed with exception %s\n", directory_path.c_str(), ex.what());
            bExists = false;
        }
        catch (...)
        {
            TRACE_ERROR("Could not verify file existence. Unknown exception\n");
            bExists = false;
        }

        TRACE_FUNC_END;
        return bExists;
    }

    /*
    Method      : RenameDirectory

    Description : Renames the directory present at directory_path to renamed_directory_name.

    Parameters  : [in] directory_path: complete path of the directory.
                  [in] new_directory_name: Targetted name of the directory

    Return      : true if the directory_path is valid and has been renamed, otherwise false.
    */
    bool RenameDirectory(
        const std::string& directory_path,
        const std::string& new_directory_path)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = true;

        namespace fs = boost::filesystem;

        try
        {
            BOOST_ASSERT(!directory_path.empty());
            BOOST_ASSERT(!new_directory_path.empty());
            fs::path oldDirPath(directory_path);
            fs::path newDirPath(new_directory_path);

            fs::rename(oldDirPath, newDirPath);
        }
        catch (fs::filesystem_error const & e)
        {
            TRACE_ERROR("Renaming directory failed with exception %s\n", e.what());
            bSuccess = false;
        }
        catch (const std::exception& ex)
        {
            TRACE_ERROR("Renaming directory failed with exception. %s\n", ex.what());
            bSuccess = false;
        }
        catch (...)
        {
            TRACE_ERROR("Renaming directory failed with exception.\n");
            bSuccess = false;
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : Copy_Directory_Recursively

    Description : Recursively copies a directory and all its content from source to destination path.

    Parameters  : [in] source_path: complete path of the source directory.
                  [in] dest_path: complete path of the destination directory under which source directory gets copied.

    Return      : true if the dest_path does not exist already and all the content gets copied, otherwise false.
    */
    bool CopyDirectoryRecursively(
        const std::string& source_path,
        const std::string& dest_path)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = true;

        namespace fs = boost::filesystem;

        try
        {
            BOOST_ASSERT(!source_path.empty());
            BOOST_ASSERT(!dest_path.empty());

            fs::path srcPath(source_path);
            fs::path destPath(dest_path);

            // Check whether the source and destination parameters are valid for directory copy.
            if (
                !fs::exists(srcPath) ||
                !fs::is_directory(srcPath)
                )
            {
                TRACE_ERROR("The source directory \"%s\" does not exist or is not a directory.\n", source_path.c_str());
                TRACE_FUNC_END;
                return false;
            }
            if (fs::exists(dest_path))
            {
                TRACE_ERROR("The destination directory \"%s\" already exists.\n", dest_path.c_str());
                TRACE_FUNC_END;
                return false;
            }
            // Create the destination directory
            if (!fs::create_directory(destPath))
            {
                TRACE_ERROR("The destination directory \"%s\" could not be created.\n", dest_path.c_str());
                TRACE_FUNC_END;
                return false;
            }

            // Iterate through the source directory
            for (
                fs::directory_iterator file(srcPath);
                file != fs::directory_iterator();
                ++file
                )
            {
                fs::path current(file->path());
                if (fs::is_directory(current))
                {
                    fs::path newDestPath = (dest_path / current.filename());
                    // Found a directory: Recursively copy the current directory.
                    if (!CopyDirectoryRecursively(
                        fs::canonical(current).string(),
                        newDestPath.string()))
                    {
                        return false;
                    }
                }
                else
                {
                    // Found a file: Copy the file.
                    fs::copy_file(
                        current,
                        dest_path / current.filename()
                    );
                }
            }
        }
        catch (fs::filesystem_error const & e)
        {
            TRACE_ERROR("Copying directory failed with exception %s\n", e.what());
            return false;
        }
        catch (const std::exception& ex)
        {
            TRACE_ERROR("Copying directory failed with exception. %s\n", ex.what());
            return false;
        }
        catch (...)
        {
            TRACE_ERROR("Copying directory failed with exception.\n");
            TRACE_FUNC_END;
            return false;
        }

        TRACE_FUNC_END;
        return true;
    }

    /*
    Method      : FileExists

    Description : Verifies does the file_path is a valid file path and the file exist.

    Parameters  : [in] file_path: complete path of a file.

    Return      : true if the file_path is valid and exist, otherwise false.
    */
    bool FileExists(const std::string& file_path)
    {
        TRACE_FUNC_BEGIN;
        bool bExist = false;
        BOOST_ASSERT(!file_path.empty());

        try
        {
            boost::filesystem::path fpath(file_path);

            bExist = boost::filesystem::exists(fpath);
        }
        catch (const std::exception& e)
        {
            TRACE_ERROR("Could not verify file existence. Exception: %s\n", e.what());
            bExist = false;
        }
        catch (...)
        {
            TRACE_ERROR("Could not verify file existence. Unknown exception\n");
            bExist = false;
        }

        TRACE_FUNC_END;
        return bExist;
    }

    /*
    Method      : VerifyFiles

    Description : Verifies does the file_path is a valid file path and the file exist.

    Parameters  : [in] mountpoint: Mountpoint or drive name.
                  [in] relative_paths: list of relative paths to verify.
                  [in] verify_all: if its true then all the relative_paths should exist to return true.
                                   if its false then any one of the relative_paths existance will return true.

    Return      : true if relative_paths are valid and exist, otherwise false.

    */
    bool VerifyFiles(const std::string &mountpoint,
        const std::vector<std::string> &relative_paths,
        bool verify_all)
    {
        TRACE_FUNC_BEGIN;
        bool bExist = false;

        std::string base_path = boost::algorithm::trim_right_copy_if(
            mountpoint,
            boost::is_any_of(ACE_DIRECTORY_SEPARATOR_STR));

        TRACE_INFO("Base path: %s.\n", mountpoint.c_str());

        BOOST_FOREACH(const std::string &relative_path, relative_paths)
        {
            std::stringstream file_path;
            file_path << base_path
                << ACE_DIRECTORY_SEPARATOR_STR
                << boost::algorithm::trim_left_copy_if(relative_path,
                    boost::is_any_of(ACE_DIRECTORY_SEPARATOR_STR));

            bExist = FileExists(file_path.str());

            TRACE("File %s %s.\n", file_path.str().c_str(),
                bExist ? "exist" : "does not exist");

            if ((verify_all && !bExist) || (!verify_all && bExist))
                break;
        }

        TRACE_FUNC_END;
        return bExist;
    }

    /*
    Method      : RenameFile

    Description : Renames the from_path to to_path.

    Parameters  : [in] from_path: complete path of a file.
                  [in] to_path: complete path of a file.

    Return      : true on successful rename, otherwise false.

    */
    bool RenameFile(const std::string& from_path, const std::string& to_path)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = true;

        BOOST_ASSERT(!from_path.empty());
        BOOST_ASSERT(!to_path.empty());
        try
        {
            boost::filesystem::path from(from_path), to(to_path);
            boost::filesystem::rename(from, to);
        }
        catch (const std::exception& e)
        {
            TRACE_ERROR("Could not rename file %s to %s\n. Exception: %s\n",
                from_path.c_str(),
                to_path.c_str(),
                e.what()
            );
            bSuccess = false;
        }
        catch (...)
        {
            TRACE_ERROR("Could not rename file %s to %s\n. Unknown exception\n",
                from_path.c_str(),
                to_path.c_str()
            );
            bSuccess = false;
        }

        TRACE_FUNC_END;
        return bSuccess;
    }


    /*
    Method      : CopyXmlFile

    Description : Copies the xml file to destination.

    Parameters  : [in] sourceXml: complete path of source xml file.
                  [in] destXml: destination xml file path.
                  [in, optional] bForce: If true then the copy will happen force
                   by deleting existing destination xml file.

    Return      : true if the file_path is valied and exist, otherwise false.

    */
    bool CopyXmlFile(const std::string& sourceXml, const std::string& destXml, bool bForce)
    {
        TRACE_FUNC_BEGIN;
        bool bExist = true;
        BOOST_ASSERT(!sourceXml.empty());
        BOOST_ASSERT(!destXml.empty());

        try
        {
            //
            // Validate the xml file content by reading it.
            //
            boost::property_tree::ptree xmlPt;
            boost::property_tree::xml_parser::read_xml(sourceXml, xmlPt);

            //
            // Verify if destination file already exist, and delete it if bForce=true
            //
            boost::filesystem::path destpath(destXml);
            if (bForce && boost::filesystem::exists(destpath))
            {
                TRACE_WARNING("Destination file %s already exist. Deleting it.\n", destXml.c_str());
                boost::filesystem::remove(destpath);
            }

            //
            // Write the content to destination xml file
            //
            boost::property_tree::xml_writer_settings<std::string> writer_settings('\t', 1);
            boost::property_tree::xml_parser::write_xml(destXml, xmlPt, std::locale(), writer_settings);
        }
        catch (const std::exception& e)
        {
            TRACE_ERROR("Could not verify file existence. Exception: %s\n", e.what());
            bExist = false;
        }
        catch (...)
        {
            TRACE_ERROR("Could not verify file existence. Unknown exception\n");
            bExist = false;
        }

        TRACE_FUNC_END;
        return bExist;
    }

    /*
    Method      : CopyAndReadTextFile

    Description : Copies file to destination file and then reads its content to fill the fileData stream. If destFile is
                  not specified then it will generate a randome file name and deletes it after reading the content. This
                  function is useful when reading a file which is beeing updated by other other process.

    Parameters  : [in] sourceFile: source text file.
                  [Out] fileData: out stream filled with file content.
                  [in, optional] destFile: destination file name, its optional, if not specified then a random file will
                       be created.

    Return      : true if the content was read successfully, otherwise false.

    */
    bool CopyAndReadTextFile(const std::string& sourceFile,
        std::stringstream& fileData,
        const std::string& destFile)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = true;
        BOOST_ASSERT(!sourceFile.empty());

        try
        {
            // Generate random destination file name if caller don't specify.
            std::string destinationfile;
            if (destFile.empty())
            {
                GenerateUuid(destinationfile);
                destinationfile += "_tempCopy.log";
            }
            else
            {
                destinationfile = destFile;
            }

            // Copy the file
            boost::filesystem::path sourcepath(sourceFile), destpath(destinationfile);
            boost::filesystem::copy_file(sourcepath, destpath, boost::filesystem::copy_option::overwrite_if_exists);

            // Read the content from copied file
            std::ifstream readStream(destpath.filename().c_str());
            if (readStream)
            {
                fileData << readStream.rdbuf();
                readStream.close();
            }
            else
            {
                TRACE_ERROR("Bad destination file stream.\n");
                bSuccess = false;
            }


            // If destination file is generated randomely then delete it.
            if (destFile.empty())
                boost::filesystem::remove(destpath);
        }
        catch (const std::exception& e)
        {
            TRACE_ERROR("Could not read file content. Exception: %s\n", e.what());
            bSuccess = false;
        }
        catch (...)
        {
            TRACE_ERROR("Could not read file content. Unknown exception\n");
            bSuccess = false;
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : GetCurrentDir

    Description : retrieves the current working directory of the process.

    Parameters  : None

    Return      : current directory on success, otherwise empty string

    */
    std::string GetCurrentDir()
    {
        std::string curr_dir;
        TRACE_FUNC_BEGIN;

        try
        {
            boost::filesystem::path curr_dir_path(boost::filesystem::current_path());

            curr_dir = curr_dir_path.string();
        }
        catch (const std::exception& e)
        {
            TRACE_ERROR("Could not verify file existence. Exception: %s\n", e.what());
        }
        catch (...)
        {
            TRACE_ERROR("Could not verify file existence. Unknown exception\n");
        }

        TRACE_FUNC_END;
        return curr_dir;
    }


    /*
    Method      : CreateDirIfNotExist

    Description : Creates the directory if not already exist.

    Parameters  : [int] dir_path: directory path.

    Return      : true if dir already exists or created successfully,
                  otherwise false.

    */
    bool CreateDirIfNotExist(const std::string &dir_path)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = true;
        BOOST_ASSERT(!dir_path.empty());

        try
        {
            boost::filesystem::path dir(dir_path);
            if (!boost::filesystem::exists(dir))
            {
                bSuccess = boost::filesystem::create_directory(dir);
            }
        }
        catch (const std::exception& e)
        {
            TRACE_ERROR("Could not create directory. Exception: %s\n", e.what());
            bSuccess = false;
        }
        catch (...)
        {
            TRACE_ERROR("Could not create directory. Unknown exception\n");
            bSuccess = false;
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : GenerateUniqueMntDirectory

    Description : Generates a unique directory name under system root path and then creates the directory.

    Parameters  : [out] mntDir: generated uniuque directory path.

    Return      : 0 -> success
                  1 -> Failure generating unique mount directory
                  2 -> Unique directory name was generated but could not create that directory.

    */
    DWORD GenerateUniqueMntDirectory(std::string& mntDir)
    {
        TRACE_FUNC_BEGIN;
        namespace fs = boost::filesystem;
        DWORD dwRet = 0;
        size_t max_retry = 100;

        try
        {
            for (; --max_retry > 0;)
            {
                std::string strUuid;
                if (!GenerateUuid(strUuid))
                {
                    TRACE_ERROR("Failed to generate uuid for mount point.");
                    dwRet = 1;
                    break;
                }

                boost::filesystem::path mntPath(
                    std::string(SOURCE_OS_MOUNT_POINT_ROOT)
                    + MOUNT_PATH_PREFIX
                    + strUuid
                );

                if (!fs::exists(mntPath))
                {
                    TRACE_INFO("Creating mount path is: %s\n", mntPath.string().c_str());

                    if (fs::create_directory(mntPath))
                    {
                        TRACE_INFO("Unique mount path is: %s\n", mntPath.string().c_str());
                        mntDir = mntPath.string();
                    }
                    else
                    {
                        TRACE_ERROR("Mount directory creation failed\n");
                        dwRet = 2;
                    }

                    break;
                }
            }
        }
        catch (const std::exception& e)
        {
            TRACE_ERROR("Could not generate unique mount path. Exception: %s\n", e.what());
            dwRet = 1;
        }
        catch (...)
        {
            TRACE_ERROR("Could not generate unique mount path. Unknown exception\n");
            dwRet = 1;
        }

        TRACE_FUNC_END;
        return dwRet;
    }

    /*
    Method      : RunCommand

    Description : Runs the command, waits MAX_CHILD_PROC_WAIT_TIME sec for child process then returns
                  the child process exit code. If the process did not finish within the timelimit then
                  the child process will be terminated and return code is set to 1.

    Parameters  : [in] cmd: Command or script path
                  [in] args: process arguments
                  [out] outStream: Filled with child process stdout & stderr

    Return      : child process exit code,
                  1 if any failure happened in forking process, timeout and collecting stdout&stderr.

    */
    DWORD RunCommand(const std::string& cmd, const std::string& args, std::stringstream& outStream)
    {
        TRACE_FUNC_BEGIN;
        DWORD dwRet = 0;

        do
        {
            CProcess CmdProc(cmd, args);

            TRACE("Executing the command: %s %s\n",
                cmd.c_str(),
                args.c_str());


            if (!CmdProc.Run())
            {
                TRACE_ERROR("Error executing the command: %s %s\n",
                    cmd.c_str(),
                    args.c_str());

                dwRet = 1;
                break;
            }

            if (CmdProc.Wait(MAX_CHILD_PROC_WAIT_TIME) <= 0)
            {
                TRACE_ERROR("Time-out or Wait error has occurred for the process: %s %s\n",
                    cmd.c_str(),
                    args.c_str());

                //
                // Forcefully killing the child process.
                //
                CmdProc.Terminate();

                //
                // Wait for sometime before collecting logs as child process has to terminate.
                //
                ACE_OS::sleep(3);
                CmdProc.GetConsoleStream(outStream);

                dwRet = 1;
                break;
            }

            dwRet = CmdProc.GetExitCode();
            if (0 != dwRet)
            {
                TRACE_ERROR("The command [%s %s] failed with error code: %d\n",
                    cmd.c_str(),
                    args.c_str(),
                    dwRet);
            }

            if (!CmdProc.GetConsoleStream(outStream))
            {
                TRACE_ERROR("Could not collect command STDOUT.\n");

                dwRet = 1;
            }
        } while (false);

        TRACE_FUNC_END;
        return dwRet;
    }

    /*
    Method      : RunCommand

    Description :

    Parameters  : [in] cmd: Command or script path
                  [in] args: process arguments

    Return      : child process exit code,
                  1 if any failure happened in forking process, timeout and collecting stdout&stderr.

    */
    DWORD RunCommand(const std::string& cmd, const std::string& args)
    {
        std::stringstream placeholder;

        return RunCommand(cmd, args, placeholder);
    }

    /*
    Method      : SanitizeDiskId_Copy, SanitizeDiskId

    Description : 1. Replace '/' with '-'
                  2. Replace '\' and '#' with '_'

    Parameters  : [in] diskId: Original string to be sanitized

    Return      : sanitized version of 'diskId'

    */
    std::string SanitizeDiskId_Copy(const std::string& diskId)
    {
        std::string strSanitized = diskId;
        // 1. Replace '/' with '-'
        boost::replace_all(strSanitized, "/", "-");

        // 2. Replace '\' and '#' with '_'
        boost::replace_all(strSanitized, "\\", "_");
        boost::replace_all(strSanitized, "#", "_");

        return strSanitized;
    }
    void SanitizeDiskId(std::string& diskId)
    {
        diskId = SanitizeDiskId_Copy(diskId);
    }

}
