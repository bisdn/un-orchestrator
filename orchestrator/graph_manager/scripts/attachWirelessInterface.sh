#!/bin/bash

#Author: Ivano Cerrato
#Date: Aug 21th 2014
#Brief: creates a bridge, and attaches two interfaces to it.

#command line:
#	sudo ./graph_manager/scripts/attachWirelessInterface.sh 2 wlan0

#$1 LSI ID						(e.g., 2)
#$2 port name					(e.g., wlan0)

if (( $EUID != 0 ))
then
    echo "[attachWirelessInterface] This script must be executed with ROOT privileges"
    exit 0
fi

bridgeName=`echo br_$1`
bridgeName+=$2

echo "[attachWirelessInterface] Creating the bridge $bridgeName"
sudo brctl addbr $bridgeName

kni=`echo $1_$2`
echo "[attachWirelessInterface] Attaching the interface $kni to the bridge $bridgeName"
sudo brctl addif $bridgeName $kni

echo "[attachWirelessInterface] Attaching the interface $2 to the bridge $bridgeName"
sudo brctl addif $bridgeName $2

ifconfig $bridgeName up

exit 1

