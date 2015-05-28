Libraries required to compile the un-orchestrator:

* Build essential  
	apt-get install build-essential

* cmake  
	apt-get install cmake

* ccmake  
	sudo apt-get install cmake-curses-gui

* Boost  
	apt-get install libboost-all-dev

* JSON spirit  
 	git clone https://github.com/sirikata/json-spirit  
	Install it according to the description provided in the downloaded folder

* Libmicrohttpd  
	apt-get install libmicrohttpd-dev

* ROFL-common  
	git clone https://github.com/bisdn/rofl-common  
	cd rofl-core/  
	Install it according to the description provided in the downloaded folders

===============================================================================

Virtual Switch.

* extensible Data-Path deamon (xDPd)

	git clone https://github.com/bisdn/xdpd  
	cd xdpd/  
	Install all the libraries required by the README provided in this folder  
	bash autogen  
	cd build  
	../configure --with-hw-support=gnu-linux-dpdk --with-plugins="node_orchestrator rest"   
	make
  
	Now the DPDK library, exploited by xDPd, must be properly configured:  
	cd ../libs/dpdk/tools  
	sudo ./setup.sh  
	Through this script, you should:  
	* build the environment x86_64-native-linuxapp-gcc
	* Insert IGB UIO module
  	* Insert KNI module
	* Setup hugepage mappings for non-NUMA systems (1000 could be a reasonable
    number)
	* Bind Ethernet device to IGB UIO module (bind all the ethernet interfaces 
    that you want to use)

* OF-CONFIG

	git clone https://https://github.com/openvswitch/of-config  
	For install follow the INSTALL.md readme provided in the root folder  

===============================================================================

Docker environment for network functions

* On Ubuntu:   
	follow the instruction provided here:  
		http://docs.docker.com/installation/  
	apt-get install lxc -y  
	echo 'DOCKER_OPTS="-e lxc"' >> /etc/default/docker  
	service docker restart

* On Debian:  
	follow the instruction provided here:  
		https://scottlinux.com/2014/05/04/how-to-install-and-run-docker-on-debian-wheezy/ 
	apt-get install docker -y  
	echo 'DOCKER_OPTS="-e lxc"' >> /etc/default/docker  
	service docker.io restart
        
===============================================================================
Compile the un-orchestrator

	cd orchestrator  
	ccmake .  
	Here you can select some configuration parameters for the un-orchestrator,  
	e.g., the vSwitch implementation, the Openflow version, etc.  
	make
    
===============================================================================

Please check README_RUN to startup the un-orchestrator.
