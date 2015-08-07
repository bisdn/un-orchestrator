#!/bin/bash

#Author: UNIFY Consortium
#Date: 2015-07-02 
#Brief: Add a port to a bridge

#$1 LSI id
#$2 controller ip
#$3 controller port

bridgeName=`echo br_$1`
controller_ip=$2
controller_port=$3
ofp_version=$4

. ./network_controller/switch_manager/plugins/ovs-dpdk/scripts/ovs.conf

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

VSCTL="$OVS_DIR/utilities/ovs-vsctl"


echo "[$0] Creating bridge $bridgeName"
$VSCTL --no-wait add-br $bridgeName
$VSCTL --no-wait set bridge $bridgeName datapath_type=netdev
$VSCTL --no-wait set bridge $bridgeName protocols=$ofp_version
$VSCTL --no-wait set-controller $bridgeName tcp:$controller_ip:$controller_port

exit 1

