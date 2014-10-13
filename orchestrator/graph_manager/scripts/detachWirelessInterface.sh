#!/bin/bash

#Author: Ivano Cerrato
#Date: Aug 21th 2014
#Brief: destroys a bridge

#command line:
#	sudo ./graph_manager/scripts/detachWirelessInterface.sh 2 wlan0

#$1 LSI ID						(e.g., 2)
#$2 port name					(e.g., wlan0)

if (( $EUID != 0 ))
then
    echo "[detachWirelessInterface] This script must be executed with ROOT privileges"
    exit 0
fi

bridgeName=`echo br_$1`
bridgeName+=$2

echo "[detachWirelessInterface] Deleting the bridge $bridgeName"

sudo ifconfig $bridgeName down
sudo brctl delbr $bridgeName

exit 1

