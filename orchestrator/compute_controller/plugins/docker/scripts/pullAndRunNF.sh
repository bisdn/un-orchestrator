#!/bin/bash

#Author: Ivano Cerrato
#Date: June 16th 2014
#Brief: pull a NF from a docker repository, and run it. 

#command line: sudo ./pullAndRunNF.sh $1 $2 $3 $4 [$5 ...]

#$1 LSI ID				(e.g., 2)
#$2 NF name				(e.g., firewall)
#$3 registry/nf[:tag] 	(e.g., localhost:5000/pcap:latest)
#$4 number_of_ports		(e.g., 2)
#The next $4 parameters are port names to be provided to the container (e.g., vEth0 vEth1)
#The next $4 parameters are IPv4 addresses / network to be associated with that ports (e.g., 10.0.0.1/24)
#	0 if no IPv4 address must be associated
#The next $4 parameters are Eth addresses to be associated with that ports (e.g., aa:aa:aa:aa:aa:aa)
#	0 if no Ethernet address must be associated

tmp_file="$1_$2_tmp"

if (( $EUID != 0 )) 
then
    echo "[pullAndRunNF] This script must be executed with ROOT privileges"
    exit 0
fi

echo -ne "" >> downloaded

find=`cat downloaded | grep $3 | wc -l`

if [ $find -eq 0 ]
then 
#	IVANO: uncomment the following rows in case you want that the images are downloaded from a repositotory.
#		Otherwise, the code supposes that the VNFs images are available locally.

#	#The image must be downloaded from the ropository
#
#	sudo docker pull $3
#	#docker pull returns 0 in case of success
#	ret=`echo $?`
#
#	if [ $ret -eq 0 ]
#	then
#		echo "[pullAndRunNF] Image '"$3"' retrieved"
#	else
#		echo "[pullAndRunNF] Impossible to retrieve image '"$3"'"
#		exit 0
#	fi
	
	echo $3 >> downloaded
fi

#prepare the command
#LXC parameters available at
#		https://github.com/dotcloud/docker/blob/master/daemon/execdriver/lxc/lxc_template.go
#		http://manpages.ubuntu.com/manpages/lucid/man5/lxc.conf.5.html
#
#Paramters that could be usefull
#
#	lxc.network.ipv6	#set the IPv6 address to an interface

echo -ne "sudo docker run -d --name $1_$2 "   > $tmp_file

skip_lxc_networking=1

if [ $skip_lxc_networking -eq 0 ]
then
current=5
currentIp=`expr $current + $4`
currentEthernet=`expr $currentIp + $4`
for (( c=0; c<$4; c++ ))
do
 	echo -ne --lxc-conf=\"lxc.network.type=phys\" --lxc-conf=\"lxc.network.link=${!current}\" --lxc-conf=\"lxc.network.name=eth$c\" --lxc-conf=\"lxc.network.flags=up\" " ">> $tmp_file

 	if [ ${!currentEthernet} != 0 ]
 	then
 		echo -ne --lxc-conf=\"lxc.network.hwaddr=${!currentEthernet}\" " ">> $tmp_file
 	fi
 	
 	if [ ${!currentIp} != 0 ]
 	then
 		echo -ne --lxc-conf=\"lxc.network.ipv4=${!currentIp}\" " ">> $tmp_file
 	fi
	
	current=`expr $current + 1`
	currentIp=`expr $currentIp + 1`
	currentEthernet=`expr $currentEthernet + 1`
done 
fi
 
#echo --networking=\"false\"  --privileged=true  $3 >> $tmp_file
echo --net=\"none\"  --privileged=true  $3 >> $tmp_file

echo [`date`]"[pullAndRunNF] Executing command: '"`cat $tmp_file`"'"

ID=`bash $tmp_file`

#docker run returns 0 in case of success
ret=`echo $?`

if [ $ret -eq 0 ]
then
	echo [`date`]"[pullAndRunNF] Container $2 started with ID: '"$ID"'"
else
	echo "[pullAndRunNF] An error occurred while starting the container"
	rm $tmp_file
	exit 0
fi

#Save the binding lsi-nf-docker id on a file
file="$1_$2"
echo $ID >> $file

rm $tmp_file

if [ $skip_lxc_networking -eq 1 ]
then
	# Insert the veth interfaces into the containers
	# (normally done with --lxc-conf flags, but some setups it doen't work)  
	PID=`docker inspect --format '{{ .State.Pid }}' $ID`
	current=5
	for (( c=0; c<$4; c++ ))
	do
		ip link set ${!current} netns $PID
		current=`expr $current + 1`
	done
fi

exit 1
