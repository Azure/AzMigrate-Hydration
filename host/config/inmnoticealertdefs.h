#ifndef INM_NOTICE_ALERT_DEFS_H
#define INM_NOTICE_ALERT_DEFS_H

#include <sstream>
#include <boost/lexical_cast.hpp>
#include "inmnoticealertimp.h"
#include "inmnoticealertnumbers.h"
#include "svtypes.h"


class RetentionDiskUsageAlert : public InmNoticeAlertImp
{
public:
    RetentionDiskUsageAlert(const std::string &mntpnt, const SV_UINT &usedpercent,
                            const SV_UINT &thresholdpercent)
    {
        Parameters_t p;
        p["RetentionFolderName"] = mntpnt;
        p["UsedPercentage"] = boost::lexical_cast<std::string>(usedpercent);
        p["ThresholdPercentage"] = boost::lexical_cast<std::string>(thresholdpercent);
    
        std::stringstream m;
        m << "Disk usage on retention folder "
          << mntpnt
          << " now at "
          << usedpercent
          << "%, alert threshold is configured at "
          << thresholdpercent
          << "%";

        SetDetails(E_RETENTION_DISK_USAGE_ALERT, p, m.str());
    }
};


class VsnapUnmountOnPruneAlert : public InmNoticeAlertImp
{
public:
    VsnapUnmountOnPruneAlert(const std::string &name, const std::string &targetname)
    {
        Parameters_t p;
        p["DeviceName"] = name;
        p["VolumeName"] = targetname;

        std::stringstream m;
        m << "Vsnap: " << name
          << " is unmounted because of retention pruning, "
          << "on target volume "
          << targetname;

        SetDetails(E_VSNAP_UNMOUNT_ON_PRUNE_ALERT, p, m.str());
    }
};


class VsnapUnmountAlert : public InmNoticeAlertImp
{
public:
    VsnapUnmountAlert(const std::string &name, const std::string &targetname)
    {
        Parameters_t p;
        p["DeviceName"] = name;
        p["VolumeName"] = targetname;

        std::stringstream m;
        m << "Vsnap: " << name
          << " is unmounted on target volume "
          << targetname;

        SetDetails(E_VSNAP_UNMOUNT_ALERT, p, m.str());
    }
};


class VsnapUnmountOnCoalesceAlert : public InmNoticeAlertImp
{
public:
    VsnapUnmountOnCoalesceAlert(const std::string &name, const std::string &targetname)
    {
        Parameters_t p;
        p["DeviceName"] = name;
        p["VolumeName"] = targetname;

        std::stringstream m;
        m << "Vsnap: " << name
          << " is unmounted because of retention coalesce, "
          << "on target volume "
          << targetname;

        SetDetails(E_VSNAP_UNMOUNT_ON_COALESCE_ALERT, p, m.str());
    }
};


class RetentionDirLowSpaceAlert : public InmNoticeAlertImp
{
public:
    RetentionDirLowSpaceAlert(const std::string &dir, const SV_ULONGLONG &free)
    {
        Parameters_t p;
        p["RetentionFolderName"] = dir;
        p["FreeSpace"] = boost::lexical_cast<std::string>(free);

        std::stringstream m;
        m << "Running out of free space on retention folder :"
          << dir << ". "
          << "Free Space : " << free << " bytes";

        SetDetails(E_RETENTION_DIR_LOW_SPACE_ALERT, p, m.str());
    }
};


class RetentionDirFreedUpAlert : public InmNoticeAlertImp
{
public:
    RetentionDirFreedUpAlert(const std::string &dir)
    {
        Parameters_t p;
        p["RetentionFolderName"] = dir;

        std::stringstream m;
        m << "Running out of free space on retention folder "
          << dir
          << ". Retention folder was cleaned to free up space";

        SetDetails(E_RETENTION_DIR_FREED_UP_ALERT, p, m.str());
    }
};


class VsnapDeleteAlert : public InmNoticeAlertImp
{
public:
    VsnapDeleteAlert(const std::string &mountpoint, const std::string &name, const std::string &targetname)
    {
        Parameters_t p;
        p["MountPointName"] = mountpoint;
        p["DeviceName"] = name;
        p["VolumeName"] = targetname;

        std::stringstream m;
        m << "Deleting VSNAP " << mountpoint << " ("
          << name << ") of Target Volume " << targetname;

        SetDetails(E_VSNAP_DELETE_ALERT, p, m.str());
    }
};


class RetentionPurgedAlert : public InmNoticeAlertImp
{
public:
    RetentionPurgedAlert(const std::string &dir)
    {
        Parameters_t p;
        p["RetentionFolderName"] = dir;

        std::stringstream m;
        m << "Running out of free space on retention folder "
          << dir
          << ". Retention data was purged to free up space";

        SetDetails(E_RETENTION_PURGED_ALERT, p, m.str());
    }
};

#endif /* INM_NOTICE_ALERT_DEFS_H */
