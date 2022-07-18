/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File        :   LinuxConstants.h

Description :   String constants & type definitions

History     :   29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/

#ifndef AZURE_RECOVERY_LINUX_CONSTANTS_H
#define AZURE_RECOVERY_LINUX_CONSTANTS_H

#define TRIM_SEQUENCE_CHARS       " \t\f\v\n\r"

namespace AzureRecovery
{
    const char ROOT_MOUNT_POINT[]           = "/";
    const char ETC_MOUNT_POINT[]            = "/etc";
    const char BOOT_MOUNT_POINT[]           = "/boot";
    const char USR_MOUNT_POINT[]            = "/usr";
    const char VAR_MOUNT_POINT[]            = "/var";
    const char USR_LOCAL_MOUNT_POINT[]      = "/usr/local";
    const char OPT_MOUNT_POINT[]            = "/opt";
    const char HOME_MOUNT_POINT[]           = "/home";
    const char EFI_MOUNT_POINT[]            = "/boot/efi";
    const char SOURCE_OS_MOUNT_POINT_ROOT[] = "/mnt/";
    const char DEV_MAPPER_PREFIX[]          = "/dev/mapper/";
    const char SMS_AZURE_CHROOT[]           = "/mnt/sms_azure_chroot";
    const char SMS_AZURE_CHROOT_FSTAB[]     = "/mnt/sms_azure_chroot/etc/fstab";
    const char SMS_AZURE_CHROOT_FSTAB_NEW[] = "/mnt/sms_azure_chroot/etc/fstab.new";
    const char SMS_AZURE_CHROOT_FSTAB_BCK[] = "/mnt/sms_azure_chroot/etc/fstab.azure_sms_bck";
    const char SMS_AZURE_BLKID_OUT[]        = "/mnt/sms_azure_chroot/etc/azure_sms_blkid.out";
    const char SMS_AZURE_TMP_MNT[]          = "/mnt/sms_azure_temp_mnt";
    
    const char AZURE_REC_PREP_TGT_CONF_EXT[] = ".PrepareTargetFile";
    const char AZURE_REC_SRC_TGT_MAP_CONF_EXT[] = ".PROTECTED_DISKS_LIST";
    
    const char AZURE_REC_CONF_DELEMETER[]   = "!@!@!";
    
    const char PREPARE_OS_FOR_AZURE_SCRIPT[]= "prepare-os-for-azure.sh";
    const char AZURE_PRE_RECOVERY_SCRIPT[]  = "post-rec-script.sh";
    const char AZURE_RECOVRY_PREPARE_TGT[]  = "PrepareTarget.sh";
    const char AZURE_OS_DETAILS_TGT[]       = "OS_details_target.sh";
    const char MOUNT_FSTAB[]                = "MountFS.sh";
    
    namespace SystemPartitionName
    {
        const char Root[] = "root";
        const char Boot[] = "boot";
        const char Etc[]  = "etc";
    }
    
    namespace MobilityAgents
    {
        const char VXAgent[] = "vxagent";
        const char UARespawn[] = "uarespawnd";
        const char FXAgent[] = "svagent";
    }
    
    namespace CmdLineTools
    {
        const char LSSCSI[]     = "lsblk -Snp -o HCTL,TYPE,VENDOR,MODEL,REV,NAME";
        const char VgScan[]     = "vgscan";
        const char RescanDisk[] = "blockdev --rereadpt";
        const char BlockDevFlushBuff[] = "blockdev --flushbufs";
        const char ActivateVG[]   = "vgchange -ay";
        const char DeActivateVG[] = "vgchange -an";
        const char VGMkNodes[]  = "vgmknodes";
        const char Mount[]      = "mount";
        const char UMount[]     = "umount";
        const char DmSetupfRemove[] = "dmsetup remove -f";
        const char DMesgTail[] = "dmesg | tail";
        const char Lsblk[] = "lsblk -Jp -o NAME,UUID,PARTFLAGS,MOUNTPOINT,LABEL,FSTYPE,TYPE";
        const char ListVGs[] = "vgs";
        const char FindMnt[] = "findmnt -R";
        const char BtrfsListSubVols[] = "btrfs subvolume list";
    }
    
    namespace LinuxConfileName
    {
        const char lvm[]     = "/etc/lvm/lvm.conf";
        const char lvm_tmp[] = "/etc/lvm/lvm.conf.temp";
        const char lvm_org[] = "/etc/lvm/lvm.conf.org";
        const char waagent[] = "/etc/waagent.conf";
    }

    namespace PrepareForAzureScript
    {
        const char ERROR_DATA_LINE_BEGIN[] = "[Sms-Scrip-Error-Data]:";
        const char TELEMETRY_DATA_LINE_BEGIN[] = "[Sms-Telemetry-Data]:";

        const unsigned int E_INTERNAL = 1;
        const unsigned int E_SCRIPT_SYNTAX_ERROR = 2;
        const unsigned int E_TOOLS_MISSING = 3;
        const unsigned int E_ARGS = 4;
        const unsigned int E_OS_UNSUPPORTED = 5;
        const unsigned int E_CONF_MISSING = 6;
        const unsigned int E_INITRD_IMAGE_GENERATION_FAILED = 7;
        const unsigned int E_HV_DRIVERS_MISSING = 8;
        const unsigned int E_AZURE_GA_INSTALLATION_FAILED = 9;
        const unsigned int E_ENABLE_DHCP_FAILED = 10;
    }

    // TODO: Make it as config setting.
    const char SupportedLinuxDistros[] = "OL6,OL7,OL8,RHEL6,RHEL7,RHEL8,RHEL9,CENTOS6,CENTOS7,CENTOS8,CENTOS9,SLES11,SLES12,SLES15,UBUNTU14,UBUNTU16,UBUNTU18,UBUNTU19,UBUNTU20,UBUNTU21,UBUNTU22,DEBIAN,KALI";
}
#endif // ~AZURE_RECOVERY_LINUX_CONSTANTS_H
