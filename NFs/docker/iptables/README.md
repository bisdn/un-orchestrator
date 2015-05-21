This network function is a firewall based on iptables.

To change the configuration of the firewall, edit the file start.sh

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

docker build --tag="localhost:5000/iptables" .

===============================================================================

* If you are going to use this network function together with the un-orchestrator, 
you can skip this part *

* in interactive mode

	docker run -i -t --lxc-conf="lxc.network.type=phys" --lxc-conf="lxc.network.link=vEth0" \
		--lxc-conf="lxc.network.name=eth0" --lxc-conf="lxc.network.flags=up" \
		--lxc-conf="lxc.network.type=phys" --lxc-conf="lxc.network.link=vEth1" \
		--lxc-conf="lxc.network.name=eth1" --lxc-conf="lxc.network.flags=up" --net="none" \
		--privileged="true" localhost:5000/iptables /bin/bash

	./start.sh

* in background mode

	docker run -d --lxc-conf="lxc.network.type=phys" --lxc-conf="lxc.network.link=vEth0" \
		--lxc-conf="lxc.network.name=eth0" --lxc-conf="lxc.network.flags=up" \
		--lxc-conf="lxc.network.type=phys" --lxc-conf="lxc.network.link=vEth1" \
		--lxc-conf="lxc.network.name=eth1" --lxc-conf="lxc.network.flags=up" --net="none" \  
		--privileged="true" localhost:5000/iptables
		
In both the cases, vEth0 and vEth1 are vEth interfaces available in the host.

