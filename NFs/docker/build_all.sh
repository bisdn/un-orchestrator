#!/bin/bash

#Author: Ivano Cerrato
#Date: Jul. 29th, 2014

sudo docker run -d -p 5000:5000 samalba/docker-registry

sleep 1

echo "Example..."
cd example
sudo docker build --tag="localhost:5000/example" .
sudo docker push localhost:5000/example
cd ..

sleep 1

echo "iptables..."
cd iptables
sudo docker build --tag="localhost:5000/iptables" .
sudo docker push localhost:5000/iptables
cd ..

