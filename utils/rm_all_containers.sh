#!/bin/bash

#Author: Ivano Cerrato
#Date: Jul 23rd, 2014
#Brief: this script *deletes* all the container images that are on this machine.

# USE IT ONLY IF YOU ARE SURE ON WHAT YOU ARE DOING

sudo docker ps -a | while read c
do 
	awk {'print $1'} >> tmp
done

cat tmp | while read c
do 
	sudo docker rm $c
done

rm tmp
