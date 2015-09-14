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

type_cmd="type=$port_type"

if [[ "$port_type" == "dpdkvhostuser" ]]
then
    rm -f "$5/$port"   # Cleanup vhostuser socket
fi

if [[ "$port_type" == "veth" ]]
then
    ip link add $port type veth peer name $port.lxc
    ip link set $port up
    ip link set $port.lxc up

    # The veth pair peer given to the container is the one with the port_name as derived from the NF-FG.
    # This avoids having to alter the Docker command line based on what this script does. As a result, the
    # veth pair peer we add to OVS is the one with  a decorated name:  
    port=$port.lxc
    
    type_cmd=""
fi

echo "type_cmd=$type_cmd"

$VSCTL --no-wait add-port $bridgeName $port -- set Interface $port $type_cmd ofport_request=$port_id

exit 1