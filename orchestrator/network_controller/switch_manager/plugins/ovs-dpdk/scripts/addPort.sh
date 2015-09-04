#!/bin/bash

#Author: UNIFY Consortium
#Date: 2015-07-02 
#Brief: Add a port to a bridge

#$1 LSI id
#$2 port name
#$3 type (dpdk, dpdkr, dpdkvhostuser)
#$4 port id to assign

bridgeName=`echo br_$1`
port=$2
port_type=$3
port_id=$4

. ./network_controller/switch_manager/plugins/ovs-dpdk/scripts/ovs.conf

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

VSCTL="$OVS_DIR/utilities/ovs-vsctl"

echo "[$0] Adding port $port to bridge $bridgeName (type=$port_type id=$port_id)"

if (( "$port_type" == "dpdkvhostuser" ))
then
    rm -f "$5/$port"
fi

$VSCTL --no-wait add-port $bridgeName $port -- set Interface $port type=$port_type ofport_request=$port_id

exit 1