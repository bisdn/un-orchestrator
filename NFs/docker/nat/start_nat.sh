#! /bin/bash

#Assign the ip addresses dinamically
cp /sbin/dhclient /usr/sbin/dhclient && /usr/sbin/dhclient eth0 -v && /usr/sbin/dhclient eth1 -v  

ifconfig

iptables -t nat -A POSTROUTING -o eth1 -j MASQUERADE

#Keep the container alive
while true
do
	sleep 100
done

