#include "portablehelpers.h"
#include "portablehelperscommonheaders.h"
#include "portablehelpersmajor.h"
#include "portablehelpersminor.h"
#include "inmdefines.h"
#include "configwrapper.h"
#include "inmsafecapis.h"
#include "inmsafeint.h"
#include "inmageex.h"

#include <mntent.h>
#include <fstab.h>
#include <linux/fs.h>
#include <sys/utsname.h>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include<boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include "file.h"

#define THRSHOLD_NUM_FIELDSINFSTAB 4
#define FLAG_FIELD_IN_FSTAB 3

bool ShouldCollectNicInfo(const int &sockfd, struct ifreq *pifr, const char *ipaddr);
void GetNetIfParameters(const std::string &ifrName, std::string& hardwareAddress, std::string &netmask);
bool FillNicInfos(const int &sockfd, struct ifconf *pifc, size_t sizeofifreq, NicInfos_t & nicInfos);
void InsertNicInfo(NicInfos_t &nicInfos, const std::string &name, const std::string &hardwareaddress, const std::string &ipaddress, 
                   const std::string &netmask, const E_INM_TRISTATE &isdhcpenabled);
void UpdateNicInfoAttr(const std::string &attr, const std::string &value, NicInfos_t &nicinfos);
void GetNicNamesFromProc(std::set<std::string> &nicnamesfromproc);
bool getDHCPValue( std::string interface, std::string& value );


/* LINUX SPECIFIC */
SVERROR GetVolSize(ACE_HANDLE handle, unsigned long long *pSize)
{
    unsigned long nsec;
    unsigned long long sz = 0ULL;

    if (handle == ACE_INVALID_HANDLE || pSize == NULL)
    {
        return EINVAL;
    }
    if (ioctl(handle, BLKGETSIZE, &nsec))
    {
        nsec = 0;
    }
    if (ioctl(handle, BLKGETSIZE64, &sz))
    {
        sz = 0;
    }
    if (sz == 0 || sz == nsec)
    {
        sz = ((unsigned long long) nsec) * 512;
    }
    *pSize = sz;
    return SVS_OK;
}


/* LINUX SPECIFIC : BUT CAN BE MADE COMMON TO UNIX */
SVERROR GetFsVolSize(const char *volName, unsigned long long *pfsSize)
{
    if (pfsSize == NULL) {
        DebugPrintf(SV_LOG_ERROR, "GetFsVolSize() failed, err = EINVAL\n");
        return SVE_FAIL;
    }

    SVERROR res = SVS_OK;
    int fd;
    if ((fd = open(volName, O_RDONLY))< 0)
    {
        res = SVE_FAIL;

    }
    else if (GetVolSize(fd, pfsSize).failed())
    {
        res = SVE_FAIL;
    }
    close(fd);

    return res;
}


/* LINUX SPECIFIC */
SVERROR GetFsSectorSize(const char *volName, unsigned int *psectorSize)
{
    if (psectorSize == NULL) {
        DebugPrintf(SV_LOG_ERROR, "GetFsVolSize() failed, err = EINVAL\n");
        return SVE_FAIL;
    }
    unsigned int sector_size = 512;
    int ssize;

    int fd;

    if ((fd = open(volName, O_RDONLY))< 0)
        return SVE_FAIL;

    if(ioctl(fd, BLKSSZGET, &ssize) == 0)
        sector_size = (unsigned int)ssize;

    close(fd);

    *psectorSize = sector_size;

    return SVS_OK;
}



/* LINUX SPECIFIC */

SVERROR GetHardwareSectorSize(ACE_HANDLE handleVolume, unsigned int *pSectSize)
{
    SVERROR sve = SVS_OK;
    unsigned int sector_size = 0;



    do
    {
        if (pSectSize == NULL || handleVolume == ACE_INVALID_HANDLE)
        {
            sve = EINVAL;
            DebugPrintf( SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_ERROR, "FAILED GetHardwareSectorSize, err = EINVAL\n");
            break;
        }

        int ssize;

        if(ioctl(handleVolume, BLKSSZGET, &ssize) == 0)
            sector_size = (unsigned int)ssize;    
        else
            sector_size = 512;

    } while (FALSE);

    if (!sve.failed())
        *pSectSize = sector_size;

    return sve;
}


/* LINUX SPECIFIC */

/*
* FUNTION NAME : removeFSEntry
*
* DESCRIPTION : 
*
*   This is a lowlevel function of DeleteFSEnt() which takes the fstab entries and
*   the device name on which the operation is performed, deletes the matching 
*   readonly or default entries for this device, comments the maching entries which
*   contain other flags like noexec,nosuid. It leaves the other entries as they are
*
* ALGORITHM : 
*
*   for each entry fsent in fsents
*       if the device name or label matches
*           if the flags contain 'ro' OR only 'defaults' OR only 'rw'
*               skip this entry
*           else
*               comment this entry
*       else write the entry as it is
*
* INPUT PARAMETERS :  
*   
*   device  : name of the device on which action is performed
*   label   : lable of the device on which action is performed
*   fsents  : contains all the entries in the fstab file
*
* OUTPUT PARAMETERS : 
*
*   fsents  : contains the modified entries after the operation is performed
*
* RETURN VALUE : true if succeeds false otherwise.
*/
bool removeFSEntry(const std::string& device, const std::string& label, std::vector<std::string>& fsents)
{
	bool rv=true;
	std::string realDevice = device;
	GetDeviceNameFromSymLink(realDevice);

    std::vector<std::string>::iterator fsent_iter = fsents.begin();
    std::vector<std::string>::iterator fsent_end = fsents.end();
	std::vector<std::string> tempFSEnts;

    for(; fsent_iter != fsent_end; ++fsent_iter)
    {	
        std::string fsent = *fsent_iter;

		svector_t fields;

		Tokenize(fsent, fields, " \t\n");
		if(fields.size()<THRSHOLD_NUM_FIELDSINFSTAB)
		{
			tempFSEnts.push_back(fsent);
			continue;
		}
		std::string fstab_dev = fields[0];
		std::string fstab_flags = fields[FLAG_FIELD_IN_FSTAB];
		svector_t flags = split(fstab_flags, "," );

        if(fstab_dev.find("LABEL=",0) != std::string::npos)
        {
            if(!label.empty() && (label == fstab_dev))
            {//if the lable matches this fstab entry
				if( (find(flags.begin(), flags.end(), "ro") != flags.end()) ||
					((find(flags.begin(), flags.end(), "rw") != flags.end()) && (flags.size() == 1)) ||
					((find(flags.begin(), flags.end(), "defaults") != flags.end()) && (flags.size() == 1)) )
					;//do not include this entry. equivalent to deleting.
				else
				{
					std::string tempFSEnt = "#" + *fsent_iter;
					tempFSEnts.push_back(tempFSEnt);
				}
            }
			else
			{//if the lable doesnt match this fstab entry we write it as it is.
				tempFSEnts.push_back(fsent);
			}
        }
		else if(GetDeviceNameFromSymLink(fstab_dev) && (fstab_dev == realDevice))
        {//if the device matches this fstab entry
			if( (find(flags.begin(), flags.end(), "ro") != flags.end()) ||
				((find(flags.begin(), flags.end(), "rw") != flags.end()) && (flags.size() == 1)) ||
				((find(flags.begin(), flags.end(), "defaults") != flags.end()) && (flags.size() == 1)) )
				;//do not include this entry. equivalent to deleting.
			else
			{
				std::string tempFSEnt = "#" + *fsent_iter;
				tempFSEnts.push_back(tempFSEnt);
			}
        }
		else
		{//if the entry does not match the device or lable we are looking write it as it is.
			tempFSEnts.push_back(fsent);
		}
    } // for loop

	fsents.assign(tempFSEnts.begin(),tempFSEnts.end());

	return rv;
}


/* LINUX SPECIFIC */

/*
* FUNTION NAME : addRWFSEntry
*
* DESCRIPTION : 
*
*   This is a lowlevel function for AddFSEnt() which takes the fstab entries and the
*   device name on which the operation is performed and uncomments the maching commented
*   entry OR adds a new one
*
* ALGORITHM : 
*
*   call removeFSEntry for this device. //This cleans the old entries which may not be needed
*   for each entry fsent in fsents
*       if this is a commented entry
*           if the device/label maches
*               if the filesystem type maches
*                   uncomment this entry
*               else
*                   delete this entry //as the file system changed we cant use this
*           else
*               write the entry as it is
*       else
*           write the entry as it is
*   //end of for
*   if we did not uncomment any existing entry ans a new one
*
* INPUT PARAMETERS :  
*   
*   device  : name of the device on which action is performed
*   label   : lable of the device on which action is performed
*   mountpoint: mountpoint for the device being mounted
*   type    : filesystem type contained in the device being mounted
*   fsents  : contains all the entries in the fstab file
*
* OUTPUT PARAMETERS : 
*
*   fsents  : contains the modified entries after the operation is performed
*
* RETURN VALUE : true if succeeds false otherwise.
*/
bool addRWFSEntry(const std::string& device, const std::string& label, const std::string& mountpoint, 
                  const std::string& type,
				  std::vector<std::string>& fsents)
{
	bool rv=true;

	do
	{
		if(!removeFSEntry(device, label, fsents))
		{
			rv =false;
			break;
		}

		std::string realDevice = device;
		GetDeviceNameFromSymLink(realDevice);

		std::vector<std::string>::iterator fsent_iter = fsents.begin();
		std::vector<std::string>::iterator fsent_end = fsents.end();
		std::vector<std::string> tempFSEnts;
		bool addNewEntry = true;
		for(; fsent_iter != fsent_end; ++fsent_iter)
		{	
			std::string fsent = *fsent_iter;

			svector_t fields;
			Tokenize(fsent, fields, " \t\n");
			if(fields.size() <6)
			{
				tempFSEnts.push_back(fsent);
				continue;
			}

			// case: commented entry
			std::string field1 = fields[0];
			if(field1[0] == '#' )
			{
				std::string fstab_dev(fields[0].begin()+1, fields[0].end());
				std::string fstab_mnt = fields[1];
				std::string fstab_type = fields[2];
				std::string fstab_flags = fields[3];
				std::string fstab_dump = fields[4];
				std::string fstab_fsck = fields[5];

				// non Label entry
				if(fstab_dev.find("LABEL=",0) == std::string::npos)
				{
					// case: special device entries like none or swap
					if(!GetDeviceNameFromSymLink(fstab_dev))
					{
						tempFSEnts.push_back(fsent);
						continue;
					}

					// case: non matching entry
					if(fstab_dev != realDevice)
					{
						tempFSEnts.push_back(fsent);
						continue;
					}
				}
				else
				{
					// case: label entry, non matching lavel
					if(!label.empty() && label != fstab_dev) 
					{
						tempFSEnts.push_back(fsent);
						continue;
					}
				}

				// case: commented entry matching device and fs
				if(type == fstab_type)
				{
					if(mountpoint == "" || mountpoint == fstab_mnt)
					{
						//mounting rw with no mountpoint specified or mounting at previous location; uncomment 
						std::string uncommentedfsent(fsent.begin()+1, fsent.end());
						tempFSEnts.push_back(uncommentedfsent);
					}
					else
					{
						//mounting rw with new mountpoint
						std::string uncommentedfsent = fstab_dev + "\t\t" + mountpoint + "\t\t" + fstab_type + "\t" +
							fstab_flags + "\t\t" + fstab_dump + " " + fstab_fsck + "\n";
						tempFSEnts.push_back(uncommentedfsent);
					}
					//Dont add new entry
					addNewEntry = false;
				}
				else // case: commented entry matching device, fs changed
				{
					;//do not push this entry as there is filesystem doesnt match. equivalent to delete.
				}
			}
			else // case: uncommented entries
			{
				//write all uncommented entries as they are.
				tempFSEnts.push_back(fsent);
			}
		}

		if(fsent_iter != fsent_end)
		{
			rv = false;
			break;
		}

		if(!rv)
			break;

		// if there were no matching commented entries for this in the fstab, create new one
		if(addNewEntry)
		{
			std::string newFSEntry;

			if(!label.empty())
				newFSEntry=label;
			else
				newFSEntry=device;
			newFSEntry+="\t\t" + mountpoint + "\t\t" + type + "\t" + "rw " + "\t\t" + "0" + " " + "0" + "\n";

			tempFSEnts.push_back(newFSEntry);
		}

		fsents.assign(tempFSEnts.begin(),tempFSEnts.end());

	}while(false);
	return rv;
}



/* LINUX SPECIFIC */

/*
* FUNTION NAME : addROFSEntry
*
* DESCRIPTION : 
*
*   This is a lowlevel function of AddFSEnt() which takes the fstab entries and
*   the device name on which the operation is performed and simply adds a new
*   readonly entry
*
* ALGORITHM : 
*
*   call removeFSEntry for this device. //This cleans the old entries which may
*   not be needed append a new readonly entry to the existing fsents
*
* INPUT PARAMETERS :  
*   
*   device  : name of the device on which action is performed
*   label   : lable of the device on which action is performed
*   mountpoint: mountpoint for the device being mounted
*   type    : filesystem type contained in the device being mounted
*   fsents  : contains all the entries in the fstab file
*
* OUTPUT PARAMETERS : 
*
*   fsents  : contains the modified entries after the operation is performed
*
* RETURN VALUE : true if succeeds false otherwise.
*/
bool addROFSEntry(const std::string& device, const std::string& label, const std::string& mountpoint, 
                  const std::string& type,
				  std::vector<std::string>& fsents)
{
	bool rv=true;

	do
	{
		if(!removeFSEntry(device, label, fsents))
		{
			rv =false;
			break;
		}

		std::string newFSEntry;
		if(!label.empty())
			newFSEntry=label;
		else
			newFSEntry=device;
		newFSEntry+="\t\t" + mountpoint + "\t\t" + type + "\t" + "ro " + "\t\t" + "0" + " " + "0" + "\n";

		fsents.push_back(newFSEntry);
	}while(false);
	return rv;
}


/* LINUX SPECIFIC */

bool MountDevice(const std::string& device,const std::string& mountPoint,const std::string& fsType,bool bSetReadOnly, int &exitCode,std::string &errorMsg)
{
	bool rv = true;
	do
{
    if ( mountPoint.empty())
    {
			std::stringstream strerr;
			strerr << "Empty mountpoint specified.";
			strerr << "Cannot mount device " << device << "\n";
			DebugPrintf(SV_LOG_ERROR,"%s", strerr.str().c_str());
			errorMsg = strerr.str();
			rv = false;
			break;
    }
	std::string errmsg;
	if(!IsValidMountPoint(mountPoint,errmsg))
	{
		std::stringstream strerr;
		strerr << "Cannot mount device " << device << " on mount point " << mountPoint << ". " << errmsg << "\n";
		DebugPrintf(SV_LOG_ERROR,"%s", strerr.str().c_str());
        errorMsg = strerr.str();
        rv = false;
        break;
    }
    bool bCanMount = false;
    bool remount   = false;
    std::string flag;

    if(bSetReadOnly)
        flag="ro ";
    else
        flag="rw ";


    std::string sFileSystem = fsType;
    if(sFileSystem =="")
        sFileSystem ="auto";

    std::string flags = flag;   //This is used only in case of Uncomment FSTAB. flag gets updated with -o remount where as flags doesn't.
    std::string sMount = "";
    std::string mode;
    if ( IsVolumeMounted(device,sMount,mode))
    {
        if ( sMount == mountPoint) //need to check if mode is same or not. If the mode is same, we will say already mounted, else we will do ewmount
        {
            bCanMount = true; 
            flag+=" -o remount";
        }
        else
        {
            bCanMount = false;
				std::stringstream strerr;
				strerr << "Failed to unhide volume, since volume is already visible on mount " << sMount;
				strerr << " and requested mount is  " << mountPoint;
				strerr << ".Ensure system mounts are not specified.\n";
				DebugPrintf(SV_LOG_ERROR,"%s", strerr.str().c_str());
				errorMsg = strerr.str();
				rv = false;
				break;
        }
    }
    else /* volume is  not mounted */
    {	
        bCanMount = true;	
    }

    if ( bCanMount)
    {
        sFileSystem = ToLower(sFileSystem);
        if(!strcmp(sFileSystem.c_str(),"fat32")) {
            sFileSystem = "vfat";
        }else if(!strcmp(sFileSystem.c_str(),"fat")) {
            sFileSystem = "msdos";
        }else if(!strcmp(sFileSystem.c_str(),"pcfs")) {
            sFileSystem = "vfat";
        }
        

        /*
        PR# 3079:

        if the fs is ntfs
        open the volume in rw mode
        read the magic number
        if it is not ntfs
        log a message
        write ntfs
        end if
        */

        if (!strcmp(sFileSystem.c_str(),"ntfs"))
        {
            ACE_HANDLE hVolume = ACE_OS::open(device.c_str(), O_RDWR, FILE_SHARE_READ);

            do
            {
                SVERROR sve;
                if(ACE_INVALID_HANDLE == hVolume)
                {
                    sve = ACE_OS::last_error();
                    DebugPrintf( SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
						std::stringstream strerr;
						strerr << "ACE_OS::open failed on " << device;
						strerr << " in MountVolume.err = " << sve.toString() << "\n";
						DebugPrintf(SV_LOG_ERROR,"%s", strerr.str().c_str());
						errorMsg = strerr.str();
						exitCode = ACE_OS::last_error();
                    break;
                }

#ifndef LLSEEK_NOT_SUPPORTED
                if (ACE_OS::llseek(hVolume, SV_NTFS_MAGIC_OFFSET, SEEK_SET) < 0) {
#else
                if (ACE_OS::lseek(hVolume, SV_NTFS_MAGIC_OFFSET, SEEK_SET) < 0) {
#endif
                    sve = ACE_OS::last_error();
                    DebugPrintf(SV_LOG_ERROR,"@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
						std::stringstream strerr;
						strerr << "FAILED ACE_OS::llseek on " << device;
						strerr << ", err = " << sve.toString() << "\n";
						DebugPrintf(SV_LOG_ERROR,"%s", strerr.str().c_str());
						errorMsg = strerr.str();
						exitCode = ACE_OS::last_error();
                    break;
                }

                char pchDeltaBuffer[SV_FSMAGIC_LEN +1];
                long cbRead = ACE_OS::read(hVolume, pchDeltaBuffer,SV_FSMAGIC_LEN);
                if(cbRead != SV_FSMAGIC_LEN)
                {
                    sve = ACE_OS::last_error();
                    DebugPrintf(SV_LOG_ERROR,"@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
						std::stringstream strerr;
						strerr << "FAILED ACE_OS::read on " << device;
						strerr << ", err = " << sve.toString() << "\n";
						DebugPrintf(SV_LOG_ERROR,"%s", strerr.str().c_str());
						errorMsg = strerr.str();
						exitCode = ACE_OS::last_error();
                    break;
                }

                if(0 != memcmp(&pchDeltaBuffer[0],SV_MAGIC_NTFS, SV_FSMAGIC_LEN))
                {
                    DebugPrintf(SV_LOG_ERROR, "%s did not have ntfs header. adding it manually\n", device.c_str());
                    inm_memcpy_s(pchDeltaBuffer, sizeof(pchDeltaBuffer), SV_MAGIC_NTFS, SV_FSMAGIC_LEN);

#ifndef LLSEEK_NOT_SUPPORTED
                    if (ACE_OS::llseek(hVolume, SV_NTFS_MAGIC_OFFSET, SEEK_SET) < 0) {
#else
                    if (ACE_OS::lseek(hVolume, SV_NTFS_MAGIC_OFFSET, SEEK_SET) < 0) {
#endif
                        sve = ACE_OS::last_error();
                        DebugPrintf(SV_LOG_ERROR,"@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
							std::stringstream strerr;
							strerr << "FAILED ACE_OS::llseek on " << device;
							strerr << ", err = " << sve.toString() << "\n";
							DebugPrintf(SV_LOG_ERROR,"%s", strerr.str().c_str());
							errorMsg = strerr.str();
							exitCode = ACE_OS::last_error();
                        break;
                    }

                    long cbWrite = ACE_OS::write(hVolume,&pchDeltaBuffer[0],SV_FSMAGIC_LEN);
                    if(cbWrite != SV_FSMAGIC_LEN)
                    {
                        sve = ACE_OS::last_error();
                        DebugPrintf(SV_LOG_ERROR,"@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
							std::stringstream strerr;
							strerr << "FAILED ACE_OS::write on " << device;
							strerr << ", err = " << sve.toString() << "\n";
							DebugPrintf(SV_LOG_ERROR,"%s", strerr.str().c_str());
							errorMsg = strerr.str();
							exitCode = ACE_OS::last_error();
                        break;
                    }
                }

            } while (0);

            if(ACE_INVALID_HANDLE != hVolume)
            {
                ACE_OS::close(hVolume);
                hVolume = ACE_INVALID_HANDLE;
            }

        } // end if strcmp ntfs

        std::string fsmount;

        fsmount+="mount ";
        fsmount+=device.c_str() ;
        fsmount+="  ";
        fsmount+=mountPoint.c_str() ;
        fsmount+="  ";
        fsmount+="-t ";
        fsmount+=sFileSystem.c_str();
        fsmount+="  ";
        fsmount+="-o ";
        fsmount+=flag ;

        if(flag!=flags)
            remount=true;
        // Bug: 7329
		std::string displayErrorMsg;
		displayErrorMsg = "Tried with fstype: ";
		displayErrorMsg += sFileSystem;
			int cmdExitCode, retryExitCode;
			std::string cmdErrorMsg, retryErrorMsg;
			if(!ExecuteInmCommand(fsmount,cmdErrorMsg,cmdExitCode))
        {
            sFileSystem = ToUpper(sFileSystem);
            fsmount.clear();
            fsmount+="mount ";
            fsmount+=device.c_str() ;
            fsmount+="  ";
            fsmount+=mountPoint.c_str() ;
            fsmount+="  ";
            fsmount+="-t ";
            fsmount+=sFileSystem.c_str();
            fsmount+="  ";
            fsmount+="-o ";
            fsmount+=flag;
            if(!ExecuteInmCommand(fsmount,retryErrorMsg,retryExitCode))
            {
					if((cmdExitCode == retryExitCode) && (!stricmp(retryErrorMsg.c_str(),cmdErrorMsg.c_str())))
				{
					displayErrorMsg += ", ";
					displayErrorMsg += sFileSystem;
					displayErrorMsg += "\n";
					displayErrorMsg += "Unhide mount operation failed\n";
					displayErrorMsg += retryErrorMsg;
				}
				else
				{
					displayErrorMsg += "\n";
					displayErrorMsg += "Unhide mount operation failed\n";
						displayErrorMsg += cmdErrorMsg;
					displayErrorMsg += " Tried with fstype: ";
					displayErrorMsg += sFileSystem;
					displayErrorMsg += "\n";
					displayErrorMsg += "Unhide mount operation failed\n";
                    displayErrorMsg += retryErrorMsg;
				}
				DebugPrintf(SV_LOG_ERROR,"%s\n",displayErrorMsg.c_str());
					errorMsg = displayErrorMsg;
					exitCode = cmdExitCode;
					rv = false;
					break;
				}
			}
		}
	}while(false);
	return rv;
}

bool MountVolume(const std::string& device,const std::string& mountPoint,const std::string& fsType,bool bSetReadOnly)
{
	int exitCode = 0;
    std::string errorMsg = "";
	bool bCanMount = false;
	bool remount   = false;
    std::string flags;
    if(bSetReadOnly)
        flags="ro ";
    else
        flags="rw ";
	std::string sFileSystem = fsType;
	if(sFileSystem =="")
		sFileSystem ="auto";
	std::string sMount = "";
	std::string mode;
	if ( IsVolumeMounted(device,sMount,mode))
    {
        if ( sMount == mountPoint)
		{
			bCanMount = true;
			remount = true;
		}
	}
	else
	{
		bCanMount = true;
	}	
	if ( bCanMount)
	{
		sFileSystem = ToLower(sFileSystem);
		if(!strcmp(sFileSystem.c_str(),"fat32")) {
			sFileSystem = "vfat";
		}else if(!strcmp(sFileSystem.c_str(),"fat")) {
			sFileSystem = "msdos";
		}else if(!strcmp(sFileSystem.c_str(),"pcfs")) {
			sFileSystem = "vfat";
            }
        }
    bool bmounted = MountDevice(device, mountPoint, fsType, bSetReadOnly, exitCode, errorMsg);
    std::string filepath;
    bool new_sparsefile_format = false;
    bool is_volpack = IsVolPackDevice(device.c_str(),filepath,new_sparsefile_format);
	if (bmounted && (!is_volpack))
	{
        if(!AddFSEntry(device.c_str(),mountPoint.c_str(),sFileSystem.c_str(),flags.c_str(),remount))
        {
            DebugPrintf(SV_LOG_INFO, "Note: /etc/fstab was not updated. please update it manually.\n");
        }
    }
	return bmounted;
}



/* LINUX SPECIFIC */

bool GetMountPoints(const char * dev, std::vector<std::string> &mountPoints)
{
    bool rv = false;
    FILE * fp = NULL;
    std::string proc_fsname = "";
    struct mntent *entry = NULL;
	mntent m_Ent;
	char m_Buffer[MAXPATHLEN*4];

    do
    {
		memset(&m_Ent, 0, sizeof(m_Ent));
        fp = setmntent(SV_PROC_MNTS, "r");

        if (fp == NULL)
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed to open %s \n",
                FUNCTION_NAME, LINE_NO, FILE_NAME, SV_PROC_MNTS);
            rv = false;
            break;
        }

		std::string sMountPoint="";
        while( (entry = getmntent_r(fp, &m_Ent, m_Buffer, sizeof(m_Buffer))) != NULL )
        {
            if (!IsValidDevfile(entry->mnt_fsname))
                continue;

            proc_fsname = entry->mnt_fsname;
            GetDeviceNameFromSymLink(proc_fsname);
            if ( 0 == strcmp(proc_fsname.c_str(),dev))
                mountPoints.push_back(entry->mnt_dir);
        }

        rv = true;
    }while(0);

    if ( NULL != fp )
    {
        endmntent(fp);
    }

    return rv;
}


/* LINUX SPECIFIC */

SVERROR MakeReadOnly(const char *drive, void *VolumeGUID, etBitOperation ReadOnlyBitFlag)
{
    SVERROR sve = SVS_OK;

    //Stub for unix
    //Always return failure for now

    //sve = SVS_FALSE;
    int fd;
    if ((fd = open(drive, O_RDONLY))< 0)
        return SVE_FAIL;
    int flag = (ReadOnlyBitFlag == ecBitOpReset)?0:((ReadOnlyBitFlag == ecBitOpSet)?1:0);
    bool bResult = false;
    bResult = (ioctl(fd, BLKROSET, &flag) == 0); // linux only
    close(fd);

    if (bResult) {
        if(ReadOnlyBitFlag == ecBitOpReset)
            DebugPrintf("Reset Read Only Flag on Volume %s: Succeeded\n", drive);
        else
            DebugPrintf("Set Read Only Flag on Volume %s: Succeeded\n", drive);
    } else {
        if(ReadOnlyBitFlag == ecBitOpReset)
            DebugPrintf(SV_LOG_ERROR, "FAILED to Reset Read Only flag on Volume %s: Failed\n",  drive);
        else
            DebugPrintf(SV_LOG_ERROR, "FAILED to Set Read Only flag on Volume %s: Failed\n",  drive);
        sve = SVS_FALSE;
    }

    return sve ;
}



/* LINUX SPECIFIC */

SVERROR isReadOnly(const char * drive, bool & rvalue)
{
    SVERROR sve = SVS_OK;
    DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s, drive = %s\n", LINE_NO, FILE_NAME, drive);

    if( NULL == drive )
    {
        sve = EINVAL;
        DebugPrintf( SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( SV_LOG_ERROR, "FAILED UnhideDrive_RW()... err = EINVAL\n");
        return sve;
    }

    std::string sMount = "";  // 3077
    std::string mode;
    if (!IsVolumeMounted(drive,sMount,mode))
    {
        DebugPrintf(SV_LOG_DEBUG,"Device %s is not mounted. It means it is hidden .\n",drive);
        rvalue=false;
        return SVS_FALSE ;
    }

    int fd;
    if ((fd = open(drive, O_RDONLY))< 0)
        sve = SVE_FAIL;
    int flag;
    bool bResult = false;
    bResult = (ioctl(fd, BLKROGET, &flag) == 0);
    if (bResult)
    {
        if (flag)
            rvalue = true;
        else 
            rvalue = false;
    }
    else
    {
        sve = SVS_FALSE;
    }
    close(fd);

    return sve;
}



/* LINUX SPECIFIC */

VOLUME_STATE GetVolumeState(const char * drive)
{    
    DebugPrintf( SV_LOG_DEBUG, "Entered GetVolumeState\n");
    // Algorithm:
    // Get actual device name.
    // Get all current mount points associated with the device.
    // If mount points empty, return HIDDEN
    //   For each mount point...
    //     Is it mounted read write, return UNHIDDEN_RW
    //
    // Return UNHIDDEN_RO
    //


    bool rv  = false;
    FILE *fp = NULL;
    struct mntent *entry = NULL;
	mntent m_Ent;
    bool mounted = false;
    bool rw = false;
    VOLUME_STATE vs = VOLUME_UNKNOWN;
    std::string devname = drive;
	char m_Buffer[MAXPATHLEN*4];
    do
    {
    memset(&m_Ent, 0, sizeof(m_Ent));
        if(!IsVolpackDriverAvailable())
        {
            std::string sparsefile;
            bool new_sparsefile_format = false;
            bool is_volpack = IsVolPackDevice(devname.c_str(),sparsefile,new_sparsefile_format);
            if(is_volpack)
            {
                vs = VOLUME_HIDDEN;
                break;
            }
        }
        rv = GetDeviceNameFromSymLink(devname);
        if(!rv)
        {
            DebugPrintf(SV_LOG_ERROR, "Function %s @LINE %d in FILE %s :invalid device name %s\n", FUNCTION_NAME, LINE_NO, FILE_NAME,devname.c_str());
            vs = VOLUME_UNKNOWN;
            break;
        }

        rv = IsValidDevfile(devname);
        if(!rv)
        {
            DebugPrintf(SV_LOG_ERROR, "Function %s @LINE %d in FILE %s :invalid device name %s\n", FUNCTION_NAME, LINE_NO, FILE_NAME,devname.c_str());
            vs = VOLUME_UNKNOWN;
            break;
        }

        fp = setmntent(SV_PROC_MNTS, "r");

        if (fp == NULL)
        {
            DebugPrintf(SV_LOG_ERROR, "Function %s @LINE %d in FILE %s :unable to open %s\n", FUNCTION_NAME, LINE_NO, FILE_NAME,SV_PROC_MNTS);
            vs = VOLUME_UNKNOWN;
            break;
        }

        while( (entry = getmntent_r(fp, &m_Ent, m_Buffer, sizeof(m_Buffer))) != NULL )
        {

			std::string procdevName = entry -> mnt_fsname;
            if(!GetDeviceNameFromSymLink(procdevName))
                continue;

            if (!IsValidDevfile(procdevName))
                continue;

            if ( procdevName  == devname )
            {
                mounted = true;
                if( 0 != strstr(entry->mnt_opts, "rw") )
                {
                    rw = true;
                    break;
                }
            }
        }

        endmntent(fp);

        if(mounted)
        {
            if(rw)
                vs = VOLUME_VISIBLE_RW;
            else
                vs = VOLUME_VISIBLE_RO;
        }
        else
        {
            vs = VOLUME_HIDDEN;
        }

    } while (0);

    DebugPrintf( SV_LOG_DEBUG, "Exited GetVolumeState\n");
    return vs;
}



/* LINUX SPECIFIC */

bool IsVolumeMounted(const std::string volume,std::string& sMountPoint, std::string &mode)
{
    bool bMounted = false;
    sMountPoint = "";
    if(volume.empty())
    {
        DebugPrintf(SV_LOG_ERROR,"Null volume name passed.\n");
        return false;
    }
	mntent m_Ent;
	char m_Buffer[MAXPATHLEN*4];
	memset(&m_Ent, 0, sizeof(m_Ent));
    std::string devName = volume;
    std::string proc_fsname = "";
    GetDeviceNameFromSymLink(devName);

    FILE *fp;
    struct mntent *entry = NULL;

    fp = setmntent(SV_FAST_PROC_MOUNT, "r");

    if (fp == NULL)
    {
        printf("Could not able to open %s \n", setmntent);
        return false;
    }

    while((entry = getmntent_r(fp, &m_Ent, m_Buffer, sizeof(m_Buffer))) && ( false == bMounted))
    {
        if (!IsValidDevfile(entry->mnt_fsname))
            continue;


        proc_fsname += entry->mnt_fsname;
        GetDeviceNameFromSymLink(proc_fsname);
        if ( 0 == strcmp(proc_fsname.c_str(),devName.c_str()))
        {
            bMounted = true;
            sMountPoint+= entry->mnt_dir;
            mode=entry->mnt_opts;

        }
        proc_fsname.clear();

    }
    if ( NULL != fp )
    {
        endmntent(fp);
    }
    return bMounted;
}



/* LINUX SPECIFIC */
/* SHOULD THINK OF MAKING IT COMMAN FOR ALL */

//This function flushes all pending file system buffers (both data and metadata) in a volume. The argument
// should be in the form A: or B: or C: etc

SVERROR FlushFileSystemBuffers(const char *Volume)
{
    std::string volName;
    ACE_HANDLE hVolume = ACE_INVALID_HANDLE;
    SVERROR sve = SVS_OK;

    do
    {

        SV_UINT flags = 0;
        volName = Volume;

        flags = O_RDWR | O_DIRECT;

        hVolume = ACE_OS::open(volName.c_str(), flags, ACE_DEFAULT_OPEN_PERMS);

        if(ACE_INVALID_HANDLE == hVolume)
        {
            sve = ACE_OS::last_error();
            DebugPrintf( SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_ERROR, "ACE_OS::open failed in FlushFileSystemBuffers.We still proceed %s, err = %s\n", volName.c_str(), sve.toString());
            break;
        }

        if (ACE_OS::fsync(hVolume) == -1)        
        {
            sve = ACE_OS::last_error();
            DebugPrintf(SV_LOG_WARNING, "FlushFileBuffers for Volume %s did not succeed, err = %s\n", volName.c_str(), sve.toString());
        }
        else
            DebugPrintf(SV_LOG_DEBUG, "FlushFileBuffers for Volume %s Succeeded\n", volName.c_str());

        // Flush the buffer cache
        if(ioctl(hVolume, BLKFLSBUF,0) == -1)
        {
            DebugPrintf(SV_LOG_WARNING, 
                "FlushBufferCache for Volume %s did not succeed\n", volName.c_str());
        }

        ACE_OS::close(hVolume);

    } while (FALSE);

    return sve;
}



/* LINUX SPECIFIC */
/* SINCE GetVolSize FUNCTION IS LINUX SPECIFIC */

//-----------------------------------------------------------------------------
// This is a function that works under W2K and later to get the correct volume
// size on all flavors of volumes (static/dynamic, raid/striped/mirror/spanned)
// On WXP and later, IOCTL_DISK_GET_LENGTH_INFO already does this, so that is
// what we try first. This code is a direct translation to user mode of some
// kernel code in host\driver\win32\shared\MntMgr.cpp
//
// IMPORTANT: It turns out from user mode, a file system will slightly reduce
// the size of a volume by hiding some blocks. You need to call the ioctl
// DeviceIoControl(volumeHandle,
//                 FSCTL_ALLOW_EXTENDED_DASD_IO, 
//                 NULL, 0, NULL, 0, &status, NULL);
// on any volume that uses this function, or expect to read or write ALL of a
// volume. This tells the filesystem to let the underlying volume do the bounds
// checking, not the filesystem This ioctl will probably fail on raw volumes,
// just ignore the error and call it on every volume. You may 
// need #define _WIN32_WINNT 0x0510 before your headers to get this ioctl
//
// This function expects as input a handle opened on a volume with at least
// read access. A status value from GetLastErrro() will be return if needed
// The volume size will be in bytes, although always a multiple of 512
//-----------------------------------------------------------------------------
SVERROR GetVolumeSize(ACE_HANDLE volumeHandle, 
                      unsigned long long* volumeSize)
{
    SVERROR sve = SVS_OK;
    sve = GetVolSize(volumeHandle, volumeSize);
    return sve;
}


/* LINUX SPECIFIC */
/* SINCE GetFsVolSize FUNCTION IS LINUX SPECIFIC */

///
/// Returns the total bytes in the volume (not the free space).
/// 0 returned upon failure.
///
SV_LONGLONG GetVolumeCapacity( const char *volume )
{
    SV_LONGLONG capacity = 0;

    if (GetFsVolSize(volume, (unsigned long long *)&capacity).failed())
        capacity = 0;

    return( capacity );
}


/* LINUX SPECIFIC */

bool GetVolumeRootPath(const std::string & path, std::string & root)
{
    //  char rootPath[SV_INTERNET_MAX_PATH_LENGTH];
    struct mntent *mntptr;
    size_t lenthOfmntdir = 0;
    bool found = false;
	mntent m_Ent;
	char m_Buffer[MAXPATHLEN*4];
	memset(&m_Ent, 0, sizeof(m_Ent));

    root.clear();

    FILE *fp = setmntent(SV_PROC_MNTS,"r");
    if(fp == NULL)
        return false;

    while( (mntptr = getmntent_r(fp, &m_Ent, m_Buffer, sizeof(m_Buffer))) != NULL )
    {
        lenthOfmntdir = strlen(mntptr->mnt_dir);
        if(!strncmp(mntptr->mnt_dir, path.c_str(), lenthOfmntdir)) {
            if ( lenthOfmntdir > root.size()) {
                root = mntptr->mnt_dir;
                found = true;
            }
        }
    }

	if(fp)
		endmntent(fp);

    return found;
}


/* LINUX SPECIFIC */

//
// Function: GetDeviceNameForMountPt
//  given a mount point, convert it to corresponding device name
//
bool GetDeviceNameForMountPt(const std::string & mtpt, std::string & devName)
{
    bool rv = false;

    FILE *fp;
    struct mntent *entry = NULL;
	mntent m_Ent;
	char m_Buffer[MAXPATHLEN*4];
	memset(&m_Ent, 0, sizeof(m_Ent));

    fp = setmntent(SV_PROC_MNTS, "r");

    if (fp == NULL)
    {
        DebugPrintf(SV_LOG_ERROR, "Could not able to open %s\n", SV_PROC_MNTS);
        return false;
    }


    while( (entry = getmntent_r(fp, &m_Ent, m_Buffer, sizeof(m_Buffer))) != NULL )
    {
        if (!IsValidDevfile(entry->mnt_fsname))
            continue;

        if ( 0 == strcmp(entry->mnt_dir, mtpt.c_str()))
        {
            devName = entry->mnt_fsname;

            // convert it to actual device name instead
            // of symlink
            if(!GetDeviceNameFromSymLink(devName))
                continue;

            rv = true;
            break;
        }
    }

    if ( NULL != fp )
    {
        endmntent(fp);
    }

    return rv;
}



/* LINUX SPECIFIC */

bool IsValidMntEnt(mntent *entry)
{
    bool valid = true;

    if (entry == NULL) valid = false;

    if (strcmp(entry->mnt_fsname, "none") == 0) valid = false;   
    if (strncmp(entry->mnt_dir, "/dev", strlen("/dev")) == 0) valid = false;
    if (strcmp(entry->mnt_type, "nfs") == 0) valid = false;
    //if (strcmp(entry->mnt_type, "swap") == 0) valid = false;
    if (strcmp(entry->mnt_type, "auto") == 0) valid = false; // all removable media
    if (strncmp(entry->mnt_dir, "/proc", strlen("/proc")) == 0) valid = false;

    return valid;
}


/* LINUX SPECIFIC */

/*
 * FUNCTION NAME :     mountedVolume
 *
 * DESCRIPTION :   Given a pathname, return the nested volumes beneath it
 *                  
 *
 * INPUT PARAMETERS :  pathname under which nested volumes need to be checked
 *
 * OUTPUT PARAMETERS :  nested volumes
 *
 * NOTES : doesn't count the provided mountpoint/devicename as a mounted volume
 *
 * return value : true if nested volumes are found, otherwise false
 *
*/
bool mountedvolume( const std::string& pathname, std::string& mountpoints )
{
	std::string mntpath = pathname + "/";
    FILE* fp = setmntent(SV_PROC_MNTS, "r");
	mntent m_Ent;
	char m_Buffer[MAXPATHLEN*4];
	memset(&m_Ent, 0, sizeof(m_Ent));
    if( fp && !pathname.empty() )
    {
        struct mntent* line;
        while( line = getmntent_r(fp, &m_Ent, m_Buffer, sizeof(m_Buffer)) )
        {
            if( !strncmp( line->mnt_fsname, "/dev/", 5 ) && strcmp( line->mnt_dir, pathname.c_str()))
            {
                if( !strncmp( line->mnt_dir, mntpath.c_str(), mntpath.size()))
                {
                    mountpoints += line->mnt_dir;
                    mountpoints += "\n";
                }
            }
        }
    }
	if(fp)
		endmntent(fp);
    return mountpoints.empty() ? false : true;
}



/* LINUX SPECIFIC */

int posix_fadvise_wrapper(ACE_HANDLE fd, SV_ULONGLONG offset, SV_ULONGLONG len, int advice)
{
    return posix_fadvise(fd, offset, len, advice);
}



/* LINUX SPECIFIC */
void setdirectmode(unsigned long int &access)
{
    access |= O_DIRECT;
}

void setdirectmode(int &mode)
{
    mode |= O_DIRECT;
}

bool FormVolumeLabel(const std::string device, std::string &label)
{
    bool bretval = true;



    std::string cmd(SV_LABEL);
    cmd += device;
    cmd += " 2> /dev/null ";

    std::stringstream results;

    if (!executePipe(cmd, results)) 
    {
        DebugPrintf(SV_LOG_ERROR,
                    "Function %s @LINE %d in FILE %s :failed to execute cmd %s \n", 
                    FUNCTION_NAME, LINE_NO, FILE_NAME, cmd.c_str());
        bretval = false;
    }
    else
    {
        std::string e2label = results.str();
        trim(e2label);
        if (!e2label.empty()) 
        {
            label= "LABEL=";
            label += e2label;
        }
    }

    return bretval;
}


std::string GetCommandPath(const std::string &cmd)
{
    /* APART FROM COMMON PATHS, WE SHOULD FIND AN EQUIVALENT OF "which" COMMAND
     * IN THIS CASE, COMMAND SHOULD BE ONLY COMMAND AND SHOULD NOT CONTAIN
     * SPACE OR COMMAND OPTIONS 
     */
    return "/bin:/sbin:/usr/bin:/usr/local/bin";
}


bool GetVolSizeWithOpenflag(const std::string vol, const int oflag, SV_ULONGLONG &refvolsize)
{
    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
	bool bretval = false;

    int fd = open(vol.c_str(), oflag);
	if (-1 == fd)
	{
        DebugPrintf( SV_LOG_ERROR, "@ LINE %d in FILE %s, open system call failed for volume %s\n", 
				                   __LINE__, __FILE__, vol.c_str());
	}	
	else
	{
		SV_ULONGLONG vsnapsize = 0;
		SV_ULONGLONG sectorsize = 0;
        if (ioctl(fd, BLKSSZGET, &sectorsize) < 0)
        {
    		DebugPrintf(SV_LOG_ERROR, "Failed to obtain the sector size for volume %s\n", 
	        	                     vol.c_str());
        }
		else
		{
            if(ioctl(fd, BLKGETSIZE, &vsnapsize) < 0)
            {
                DebugPrintf(SV_LOG_ERROR, "Failed to obtain the size of volume %s\n", vol.c_str());
            }
			else
			{
                refvolsize = vsnapsize * sectorsize;
				bretval = true;
			}
		}
        close(fd);
	}

    DebugPrintf( SV_LOG_DEBUG, "EXITING %s WITH RETURN VALUE %s\n", FUNCTION_NAME, (bretval?"true":"false"));
	return bretval;
}


// --------------------------------------------------------------------------
// format volume name the way the CX wants it
// final format should be one of
//   <drive>
//   <drive>:\[<mntpoint>\]
// where: <drive> is the drive letter and [<mntpoint>\] is optional
//        and <mntpoint> is the mount point name
// e.g.
//   A:\ for a drive letter
//   B:\mnt\data\ for a mount point
// --------------------------------------------------------------------------
void FormatVolumeNameForCxReporting(std::string& volumeName)
{
    // we need to strip off any leading \, ., ? if they exists
    std::string::size_type idx = volumeName.find_first_not_of("\\.?");
    if (std::string::npos != idx) {
        volumeName.erase(0, idx);
    }

    // strip off trailing :\ or : if they exist
    std::string::size_type len = volumeName.length();
    idx = len;
    if ('\\' == volumeName[len - 1] || ':' == volumeName[len - 1]) {
        --idx;
    }

    if (':' == volumeName[len - 2]) 
    {
        --idx;
    }

    if (idx < len) {
        volumeName.erase(idx);
    }
}


bool GetSectorSizeWithFd(int fd, const std::string vol, SV_ULONG &SectorSize)
{
    bool bretval = false;
    if (-1 == fd)
    {
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, invalid fd = %d\n", LINE_NO, FILE_NAME, fd); 
    }
    else
    {
        SV_ULONG volsectorsize = 0;
        if (ioctl(fd, BLKSSZGET, &volsectorsize) < 0) 
        {
            DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, failed to get sector size for volume %s\n", LINE_NO, FILE_NAME, vol.c_str()); 
        }
        else
        {
            SectorSize = volsectorsize;
            bretval = true;
        }
    }
    return bretval;
}


bool GetNumBlocksWithFd(int fd, const std::string vol, SV_ULONGLONG &NumBlks)
{
    bool bretval = false;
    if (-1 == fd)
    {
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, invalid fd = %d\n", LINE_NO, FILE_NAME, fd); 
    }
    else
    {
        SV_ULONGLONG volnumblks = 0;
        if (ioctl(fd, BLKGETSIZE, &volnumblks) < 0)
        {
            DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, failed to get number of blocks for volume %s\n", LINE_NO, FILE_NAME, vol.c_str()); 
        }
        else
        {
            NumBlks = volnumblks;
            bretval = true;
        }
    }
    return bretval;
}


/* LINUX SPECIFIC */
/* The reason is that in linux, the device name
 * is although a link to the real name, read link
 * on device name gives absolute path of the real name
 * but in solaris, readlink on device name gives the
 * relative path. This function can be made common
 * to unix once the solaris version(which should work
 * for both solaris and linux) is tested on linux)
 */

//
// Gets the root device name from a symbolic link
//
bool GetDeviceNameFromSymLink(std::string &deviceName)
{
    if (NULL == deviceName.c_str()) {
        return false;
    }

	if(deviceName.compare(0,11,"/dev/mapper") == 0)
	{
		return true;
	}

    std::string linkName = deviceName;
    // make sure device is valid
    struct stat64 volStat;

    // make sure device file exists
    if (-1 == lstat64(deviceName.c_str(), &volStat)) {
        return false;
    }
	const char UNIXROOTDIR = '/';
    const char UNIXDIRSEPARATOR = '/';

    if (S_ISLNK(volStat.st_mode)) {
        ssize_t len;
        char symlnk[255]; // note should be max path not 255
        do {
            len = readlink(deviceName.c_str(), symlnk, sizeof(symlnk) - 1);
            if (-1 == len) {
                break;
            }
            symlnk[len] = '\0';
			if (UNIXROOTDIR != symlnk[0])
			{
				std::string catenatedlinkname = dirname_r(deviceName.c_str());
				catenatedlinkname +=  UNIXDIRSEPARATOR;
				catenatedlinkname += symlnk;
				char absolutename[PATH_MAX + 1] = {'\0'};
				if (NULL == realpath(catenatedlinkname.c_str(), absolutename))
				{
					DebugPrintf(SV_LOG_ERROR,"realpath failed @ LINE %d in FILE %s for file %s with errno = %d.\n", LINE_NO, FILE_NAME, 
						deviceName.c_str(), errno);
					break;
				}
				deviceName = absolutename;
			}
			else
            	deviceName = symlnk;
        } while (true);

        if (-1 == lstat64(deviceName.c_str(), &volStat)) {
            DebugPrintf(SV_LOG_WARNING,"Unable to determine attributes of file  %s \n",linkName.c_str());
            return false;
        }

    }

    return true;

}


bool IsReportingRealNameToCx(void)
{
    return true;
}


bool GetLinkNameIfRealDeviceName(const std::string sVolName, std::string &sLinkName)
{
    sLinkName = sVolName;
    return true;
}

std::string GetRawVolumeName(const std::string &dskname)
{
    return dskname;
}


/* LINUX */

bool IsProcessRunning(const std::string &sProcessName, uint32_t numExpectedInstances)
{
    const char *procdir = "/proc";	/* standard /proc directory */
    bool rv = false;
    DIR *dirp;
    int pdlen;
    char psname[100];
    char buffer[1024];
    char filename[100];
    struct dirent *dentp;
    int cnt = 0;
    const std::string self = "self";
    const std::string thread_self = "thread-self";

    do 
    {
        if ((dirp = opendir(procdir)) == NULL) 
        {
            printf("Failed Opening proc directory \n");
            rv = false;
            break;
        }

        (void) inm_strcpy_s(psname, ARRAYSIZE(psname), procdir);
        pdlen = strlen(psname);
        psname[pdlen++] = '/';

        while (dentp = readdir(dirp))
        {
            int	psfd;	                        /* file descriptor for /proc/nnnnn/stat */

            if (dentp->d_name[0] == '.')		/* skip . and .. */
                continue;

            if (!strcmp(self.c_str(), dentp->d_name) ||
                !strcmp(thread_self.c_str(), dentp->d_name)) 
            {
                continue;
            } 
            
            (void) inm_strcpy_s(psname + pdlen, ARRAYSIZE(psname) - pdlen, dentp->d_name);
            (void) inm_strcat_s(psname, ARRAYSIZE(psname), "/stat");

            if ((psfd = open(psname, O_RDONLY)) == -1)
                continue;

            int bytes = read(psfd,buffer,1024);
            close(psfd);
            if( bytes > 0)
            {	
                int i=0,j=0;
                while(buffer[i] != '(')
                    i++;

                i++;

                for(;buffer[i]!= ')'; i++,j++)
                {
                    filename[j]=buffer[i]; 
                }
                filename[j] = '\0'; 

				std::string check = filename;
                if(!strcmp(sProcessName.c_str(),check.c_str()))
                {
                    cnt++;
                    DebugPrintf(SV_LOG_DEBUG, "Name: %s PID: %s\n", check.c_str(), dentp->d_name);
                    if (cnt > numExpectedInstances)
                    {
                        rv = true;
                        break;
                    }
                }
            }
        }
        closedir(dirp);
    }while(0);

    return rv;
}

bool isLocalMachine32Bit( std::string machine)
{
    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
	std::string uname_machine = machine;
    DebugPrintf( SV_LOG_DEBUG, "Machine Vaue %s\n", machine.c_str() );
	removeStringSpaces( uname_machine );
	bool bis32BitMachine = false;
    int i = 0;
    for( ;i < 10;i++ )
    {
        std::stringstream sub;
        sub<< "i" << i << "86";
        if( uname_machine.find( sub.str() ) != std::string::npos )
        {
            bis32BitMachine = true;
			break;
        }
    }
	DebugPrintf( SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bis32BitMachine;
}
SVERROR getLinuxReleaseValue( std::string & linuxetcReleaseValue, LINUX_TYPE& linuxFlavourType )
{
	DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
	SVERROR bRet = SVS_OK;
	std::string redhat_release_Value, SuSE_release_Value, debian_release_Value, ubuntu_release_Value, enterprise_release_Value, oracle_release_Value, release_Value;
	if( getFileContent( "/etc/enterprise-release", enterprise_release_Value ) == SVS_OK )
	{
		linuxetcReleaseValue = enterprise_release_Value;
		linuxFlavourType = OEL5_RELEASE;
	}
	else if( getFileContent( "/etc/oracle-release", oracle_release_Value ) == SVS_OK )
	{
		linuxetcReleaseValue = oracle_release_Value;
		linuxFlavourType = OEL_RELEASE;
	}
	else if( getFileContent( "/etc/redhat-release", redhat_release_Value ) == SVS_OK )
	{
		linuxetcReleaseValue = redhat_release_Value;
		linuxFlavourType = REDHAT_RELEASE;
	}
	
	else if( getFileContent( "/etc/SuSE-release", SuSE_release_Value ) == SVS_OK ) 
	{
		linuxetcReleaseValue = SuSE_release_Value;
		linuxFlavourType = SUSE_RELEASE;
	}
	else if( getFileContent( "/etc/lsb-release", ubuntu_release_Value ) == SVS_OK )
	{
		linuxetcReleaseValue = ubuntu_release_Value;
		linuxFlavourType = UBUNTU_RELEASE;
	}
	else if( getFileContent( "/etc/os-release", release_Value ) == SVS_OK )
	{
		linuxetcReleaseValue = release_Value;
		if( release_Value.find("SLES") == std::string::npos )
		{
			linuxFlavourType = DEBAIN_RELEASE;
		}
		else
		{
			linuxFlavourType = SUSE_RELEASE;
		}
	}	
	else
	{
		bRet = SVS_FALSE;
	}
	return bRet;
	DebugPrintf( SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);		
}


bool IsOpenVzVM(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
    bool bisvm = false;

    DIR *dirp;
    dirp = opendir(OPENVZVMVZDIR);
    if (dirp)
    {
        closedir(dirp);
        dirp = opendir(OPENVZVMBCDIR);
        if (dirp)
        {
            closedir(dirp);
        }
        else
        {
            hypervisorname = HYPERVISOROPENVZ;
            bisvm = true;
        }
    }

    return bisvm;
}


bool IsVMFromCpuInfo(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
    bool bisvm = false;

    std::vector<std::string> vmpatsincpuinfo;
    vmpatsincpuinfo.push_back(QEMUPAT);
    vmpatsincpuinfo.push_back(UMLPAT);
    Pats vmcpuinfopats(vmpatsincpuinfo);

    std::ifstream cpuinfostream(CPUINFOFILE);
    if (cpuinfostream.is_open())
    {
        while (!cpuinfostream.eof())
        {
            std::string line;
            std::getline(cpuinfostream, line);
            if (line.empty())
            {
                break;
            }
            bisvm = vmcpuinfopats.IsMatched(line, hypervisorname);
            if (bisvm)
            {
                break;
            }
        }

        cpuinfostream.close();
    }

    return bisvm;
}


bool IsNativeVm(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
    return false;
}


bool IsXenVm(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
    std::string output, err;
    int ecode = 0;
    bool bisxen = RunInmCommand(ISXENCMD, output, err, ecode);
    if (bisxen)
    {
        hypervisorname = XENNAME;
    }
    return bisxen;
}


bool HasNicInfoPhysicaIPs(const NicInfos_t &nicinfos)
{
    // dummy function
    return true;
}


void GetNicInfos(NicInfos_t & nicInfos)
{
    int sockfd = -1;
    bool bgotnicinfos = true;
	DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (-1 != sockfd)
    {
        struct ifconf ifc;
        int inifclen;
        void *pv = NULL;
        void *pvsby = NULL;
        int ioctlrval = -1;
        size_t sizeofifreq = sizeof (struct ifreq);
        size_t initsize;
        INM_SAFE_ARITHMETIC(initsize = ((TYPICAL_NADAPTERS - 1) * InmSafeInt<size_t>::Type(sizeofifreq)), INMAGE_EX(sizeofifreq))

        inifclen = initsize;
        ifc.ifc_req = NULL;
        do
        {
            INM_SAFE_ARITHMETIC(inifclen += InmSafeInt<size_t>::Type(sizeofifreq), INMAGE_EX(inifclen)(sizeofifreq))
            ifc.ifc_len = inifclen;
            pvsby = realloc(pv, ifc.ifc_len);
            if (pvsby)
            {
                pv = pvsby;
                ifc.ifc_req = (struct ifreq *)pv;
                ioctlrval = ioctl(sockfd, SIOCGIFCONF, &ifc);
                if (ioctlrval)
                {
                    DebugPrintf(SV_LOG_ERROR, "ioctl SIOCGIFCONF failed with errno = %d\n", errno);
                    bgotnicinfos = false;
                    break;
                }
            }   
            else
            {
                DebugPrintf(SV_LOG_ERROR, "realloc to allocate memory for network interfaces failed with errno = %d\n", errno);
                bgotnicinfos = false;
                break;
            }
        } while(inifclen <= ifc.ifc_len);
        if (bgotnicinfos)
        {
            bgotnicinfos = FillNicInfos(sockfd, &ifc, sizeofifreq, nicInfos);
        }
        if (pv)
        {
            free(pv);
        }
        close(sockfd);
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "socket call failed with errno = %d\n", errno);
        bgotnicinfos = false;
    }

    if (bgotnicinfos)
    {
        std::string gws = GetDefaultGateways(NSNicInfo::DELIM);
        DebugPrintf(SV_LOG_DEBUG, "gateways = %s\n", gws.c_str());
        if (!gws.empty())
        {
            UpdateNicInfoAttr(NSNicInfo::DEFAULT_IP_GATEWAYS, gws, nicInfos);
        }

        std::string nameservers = GetNameServerAddresses(NSNicInfo::DELIM);
        DebugPrintf(SV_LOG_DEBUG, "nameserver ip addresses = %s\n", nameservers.c_str());
        if (!nameservers.empty())
        {
            UpdateNicInfoAttr(NSNicInfo::DNS_SERVER_ADDRESSES, nameservers, nicInfos);
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "cannot collect the network interfaces information\n");
    }
	DebugPrintf( SV_LOG_DEBUG, "EXITING %s\n", FUNCTION_NAME);
}


bool FillNicInfos(const int &sockfd, struct ifconf *pifc, size_t sizeofifreq, NicInfos_t & nicInfos)
{
	DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    bool bfillnicinfos = true;
    int numifreq = (pifc->ifc_len / sizeofifreq);
    char ipaddr[INET_ADDRSTRLEN] = "\0";
    const char *prval = NULL;
    struct sockaddr_in *sa = NULL;
    bool bcollect;
    std::set<std::string> ifsfromifconf;
    std::string DHCPValue;
    E_INM_TRISTATE e;

    for (int i = 0; i < numifreq; i++)
    {
        ifsfromifconf.insert(pifc->ifc_req[i].ifr_name);
        sa = (struct sockaddr_in *)&(pifc->ifc_req[i].ifr_addr);
        prval = inet_ntop(AF_INET, &(sa->sin_addr.s_addr), ipaddr, sizeof ipaddr);

        if (prval)
        {
            bcollect = ShouldCollectNicInfo(sockfd, pifc->ifc_req + i, ipaddr);
            if (bcollect)
            {
                std::string hardwareaddress;
                std::string netmask;
				GetNetIfParameters(pifc->ifc_req[i].ifr_name, hardwareaddress, netmask);
                if (hardwareaddress.empty())
                {
                    continue;
                }
                e = getDHCPValue(pifc->ifc_req[i].ifr_name,DHCPValue ) ? E_INM_TRISTATE_TRUE : E_INM_TRISTATE_FALSE;
				InsertNicInfo(nicInfos, pifc->ifc_req[i].ifr_name, hardwareaddress, ipaddr, netmask, e);
            }
            else
            {
                DebugPrintf(SV_LOG_DEBUG, "Not collecting information for nic %s\n", pifc->ifc_req[i].ifr_name);
            }
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "converting network ip address for interface %s to" 
                                      " readble string failed with errno = %d."
                                      " processing next interface if available\n", 
                                      pifc->ifc_req[i].ifr_name, errno);
        }
    } /* for */

    std::set<std::string> ifsfromproc;
    GetNicNamesFromProc(ifsfromproc);
    std::vector<std::string> ifsleftinproc(ifsfromproc.size());
    std::vector<std::string>::iterator endit = std::set_difference(ifsfromproc.begin(), ifsfromproc.end(), 
                                                                   ifsfromifconf.begin(), ifsfromifconf.end(),
                                                                   ifsleftinproc.begin());
    for (std::vector<std::string>::const_iterator it = ifsleftinproc.begin(); it != endit; it++)
    {
        const std::string &ifname = *it;
        DebugPrintf(SV_LOG_DEBUG, "interface %s is got from proc\n", ifname.c_str());
        std::string hardwareaddress;
        std::string netmask;
        GetNetIfParameters(ifname, hardwareaddress, netmask);
        if (hardwareaddress.empty())
        {
            continue;
        }
        e = getDHCPValue(ifname,DHCPValue ) ? E_INM_TRISTATE_TRUE : E_INM_TRISTATE_FALSE;
		InsertNicInfo(nicInfos, ifname, hardwareaddress, std::string(), netmask, e);
    }

	DebugPrintf( SV_LOG_DEBUG, "EXITING %s\n", FUNCTION_NAME);
    return bfillnicinfos;
} 


void GetNicNamesFromProc(std::set<std::string> &nicnamesfromproc)
{
    const std::string PROC_NET_DEV_FILE = "/proc/net/dev";
    std::ifstream fs(PROC_NET_DEV_FILE.c_str());

    if (fs.is_open())
    {
        /* skip the two lines at start
        [root@TST-BLD-CENTOS5U5-64-10 host]# cat /proc/net/dev
        * Inter-|   Receive                                                |  Transmit
        * face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
        * lo:  199889    1374    0    0    0     0          0         0   199889    1374    0    0    0     0       0          0
        * eth0:18016576209 229921346    0    0    0     0          0         0 11744040036 9463118    0    0    0     0       0          0
        * sit0:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
        */

        const char NICNAMESEP = ':';
        std::string line;
        std::getline(fs, line);
        std::getline(fs, line);

        while (!fs.eof())
        {
            line.clear();
            std::getline(fs, line);
            if (line.empty())
            {
                break;
            }
            std::stringstream ss(line);
            std::string in;
            ss >> in;
            if (!in.empty())
            {
                size_t colonpos = in.find_first_of(NICNAMESEP);
                if (std::string::npos != colonpos)
                {
                    in.erase(colonpos);
                    nicnamesfromproc.insert(in);
                }
            }
        }

        fs.close();
    }
}


bool ShouldCollectNicInfo(const int &sockfd, struct ifreq *pifr, const char *ipaddr)
{
    bool bshouldcollect = false;
    int ioctlrval = -1;
	DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    ioctlrval = ioctl(sockfd, SIOCGIFFLAGS, pifr);
    if (!ioctlrval)
    {
        if (pifr->ifr_flags & IFF_UP)
        {
            bshouldcollect = !(pifr->ifr_flags & IFF_LOOPBACK);
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "ioctl SIOCGIFFLAGS failed on interface %s." 
                                  " However trying to report IP and Interface name\n", pifr->ifr_name);
        bshouldcollect = true;
    }
    if (bshouldcollect)
    {
        bshouldcollect = IsValidIP(ipaddr);
    }
	DebugPrintf( SV_LOG_DEBUG, "EXITING %s\n", FUNCTION_NAME);
    return bshouldcollect;
}


SV_ULONGLONG GetTotalFileSystemSpace(const struct statvfs64 &vfs)
{
    SV_ULONGLONG space = ((SV_ULONGLONG)vfs.f_blocks) * ((SV_ULONGLONG)vfs.f_bsize);
    return space;
}


SV_ULONGLONG GetFreeFileSystemSpace(const struct statvfs64 &vfs)
{
    SV_ULONGLONG space = ((SV_ULONGLONG)vfs.f_bavail) * ((SV_ULONGLONG)vfs.f_bsize);
    return space;
}


void GetNetIfParameters(const std::string &ifrName, std::string& hardwareAddress, std::string &netmask)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (-1 == fd)
    {
        DebugPrintf(SV_LOG_DEBUG, "For getting hardware address for network interface %s, "
                                  "creating socked failed with error %s\n", ifrName.c_str(),
                                  strerror(errno));
        return;
    }

    struct ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;
    inm_strncpy_s(ifr.ifr_name, ARRAYSIZE(ifr.ifr_name), ifrName.c_str(), IFNAMSIZ-1);

    if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr))
    {
        if ((unsigned char)ifr.ifr_hwaddr.sa_data[0] ||
            (unsigned char)ifr.ifr_hwaddr.sa_data[1] ||
            (unsigned char)ifr.ifr_hwaddr.sa_data[2] ||
            (unsigned char)ifr.ifr_hwaddr.sa_data[3] ||
            (unsigned char)ifr.ifr_hwaddr.sa_data[4] ||
            (unsigned char)ifr.ifr_hwaddr.sa_data[5])
        {
            char HwAddr[256] ;
			inm_sprintf_s(HwAddr, ARRAYSIZE(HwAddr), "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n",
                (unsigned char)ifr.ifr_hwaddr.sa_data[0],
                (unsigned char)ifr.ifr_hwaddr.sa_data[1],
                (unsigned char)ifr.ifr_hwaddr.sa_data[2],
                (unsigned char)ifr.ifr_hwaddr.sa_data[3],
                (unsigned char)ifr.ifr_hwaddr.sa_data[4],
                (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
            hardwareAddress = HwAddr ;
            DebugPrintf(SV_LOG_DEBUG, "Interface Name %s Hardware Address %s\n", ifrName.c_str(), hardwareAddress.c_str()) ;
        }
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "For interface Name %s, hardware address is NULL. Hence not reporting\n", ifrName.c_str());
        }
    }
    else
    {
        DebugPrintf(SV_LOG_DEBUG, "For getting hardware address for network interface %s, "
                                  "ioctl SIOCGIFHWADDR failed with error: %s\n", ifrName.c_str(),
                                  strerror(errno));
    }

    if (!hardwareAddress.empty())
    {
        if (0 == ioctl(fd, SIOCGIFNETMASK, &ifr))
        {
            char mask[INET_ADDRSTRLEN] = "\0";
            struct sockaddr_in *sa = (struct sockaddr_in *)&(ifr.ifr_netmask);
            const char *prval = inet_ntop(AF_INET, &(sa->sin_addr.s_addr), mask, sizeof mask);
            netmask = mask;
            DebugPrintf(SV_LOG_DEBUG, "Interface Name %s subnet mask %s\n", ifrName.c_str(), mask);
        }
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "For getting netmask for network interface %s, "
                                      "ioctl SIOCGIFNETMASK failed with error: %s\n", ifrName.c_str(),
                                      strerror(errno));
        }
    }

    close(fd);
}

std::string GetExecdName(void)
{
    std::string procName;
    pid_t parent = ACE_OS::getpid();
    std::stringstream proccessfile;
    proccessfile << "/proc/" << parent << "/cmdline";
    std::ifstream procfile(proccessfile.str().c_str());
    procfile >> procName;
    procfile.close();
    return procName;
}
bool RemoveVgWithAllLvsForVsnap(const std::string & device,std::string& output, std::string& error,bool needvgcleanup)
{
    return true;
}

bool getDHCPValue( std::string interface, std::string& value )
{

	std::string fileName;
	std::string line;
	std::istringstream sin;
	
	
	fileName = "/etc/sysconfig/network-scripts/ifcfg-" + interface;

	std::ifstream fin(fileName.c_str());

	if(fin.good())
	{
		while (std::getline(fin, line))
		{		

			sin.str(line.substr(line.find("=")+1));
			if (line.find("BOOTPROTO") != std::string::npos)
 			{	
			     sin >> value;
			     if(value.find("dhcp") != std::string::npos)
				     return true;
	         	 else
				     return false;		 			
      		}

		       sin.clear();
       	}
    }
	else
	{
		DebugPrintf(SV_LOG_DEBUG, "Unable to open file = %s.\n",fileName.c_str());
		return false;
	}		

	return false;
}
