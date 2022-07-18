#!/bin/bash

#DBG="set -x"

# TODO: Use Find_Device_From_UUID() from Linux_P2V_Lib.sh
get_original_dev()
{
	local uuid=$(blkid -o value -s UUID $1)
	local pat="FSTAB UUID Information"

	sed -n "/^[=]* $pat - START [=]*/,/^[=]* $pat - END [=]*/p" $2 |\
	grep $uuid | cut -f 1 -d ":" | sort | uniq
}

get_fs_opt()
{
	local pat="Filesystem table"
	
	sed -n "/^[=]* $pat - START [=]*/,/^[=]* $pat - END [=]*/p" $2 |\
	grep "$1\s" | awk '{print $4}'
}

is_root_mnt()
{
	local rdev=$(df $1 | sed -n 2p | cut -f 1 -d " ")
	
	rdev="$(readlink -f $rdev)"

	if [ "$rdev" = "`readlink -f $2`" ]; then
		echo "Root Mount"
		return 0
	else
		echo "Non Root Mount"
		return 1
	fi
}

replace_pat_in_file()
{
	$DBG

	local opat="$1"
	local npat="$2"
	local file="$3"

	echo "Convert $opat to $npat"
	
	if [ ! -f "$file" ]; then
		return 0
	fi

	sed -i "s/$opat/$npat/" "$file"	
}

fstab_convert_label_to_uuid()
{
	$DBG
	local label=$(blkid -o value -s LABEL $1)
	local uuid=$(blkid -o value -s UUID $1)

	if [ "$label" = "" ]; then
		return
	fi

	# LABEL=label -> UUID=uuid
	replace_pat_in_file "LABEL=$label" "UUID=$uuid" "$2"
	# LABEL="label" -> UUID=uuid
	replace_pat_in_file "LABEL=\\\"$label\\\"" "UUID=$uuid" "$2"
}

fstab_convert_uuid_to_uuid()
{
	$DBG
	local uuid=$(blkid -o value -s UUID $1)

	replace_pat_in_file "UUID=\\\"$uuid\\\"" "UUID=$uuid" "$2"
}

fstab_convert_dev_to_uuid()
{
	$DBG
	local uuid=$(blkid -o value -s UUID $1)
	local odevlist="$(get_original_dev $1 $2)"
	local odev=""

	echo "Original Dev: $1 -> $odevlist"
	
	for odev in $odevlist; do
		odev="$(echo $odev | sed 's/\//\\\//g')"

		# /dev/name... -> UUID=uuid
		replace_pat_in_file "$odev" "UUID=$uuid" "$3"
	done
}

fstab_mount_all_uuid()
{
	$DBG
	local fstab=$1
	local uuid="$2"
	local mnt="$3"

	echo "Mount all FS with UUID = $uuid"

	cat $fstab | grep ^"UUID=$uuid" | tr -s " " | sort -k2,2 |\
	while read line; do
		submnt="$(echo $line | tr -s " " | cut -f 2 -d " ")"
		opt="$(echo $line | tr -s " " | cut -f 4 -d " ")"

		if [ "$submnt" = "/" ]; then
			echo "Skipping /"
			continue
		fi

		submnt="$(echo $submnt | sed "s/\/$//")"

		echo "mount -o $opt -U $uuid $mnt$submnt" 
		mount -o $opt -U $uuid $mnt$submnt
	done
}

generic_mount_all()
{
	$DBG
	local rootmnt="$1"
	local dev="$2"

	local ofile="$rootmnt/etc/fstab"
	local mfile="/tmp/fstab.$$"
	local cfile="$rootmnt/etc/Configuration.info"

	local uuid=$(blkid -o value -s UUID $dev)

	cp $ofile $mfile || {
		echo "Cannot copy fstab"
		return 1
	}
	
	fstab_convert_uuid_to_uuid "$dev" "$mfile" 
	fstab_convert_label_to_uuid "$dev" "$mfile"
	fstab_convert_dev_to_uuid "$dev" "$cfile" "$mfile"

	fstab_mount_all_uuid "$mfile" "$uuid" "$rootmnt"

	cat $mfile
	rm -f $mfile
}

btrfs_find_mount_root()
{
	local vid=""
	local vidlist=""
	local cfile="$1/etc/Configuration.info"

	echo "Trying to find root subvolume"

	# Check for subvolumes with "root" in name
	vidlist=$(btrfs subvolume list $1 | awk '{print $2}')

	for vid in $vidlist; do
		echo "Mounting $vid"
		umount -f $1
		mount -t btrfs -o subvolid=$vid $2 $1
		test -f $cfile && {
			echo "Root VID: $vid"
			break
		}
	done
}

btrfs_mount_all()
{
	local cfile="$1/etc/Configuration.info"

	test -f $cfile || btrfs_find_mount_root $1 $2

	# In case it is a root snapshot, find the root 
	# device from Configuration.info
	is_root_mnt $1 $2 && {
		fsopt=$(get_fs_opt "/" "$cfile")

		echo "Remounting root: -o $fsopt"
		umount $1
		mount -t btrfs -o $fsopt $2 $1
	}

    #exec &> >(tee -a "$1/var/log/MountFS.log")
	generic_mount_all $@
	df
	return $?
}

echo "Mount subvolumes for $2: $1"

if [ "`dirname $0`" != "/tmp" -a -x "$1/MountFS.sh" ]; then
    echo "Executing local script"
    cp $1/MountFS.sh /tmp || {
    	echo "Cannot copy"
	    exit -1
    }
    /tmp/MountFS.sh $@ 
else
    fstype="$(blkid -o value -s TYPE $2)"
    if [ "$fstype" = "btrfs" ]; then
	    ${fstype}_mount_all "$1" "$2" 
    	exit $?
    else
	    exit 0
    fi
fi
