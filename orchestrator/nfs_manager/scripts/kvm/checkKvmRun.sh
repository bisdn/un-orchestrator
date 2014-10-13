#!/bin/bash

#Author: Ivano Cerrato
#Date: Aug 6th, 2014
#Brief: check if KVM is running on this machine

#command line: sudo ./checkKvmRun.sh

if (( $EUID != 0 )) 
then
    echo "[checkKvmRun] This script must be executed with ROOT privileges"
    exit 0
fi

num=`ps aux | grep kvm -i  | grep -v "checkKvmRun" | grep -v "grep" | wc -l`

if [ $num -ge 1 ]
then
	exit 1
fi

echo "[checkKvmRun] KVM is not running"

exit 0

