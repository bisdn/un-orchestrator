#! /bin/bash

#Author: Ivano Cerrato
#Date: June 26th 2014
#Brief: stop a NF 

#command line: sudo ./stopNF.sh $1 $2

#$1 LSI ID				(e.g., 2)
#$2 NF name				(e.g., firewall)

if (( $EUID != 0 )) 
then
    echo "[stopNF] This script must be executed with ROOT privileges"
    exit 0
fi

file="$1_$2"

echo "[stopNF] Executing command: '"sudo kill -2 `cat $file | awk {'print $1'}`"'"

sudo kill -2 `cat $file | awk {'print $1'}`

sudo rm `cat $file | awk {'print $2'}`

echo "[stopNF] DPDK NF stopped"

rm $file

exit 1
