#ifndef INMAGE_VOLINFO_COLLECTOR_H
#define INMAGE_VOLINFO_COLLECTOR_H

#include <stdio.h>
#include <vector>
#include <string>
//#include "logger.h"
#include "portable.h"        // todo: remove dependency on common
#include "volumegroupsettings.h"
#include "configwrapper.h"

#ifdef SV_WINDOWS
typedef unsigned int dev_t;
#endif

#define DEFAULT_SECTOR_SIZE        (512)

typedef struct volinfo_tag
{
//public:
    volinfo_tag() : systemvol(false), mounted(false), cachedirvol(false),locked(false),containpagefile(false),voltype(VolumeSummary::UNKNOWN_DEVICETYPE),capacity(0ULL),freespace(0ULL), rawcapacity(0ULL), devno(0), sectorsize(DEFAULT_SECTOR_SIZE), vendor(VolumeSummary::UNKNOWN_VENDOR), writecachestate(VolumeSummary::WRITE_CACHE_DONTKNOW), formatlabel(VolumeSummary::LABEL_UNKNOWN), volumegroupvendor(VolumeSummary::UNKNOWN_VENDOR) {};
    std::string devname;
    std::string mountpoint;
    std::string fstype;
    dev_t devno;
    bool mounted;
    bool systemvol;
    bool cachedirvol;
	bool containpagefile;
    bool locked;
    VolumeSummary::Devicetype  voltype;
    unsigned int  sectorsize;
	unsigned long long rawcapacity;
    unsigned long long capacity;
    unsigned long long freespace;
	std::string volumelabel;
	std::string deviceid;
    VolumeSummary::Vendor vendor;
    VolumeSummary::WriteCacheState writecachestate;
	VolumeSummary::FormatLabel formatlabel;
    VolumeSummary::Vendor volumegroupvendor;
    std::string volumegroupname;
	Attributes_t attributes;

    std::string GetAttribute(Attributes_t const& attrs,
        const char *key,
        bool throwIfNotFound)
    {
        ConstAttributesIter_t iter = attrs.find(key);
        if (iter != attrs.end())
        {
            return iter->second;
        }

        if (throwIfNotFound)
        {
            std::stringstream err;
            err << "Error: no attribute found for key "
                << key
                << ".";

            throw std::runtime_error(err.str().c_str());
        }

        return std::string("");
    }

}volinfo;

typedef std::map<std::string, volinfo> DiskNamesToDiskVolInfosMap;
typedef std::map<std::string, std::string> BitlockerProtectionStatusMap;
typedef std::map<std::string, std::string> BitlockerConversionStatusMap;

class volinfocollector
{

public:

    volinfocollector(){};
    ~volinfocollector(){};

    virtual void getvolumeinfo(std::vector<volinfo> &vinfo) = 0;

    void display_devlist(std::vector<volinfo> &vinfo)
    {
        for(unsigned int i = 0; i < vinfo.size(); i++)
        {
            volinfo di = vinfo[i];
    
            DebugPrintf(SV_LOG_DEBUG, "\n---- VOLUMEINFO OF %s ----    \n", di.devname.c_str());
    
            DebugPrintf(SV_LOG_DEBUG, "device            %s \n", di.devname.c_str());
            DebugPrintf(SV_LOG_DEBUG, "label             %s \n", di.volumelabel.c_str());
            DebugPrintf(SV_LOG_DEBUG, "mount point       %s \n", di.mountpoint.c_str());
            DebugPrintf(SV_LOG_DEBUG, "fstype            %s \n", di.fstype.c_str());
            DebugPrintf(SV_LOG_DEBUG, "mounted            %s \n", di.mounted ? "[MOUNTED]" : "[UNMOUNTED]");
            DebugPrintf(SV_LOG_DEBUG, "sysvol            %s \n", di.systemvol ? "[SYSVOL]" : "[NONSYSVOL]");
            DebugPrintf(SV_LOG_DEBUG, "devno            0x%x \n", di.devno);
            DebugPrintf(SV_LOG_DEBUG, "capacity          %I64u \n", di.capacity);
            DebugPrintf(SV_LOG_DEBUG, "sectorsize        0x%x \n", di.sectorsize);
            DebugPrintf(SV_LOG_DEBUG, "raw size          %I64u \n", di.rawcapacity);
			DebugPrintf(SV_LOG_DEBUG, "Write Cache State %s\n", StrWCState[di.writecachestate]);
			DebugPrintf(SV_LOG_DEBUG, "format Label      %s\n", StrFormatLabel[di.formatlabel]);
			PrintAttributes(di.attributes);
            //printf("devname = %s  mountpt = %s%s%s fstype = %s  devno = 0x%x \n", di.devname.c_str(), di.mountpoint.c_str(), di.mounted ? "[MOUNTED]" : "", di.systemvol ? "[SYSVOL]" : "", di.fstype.c_str(), di.devno);
        }
    }

};

#endif /* INMAGE_VOLINFO_COLLECTOR_H */

