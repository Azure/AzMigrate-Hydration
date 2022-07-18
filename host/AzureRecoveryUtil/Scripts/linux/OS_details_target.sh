#!/bin/bash
#
# Usage: OS_details_target.sh <root-path> [--formated-output]
#
# Formated-output will display below details:
# os=<detected-os-string>
# release-file=<release-string-file-path>
#
# Default output will display only detected os string.
# <os>
#

# 1st argument: root path.
mntpath=$1

# 2nd argument: formated-output
formated_output="no"
os_release_file=""
if [ "$2" = "--formated-output" ]; then
	formated_output="yes"
fi

if [ -f $mntpath/etc/enterprise-release ] && [ -f $mntpath/etc/redhat-release ] ; then
	if grep -q 'Enterprise Linux Enterprise Linux Server release 5.*' $mntpath/etc/enterprise-release; then
		OS="OL5-64"
	fi
	os_release_file=$mntpath/etc/enterprise-release
elif [ -f $mntpath/etc/oracle-release ] && [ -f $mntpath/etc/redhat-release ] ; then
	if grep -q 'Oracle Linux Server release 6.*' $mntpath/etc/oracle-release ; then
		OS="OL6-64"
	elif grep -q 'Oracle Linux Server release 7.*' $mntpath/etc/oracle-release ; then
		OS="OL7-64"
	elif grep -q 'Oracle Linux Server release 8.*' $mntpath/etc/oracle-release ; then
		OS="OL8-64"
	fi
	os_release_file=$mntpath/etc/oracle-release
elif [ -f $mntpath/etc/redhat-release ]; then
	if grep -q 'Red Hat Enterprise Linux Server release 5.*' $mntpath/etc/redhat-release || \
	   grep -q 'CentOS release 5.*' $mntpath/etc/redhat-release; then
			OS="RHEL5-64"
	elif grep -q 'Red Hat Enterprise Linux Server release 6.*' $mntpath/etc/redhat-release || \
		 grep -q 'Red Hat Enterprise Linux Workstation release 6.*' $mntpath/etc/redhat-release || \
		 grep -q 'CentOS Linux release 6.*' $mntpath/etc/redhat-release || \
		 grep -q 'CentOS release 6.*' $mntpath/etc/redhat-release; then
			OS="RHEL6-64"
	elif grep -q 'Red Hat Enterprise Linux Server release 7.*' $mntpath/etc/redhat-release ||
		 grep -q 'Red Hat Enterprise Linux Workstation release 7.*' $mntpath/etc/redhat-release; then
			OS="RHEL7-64"
	elif grep -q 'CentOS Linux release 7.*' $mntpath/etc/redhat-release; then
			OS="CENTOS7-64"
	elif grep -q 'Red Hat Enterprise Linux release 8.*' $mntpath/etc/redhat-release; then
			OS="RHEL8-64"
	elif grep -q 'CentOS Linux release 8.*' $mntpath/etc/redhat-release ||
		 grep -q 'CentOS Stream release 8.*' $mntpath/etc/redhat-release; then
			OS="CENTOS8-64"
	fi
	os_release_file=$mntpath/etc/redhat-release
elif [ -f $mntpath/etc/SuSE-release ]; then
	if grep -q 'VERSION = 11' $mntpath/etc/SuSE-release && grep -q 'PATCHLEVEL = 3' $mntpath/etc/SuSE-release; then
		OS="SLES11-SP3-64"
	elif grep -q 'VERSION = 11' $mntpath/etc/SuSE-release && grep -q 'PATCHLEVEL = 4' $mntpath/etc/SuSE-release; then
		OS="SLES11-SP4-64"
	fi
	if grep -q 'VERSION = 12' $mntpath/etc/SuSE-release; then
		OS="SLES12-64"
	elif grep -q 'VERSION="15' $mntpath/etc/SuSE-release; then
		OS="SLES15-64"
	fi
	os_release_file=$mntpath/etc/SuSE-release
elif [ -f $mntpath/etc/os-release ] && grep -q 'SLES' $mntpath/etc/os-release; then
	if grep -q 'VERSION="12' $mntpath/etc/os-release; then
		OS="SLES12-64"
	elif grep -q 'VERSION="15' $mntpath/etc/os-release; then
		OS="SLES15-64"
	fi
elif [ -f $mntpath/etc/lsb-release ]; then
	if grep -q 'Ubuntu 12.*' $mntpath/etc/lsb-release ; then
		OS="UBUNTU12-64"
	elif grep -q 'Ubuntu 14.*' $mntpath/etc/lsb-release ; then
		OS="UBUNTU14-64"
	elif grep -q 'Ubuntu 16.*' $mntpath/etc/lsb-release ; then
		OS="UBUNTU16-64"
	elif grep -q 'Ubuntu 18.*' $mntpath/etc/lsb-release ; then
		OS="UBUNTU18-64"
	elif grep -q 'Ubuntu 19.*' $mntpath/etc/lsb-release ; then
		OS="UBUNTU19-64"
	elif grep -q 'Ubuntu 20.*' $mntpath/etc/lsb-release ; then
		OS="UBUNTU20-64"
	elif grep -q 'Ubuntu 21.*' $mntpath/etc/lsb-release ; then
		OS="UBUNTU21-64"
	elif grep -q 'Ubuntu 22.*' $mntpath/etc/lsb-release ; then
		OS="UBUNTU22-64"
	fi
	os_release_file=$mntpath/etc/lsb-release
elif [ -f $mntpath/etc/debian_version ]; then
	if grep -q '^7.*' $mntpath/etc/debian_version; then
        OS="DEBIAN7-64"
    elif grep -q '^8.*' $mntpath/etc/debian_version; then
        OS="DEBIAN8-64"
    elif grep -q '^9.*' $mntpath/etc/debian_version; then
        OS="DEBIAN9-64"
    elif grep -q '^10.*' $mntpath/etc/debian_version; then
        OS="DEBIAN10-64"
    elif grep -q '^11.*' $mntpath/etc/debian_version; then
        OS="DEBIAN11-64"
	elif grep -q '^kali-rolling*' $mntpath/etc/debian_version; then
	    OS="KALI-ROLLING-64"
    fi
	os_release_file=$mntpath/etc/debian_version
fi

if [ $# -gt 0 ]
then
	if [ "$formated_output" = "yes" ]; then
		echo "os=$OS"
		echo "release-file=$os_release_file"
	else
	echo $OS
	fi
fi
