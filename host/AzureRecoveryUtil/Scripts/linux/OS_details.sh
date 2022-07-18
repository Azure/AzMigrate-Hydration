#!/bin/bash


case `uname` in
  Linux)
        if [ -f /etc/enterprise-release ] && [ -f /etc/redhat-release ] ; then
            if grep -q 'Enterprise Linux Enterprise Linux Server release 5.* (Carthage)' /etc/enterprise-release; then 
                if uname -a | grep -q i[0-9]86; then
                    OS="OL5-32"
                else
                    OS="OL5-64"
                fi
            fi
        elif [ -f /etc/oracle-release ] && [ -f /etc/redhat-release ] ; then
            if grep -q 'Oracle Linux Server release 6.*' /etc/oracle-release; then
                if uname -a | grep -q i[0-9]86; then
                    OS="OL6-32"
                else
                    OS="OL6-64"
                fi
            fi
            if grep -q 'Oracle Linux Server release 7.*' /etc/oracle-release; then
                if uname -a | grep -q i[0-9]86; then
                    OS="OL7-32"
                else
                    OS="OL7-64"
                fi
            fi
        elif [ -f /etc/redhat-release ]; then
            if grep -q 'Red Hat Enterprise Linux [AE]S release 4 (Nahant Update [4-9])' /etc/redhat-release || \
                grep -q 'CentOS release 4.[4-9] (Final)' /etc/redhat-release; then
                if uname -a | grep -q i[0-9]86; then
                    OS="RHEL4-32"
                else
                    OS="RHEL4-64"
                fi            
            elif grep -q 'Red Hat Enterprise Linux Server release 5.* (Tikanga)' /etc/redhat-release || \
                grep -q 'CentOS release 5.* (Final)' /etc/redhat-release; then
                if uname -a | grep -q i[0-9]86; then
                        OS="RHEL5-32"
                else
                        OS="RHEL5-64"
                fi
            elif grep -q 'Red Hat Enterprise Linux Server release 6.* (Santiago)' /etc/redhat-release || \
                grep -q 'CentOS Linux release 6.* (Final)' /etc/redhat-release || \
                grep -q 'CentOS release 6.* (Final)' /etc/redhat-release; then
                if uname -a | grep -q i[0-9]86; then
                    # Don't assign OS value if OS is RHEL6U3-32/CentOS6.3-32 as we don't support agent on it.
                    if grep -q 'Red Hat Enterprise Linux Server release 6.3 (Santiago)' /etc/redhat-release || \
                        grep -q 'CentOS Linux release 6.3 (Final)' /etc/redhat-release || \
                        grep -q 'CentOS release 6.3 (Final)' /etc/redhat-release; then
                        :
                    else
                        OS="RHEL6-32"
                    fi
                else
                    OS="RHEL6-64"
                    if grep -q 'CentOS release 6.4 (Final)' /etc/redhat-release || grep -q 'Red Hat Enterprise Linux Server release 6.4 (Santiago)' /etc/redhat-release ; then
                        if echo `uname -r`| grep -E '2.6.32-358|2.6.32-358.23.2' >/dev/null 2>&1 ; then
                            OS="RHEL6U4-64"
                        fi
                    fi
                fi
            elif grep -q 'XenServer release 4.1.0-7843p (xenenterprise)' /etc/redhat-release; then
                if uname -a | grep -q i[0-9]86; then
                    OS="CITRIX-XEN-4.1.0-7843p-32"
                else
                    OS="CITRIX-XEN-4.1.0-7843p-64"
                fi            
            elif grep -q 'XenServer release 5.0.0-10918p (xenenterprise)' /etc/redhat-release; then
                if uname -a | grep -q i[0-9]86; then
                    OS="CITRIX-XEN-5.0.0-10918p-32"
                else
                    OS="CITRIX-XEN-5.0.0-10918p-64"
                fi
            elif grep -q 'XenServer release 4.0.1-4249p (xenenterprise)' /etc/redhat-release; then
                if uname -a | grep -q i[0-9]86; then
                    OS="CITRIX-XEN-4.0.1-4249p-32"
                else
                    OS="CITRIX-XEN-4.0.1-4249p-64"
                fi
            elif grep -q 'XenServer release 5.5.0-15119p (xenenterprise)' /etc/redhat-release ||
                grep -q 'XenServer SDK release 5.5.0-15119p (xenenterprise)' /etc/redhat-release ; then
                if uname -a | grep -q i[0-9]86; then
                    OS="CITRIX-XEN-5.5.0-15119p-32"
                else
                    OS="CITRIX-XEN-5.5.0-15119p-64"
                fi
            elif grep -q 'XenServer release 5.5.0-25727p (xenenterprise)' /etc/redhat-release ||
                grep -q 'XenServer SDK release 5.5.0-25727p (xenenterprise)' /etc/redhat-release ; then
                if uname -a | grep -q i[0-9]86; then
                    OS="CITRIX-XEN-5.5.0-25727p-32"
                else
                    OS="CITRIX-XEN-5.5.0-25727p-64"
                fi            
            elif grep -q 'XenServer release 5.6.0-31188p (xenenterprise)' /etc/redhat-release ||
                grep -q 'XenServer SDK release 5.6.0-31188p (xenenterprise)' /etc/redhat-release ; then
                if uname -a | grep -q i[0-9]86; then
                    OS="CITRIX-XEN-5.6.0-31188p-32"
                else
                    OS="CITRIX-XEN-5.6.0-31188p-64"
                fi 
            elif grep -q 'XenServer release 5.6.100-39265p (xenenterprise)' /etc/redhat-release ||
                grep -q 'XenServer SDK release 5.6.100-39265p (xenenterprise)' /etc/redhat-release ; then
                if uname -a | grep -q i[0-9]86; then
                    OS="CITRIX-XEN-5.6.100-39265p-32"
                fi            
            elif grep -q 'XenServer release 5.6.100-47101p (xenenterprise)' /etc/redhat-release ||
                grep -q 'XenServer SDK release 5.6.100-47101p (xenenterprise)' /etc/redhat-release ; then
                if uname -a | grep -q i[0-9]86; then
                    OS="CITRIX-XEN-5.6.100-47101p-32"
                fi
            elif grep -q 'XenServer release 6.0.0-50762p (xenenterprise)' /etc/redhat-release ||
                grep -q 'XenServer SDK release 6.0.0-50762p (xenenterprise)' /etc/redhat-release ; then
                if uname -a | grep -q i[0-9]86; then
                    OS="CITRIX-XEN-6.0.0-50762p-32"
                fi            
            elif grep -q 'XenServer release 6.0.2-53456p (xenenterprise)' /etc/redhat-release ||
                grep -q 'XenServer SDK release 6.0.2-53456p (xenenterprise)' /etc/redhat-release ; then
                if uname -a | grep -q i[0-9]86; then
                    OS="CITRIX-XEN-6.0.2-53456p-32"
                fi            
            elif grep -q 'XenServer release 6.1.0-59235p (xenenterprise)' /etc/redhat-release ||
                grep -q 'XenServer SDK release 6.1.0-59235p (xenenterprise)' /etc/redhat-release ; then
                if uname -a | grep -q i[0-9]86; then
                    OS="CITRIX-XEN-6.1.0-59235p-32"
                fi
            fi
        elif [ -f /etc/SuSE-release ] && grep -q 'VERSION = 10' /etc/SuSE-release && ! grep -q PATCHLEVEL /etc/SuSE-release ; then	
            # This one is for SLES10 or OpenSuSE10 series
            if grep -q "SUSE LINUX 10.0" /etc/SuSE-release; then
                if uname -a | grep -q i[0-9]86; then
                    OS="OPENSUSE10-32"
                else
                    OS="OPENSUSE10-64"
                fi
            elif grep -q "SUSE Linux Enterprise Server 10" /etc/SuSE-release; then
                if uname -a | grep -q i[0-9]86; then
                    OS="SLES10-32"
                elif uname -a | grep -q x86_64; then
                    OS="SLES10-64"
                fi
            fi
        elif [ -f /etc/SuSE-release ] && grep -q 'VERSION = 10' /etc/SuSE-release && grep -q "SUSE Linux Enterprise Server 10" /etc/SuSE-release && grep -q 'PATCHLEVEL = 1' /etc/SuSE-release; then
            if uname -a | grep -q i[0-9]86; then
                OS="SLES10-SP1-32"
            elif uname -a | grep -q x86_64; then
                OS="SLES10-SP1-64"
            fi
        
        elif [ -f /etc/SuSE-release ] && grep -q 'VERSION = 10' /etc/SuSE-release && grep -q "SUSE Linux Enterprise Server 10" /etc/SuSE-release && grep -q 'PATCHLEVEL = 2' /etc/SuSE-release; then
            if uname -a | grep -q i[0-9]86; then
                OS="SLES10-SP2-32"
            elif uname -a | grep -q x86_64; then
                OS="SLES10-SP2-64"
            fi
        elif [ -f /etc/SuSE-release ] && grep -q 'VERSION = 10' /etc/SuSE-release && grep -q "SUSE Linux Enterprise Server 10" /etc/SuSE-release && grep -q 'PATCHLEVEL = 3' /etc/SuSE-release; then
            if uname -a | grep -q i[0-9]86; then
                OS="SLES10-SP3-32"
            elif uname -a | grep -q x86_64; then
                OS="SLES10-SP3-64"
            fi
        elif [ -f /etc/SuSE-release ] && grep -q 'VERSION = 10' /etc/SuSE-release && grep -q "SUSE Linux Enterprise Server 10" /etc/SuSE-release && grep -q 'PATCHLEVEL = 4' /etc/SuSE-release; then
            if uname -a | grep -q i[0-9]86; then
                    OS="SLES10-SP4-32"
            elif uname -a | grep -q x86_64; then
                    OS="SLES10-SP4-64"
            fi
        elif [ -f /etc/SuSE-release ] && grep -q 'VERSION = 10' /etc/SuSE-release && \
            grep -q 'PATCHLEVEL = 1' /etc/SuSE-release ; then
            # This one is for SLES10 SP1 or OpenSuSE10 SP1 series
            if grep -q "SUSE LINUX 10.0" /etc/SuSE-release; then
                if uname -a | grep -q i[0-9]86; then
                    OS="OPENSUSE10-SP1-32"
                elif uname -a | grep -q x86_64; then
                    OS="OPENSUSE10-SP1-64"
                fi
            fi
        elif [ -f /etc/SuSE-release ] && grep -q 'VERSION = 11' /etc/SuSE-release && grep -q "SUSE Linux Enterprise Server 11" /etc/SuSE-release && grep -q 'PATCHLEVEL = 0' /etc/SuSE-release; then
            # This one is for SLES11 Base
            if grep -q "SUSE Linux Enterprise Server 11" /etc/SuSE-release; then
                if uname -a | grep -q i[0-9]86; then
                    OS="SLES11-32"
                elif uname -a | grep -q x86_64; then
                    OS="SLES11-64"
                fi            
            fi
        elif [ -f /etc/SuSE-release ] && grep -q 'VERSION = 11' /etc/SuSE-release && grep -q "SUSE Linux Enterprise Server 11" /etc/SuSE-release && grep -q 'PATCHLEVEL = 1' /etc/SuSE-release; then
            # This one is for SLES11 Base with SP1
            if grep -q "SUSE Linux Enterprise Server 11" /etc/SuSE-release; then
                if uname -a | grep -q i[0-9]86; then
                    OS="SLES11-SP1-32"
                elif uname -a | grep -q x86_64; then
                    OS="SLES11-SP1-64"
                fi
            fi
        elif [ -f /etc/SuSE-release ] && grep -q 'VERSION = 11' /etc/SuSE-release && grep -q "SUSE Linux Enterprise Server 11" /etc/SuSE-release && grep -q 'PATCHLEVEL = 2' /etc/SuSE-release; then
            # This one is for SLES11 Base with SP2
            if grep -q "SUSE Linux Enterprise Server 11" /etc/SuSE-release; then
                if uname -a | grep -q i[0-9]86; then
                    OS="SLES11-SP2-32"
                elif uname -a | grep -q x86_64; then
                    OS="SLES11-SP2-64"
                fi
            fi    
        elif [ -f /etc/SuSE-release ] && grep -q 'VERSION = 11' /etc/SuSE-release && grep -q "SUSE Linux Enterprise Server 11" /etc/SuSE-release && grep -q 'PATCHLEVEL = 3' /etc/SuSE-release; then
            # This one is for SLES11 Base with SP2
            if grep -q "SUSE Linux Enterprise Server 11" /etc/SuSE-release; then
                if uname -a | grep -q i[0-9]86; then
                    OS="SLES11-SP3-32"
                elif uname -a | grep -q x86_64; then
                    OS="SLES11-SP3-64"
                fi
            fi
		elif [ -f /etc/SuSE-release ] && grep -q 'VERSION = 11' /etc/SuSE-release && grep -q "SUSE Linux Enterprise Server 11" /etc/SuSE-release && grep -q 'PATCHLEVEL = 4' /etc/SuSE-release; then
            if grep -q "SUSE Linux Enterprise Server 11" /etc/SuSE-release; then
                if uname -a | grep -q i[0-9]86; then
                    OS="SLES11-SP4-32"
                elif uname -a | grep -q x86_64; then
                    OS="SLES11-SP4-64"
                fi
            fi
        elif [ -f /etc/SuSE-release ]; then 
            # This one is for SLES9 with SP3
            if grep -q 'VERSION = 9' /etc/SuSE-release; then
                if grep -q 'PATCHLEVEL = 3' /etc/SuSE-release; then
                    if uname -a | grep -q i[0-9]86; then
                        OS="SLES9-SP3-32"
                    elif uname -a | grep -q x86_64; then
                        OS="SLES9-SP3-64"
                    fi
                fi
            fi
        elif [ -f /etc/lsb-release ] ; then
            if grep -q 'Ubuntu 8.04.2' /etc/lsb-release || 
                grep -q 'Ubuntu 8.04.3' /etc/lsb-release ; then
                if uname -a | grep -q x86_64; then
                    OS="UBUNTU8-64"
                fi
            elif grep -q 'Ubuntu 10.04.4 LTS' /etc/lsb-release ; then
                if uname -a | grep -q x86_64; then
                    OS="UBUNTU-10.04.4-64"
                fi
	    elif grep -q 'Ubuntu 12.04.4 LTS' /etc/lsb-release ; then
                if uname -a | grep -q x86_64; then
                    OS="UBUNTU-12.04.4-64"
                fi
            fi
        elif [ -f /etc/debian_version ]; then
            if grep -q '4.0' /etc/debian_version; then
                if uname -a | grep -q i[0-9]86; then
                    OS="DEBIAN-ETCH-32"
                elif uname -a | grep -q x86_64; then
                    OS="DEBIAN-ETCH-64"
                fi
            fi    
            if grep -q '6.0.1' /etc/debian_version; then
                if uname -a | grep -q x86_64; then
                    OS="DEBIAN6U1-64"
                fi
            fi
        fi
    ;;

 HP-UX)
        # Itanium is identified by 'ia64' in the output of 'uname -a'
        if [ `uname -r` = "B.11.11" ] && [ `uname -m` != "ia64" ]; then
            OS="HP-UX-PA-RISC"
        elif [ `uname -r` = "B.11.23" ] && [ `uname -m` = "ia64" ]; then
            OS="HP-UX-Itanium-11iv2"
        elif [ `uname -r` = "B.11.31" ] && [ `uname -m` = "ia64" ]; then
            OS="HP-UX-Itanium-11iv3"
        fi
    ;;

 AIX)
        case `uname -v` in
            5)
                if [ `uname -r` = "3" ]; then
                    case `uname -p` in
                        powerpc*)
                            if [ `getconf KERNEL_BITMODE` = "64" ]; then
                                OS="AIX53"
                            fi
                        ;;
                    esac
                fi
            ;;
            6)
                if [ `uname -r` = "1" ] ; then
                    case `uname -p` in
                        powerpc*)
                            if [ `getconf KERNEL_BITMODE` = "64" ]; then
                                OS="AIX61"
                            fi
                        ;;
                    esac
                fi
            ;;
            7)
                if [ `uname -r` = "1" ] ; then
                    case `uname -p` in
                        powerpc*)
                            if [ `getconf KERNEL_BITMODE` = "64" ]; then
                                    OS="AIX71"
                            fi
                        ;;
                    esac
                fi
            ;;
        esac
    ;;

 SunOS)
        if [ `uname -r` = "5.8" ]; then
            case `uname -p` in
                sparc*)
                    if [ `isainfo -b` = "64" ]; then
                        OS="Solaris-5-8-Sparc"
                    fi
                ;;
            esac        
        elif [ `uname -r` = "5.9" ]; then
            case `uname -p` in
                sparc*)
                    if [ `isainfo -b` = "64" ]; then
                        OS="Solaris-5-9-Sparc"
                    fi
                ;;
            esac    
        elif [ `uname -r` = "5.10" ]; then
            case `uname -p` in
                i386|x86*)
                    if [ `isainfo -b` = "64" ]; then		  
                        OS="Solaris-5-10-x86-64"
                    fi
                ;;
                sparc*)
                    if [ `isainfo -b` = "64" ]; then
                        OS="Solaris-5-10-Sparc"
                    fi
                ;;
            esac
        elif [ `uname -r` = "5.11" ] && grep 'Oracle Solaris' /etc/release >/dev/null ; then
            case `uname -p` in
                i386|x86*)
                    if [ `isainfo -b` = "64" ]; then
                        OS="Solaris-5-11-x86-64"
                    fi
                ;;
                sparc*)
                    if [ `isainfo -b` = "64" ]; then
                        OS="Solaris-5-11-Sparc"
                    fi
                ;;
            esac
        elif [ `uname -r` = "5.11" ] && grep 'OpenIndiana Development oi_151.1.7 X86' /etc/release >/dev/null ; then
            case `uname -p` in
                i386|x86*)
                    if [ `isainfo -b` = "64" ]; then
                        OS="OpenIndiana-151a7-x86-64"
                    fi
                ;;
            esac
        elif [ `uname -r` = "5.11" ] && grep 'OpenSolaris' /etc/release >/dev/null ; then
            case `uname -p` in
                i386|x86*)
                    if [ `isainfo -b` = "64" ]; then
                        OS="OpenSolaris-5-11-x86-64"
                    fi
                ;;
                sparc*)
                    if [ `isainfo -b` = "64" ]; then
                        OS="OpenSolaris-5-11-Sparc"
                    fi
                ;;
            esac
        fi
    ;;
esac

if [ $# -gt 0 ]
then
    echo $OS
fi

