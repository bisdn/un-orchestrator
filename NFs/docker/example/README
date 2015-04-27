This is a simple network function, written in C, to be run in a Docker container.

When a packet is received on the interface eth0, its destination MAC address is 
changed, and the packet is sent back on the interface eth1

===============================================================================

Docker environment for network functions

 On Ubuntu: 
      follow the instruction provided here:
           http://docs.docker.com/installation/
      apt-get install lxc -y
      echo 'DOCKER_OPTS="-e lxc"' >> /etc/default/docker
      service docker restart

 On Debian:
      follow the instruction provided here:
          https://scottlinux.com/2014/05/04/how-to-install-and-run-docker-on-debian-wheezy/
          apt-get install docker -y
          echo 'DOCKER_OPTS="-e lxc"' >> /etc/default/docker
          service docker.io restart

===============================================================================

Create the Docker image which contains this network function:

docker build --tag="localhost:5000/example" .

===============================================================================

* If you are going to use this network function together with the un-orchestrator, 
you can skip this part *

Run the container:

* in interactive mode

	docker run -i -t --lxc-conf="lxc.network.type=phys" --lxc-conf="lxc.network.link=vEth0" \
		--lxc-conf="lxc.network.name=eth0" --lxc-conf="lxc.network.flags=up" \
		--lxc-conf="lxc.network.type=phys" --lxc-conf="lxc.network.link=vEth1" \
		--lxc-conf="lxc.network.name=eth1" --lxc-conf="lxc.network.flags=up" --net="none" \  
		--privileged="true" localhost:5000/example /bin/bash

	./example

* in background mode

	docker run -d --lxc-conf="lxc.network.type=phys" --lxc-conf="lxc.network.link=vEth0" \
		--lxc-conf="lxc.network.name=eth0" --lxc-conf="lxc.network.flags=up" \
		--lxc-conf="lxc.network.type=phys" --lxc-conf="lxc.network.link=vEth1" \
		--lxc-conf="lxc.network.name=eth1" --lxc-conf="lxc.network.flags=up" --net="none" \  
		--privileged="true" localhost:5000/example
		
In both the cases, vEth0 and vEth1 are vEth interfaces available in the host.
