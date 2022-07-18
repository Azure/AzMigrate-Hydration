#include <string>
#include <sstream>

#include <sys/types.h>
#include <fcntl.h>
#include <sys/procfs.h>

#include <sys/mntctl.h>
#include <sys/vmount.h>
#include <sys/devinfo.h>
#include <sys/scsi.h>
#include <sys/ioctl.h>
#include <alloca.h>
#include <stdlib.h>

#include <odmi.h>
#include <sys/cfgodm.h>
#include <sys/cfgdb.h>
#include <sys/sysmacros.h>

#include "portablehelpers.h"
#include "portablehelperscommonheaders.h"
#include "portablehelpersmajor.h"
#include "portablehelpersminor.h"
#include "configwrapper.h"
#include "boost/algorithm/string/replace.hpp"
#include "cdplock.h"
#include <ace/Guard_T.h>
#include "ace/Thread_Mutex.h"
#include "ace/Recursive_Thread_Mutex.h"
#include "inmsafecapis.h"
#include "inmsafeint.h"
#include "inmageex.h"

#define SIZE_LIMIT_TOP     0x100000000000LL
#define NUM_SECTORS_PER_TB    (1LL << 31)
#define PSNAMELEN 100

#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define SIZE(p) MAX((p).sa_len, sizeof(p))

ACE_Recursive_Thread_Mutex g_lockforgetbasicvolinfo;

#define THRSHOLD_NUM_FIELDSINFSTAB 7
#define FLAG_FIELD_IN_FSTAB 6

bool ShouldCollectNicInfo(const int &sockfd, struct ifreq *pifr, const char *ipaddr);
bool FillNicInfos(const int &sockfd, struct ifconf *pifc, size_t sizeofifreq, NicInfos_t & nicInfos);
void InsertNicInfo(NicInfos_t &nicInfos, const std::string &name, const std::string &hardwareaddress, const std::string &ipaddress, 
                   const std::string &netmask, const E_INM_TRISTATE &isdhcpenabled);
void UpdateNicInfoAttr(const std::string &attr, const std::string &value, NicInfos_t &nicinfos);
void GetNetIfParameters(const std::string &ifrName, std::string& hardwareAddress, std::string &netmask);
void CollectDhcpEnabledInterfaces(std::set<std::string> &dhcpenabledinterfaces);

SVERROR GetFsVolSize(const char *volName, unsigned long long *pfsSize)
{
	SVERROR sv = SVE_FAIL;
	if(volName && pfsSize)
	{
	    *pfsSize = GetRawVolSize(volName);
		if(*pfsSize)
		{
			sv = SVS_OK;
		}
		else
		{
			DebugPrintf(SV_LOG_ERROR,"@ LINE %d in FILE %s, GetRawVolSize failed for volume %s\n", LINE_NO, FILE_NAME, volName);
		} 
	}
	else
	{
		DebugPrintf(SV_LOG_ERROR,"@ LINE %d in FILE %s, invalid arguements supplied\n", LINE_NO, FILE_NAME);
	}
	return sv;
}

SV_ULONGLONG GetRawVolSize(const std::string sVolume)
{
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s with sVolume = %s\n",FUNCTION_NAME, sVolume.c_str());
	SV_ULONGLONG rawsize = 0;
    SV_ULONGLONG mulfactor;
    unsigned int low, high;
    std::stringstream ss;

	if(sVolume.empty() == false)
	{
		ACE_Guard<ACE_Recursive_Thread_Mutex> guard(g_lockforgetbasicvolinfo);
		if(guard.locked())
		{
			std::string sRawVolName = GetRawVolumeName(sVolume);
			int fd;
			fd = open(sRawVolName.c_str(), O_NDELAY | O_RDONLY);
			if(fd != 0)
			{
				struct devinfo devinfo;
				if (ioctl(fd, IOCINFO, &devinfo) != -1) 
				{
					switch(devinfo.devtype) 
					{
						case DD_DISK:   /* disk */
                            if (devinfo.flags & DF_LGDSK)
                            {
                                ss << "For DD_DISK, DF_LGDSK is set. "
                                   << " devinfo.flags = " << (unsigned int)devinfo.flags
                                   << " bytpsec = " << devinfo.un.dk64.bytpsec
                                   << " secptrk = " << devinfo.un.dk64.secptrk
                                   << " trkpcyl = " << devinfo.un.dk64.trkpcyl
                                   << " lo_numblks = " << devinfo.un.dk64.lo_numblks
                                   << " segment_size = " << devinfo.un.dk64.segment_size
                                   << " segment_count = " << devinfo.un.dk64.segment_count
                                   << " byte_count = " << devinfo.un.dk64.byte_count
                                   << " hi_numblks = " << devinfo.un.dk64.hi_numblks;

                                mulfactor = pow(2, sizeof(devinfo.un.dk64.lo_numblks)*8);
                                low = (unsigned int)devinfo.un.dk64.lo_numblks;
                                high = (unsigned int)devinfo.un.dk64.hi_numblks;
                                rawsize = ((mulfactor*high) + low) * devinfo.un.dk64.bytpsec;
                            }
                            else
                            {
                                ss << "For DD_DISK, DF_LGDSK is not set. "
                                   << " devinfo.flags = " << (unsigned int)devinfo.flags
                                   << " bytpsec = " << devinfo.un.dk.bytpsec
                                   << " secptrk = " << devinfo.un.dk.secptrk
                                   << " trkpcyl = " << devinfo.un.dk.trkpcyl
                                   << " numblks = " << devinfo.un.dk.numblks
                                   << " segment_size = " << devinfo.un.dk.segment_size
                                   << " segment_count = " << devinfo.un.dk.segment_count
                                   << " byte_count = " << devinfo.un.dk.byte_count;
                                low = (unsigned int)devinfo.un.dk.numblks;
                                rawsize = low * devinfo.un.dk.bytpsec;
                            }
                            DebugPrintf(SV_LOG_DEBUG, "For device %s, %s\n", sVolume.c_str(), ss.str().c_str());
							break;
						case DD_SCDISK: /* scsi disk */
                            if (devinfo.flags & DF_LGDSK)
                            {
                                ss << "For DD_SCDISK, DF_LGDSK is set. "
                                   << " devinfo.flags = " << (unsigned int)devinfo.flags
                                   << " blksize = " << devinfo.un.scdk64.blksize
                                   << " lo_numblks = " << devinfo.un.scdk64.lo_numblks
                                   << " lo_max_request = " << devinfo.un.scdk64.lo_max_request
                                   << " segment_size = " << devinfo.un.scdk64.segment_size
                                   << " segment_count = " << devinfo.un.scdk64.segment_count
                                   << " byte_count = " << devinfo.un.scdk64.byte_count
                                   << " hi_numblks = " << devinfo.un.scdk64.hi_numblks
                                   << " hi_max_request = " << devinfo.un.scdk64.hi_max_request;

                                mulfactor = pow(2, sizeof(devinfo.un.scdk64.lo_numblks)*8);
                                low = (unsigned int)devinfo.un.scdk64.lo_numblks;
                                high = (unsigned int)devinfo.un.scdk64.hi_numblks;
                                rawsize = ((mulfactor*high) + low) * devinfo.un.scdk64.blksize;
                            }
                            else
                            {
                                ss << "For DD_SCDISK, DF_LGDSK is not set. "
                                   << " devinfo.flags = " << (unsigned int)devinfo.flags
                                   << " blksize = " << devinfo.un.scdk.blksize
                                   << " numblks = " << devinfo.un.scdk.numblks
                                   << " max_request = " << devinfo.un.scdk.max_request
                                   << " segment_size = " << devinfo.un.scdk.segment_size
                                   << " segment_count = " << devinfo.un.scdk.segment_count
                                   << " byte_count = " << devinfo.un.scdk.byte_count;
                                low = (unsigned int)devinfo.un.scdk.numblks;
                                rawsize = low * devinfo.un.scdk.blksize;
                            }
                            DebugPrintf(SV_LOG_DEBUG, "For device %s, %s\n", sVolume.c_str(), ss.str().c_str());
							break;
						default:        /* unknown */
							break;
					}
				}
				else
				{
					DebugPrintf(SV_LOG_ERROR, "ioctl IOCINFO Failed volumepath: %s, error: %s\n", sRawVolName.c_str(), strerror(errno));
				}
				(void)close(fd);
			}
			else
			{
				DebugPrintf(SV_LOG_ERROR, "open() failed for raw volume %s at line %d in file %s with errno = %d\n", sRawVolName.c_str(),
			__LINE__, __FILE__, errno);
			}
		}
		else
		{
			DebugPrintf(SV_LOG_WARNING, "@LINE %d in FILE %s unable to apply lock\n", __LINE__, __FILE__);
		}
	}
	else
	{
	   DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, invalid arguement VolumeName\n", 
			LINE_NO, FILE_NAME);
	}
	if( rawsize == 0 )
	{
		std::list<std::string> vgs ;
		std::stringstream stream ;
		executePipe("lsvg", stream) ;
		std::string lvname = sVolume.substr(sVolume.find_last_of("/")+1) ;
        while( stream.good() && !stream.eof() )
        {
            std::string str ;
            stream >> str ;
            if( str.empty() )
            {
                break ;
            }
            vgs.push_back(str) ;
        }
		stream.clear() ;
        stream.str("") ;
		int ppsize ;
		std::list<std::string>::iterator begIter = vgs.begin() ;
		std::list<std::string>::iterator endIter = vgs.end() ;
		while(begIter != endIter )
		{
			
			std::string cmd = "lsvg -l " ;
			cmd += *begIter ;
			cmd += " |grep " ;
			cmd += lvname ;
			executePipe(cmd, stream) ;
			if( stream.str().compare("") != 0 )
			{
				DebugPrintf(SV_LOG_DEBUG, "lv %s belongs to vg %s\n", lvname.c_str(), begIter->c_str()) ;
				break ;
			}
			begIter++ ;
		}
		stream.str("") ;
		stream.clear() ;
		executePipe("lsvg " + *begIter+ " |grep 'PP SIZE:' | awk '{print $6}'", stream) ;
        stream >> ppsize ;
		stream.clear() ;
        stream.str("") ;
		executePipe("/usr/sbin/lsvg -l " + *begIter+ "| /usr/bin/grep -v " + *begIter+ " | /usr/bin/grep -v 'LV NAME'" + "| /usr/bin/grep " + lvname , stream) ;
		std::string name, lvtype, lvstate, mountPt , lps, pps, pvs ;
		stream >> name >> lvtype >> lps >> pps >> pvs >> lvstate >> mountPt ;
		rawsize = (long)ppsize * boost::lexical_cast<long>(lps) * 1024 * 1024 ;

	}
	DebugPrintf(SV_LOG_DEBUG,"EXITED %s with rawsize = " ULLSPEC "\n",FUNCTION_NAME, rawsize);
	return rawsize;
}

SVERROR GetFsSectorSize(const char *volName, unsigned int *psectorSize)
{
	DebugPrintf(SV_LOG_DEBUG, "ENTERED %s with sVolume = %s\n",FUNCTION_NAME, volName);
	SVERROR retStatus = SVS_FALSE;
	*psectorSize = 512;
	if(volName != NULL)
	{
		ACE_Guard<ACE_Recursive_Thread_Mutex> guard(g_lockforgetbasicvolinfo);
		if(guard.locked())
		{
			std::string sRawVolName = GetRawVolumeName(volName);
			int fd;
			fd = open(sRawVolName.c_str(), O_NDELAY | O_RDONLY);
			if(fd != -1)
			{
				struct devinfo devinfo;
				if(ioctl(fd, IOCINFO, &devinfo) != -1) 
				{
					switch(devinfo.devtype) 
					{
						case DD_DISK:   /* disk */
							*psectorSize = (devinfo.flags&DF_LGDSK) ?  devinfo.un.dk64.bytpsec : devinfo.un.dk.bytpsec;
							break;
						case DD_SCDISK: /* scsi disk */
							*psectorSize = (devinfo.flags&DF_LGDSK) ? devinfo.un.scdk64.blksize : devinfo.un.scdk.blksize;
							break;
						default:        /* unknown */
							break;
					}
					retStatus = SVS_OK;
				}
				else
				{
					DebugPrintf(SV_LOG_ERROR, "ioctl IOCINFO Failed volumepath: %s, error: %s\n", sRawVolName.c_str(), strerror(errno));
				}
				(void)close(fd);
			}
			else
			{
				DebugPrintf(SV_LOG_ERROR, "open() failed for raw volume %s at line %d in file %s with errno = %d\n", sRawVolName.c_str(),
			__LINE__, __FILE__, errno);
			}
		}
		else
		{
			DebugPrintf(SV_LOG_WARNING, "@LINE %d in FILE %s unable to apply lock\n", __LINE__, __FILE__);
		}
	}
	else
	{
	   DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, invalid arguement volName = %p\n", 
			LINE_NO, FILE_NAME, volName);
	}
	DebugPrintf(SV_LOG_DEBUG,"EXITED %s with sectorsize = %d \n", FUNCTION_NAME, *psectorSize);
	return retStatus;
}

bool addFSEntryWithOptions(const std::string& device1, const std::string& label, const std::string& mountpoint1, const std::string& type,
                           bool readonlyflag,std::vector<std::string>& fsents)
{
    bool bRet = true;
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s for device %s\n", FUNCTION_NAME, device1.c_str());
    std::string device = device1;
    std::string mountPoint = mountpoint1;
    removeStringSpaces(device);  
    removeStringSpaces(mountPoint); 
    do
    {
        if(device.empty() || mountPoint.empty())
        {
            DebugPrintf(SV_LOG_ERROR, "DeviceName and Mount Point Names are empty. %s, %s", device.c_str(), mountPoint.c_str());
            bRet = false;
            break;
        }

        std::string realDevice = device;
        if(!GetDeviceNameFromSymLink(realDevice))
        {
            DebugPrintf(SV_LOG_ERROR, "GetDeviceNameFromSymLink Failed for device %s \n. ", realDevice.c_str());
            bRet = false;
            break;
        }

        if(!removeFSEntry(device,"",fsents))
        {
            DebugPrintf(SV_LOG_INFO, "Unable to add FS entry.\n");
            bRet = false;
            break;
        }            

        std::string lineStr = "\n";
        lineStr += mountPoint;
        lineStr += ":";
        fsents.push_back(lineStr);
        lineStr = "\n        dev             = ";
        lineStr += device;
        fsents.push_back(lineStr);
        lineStr = "\n        vfs             = ";
        lineStr += type;
        fsents.push_back(lineStr);
        lineStr = " \n       log             = INLINE";
        fsents.push_back(lineStr);
        lineStr = "\n        mount           = true";
        fsents.push_back(lineStr);
        if(readonlyflag)
            lineStr = "\n        options         = ro";
        else
            lineStr = "\n        options         = rw";
        fsents.push_back(lineStr);
        lineStr = "\n        account         = false\n";
        fsents.push_back(lineStr);
        break;

    }while(false);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s for device %s\n", FUNCTION_NAME, device1.c_str());
    return bRet;
}
bool addRWFSEntry(const std::string& device1, const std::string& label, const std::string& mountpoint1, const std::string& type,
                  std::vector<std::string>& fsents)
{
    return addFSEntryWithOptions(device1,label,mountpoint1,type,false,fsents);
}

bool addROFSEntry(const std::string& device1, const std::string& label, const std::string& mountpoint1, const std::string& type,
				  std::vector<std::string>& fsents)
{
	return addFSEntryWithOptions(device1,label,mountpoint1,type,true,fsents);
}

bool MountVolume(const std::string& device,const std::string& mountPoint,const std::string& fsType,bool bSetReadOnly)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    int exitCode = 0;
	std::string errorMsg = "";
	bool bmounted = MountDevice(device, mountPoint, fsType, bSetReadOnly, exitCode, errorMsg); 
	std::string flags = "ro";

	if(!bSetReadOnly)
		flags="rw ";

	std::string filepath;
	if (bmounted )
	{
        bool new_sparsefile_format = false;
        bool is_volpack = IsVolPackDevice(device.c_str(),filepath,new_sparsefile_format);
        if(!is_volpack)
        {
    		if(AddFSEntry(device.c_str(), mountPoint.c_str(), fsType.c_str(), flags.c_str(), false))
    		{
    			DebugPrintf(SV_LOG_DEBUG, "/etc/filesystem was updated successfully. \n");
    		}
            else
            {
                DebugPrintf(SV_LOG_INFO, "Note: /etc/filesystem was not updated. please update it manually.\n");
            }
        }
        else
        {
            DebugPrintf(SV_LOG_INFO, "IsVolPackDevice() failed \n");
        }
	}
    else
    {
        DebugPrintf(SV_LOG_INFO, "MountDevice() failed \n");
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
	return bmounted;
}

int posix_fadvise_wrapper(ACE_HANDLE fd, SV_ULONGLONG offset, SV_ULONGLONG len, int advice)
{
	return 0; //SUCCESS
}

void setdirectmode(unsigned long int &access)
{
	 access |= O_DIRECT;
}

void setdirectmode(int &mode)
{
	 mode |= O_DIRECT;
}

bool removeFSEntry(const std::string& device, const std::string& label, std::vector<std::string>& fsents)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
	bool bRet = true;
    std::string deviceName = device;
    std::vector<std::string>::iterator fsents_mntpt_start_iter;
    std::vector<std::string>::iterator fsents_iter = fsents.begin();
    std::vector<std::string>::iterator fsents_end_iter = fsents.end();
    bool found = false;
    
    for(;fsents_iter != fsents_end_iter;++fsents_iter)
    {
        std::string fsent = *fsents_iter;
        removeStringSpaces(fsent);
        std::string::size_type index = std::string::npos;
        index = fsent.find("=");
        /* Mount point found and store in mountpoint iter */
        if((!fsent.empty()) && (index == std::string::npos) && (fsent.find_last_of(":") == fsent.size()-1))
        {
            if(found)
            {                
                break;
            }
            fsents_mntpt_start_iter = fsents_iter;            
        }
        /* checking for dev entry if the entry their and the value of the entry match with the device 
           then the device found*/
        else if(index != std::string::npos) 
        {
            std::string leftString = fsent.substr(0, index);
            std::string rightString = fsent.substr(index+1);
            removeStringSpaces(leftString);
            removeStringSpaces(rightString);
            if((leftString.compare("dev") == 0) && (rightString.compare(deviceName) == 0))
            {
                found = true;
            }            
        }        
    }
    
    if(found)
    {
        fsents.erase(fsents_mntpt_start_iter,fsents_iter);
    }
        
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bRet;
}

bool getMountPointName(const std::string& deviceName, std::string& mountPoint, bool bFromFileSytemFile)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    bool bFound = false;
    if(bFromFileSytemFile == false)
    {
        // discover from mount api
        std::vector<mount_info> mountinfovector;
        if(getMountedInfo(mountinfovector) == SVS_OK)
        {
            std::string mountpoint = "";
            std::vector<mount_info>::iterator mountinfolistbegiter = mountinfovector.begin();
            std::vector<mount_info>::iterator mountinfolistienditer = mountinfovector.end();
            while(mountinfolistbegiter != mountinfolistienditer)
            {
                if(mountinfolistbegiter->m_deviceName.compare(deviceName)== 0)
                {
                   mountpoint = mountinfolistbegiter->m_mountedOver;
                   bFound = true;
                   break;
                }
                mountinfolistbegiter++;
            }
        }
        else
        {
             DebugPrintf(SV_LOG_ERROR, "Function %s @LINE %d in FILE %s :getMountedInfo failed.\n", 
                 FUNCTION_NAME, LINE_NO, FILE_NAME);    
        }
    }
    else
    {
        // discover from /etc/filesytems file.
        CDPLock lock(SV_FSTAB,10);
        if(lock.acquire())
        {
            std::ifstream ifstab(SV_FSTAB, std::ifstream::in);
            if(ifstab.is_open())
            {
                std::string line = "";
                std::string currentMountPoint = "";
                while(std::getline(ifstab, line))
                {
                    removeStringSpaces(line);
                    if(line.empty() == false)
                    {
                        std::string::size_type index = std::string::npos;
                        index = line.find("=");
                        if(index == std::string::npos && line.find_last_of(":") == line.size()-1)
                        {
                           currentMountPoint = line;
                        }
                        else if(index != std::string::npos)
                        {
                            std::string leftString = line.substr(0, index);
                            removeStringSpaces(leftString);
                            if(leftString.compare("dev") == 0)
                            {
                                std::string rightString = line.substr(index+1);
                                removeStringSpaces(rightString);
                                if(rightString.compare(deviceName) == 0)
                                {
                                    bFound = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                if(bFound == true && currentMountPoint.empty() == false)
                {
                    mountPoint = currentMountPoint.substr(0, currentMountPoint.size()-1);
                }
                ifstab.close();
            }
            else
            {
                DebugPrintf(SV_LOG_ERROR, "Function %s @LINE %d in FILE %s :failed to open %s \n", 
                                FUNCTION_NAME, LINE_NO, FILE_NAME, SV_FSTAB);
            }            
        }
        else
        {
                DebugPrintf(SV_LOG_ERROR, "Function %s @LINE %d in FILE %s :failed to aquire read lock for         /etc/filesytems file %s \n", 
                                            FUNCTION_NAME, LINE_NO, FILE_NAME, SV_FSTAB);
        }
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bFound;
}

bool GetMountPoints(const char * dev, std::vector<std::string> &mountPoints)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
	bool bRet = false;
	if(dev != NULL)
    {
        std::string realdev = dev;
        if (GetDeviceNameFromSymLink(realdev))
        {
            std::string deviceName = dev;
            std::vector<mount_info> mountInfoVector; 
            if(getMountedInfo(mountInfoVector) == SVS_OK)
            {
                std::vector<mount_info>::iterator mountInfoListBegIter = mountInfoVector.begin();
                std::vector<mount_info>::iterator mountInfoListiEndIter = mountInfoVector.end();
                while(mountInfoListBegIter != mountInfoListiEndIter)
                {
                   if(mountInfoListBegIter->m_deviceName.compare(deviceName) == 0)
                   {
                      mountPoints.push_back(mountInfoListBegIter->m_mountedOver);
                      break;
                   }
                   mountInfoListBegIter++;
                }
                bRet = true;
            }
            else
            {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :getMountedInfo failed.\n", 
                FUNCTION_NAME, LINE_NO, FILE_NAME);        
            }          
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :GetDeviceNameFromSymLink failed.\n", 
                FUNCTION_NAME, LINE_NO, FILE_NAME);
            
        }         
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR,
			"Function %s @LINE %d in FILE %s : invalid arguement dev is NULL\n", 
			FUNCTION_NAME, LINE_NO, FILE_NAME);
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
	return bRet;
}

SVERROR getMountedInfo(std::vector<mount_info>& mountInfoVector)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    SVERROR retStatus = SVS_OK;
    int num, size;
    num = mntctl(MCTL_QUERY, sizeof(size), (char *) &size);
    if(num >= 0)
    {
        INM_SAFE_ARITHMETIC(size *= InmSafeInt<int>::Type(2), INMAGE_EX(size))
/*
  * Double the needed size, so that even when the user mounts a
  * filesystem between the previous and the next call to mntctl
  * the buffer still is large enough.
*/
        char * buf;
        buf = (char *)alloca(size); 
        num = mntctl(MCTL_QUERY, size, buf);
        if(num >= 0)
        {
            DebugPrintf(SV_LOG_DEBUG, "No of file systems found %d \n", num);
            struct vmount * vm;
            int i, dataLength;
            char *data;
            for (vm = ( struct vmount *)buf, i = 0; i < num; i++)
            {
               if(vm != NULL)
               {
                   dataLength = vm->vmt_data[VMT_OBJECT].vmt_size;
                   data = NULL;
                   size_t dataalloclen;
                   INM_SAFE_ARITHMETIC(dataalloclen = InmSafeInt<int>::Type(dataLength) + 1, INMAGE_EX(dataLength))
                   data = (char*)malloc(dataalloclen);
                   inm_strncpy_s(data, (dataLength + 1), (char *)vm + vm->vmt_data[VMT_OBJECT].vmt_off, dataLength);
                   mount_info mInfo;
                   mInfo.m_deviceName = data;
                   if(data != NULL)
                   {
                     free(data);
                   }
                   dataLength = vm->vmt_data[VMT_STUB].vmt_size;
                   INM_SAFE_ARITHMETIC(dataalloclen = InmSafeInt<int>::Type(dataLength) + 1, INMAGE_EX(dataLength))
                   data = (char*)malloc(dataalloclen);
                   inm_strncpy_s(data, (dataLength + 1), (char *)vm + vm->vmt_data[VMT_STUB].vmt_off, dataLength);
                   mInfo.m_mountedOver = data;
                   if(data != NULL)
                   {
                     free(data);
                   }
                   dataLength = vm->vmt_data[VMT_ARGS].vmt_size;
                   INM_SAFE_ARITHMETIC(dataalloclen = InmSafeInt<int>::Type(dataLength) + 1, INMAGE_EX(dataLength))
                   data = (char*)malloc(dataalloclen);
                   inm_strncpy_s(data, (dataLength + 1), (char *)vm + vm->vmt_data[VMT_ARGS].vmt_off, dataLength);
                   mInfo.m_options = data;
                   if(data != NULL)
                   {
                     free(data);
                   }
                   mInfo.m_flag = getPermissionType(mInfo.m_options);                   
                   dataLength = vm->vmt_data[VMT_HOSTNAME].vmt_size;
                   INM_SAFE_ARITHMETIC(dataalloclen = InmSafeInt<int>::Type(dataLength) + 1, INMAGE_EX(dataLength))
                   data = (char*)malloc(dataalloclen);
                   inm_strncpy_s(data, (dataLength + 1), (char *)vm + vm->vmt_data[VMT_HOSTNAME].vmt_off, dataLength);
                   mInfo.m_nodeName = data;
                   if(data != NULL)
                   {
                     free(data);
                   }
				   mInfo.m_vfs = getFSString(vm->vmt_gfstype);
                   mountInfoVector.push_back(mInfo);
               }
               vm = (struct vmount *)((char *)vm + vm->vmt_length);
            }
            //dumpMountInfo(mountInfoVector);            //debugging purpose.
        }
        else
        {
             DebugPrintf(SV_LOG_ERROR, "Failed to get the size i. mntctl() failed \n");
             retStatus = SVS_FALSE;           
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to get the size. mntctl() failed \n");
        retStatus = SVS_FALSE;
    }
    
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return retStatus;
}
SVERROR MakeReadOnly(const char *drive, void *VolumeGUID, etBitOperation ReadOnlyBitFlag)
{
	DebugPrintf("MakeReadOnly (AIX) on %s: not implemented\n", drive);
	return SVE_FAIL;
}

SVERROR isReadOnly(const char * drive, bool & bReadOnly)
{
	SVERROR retrunStatus = SVS_OK;
	if(drive != NULL)
	{
		VOLUME_STATE volState = GetVolumeState(drive);
		if(volState == VOLUME_VISIBLE_RW)
		{
			bReadOnly = false;
		}
		else if(volState == VOLUME_VISIBLE_RO)
		{
			bReadOnly = true;
		}
		else if(volState == VOLUME_HIDDEN)
		{
			bReadOnly = false;
		}
		else
		{
			DebugPrintf(SV_LOG_ERROR, "UnKnown Volume: %s .\n", drive);
			retrunStatus = SVS_FALSE;
		}
	}
	else
	{
		DebugPrintf(SV_LOG_ERROR, "Empty Drive name passed.\n");
		retrunStatus = SVS_FALSE;
	}
	return retrunStatus;
}

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
	bool mounted = false;
	bool rw = false;
	VOLUME_STATE vs = VOLUME_UNKNOWN;
	std::string realdevname = drive;
	std::string devname = drive;

	do 
	{
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
		rv = GetDeviceNameFromSymLink(realdevname);
		if(!rv)
		{
			DebugPrintf(SV_LOG_ERROR, "Function %s @LINE %d in FILE %s :invalid device name %s\n", FUNCTION_NAME, LINE_NO, FILE_NAME,realdevname.c_str());
			vs = VOLUME_UNKNOWN;
			break;
		}

		rv = IsValidDevfile(realdevname);
		if(!rv)
		{
			DebugPrintf(SV_LOG_ERROR, "Function %s @LINE %d in FILE %s :invalid device name %s\n", FUNCTION_NAME, LINE_NO, FILE_NAME,realdevname.c_str());
			vs = VOLUME_UNKNOWN;
			break;
		}
        std::vector<mount_info> mountInfoVector;
        if(getMountedInfo(mountInfoVector) == SVS_OK)
        {
            std::vector<mount_info>::iterator mountInfoVectorBegIter = mountInfoVector.begin();
            std::vector<mount_info>::iterator mountInfoVectorEndIter = mountInfoVector.end();
            while(mountInfoVectorBegIter != mountInfoVectorEndIter)
            {
    			std::string procdevName = mountInfoVectorBegIter->m_deviceName;
    			std::string realprocdevName = procdevName;
    			if(GetDeviceNameFromSymLink(realprocdevName) && 
                    IsValidDevfile(realprocdevName))
                {
				    if(realprocdevName == realdevname)
        			{
        				mounted = true;
                        if(  mountInfoVectorBegIter->m_flag == MOUNT_RW || 
                             mountInfoVectorBegIter->m_flag == MOUNT_RBW ||
                             mountInfoVectorBegIter->m_flag == MOUNT_RBRW                           )
        				{
        					rw = true;
        					break;
        				}
        			}
                }
                mountInfoVectorBegIter++;
            }
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
        }
        else
        {
             DebugPrintf(SV_LOG_ERROR,
                                "Function %s @LINE %d in FILE %s :getMountedInfo failed.\n", 
                                    FUNCTION_NAME, LINE_NO, FILE_NAME);   
        }
	} while (0);

	DebugPrintf( SV_LOG_DEBUG, "Exited GetVolumeState\n");
	return vs;
        
}

bool IsVolumeMounted(const std::string volume,std::string& sMountPoint, std::string &mode)
{
	bool bMounted = false;
	sMountPoint = "";
	if(volume.empty())
	{
		DebugPrintf(SV_LOG_ERROR,"Null volume name passed.\n");
		return false;
	}
	std::string devName = volume;
	std::string realdevName = volume;
	std::string proc_fsname = "";
	if(GetDeviceNameFromSymLink(realdevName) && IsValidDevfile(realdevName))
    {
        std::vector<mount_info> mountInfoVector;
        if(getMountedInfo(mountInfoVector) == SVS_OK)
        {
            size_t lenthOfmntdir = 0;
            std::vector<mount_info>::iterator mountInfoVectorBegIter = mountInfoVector.begin();
            std::vector<mount_info>::iterator mountInfoVectorEndIter = mountInfoVector.end();
            while(mountInfoVectorBegIter != mountInfoVectorEndIter)
            {
               std::string realproc_fsname = mountInfoVectorBegIter->m_deviceName;
               if(GetDeviceNameFromSymLink(realproc_fsname) && IsValidDevfile(realdevName))
               {
                   if(realproc_fsname == realdevName)
                   {
                      bMounted = true;
                      sMountPoint =  mountInfoVectorBegIter->m_mountedOver;
                      break;              
                   }
               }
               mountInfoVectorBegIter++;
            }
         }
         else
         {
             DebugPrintf(SV_LOG_ERROR,
                 "Function %s @LINE %d in FILE %s :getMountedInfo failed.\n", 
                 FUNCTION_NAME, LINE_NO, FILE_NAME);        
         } 
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR,
                 "Function %s @LINE %d in FILE %s :GetDeviceNameFromSymLink()|IsValidDevfile() failed.\n", FUNCTION_NAME, LINE_NO, FILE_NAME); 
    }     
	return bMounted;
}

SVERROR FlushFileSystemBuffers(const char *Volume)
{
	std::string volName;
	ACE_HANDLE hVolume = ACE_INVALID_HANDLE;
	SVERROR sve = SVS_OK;

	do
	{
		SV_UINT flags = 0;
		volName = Volume;

		flags = O_RDWR | O_SYNC | O_RSYNC ;

		hVolume = ACE_OS::open(getLongPathName(volName.c_str()).c_str(), flags, ACE_DEFAULT_OPEN_PERMS);

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

		ACE_OS::close(hVolume);

	} while (FALSE);

	return sve;
}

bool GetVolumeRootPath(const std::string & path, std::string & root)
{
	bool found = false;
    std::vector<mount_info> mountInfoVector;
    if(getMountedInfo(mountInfoVector) == SVS_OK)
    {
        size_t lenthOfmntdir = 0;
        std::vector<mount_info>::iterator mountInfoVectorBegIter = mountInfoVector.begin();
        std::vector<mount_info>::iterator mountInfoVectorEndIter = mountInfoVector.end();
        while(mountInfoVectorBegIter != mountInfoVectorEndIter)
        {
            lenthOfmntdir = strlen(mountInfoVectorBegIter->m_mountedOver.c_str());
            if(!strncmp(mountInfoVectorBegIter->m_mountedOver.c_str(), path.c_str(), lenthOfmntdir)) {
                if ( lenthOfmntdir > root.size()) 
                {
                    root = mountInfoVectorBegIter->m_mountedOver;
                    found = true;
                }
            }
            mountInfoVectorBegIter++;
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR,
            "Function %s @LINE %d in FILE %s :getMountedInfo failed.\n", 
            FUNCTION_NAME, LINE_NO, FILE_NAME);        
    }
	return found;
}

//
// Function: GetDeviceNameForMountPt
//  given a mount point, convert it to corresponding device name
//
bool GetDeviceNameForMountPt(const std::string & mtpt, std::string & devName)
{
	bool bRet = false;
    std::vector<mount_info> mountInfoVector; 
    if(getMountedInfo(mountInfoVector) == SVS_OK)
    {
        std::vector<mount_info>::iterator mountInfoListBegIter = mountInfoVector.begin();
        std::vector<mount_info>::iterator mountInfoListiEndIter = mountInfoVector.end();
        while(mountInfoListBegIter != mountInfoListiEndIter)
        {
           if(mountInfoListBegIter->m_mountedOver.compare(mtpt)== 0)
           {
              devName = mountInfoListBegIter->m_deviceName;
              bRet = true;
              break;
           }
           mountInfoListBegIter++;
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR,
        "Function %s @LINE %d in FILE %s :getMountedInfo failed.\n", 
        FUNCTION_NAME, LINE_NO, FILE_NAME);        
    }      
	return bRet;
}

bool mountedvolume( const std::string& pathname, std::string& mountpoints )
{
	return false;
}

bool FormVolumeLabel(const std::string device, std::string &label)
{
	bool bretval = true;
    label = ""; 
	return bretval;
}

std::string GetCommandPath(const std::string &cmd)
{
	/* APART FROM COMMON PATHS, WE SHOULD FIND AN EQUIVALENT OF "which" COMMAND */
	return "/bin:/sbin:/usr/bin:/usr/local/bin:/usr/sbin";
}

bool GetVolSizeWithOpenflag(const std::string vol, const int oflag, SV_ULONGLONG &refvolsize)
{
	bool bretval = true;
	refvolsize = GetRawVolSize(vol);
	if (!refvolsize)
	{
		DebugPrintf(SV_LOG_ERROR,"@ LINE %d in FILE %s, GetRawVolSize failed for volume %s\n", LINE_NO, FILE_NAME, vol.c_str());
		bretval = false;
	}
	return bretval;
}

void FormatVolumeNameForCxReporting(std::string& volumeName)
{
    // we need to strip off any leading \, ., ? if they exists
	std::string::size_type idx = volumeName.find_first_not_of("\\.?");
	if (std::string::npos != idx){
		volumeName.erase(0, idx);
	}
	// strip off trailing :\ or : if they exist
	std::string::size_type len = volumeName.length();
	idx = len;
	if('\\' == volumeName[len - 1] || ':' == volumeName[len - 1]) {
		--idx;
	}
	if (idx < len){
		volumeName.erase(idx);
	}
}

bool GetSectorSizeWithFd(int fd, const std::string vol, SV_ULONG &SectorSize)
{
	bool bretval = false;
	SectorSize = 512;
	if (fd <= 0)
	{
		DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, invalid fd = %d\n", LINE_NO, FILE_NAME, fd); 
	}
	else
	{
		unsigned int volsectorsize = 0;
		if (vol.empty() == false)
		{
			ACE_Guard<ACE_Recursive_Thread_Mutex> guard(g_lockforgetbasicvolinfo);
			if(guard.locked())
			{
				struct devinfo devinfo;
				if (ioctl(fd, IOCINFO, &devinfo) != -1) 
				{
					switch (devinfo.devtype) 
					{
						case DD_DISK:   /* disk */
							SectorSize = (devinfo.flags&DF_LGDSK) ?  devinfo.un.dk64.bytpsec : devinfo.un.dk.bytpsec;
							break;
						case DD_SCDISK: /* scsi disk */
							SectorSize = (devinfo.flags&DF_LGDSK) ? devinfo.un.scdk64.blksize : devinfo.un.scdk.blksize;
							break;
						default:        /* unknown */
							break;
					}
					bretval = true;
				}
				else
				{
					DebugPrintf(SV_LOG_ERROR, "ioctl IOCINFO Failed volumepath: %s, error: %s\n", vol.c_str(), strerror(errno));
				}
			}
			else
			{
				DebugPrintf(SV_LOG_WARNING, "@LINE %d in FILE %s unable to apply lock\n", __LINE__, __FILE__);
			}
		}
		else
		{
            DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, invalid arguement vol = %s\n", LINE_NO, FILE_NAME, vol.c_str());
		}		
	}
	return bretval;
}

bool GetNumBlocksWithFd(int fd, const std::string vol, SV_ULONGLONG &NumBlks)
{
	bool bretval = false;
    SV_ULONGLONG mulfactor;
    unsigned int low, high;
	if(fd <= 0)
	{
		DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, invalid fd = %d\n", LINE_NO, FILE_NAME, fd); 
	}
	else
	{
		if(vol.empty() == false)
		{
			ACE_Guard<ACE_Recursive_Thread_Mutex> guard(g_lockforgetbasicvolinfo);
			if(guard.locked())
			{
				struct devinfo devinfo;
				if (ioctl(fd, IOCINFO, &devinfo) != -1) 
				{
					switch (devinfo.devtype) 
					{
						case DD_DISK:   /* disk */
                            if (devinfo.flags & DF_LGDSK)
                            {
                                mulfactor = pow(2, sizeof(devinfo.un.dk64.lo_numblks)*8);
                                low = (unsigned int)devinfo.un.dk64.lo_numblks;
                                high = (unsigned int)devinfo.un.dk64.hi_numblks;
                                NumBlks = ((mulfactor*high) + low);
                            }
                            else
                            {
                                NumBlks = (unsigned int)devinfo.un.dk.numblks;
                            }
							break;
						case DD_SCDISK: /* scsi disk */
                            if (devinfo.flags & DF_LGDSK)
                            {
                                mulfactor = pow(2, sizeof(devinfo.un.scdk64.lo_numblks)*8);
                                low = (unsigned int)devinfo.un.scdk64.lo_numblks;
                                high = (unsigned int)devinfo.un.scdk64.hi_numblks;
                                NumBlks = ((mulfactor*high) + low);
                            }
                            else
                            {
                                NumBlks = (unsigned int)devinfo.un.scdk.numblks;
                            }
							break;
						default:        /* unknown */
							NumBlks = 0;
							break;
					}
					bretval = true;
				}
				else
				{
					DebugPrintf(SV_LOG_ERROR, "ioctl IOCINFO Failed volumepath: %s, error: %s\n", vol.c_str(), strerror(errno));
				}
			}
			else
			{
				DebugPrintf(SV_LOG_WARNING, "@LINE %d in FILE %s unable to apply lock\n", __LINE__, __FILE__);
			}
		}
		else
		{
		   DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, invalid arguement vol = %p\n", 
				LINE_NO, FILE_NAME, vol.c_str());
		}
	}
	return bretval;
}

bool GetDeviceNameFromSymLink(std::string &deviceName)
{
	bool bretval = false;
	const std::string saveDeviceName = deviceName;
	struct stat64 st_buf;

	if (NULL == deviceName.c_str()) 
	{
		DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, no device name supplied\n", LINE_NO, FILE_NAME);
	}
	else if(stat64(deviceName.c_str(), &st_buf))
	{
		DebugPrintf(SV_LOG_WARNING, "@ LINE %d in FILE %s, stat failed on %s with errno = %d\n", LINE_NO, FILE_NAME, deviceName.c_str(), errno);
	} 
	else
	{
		/**
		*
		* Commenting below debug printf since it fills in the log file. 
		* Can be uncommented when needed.
		*
		*/
		/* DebugPrintf(SV_LOG_DEBUG, "ENTERED %s with deviceName = %s\n",FUNCTION_NAME, deviceName.c_str()); */
		const char UNIXROOTDIR = '/';
		const char UNIXDIRSEPARATOR = '/';

		// make sure device is valid
		struct stat64 volStat;

		// make sure device file exists
		if (0 == lstat64(deviceName.c_str(), &volStat)) 
		{
			if (S_ISLNK(volStat.st_mode)) 
			{
				ssize_t len;
				/* Not initialized since terminating finally with \0 */
				char symlnk[PATH_MAX + 1]; 
				do 
				{
					len = readlink(deviceName.c_str(), symlnk, sizeof(symlnk) - 1);
					if (-1 == len) 
					{
						/* This is the final device. so return true */
						if (0 == lstat64(deviceName.c_str(), &volStat)) 
						{
							bretval = true;
						}
						break;
					}
					symlnk[len] = '\0';
					if (UNIXROOTDIR != symlnk[0])
					{
						/* This is a relative path */
						char dirnameofdevice[PATH_MAX + 1] = {'\0'};
						SVERROR sv = DirName(deviceName.c_str(), dirnameofdevice);
						if (sv.failed())
						{
							DebugPrintf(SV_LOG_ERROR,"DirName failed @ LINE %d in FILE %s.\n", LINE_NO, FILE_NAME);
							break;
						}
						std::string catenatedlinkname = dirnameofdevice;
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
					{
						/* symlink is in absolute form */
						deviceName = symlnk;
					}
				} while (true);
			}
			else 
			{
				/* This is not a link file */
				bretval = true;
			}
		} 
	} /* else */

	if (!bretval)
	{
		DebugPrintf(SV_LOG_WARNING, "Unable to determine attributes of file  %s @LINE %d in FILE %s\n",saveDeviceName.c_str(),
			LINE_NO, FILE_NAME);
	}

	/**
	*
	* Commenting below debug printf since it fills in the log file. 
	* Can be uncommented when needed.
	*
	*/
	/* DebugPrintf(SV_LOG_DEBUG,"EXITING %s with deviceName = %s and bretval = %s\n",FUNCTION_NAME, deviceName.c_str(), bretval?"true":"false"); */
	return bretval;
}

bool IsReportingRealNameToCx(void)
{
	return false;
}

bool GetLinkNameIfRealDeviceName(const std::string sVolName, std::string &sLinkName)
{
	bool bretval = false;
	sLinkName = sVolName;
	//pending ... need to find in aix that whether the link name is sane as volume name.
	sLinkName = sVolName ;
	return true ;
}

std::string GetRawVolumeName(const std::string &dskname)
{
    std::string rdskname = dskname;
    boost::algorithm::replace_first(rdskname, "/dev/hdisk", "/dev/rhdisk");
    boost::algorithm::replace_first(rdskname, DMPDIR, RDMPDIR);
    boost::algorithm::replace_first(rdskname, DSKDIR, RDSKDIR);
    return rdskname;
}

bool IsProcessRunning(const std::string &sProcessName)
{
	bool rv = false;
    const char *procdir = "/proc";  /* standard /proc directory */
    psinfo_t info; /* process information structure from /proc */
    DIR *dirp;
    size_t pdlen;
    char psname[PSNAMELEN];
    struct dirent *dentp;
    int cnt = 0;
    do
    {
        if ((dirp = opendir(procdir)) == NULL)
        {
            DebugPrintf(SV_LOG_ERROR,"Failed Opening proc directory \n");
            rv = false;
            break;
        }

        (void) inm_strcpy_s(psname, ARRAYSIZE(psname), procdir);
        pdlen = strlen(psname);
        psname[pdlen++] = '/';
        /* for each active process --- */
        while (dentp = readdir(dirp))
        {
            int psfd;                           /* file descriptor for /proc/nnnnn/stat */

            if (dentp->d_name[0] == '.')        /* skip . and .. */
                continue;

            (void) inm_strcpy_s(psname + pdlen, (ARRAYSIZE(psname) - pdlen), dentp->d_name);
            (void) inm_strcat_s(psname, ARRAYSIZE(psname), "/psinfo");

            if ((psfd = open(psname, O_RDONLY)) == -1)
                continue;

            memset(&info, 0, sizeof info);
            ssize_t bytesread = read(psfd, &info, sizeof (info));
            (void) close(psfd);
            if (bytesread == sizeof(info))
            {
                if (0 == strcmp(sProcessName.c_str(), info.pr_fname))
                {
                    cnt++;
                    if (cnt > 1)
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

bool MountDevice(const std::string& device,const std::string& mountPoint,const std::string& fsType, bool bSetReadOnly, int &exitCode, 
                 std::string &errorMsg)
{
    bool bRet = true;
    do
    {
        if(mountPoint.empty() || device.empty())
        {
            std::stringstream strErr;
            strErr << "empty mountpoint specified.";
            strErr << "cannot mount device " << device << '\n';
            DebugPrintf(SV_LOG_ERROR,"%s", strErr.str().c_str());
            errorMsg = strErr.str();
            bRet = false;
            break;
        }

        std::string sFileSystem = fsType;
        if(sFileSystem.empty())
        {
            std::stringstream strErr;
            strErr << "the filesystem supplied to mount is empty.";
            strErr << "hence cannot mount device " << device << '\n';
            DebugPrintf(SV_LOG_ERROR,"%s", strErr.str().c_str());
            errorMsg = strErr.str();
            bRet = false;
            break;
        }
		sFileSystem = ToLower(sFileSystem);

        if(SVMakeSureDirectoryPathExists(mountPoint.c_str()).failed())
        {
            std::stringstream strErr;
            strErr << "failed to create mount point directory " << mountPoint
                << '\n';
            DebugPrintf(SV_LOG_ERROR,"%s", strErr.str().c_str());
            errorMsg = strErr.str();
            bRet = false;
            break;
        }

        std::string sMount = "";
        std::string mode; 
        std::string flags = "ro";
        if(!bSetReadOnly)
        {
            flags = "rw";
        }
        if(IsVolumeMounted(device, sMount, mode))
        {
            flags += ",remount";
        }

        std::string fsMount;
        fsMount = MOUNT_COMMAND;
        fsMount += " ";
        fsMount += OPTION_TO_SPECIFY_FILESYSTEM;
        fsMount += " ";
        fsMount += sFileSystem.c_str();
        fsMount += " ";
        fsMount += "-o ";
        fsMount += flags;
        fsMount += ",log=INLINE ";
        fsMount += " ";
        fsMount += device.c_str() ;
        fsMount += " ";
        fsMount += "\"";
        fsMount += mountPoint.c_str() ;
        fsMount += "\"";
        DebugPrintf(SV_LOG_DEBUG, "@ line %d in file %s the mount command formed is %s\n", LINE_NO, FILE_NAME, fsMount.c_str());
        int retires = 0;
        std::string fsck_replay;
        bool bfsck_replay_done = true;
        if(bSetReadOnly)
        {
            bfsck_replay_done = false;
            fsck_replay = FSCK_COMMAND;
            fsck_replay += " ";
            fsck_replay += FSCK_FS_OPTION ;
            fsck_replay += " -y ";
            fsck_replay += device.c_str() ;
        }
        while( retires < RETRIES_FOR_MOUNT)
        {
            DebugPrintf(SV_LOG_INFO, "executing %s...\n", fsMount.c_str());
            if(ExecuteInmCommand(fsMount, errorMsg, exitCode))
            {
                bRet = true;
                break;
            }
            else
            {
                bRet = false;
                DebugPrintf(SV_LOG_DEBUG, "mount %s with filesystem %s failed on %s with error message = %s. \n", device.c_str(), sFileSystem.c_str(), mountPoint.c_str(),  errorMsg.c_str());
                if(!bfsck_replay_done)
                {
                    DebugPrintf(SV_LOG_INFO, "executing %s...\n", fsck_replay.c_str());
                    if(!ExecuteInmCommand(fsck_replay, errorMsg, exitCode))
                    {
                        DebugPrintf(SV_LOG_INFO, "%s failed with exit code (%d) error message = %s.\n", fsck_replay.c_str(), exitCode, errorMsg.c_str());
                    }
                    bfsck_replay_done = true;  
                }
            }
            retires++;
        }
    } while(false);
    return bRet;
}

std::string GetExecdName(void)
{
	std::string ExecdName;    

	psinfo_t info;
	memset(&info, 0, sizeof info);
	pid_t pid = getpid();
	std::stringstream proccessfile;
	proccessfile << "/proc/" << pid << "/psinfo";
	int psfd;           
	do
	{
		if ((psfd = open(proccessfile.str().c_str(), O_RDONLY)) == -1)
			break;
		if (read(psfd, &info, sizeof (info)) == sizeof (info))
		{
			ExecdName = info.pr_fname;
		}
		(void) close(psfd);
	}while(false);
	return ExecdName;
}

bool IsOpenVzVM(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
	bool bisvm = false;
	return bisvm;
}

bool IsVMFromCpuInfo(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
	bool bisvm = false;
	return bisvm;
}

bool IsXenVm(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
	bool bisvm = false;
	return bisvm;
}

bool IsNativeVm(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
	return false;
}

void GetNicInfos(NicInfos_t & nicInfos)
{
	DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
	bool bretStatus = true;
	struct ifconf l_ifc;
	struct ifconf *ifc = &l_ifc;
    int bsz = sizeof(struct ifreq);
    int sd = -1;
    int prevsz=bsz;
    ifc->ifc_req = NULL;
    ifc->ifc_len = bsz;
    do
    {
		ifc->ifc_req = (struct ifreq *)realloc(ifc->ifc_req, bsz);
		if(!ifc->ifc_req)
		{
			DebugPrintf(SV_LOG_ERROR, "Malloc failed :\n");
			bretStatus = false;
			break; 
		}
        ifc->ifc_len=bsz;
        if (-1 != sd)
        {
            close(sd);
        }
	    sd = socket(PF_INET,SOCK_DGRAM,0);	   
        if(ioctl(sd, SIOCGIFCONF, (caddr_t)ifc)==-1)
	    {
		   DebugPrintf(SV_LOG_ERROR, "Error getting size of interface :\n");
		   bretStatus = false;
		   break; 
        }
        if(prevsz==ifc->ifc_len)
           break;
        else
		{
			INM_SAFE_ARITHMETIC(bsz *= InmSafeInt<int>::Type(2), INMAGE_EX(bsz))
			prevsz=(0 == ifc->ifc_len ? bsz : ifc->ifc_len);
        }		
	}while(true);
	if(bretStatus == true)
	{
		ifc->ifc_req=(struct ifreq *)realloc(ifc->ifc_req,prevsz);
		bretStatus = FillNicInfos(sd, ifc, sizeof(struct ifreq), nicInfos);
        close(sd);
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
	DebugPrintf( SV_LOG_DEBUG, "EXITING %s\n", FUNCTION_NAME);
}

bool FillNicInfos(const int &sockfd, struct ifconf *pifc, size_t sizeofifreq, NicInfos_t & nicInfos)
{
	DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    bool bcollect;
	bool bretStatus = true;
	char *cp, *cplim, addr[INET6_ADDRSTRLEN];

    std::set<std::string> dhcpenabledinterfaces;
    CollectDhcpEnabledInterfaces(dhcpenabledinterfaces);
    DebugPrintf(SV_LOG_DEBUG, "dhcp enabled interaces:\n");
    for (std::set<std::string>::const_iterator it = dhcpenabledinterfaces.begin(); it != dhcpenabledinterfaces.end(); it++)
    {
        DebugPrintf("%s\n", it->c_str());
    }
    E_INM_TRISTATE e;

	struct ifreq *ifr = pifc->ifc_req;
    cp = (char *)pifc->ifc_req;
    cplim = cp+pifc->ifc_len;
	for( ; cp < cplim;cp += (sizeof(ifr->ifr_name) + SIZE(ifr->ifr_addr)))
	{
        ifr = (struct ifreq *)cp;
		struct sockaddr *sa;
		sa = (struct sockaddr *)&(ifr->ifr_addr);
		if(sa->sa_family == AF_INET)
		{
			inet_ntop(AF_INET,(struct in_addr *)&(((struct sockaddr_in *)sa)->sin_addr),
			addr,INET_ADDRSTRLEN);
			bcollect = ShouldCollectNicInfo(sockfd, ifr, addr);
            if(bcollect)
            {
                std::string hardwareaddress;
                std::string netmask;
				GetNetIfParameters(ifr->ifr_name, hardwareaddress, netmask);
                if (hardwareaddress.empty())
                {
                    continue;
                }
                e = (dhcpenabledinterfaces.end() == dhcpenabledinterfaces.find(ifr->ifr_name)) ? 
                    E_INM_TRISTATE_FALSE : E_INM_TRISTATE_TRUE;
                InsertNicInfo(nicInfos, ifr->ifr_name, hardwareaddress, addr, netmask, e);
			}			
			else
			{
                DebugPrintf(SV_LOG_DEBUG, "Not collecting information for nic %s\n", ifr->ifr_name);
			}
		}		
    }	
	DebugPrintf( SV_LOG_DEBUG, "EXITING %s\n", FUNCTION_NAME);
	return bretStatus;
}


void CollectDhcpEnabledInterfaces(std::set<std::string> &dhcpenabledinterfaces)
{
    const char *file = "/etc/dhcpcd.ini";
    std::ifstream in(file);

    if (in.is_open())
    {
        const char COMMENTCHAR = '#';
        const char IFTOK[] = "interface";
        std::string line;
        std::string tok, ifc;

        while (!in.eof())
        {
            line.clear();
            tok.clear();
            ifc.clear();
            std::getline(in, line);
            if (!line.empty() && (COMMENTCHAR == line[0]))
            {
                continue;
            }
            std::stringstream ss(line);
            ss >> tok >> ifc;
            if ((tok == IFTOK) && !ifc.empty())
            {
                dhcpenabledinterfaces.insert(ifc);
            }
        }
        in.close();
    }
    else
    {
        DebugPrintf(SV_LOG_DEBUG, "could not open file %s to find dhcp enabled interfaces\n", file);
    }
}


void GetNetIfParameters(const std::string &ifrName, std::string& hardwareAddress, std::string &netmask)
{
    /* not ioctl to get hardware address */
    hardwareAddress = ifrName;

    int fd = socket(PF_INET, SOCK_DGRAM, 0);

    if (-1 == fd)
    {
        DebugPrintf(SV_LOG_DEBUG, "For getting hardware address for network interface %s, "
                                  "creating socked failed with error %s\n", ifrName.c_str(),
                                  strerror(errno));
        return;
    }

    struct ifreq ifr;
    inm_strncpy_s(ifr.ifr_name, ARRAYSIZE(ifr.ifr_name), ifrName.c_str(), IFNAMSIZ-1);

    if (0 == ioctl(fd, SIOCGIFNETMASK, &ifr))
    {
        char mask[INET_ADDRSTRLEN] = "\0";
        struct sockaddr_in *sa = (struct sockaddr_in *)&(ifr.ifr_addr);
        const char *prval = inet_ntop(AF_INET, (struct in_addr *)&(((struct sockaddr_in *)sa)->sin_addr), mask, sizeof mask);
        if (prval)
        {
            netmask = mask;
            DebugPrintf(SV_LOG_DEBUG, "Interface Name %s subnet mask %s\n", ifrName.c_str(), mask);
        }
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "For getting netmask for network interface %s, converting to dotted form failed\n", ifrName.c_str());
        }
    }
    else
    {
        DebugPrintf(SV_LOG_DEBUG, "For getting netmask for network interface %s, "
                                  "ioctl SIOCGIFNETMASK failed with error: %s\n", ifrName.c_str(),
                                  strerror(errno));
    }

    close(fd);
}


bool ShouldCollectNicInfo(const int &sockfd, struct ifreq *pifr, const char *ipaddr)
{
	DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    bool bshouldcollect = false;
    int ioctlrval = -1;
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
    SV_ULONGLONG space = ((SV_ULONGLONG)vfs.f_blocks) * ((SV_ULONGLONG)vfs.f_frsize);
    return space;
}

SV_ULONGLONG GetFreeFileSystemSpace(const struct statvfs64 &vfs)
{
    SV_ULONGLONG space = ((SV_ULONGLONG)vfs.f_bavail) * ((SV_ULONGLONG)vfs.f_frsize);
    return space;
}

MOUNT_FLAGS getPermissionType( std::string& optionsStr )
{
    MOUNT_FLAGS flag = MOUNT_UNKNOWN;
    std::string::size_type index = optionsStr.find_first_of(",");
    if( index != std::string::npos)
    {
       std::string permStr = optionsStr.substr(0, index);
       if(strcmp(permStr.c_str(), "ro") == 0)
       {
          flag = MOUNT_RO;
       }
       else if(strcmp(permStr.c_str(), "rw") == 0)
       {
          flag = MOUNT_RW;
       }
       else if(strcmp(permStr.c_str(), "rbr") == 0)
       {
          flag = MOUNT_RBR;
       }
       else if(strcmp(permStr.c_str(), "rbw") == 0)
       {
          flag = MOUNT_RBW;
       }
       else if(strcmp(permStr.c_str(), "rbrw") == 0)
       {
          flag = MOUNT_RBRW;
       }
     }
     return flag;
}

void dumpMountInfo(const std::vector<mount_info>& mountInfoList)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    DebugPrintf("  node       mounted        mounted over    vfs       date        options \n");
    DebugPrintf("===================================================================================");
    std::vector<mount_info>::const_iterator mountInfoListBegIter = mountInfoList.begin();
    std::vector<mount_info>::const_iterator mountInfoListiEndIter = mountInfoList.end();
    while(mountInfoListBegIter != mountInfoListiEndIter)
    {
         DebugPrintf("\n");
         DebugPrintf(" Node Name: %s \n Device Name: %s \n Mounted Over: %s \n FS: %s \n Date: %s \n Falg: %d \n Options: %s \n\n",  mountInfoListBegIter->m_nodeName.c_str(), mountInfoListBegIter->m_deviceName.c_str(), mountInfoListBegIter->m_mountedOver.c_str(), mountInfoListBegIter->m_vfs.c_str(), mountInfoListBegIter->m_date.c_str(), mountInfoListBegIter->m_flag, mountInfoListBegIter->m_options.c_str());
         mountInfoListBegIter++;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
}

std::string getFSString( int code)
{
	std::string fsType = "";
	switch(code)
	{
		case 0: fsType = "JFS2";
		break;
		case 1: fsType = "NAMEFS";
		break;
		case 2: fsType = "NFS";
		break;
		case 3: fsType = "JFS";
		break;
		case 5: fsType = "CDROM";
		break;
		case 6: fsType = "PROCFS";
		break;
		case 8: fsType = "USRVFS";
		break;
		case 16: fsType = "SFS";
		break;
		case 17: fsType = "CACHEFS";
		break;
		case 18: fsType = "NFS3";
		break;
		case 19: fsType = "AUTOFS";
		break;
		case 20: fsType = "POOLFS";
		break;
		case 31: fsType = "USRLAST";
		break;		
		case 32: fsType = "VXFS";
		break;		
		case 33: fsType = "VXODM";
		break;	
		case 34: fsType = "UDF";
		break;		
		case 35: fsType = "NFS4";
		break;		
		case 36: fsType = "RFS4";
		break;
		case 37: fsType = "CIFS";
		break;		
		case 38: fsType = "PMEMFS";
		break;		
		case 39: fsType = "AHAFS";
		break;
		case 40: fsType = "STNFS";
		break;		
		case 41: fsType = "ASMFS";
		break;
		case 47: fsType = "AIXLAST";
		break;		
		case -1: fsType = "BADVFS";
		break;	
	}
	return fsType;
}


std::string GetCustomizedAttributeValue(const std::string &name, const std::string &attribute, std::string &errmsg)
{
    std::string value;

    odm_initialize();

    struct CuAt stCuAt;
    std::string criteria;
    criteria = "name = \'";
    criteria += name;
    criteria += '\'';
    criteria += " and attribute = \'";
    criteria += attribute;
    criteria += '\'';
    struct CuAt *ret = (struct CuAt *)odm_get_obj(CuAt_CLASS, (char *)criteria.c_str(), &stCuAt, ODM_FIRST);
    if (ret == ( struct CuAt *)-1)
    {
        std::stringstream ss;
        ss << odmerrno;
        errmsg = "querying value from odm for name ";
        errmsg += name;
        errmsg += " and attribute ";
        errmsg += attribute;
        errmsg += " failed with odm error ";
        errmsg += ss.str();
    }
    else if (ret == 0)
    {
        errmsg = std::string("value does not exist in odm for name ") + name + " and attribute " + attribute;
    }
    else
    {
        value = stCuAt.value;
    }

    odm_terminate();

    return value;
}


std::string GetCustomizedDeviceParent(const std::string &device, std::string &errmsg)
{
    std::string parent;

    odm_initialize();

    struct CuDv stCuDv;
    std::string criteria;
    criteria = "name = \'";
    criteria += device;
    criteria += '\'';
    struct CuDv *ret = (struct CuDv *)odm_get_obj(CuDv_CLASS, (char *)criteria.c_str(), &stCuDv, ODM_FIRST);
    if (ret == ( struct CuDv *)-1)
    {
        std::stringstream ss;
        ss << odmerrno;
        errmsg = "querying parent from odm for ";
        errmsg += device;
        errmsg += " failed with odm error ";
        errmsg += ss.str();
    }
    else if (ret == 0)
    {
        errmsg = std::string("parent does not exist in odm for ") + device;
    }
    else
    {
        parent = stCuDv.parent;
    }

    odm_terminate();

    return parent;
}

/*
INPUT: pvname
OUTPUT: vgname and the return value indicate whether the vgexist or not
Description:
executing lspv <pvname>
-bash-4.2# lspv cli10in
PHYSICAL VOLUME:    cli10in                  VOLUME GROUP:     vg1
PV IDENTIFIER:      000d772cf1c91f57 VG IDENTIFIER     000d772c00004c0000000136f1c94160
PV STATE:           active
STALE PARTITIONS:   0                        ALLOCATABLE:      yes
PP SIZE:            8 megabyte(s)            LOGICAL VOLUMES:  2
TOTAL PPs:          524 (4192 megabytes)     VG DESCRIPTORS:   2
FREE PPs:           137 (1096 megabytes)     HOT SPARE:        no
USED PPs:           387 (3096 megabytes)     MAX REQUEST:      256 kilobytes
FREE DISTRIBUTION:  105..00..00..00..32
USED DISTRIBUTION:  00..105..104..105..73

*/

bool GetVgNameForPvIfExist(const std::string & pvname,std::string & vgname)
{
	bool rv = true;
	vgname.clear();
	do
	{
		std::string volstr("VOLUME");
		std::string grpstr("GROUP:");
		std::string vgcmdforpv = LISTALLPVSCMD;
		vgcmdforpv += pvname;
		vgcmdforpv += " 2>/dev/null";
			std::stringstream pvinfo;
		if(!executePipe(vgcmdforpv.c_str(),pvinfo))
		{
			rv = false;
			break;
		}

		if(!pvinfo.good() || pvinfo.eof())
		{
			rv = false;
			break;
		}
		std::string str1,str2,str3,str4,str5,str6;
		std::string line;
		std::getline(pvinfo,line);
		if(line.empty())
		{
			rv = false;
			break;
		}
		std::stringstream linestream(line) ;
		linestream >> str1 >> str2 >> str3 >> str4 >> str5 >> str6;
		if((str4 == volstr) && (str5 == grpstr))
		{
			vgname = str6;
		}
		else
		{
			rv = false;
			break;
		}

		if(vgname.empty())
		{
			rv = false;
			break;
		}		

	}while(false);

	return rv;
}


/*
INPUT: vgname
OUTPUT: list of lvs
Description:
Getting all the lv for the vg by executing "lsvg -l <vgname>"
-bash-4.2# lsvg -l mytest
mytest:
LV NAME             TYPE       LPs   PPs   PVs  LV STATE      MOUNT POINT
fsoratgtLV1         jfs2       386   386   1    closed/syncd  /fs/home/tgtmount1
fsoratgtlogLV1      jfs2log    1     1     1    closed/syncd  N/A

-bash-4.2#
*/

bool GetAllLvsForVg(const std::string & vgname,std::vector<std::string> & lvs)
{
	bool rv = true;
	do
	{
		std::stringstream lvlist;
		std::string lvcmd = LISTLVSFROMVGCMD;
		lvcmd += vgname;
		lvcmd += " 2>/dev/null";
		if(!executePipe(lvcmd.c_str(),lvlist))
		{
			rv = false;
			break;
		}
		int linecnt = 0;
		while(!lvlist.eof() )
		{
			std::string line;
			std::getline(lvlist, line);
			++linecnt;
			if(linecnt > 2)
			{
				if(line.empty())
					break;
				std::stringstream linestream(line) ;
				std::string str ;
				linestream >> str ;
				if( str.empty() )
				{
					break ;
				}
				lvs.push_back(str);
			}
		}
	}while(false);
	return rv;
}

/*
INPUT: list of lvs
Description:
Calling hide drive for each of the lvs in the list
*/
bool HideAllLvs(std::vector<std::string> & lvs,std::string& output, std::string& error)
{
	bool rv = true;
	std::vector<std::string>::iterator lviter = lvs.begin();
	for(;lviter != lvs.end(); ++lviter)
	{
		std::string lvname = AIX_DEVICE_DIR;
		lvname += (*lviter);
		if(HideDrive(lvname.c_str(),"",output,error,false).failed())
		{
			rv = false;
			break;
		}
	}

	return rv;
}

/*
INPUT: Device
Description:
get the vgname
get all lvs to the vgname
unmount all lvs
deactivate the vg
export the vg
*/

bool RemoveVgWithAllLvsForVsnap(const std::string & device,std::string& output, std::string& error,bool needvgcleanup)
{
	bool rv = true;
	std::string pvname = basename_r(device.c_str());
	do
	{
		std::string vgname;
		std::vector<std::string> lvs;
		if(!GetVgNameForPvIfExist(pvname,vgname))
		{
			break;
		}

		if(!GetAllLvsForVg(vgname,lvs))
		{
			rv = false;
			error = "Unable to get the logivcal volume information for the volume group ";
			error += vgname;
			error += "\n";
			break;
		}
		if(!HideAllLvs(lvs,output,error))
		{
			rv = false;
			break;
		}

		DebugPrintf(SV_LOG_INFO,"Deactivating the VG %s\n",vgname.c_str());
		std::stringstream result;
		std::string command = DEACTIVATEVGCMD;
		command += vgname;
		if(!executePipe(command.c_str(),result))
		{
			DebugPrintf(SV_LOG_ERROR,
				"Function %s @LINE %d in FILE %s :failed to remove VG %s\n",
				FUNCTION_NAME, LINE_NO, FILE_NAME,vgname.c_str());
			error = "Failed to execute the command ";
			error += command;
			error += command;
			rv = false;
			break;
		}

		if(needvgcleanup)
		{
			DebugPrintf(SV_LOG_INFO,"Exporting the VG %s\n",vgname.c_str());
			command = CLEANUPVGANDLVSCMD;
			command += vgname;
			if(!executePipe(command.c_str(),result))
			{
				DebugPrintf(SV_LOG_ERROR,
					"Function %s @LINE %d in FILE %s :failed to export VG %s\n",
					FUNCTION_NAME, LINE_NO, FILE_NAME,vgname.c_str());
				error = "Failed to execute the command ";
				error += command;
				rv = false;
				break;
			}
		}

	}while(false);
	return rv;
}

/*
INPUT: Devicename
Description: This function will delete the stalevg for the vsnap 
and recreate the vg in case vsnap remount

-bash-4.2# /usr/sbin/lqueryvg -tAp cli10in
Max LVs:        256
PP Size:        23
Free PPs:       137
LV count:       2
PV count:       1
Total VGDAs:    2
Conc Allowed:   0
MAX PPs per PV  1016
MAX PVs:        32
Conc Autovaryo  0
Varied on Conc  0
Logical:        000d772c00004c0000000136f1c94160.1   fsoratgtLV1 1
000d772c00004c0000000136f1c94160.2   fsoratgtlogLV1 1
Physical:       000d772cf1c91f57                2   0
Total PPs:      524
LTG size:       128
HOT SPARE:      0
AUTO SYNC:      0
VG PERMISSION:  0
SNAPSHOT VG:    0
IS_PRIMARY VG:  0
PSNFSTPP:       4352
VARYON MODE:    0
VG Type:        0
Max PPs:        32512
-bash-4.2# /usr/sbin/getlvodm -t 000d772c00004c0000000136f1c94160
vg1
-bash-4.2#

*/


bool CleanStaleVgAndRecreateVgForVsnapOnReboot(const std::string & devname)
{
	bool rv = true;
	std::string pvname = basename_r(devname.c_str());
	do
	{
		std::string cmd = QUERYVGFROMPVCMD;
		cmd += pvname;
		cmd += " 2>/dev/null | grep '^Logical:' ";
		std::stringstream out;
		if(!executePipe(cmd.c_str(),out))
		{
			break;
		}
		std::string tag, id, lv,value;
		if(out.good() && !out.eof())
		{			
			out >> tag >> id >> lv >> value;
		}
		if(tag.empty() || id.empty())
		{
			break;
		}
		if(tag != "Logical:")
		{
			break;
		}
		std::string::size_type idx = id.rfind(".");
		id.erase(idx,id.length());
		cmd = VGNAMEFROMVGIDCMD;
		cmd += id;
		cmd += " 2>/dev/null";
		std::string vgname;
		out.str("");
		if(!executePipe(cmd.c_str(),out))
		{
			break;
		}
		if(out.good() && !out.eof())
		{			
			out >> vgname;
		}
		if(vgname.empty())
		{
			break;
		}
		out.str("");
		cmd = DEACTIVATEVGCMD;
		cmd += vgname;
		executePipe(cmd.c_str(),out);
		cmd = CLEANUPVGANDLVSCMD;
		cmd += vgname;
		executePipe(cmd.c_str(),out);
		out.str("");
		cmd = RECREATEVGCMD;
		cmd += vgname;
		cmd += " ";
		cmd += " -Y NA -L / ";
		cmd += pvname;
		if(!executePipe(cmd.c_str(),out))
		{
			DebugPrintf(SV_LOG_ERROR,
				"Function %s @LINE %d in FILE %s :failed to execute %s\n",
				FUNCTION_NAME, LINE_NO, FILE_NAME,cmd.c_str());
			rv = false;
			break;
		}

	}while(false);
	return rv;
}
