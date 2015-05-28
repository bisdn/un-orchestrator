#!/bin/bash
number=$(sudo ovs-ofctl show $1 | grep $2 | awk -F '[(]' '{print $1}'| tr -d '[[:space:]]')
exit $number
