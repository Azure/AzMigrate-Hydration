#ifndef PLATFORM_FILECONFIGURATORDEFS_H
#define PLATFORM_FILECONFIGURATORDEFS_H

#define DEFAULT_VM_CMDS_FOR_PATS ""
#define DEFAULT_VM_PATS ""
#define DEFAULT_VM_CMDS ""
/*
#define DEFAULT_VM_CMDS_FOR_PATS "dmidecode"
#define DEFAULT_VM_PATS "Manufacturer: VMware,"\
                        "Manufacturer: Microsoft Corporation,"\
                        "Manufacturer: innotek GmbH"
*/

#define POWERMTCMD "/sbin/powermt"
#define VXDMPCMD "/sbin/vxdmpadm"
#define VXDISKCMD "/sbin/vxdisk"
#define VXVSETCMD "/sbin/vxvset"
#define GREPCMD  "grep"
#define AWKCMD  "awk"

#endif /* PLATFORM_FILECONFIGURATORDEFS_H */
