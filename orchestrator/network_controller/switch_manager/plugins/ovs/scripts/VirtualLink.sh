#!/bin/bash
sudo ovs-vsctl add-port $1 $3
sudo ovs-vsctl add-port $2 $4
sudo ovs-vsctl set interface $3 type=patch
sudo ovs-vsctl set interface $4 type=patch
sudo ovs-vsctl set interface $3 options:peer=$4
sudo ovs-vsctl set interface $4 options:peer=$3
