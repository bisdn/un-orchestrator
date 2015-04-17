#! /bin/bash

#Author: Ivano Cerrato
#Date: June 16th 2014
#Brief: stop a NF

#command line: sudo ./stopNF.sh $1 $2

#$1 LSI ID				(e.g., 2)
#$2 NF name				(e.g., firewall)

timestamp=$(date +%s)

if (( $EUID != 0 ))
then
    echo "[stopNF] This script must be executed with ROOT privileges"
    exit 0
fi

image_name=`echo $1"_"$2`

echo "[stopNF] Executing command: '"sudo virsh destroy $image_name"'"
sudo virsh destroy $image_name

echo "[stopNF] Executing command: '"sudo virsh undefine $image_name --remove-all-storage"'"
sudo virsh undefine $image_name --remove-all-storage

echo "[stopNF] Executing command: '"sudo rm /var/lib/libvirt/images/$image_name.qcow2"'"
sudo rm /var/lib/libvirt/images/$image_name.qcow2

#TODO: check if everityng is ok

#cat bindings$1 | grep "#$image_name#" | cut -d " " -f 2- > $timestamp
cat $image_name | cut -d " " -f 2- > $timestamp

columns=`cat $timestamp | wc -w`
columns=$(($columns * 2))	#There are spaces between two "real" columns

for (( i=1; i<=$columns; i++ ))
do
	value=`cat $timestamp | cut -d " " -f $i`

	if [ "$value" != "" ]; then
		sudo ifconfig $value down
		sudo brctl delbr $value
	fi

done

rm $image_name

rm $timestamp

exit 1
