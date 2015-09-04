#!/bin/bash

#Author: UNIFY Consortium
#Date: 2015-07-05 
#Brief: Add a Virtual Link between two bridges (using patch ports)

s_br=`echo br_$1`
d_br=`echo br_$2`
s_port_name=`echo VLink_$1-to-$2_$5`
d_port_name=`echo VLink_$2-to-$1_$5`
s_port_id=$3
d_port_id=$4

. ./network_controller/switch_manager/plugins/ovs-dpdk/scripts/ovs.conf

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

VSCTL="$OVS_DIR/utilities/ovs-vsctl"

$VSCTL add-port $s_br $s_port_name -- set Interface $s_port_name type=patch ofport_request=$s_port_id options:peer=$d_port_name
$VSCTL add-port $d_br $d_port_name -- set Interface $d_port_name type=patch ofport_request=$d_port_id options:peer=$s_port_name

exit 1