# How to compile the un-orchestrator

In order to execute the un-orchestrator, we need to setup different components, namely:

  * a set of libraries needed to compile the un-orchestrator code
  * a virtual switch (either xDPd or OpenvSwitch) as a base switch for
    our platform
  * an execution environment for network functions, e.g., KVM for 
    executing VM, Docker, or other.

### Required libraries

Several libraries are required to compile the un-orchestrator.
In the following we list the steps required on an Ubuntu 14.04.

	; Install build essential (it includes GCC, basic libraries, etc)
	$ sudo apt-get install build-essential
	
	; Install cmake (to create cross-platform makefiles)
	$ sudo apt-get install cmake
	
	; Install ccmake (nice 'gui' to edit cmake files)
	$ sudo apt-get install cmake-curses-gui
	
	; Install boost (nice c++ library with tons of useful functions)
	$ sudo apt-get install libboost-all-dev
	
	; Install JSON Spirit (nice library to parse JSON files)
	$ git clone https://github.com/sirikata/json-spirit 
	$ cd json-spirit/
	
	; Now install the library it according to the description provided 
	; in the cloned folder
	
	; Install libmicrohttpd (embedded micro http server)  
	$ sudo apt-get install libmicrohttpd-dev
	
	; Install ROFL-common  (library to parse OpenFlow messages)
	$ git clone https://github.com/bisdn/rofl-common  
	$ cd rofl-core/
	
	; Now install the library it according to the description provided 
	; in the cloned folder

### NF-FG library

These steps are mandatory only if you plan to use the Network Functions - 
Forwarding Graph (NF-FG) defined within the Unify project.

	; Retrieve the NF-FG library. In order to do this operation, you need
	; access to such a library.
	
	; Copy the library in the un-orchestrator folder
	$ cp [nffg]/virtualizer3.pyc [un-orchestrator]/orchestrator/node_resource_manager/virtualizer

### Virtual Switches

You must install one of the following virtual switches:

  * extensible Data-Path daemon (xDPd)
  * Open vSwitch (OvS)

##### Install the xDPd virtual switch

In order to install xDPd, you have to follow the steps below.

	git clone https://github.com/bisdn/xdpd  
	cd xdpd/  

	;Install all the libraries required by the README provided in this folder  
	bash autogen  
	cd build  
	../configure --with-hw-support=gnu-linux-dpdk --with-plugins="node_orchestrator rest"   
	make
  
Now the DPDK library, which is being used by xDPd, must be properly
configured, which can be done by launching a script that allows you to: 

  * build the environment x86_64-native-linuxapp-gcc
  * Insert IGB UIO module
  * Insert KNI module
  * Setup hugepage mappings for non-NUMA systems (1000 could be a
    reasonable number)
  * Bind Ethernet device to IGB UIO module (bind all the ethernet
    interfaces that you want to use)

Let's now launch the DPDK setup script:

	$ cd ../libs/dpdk/tools  
	$ sudo ./setup.sh  


##### Install the Open vSwitch (OvS) virtual switch

Compile and install libnetconf as described here:

	; Clone the libnetconf repository
	$ git clone https://code.google.com/p/libnetconf/

Install the libnetconf library by following the instructions in the 
INSTALL file contained in the root folder of this library.

You can now install OpenvSwitch:
	  
	; Clone the openvswitch repository
	$ git clone https://github.com/openvswitch/of-config    

Follow the instructions as described in the file INSTALL.md provided
in the root folder of that repository.


### Virtual Execution Environment for network functions

##### Docker

This is needed in order to suppor the Docker execution environment.

**On Ubuntu**
   
Follow the instruction provided here:

	http://docs.docker.com/installation/  
	
	$ sudo apt-get install lxc -y  
	$ echo 'DOCKER_OPTS="-e lxc"' >> /etc/default/docker  
	$ service docker restart

**On Debian**

Follow the instruction provided here: 

	https://scottlinux.com/2014/05/04/how-to-install-and-run-docker-on-debian-wheezy/ 

	$ sudo apt-get install docker -y  
	$ echo 'DOCKER_OPTS="-e lxc"' >> /etc/default/docker  
	$ service docker.io restart

##### Libvirt (and KVM)

This is needed in order to run network functions in KVM-based virtual machines.

	; Clone the libvirt repository
	git clone git://libvirt.org/libvirt.git  

To compile and install libvirt, follow the instructions in the 
INSTALL.md file present in the libvirt root folder.
      

### Compile the un-orchestrator

We are now ready to compile the un-orchestrator.

	$ cd orchestrator

	; Choose among possible compilation options
	$ ccmake .  

Here you can select some configuration parameters for the un-orchestrator,
e.g., the NF-FG description, the vSwitch implementation, the Openflow version, etc.
when you're finished, exit from the 'ccmake' interface and type:

	; Create makefile scripts based on the previously selected options
	$ cmake .

	; Compile and create the executables
	$ make
