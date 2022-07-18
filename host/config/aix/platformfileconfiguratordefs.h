#ifndef PLATFORM_FILECONFIGURATORDEFS_H
#define PLATFORM_FILECONFIGURATORDEFS_H

#define DEFAULT_VM_CMDS_FOR_PATS ""
#define DEFAULT_VM_PATS ""
#define DEFAULT_VM_CMDS ""

/*
#define DEFAULT_VM_CMDS_FOR_PATS "/usr/sbin/smbios,"\
                                 "/usr/sbin/prtdiag,"\
                                 "/usr/sbin/prtconf"

#define DEFAULT_VM_PATS "Manufacturer: VMware,"\
                        "Manufacturer: Microsoft Corporation,"\
                        "Manufacturer: innotek GmbH"
*/

/* seems to be in "/etc" */
#define POWERMTCMD "/etc/powermt"
#define VXDMPCMD "/usr/sbin/vxdmpadm"
#define VXDISKCMD "/usr/sbin/vxdisk"
#define VXVSETCMD "/usr/sbin/vxvset"
#define GREPCMD  "/usr/bin/grep"
#define AWKCMD  "/usr/bin/awk"

#endif /* PLATFORM_FILECONFIGURATORDEFS_H */
