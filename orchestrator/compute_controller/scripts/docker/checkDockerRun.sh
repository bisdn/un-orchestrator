#!/bin/bash

#Author: Ivano Cerrato
#Date: June 16th 2014
#Brief: check if the Docker deamon is running on this machine
#		Docker should be started with the LXC implementation

#command line: sudo ./checkDockerRun.sh

if (( $EUID != 0 )) 
then
    echo "[checkDockerRun] This script must be executed with ROOT privileges"
    exit 0
fi

#IVANO: probably the check on the version can be removed. To me it seems that both Ubuntu and
#Debian use the same syntax.

version=`cat /etc/*-release | grep "ID=ubuntu" | wc -l`
if [ $version -ge 1 ]
then
	#The distribution is Ubuntu
	num=`ps aux | grep "docker -d" | grep "lxc" | grep -v "grep" | wc -l`
else
	#I'm assuming that the distribution is Debian
	num=`ps aux | grep "/usr/bin/docker -d" | grep "lxc" | grep -v "grep" | wc -l`
fi

if [ $num -ge 1 ]
then
	exit 1
fi

echo "[checkDockerRun] Docker is not running (at least with the LXC engine)"

exit 0

