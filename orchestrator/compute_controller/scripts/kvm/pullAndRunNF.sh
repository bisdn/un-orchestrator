#!/bin/bash

#Author: Ivano Cerrato
#Date: Aug 5th 2014
#Brief: create a NF from a local master image, and run it.

#command line:
#	sudo ./nfs_manager/scripts/kvm/pullAndRunNF.sh 2 example /var/lib/libvirt/images/nf.qcow2 2 2_example_1 2_example_2

#$1 LSI ID						(e.g., 2)
#$2 NF name						(e.g., firewall)
#$3 path of the master image	(e.g., /var/lib/libvirt/images/nf.qcow2)
#$4 number_of_ports				(e.g., 2)
#The next $4 parameters are port names to be attache with the bridges, which are in turn
#attached to the network function

tmp_file="$1_$2_tmp"

if (( $EUID != 0 ))
then
    echo "[pullAndRunNF] This script must be executed with ROOT privileges"
    exit 0
fi

image_name=`echo $1"_"$2`

echo "[pullAndRunNF] Executing command: '"sudo virt-clone --original nf-master --name $image_name --file=/var/lib/libvirt/images/$image_name.qcow2 --preserve-data"'"
sudo virt-clone --original nf-master --name $image_name --file=/var/lib/libvirt/images/$image_name.qcow2 --preserve-data

echo "[pullAndRunNF] Executing command: '"sudo qemu-img create -b $3 -f qcow2 /var/lib/libvirt/images/$image_name.qcow2"'"
sudo qemu-img create -b $3 -f qcow2 /var/lib/libvirt/images/$image_name.qcow2

current=5
for (( c=0; c<$4; c++ ))
do
	bridgeName=`echo br_$tmp_file`
	bridgeName+=$c

	echo -ne $bridgeName " " >> $tmp_file

	#Create a bridge
	sudo brctl addbr $bridgeName

	#Attach the KNI port to this bridge
	sudo brctl addif $bridgeName ${!current}

	#Attach the VM to this bridge
	echo "[pullAndRunNF] Executing command: '"sudo virsh attach-interface --config --model virtio $image_name bridge $bridgeName"'"
	sudo virsh attach-interface --config --model virtio $image_name bridge $bridgeName 

	ifconfig $bridgeName up

	current=`expr $current + 1`
done

echo "" >> $tmp_file

#Start the VM
virsh start $image_name

num=`sudo virsh list | grep $image_name | wc -l`

if [ $num -eq 0 ]
then
	echo "[pullAndRunNF] An error occurred while starting the virtual machine $image_name"
	exit 0
else
	echo "[pullAndRunNF] The virtul machine $image_name is now running"
fi

cat $tmp_file >> $image_name

sudo rm $tmp_file

exit 1

