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

	; Install required libraries
	; - build-essential: it includes GCC, basic libraries, etc
	; - cmake: to create cross-platform makefiles
	; - cmake-curses-gui: nice 'gui' to edit cmake files
	; - libboost-all-dev: nice c++ library with tons of useful functions
	;  -libmicrohttpd-dev: embedded micro http server
	$ sudo apt-get install build-essential cmake cmake-curses-gui libboost-all-dev libmicrohttpd-dev

	; Install JSON Spirit (nice library to parse JSON files)
	$ git clone https://github.com/sirikata/json-spirit
	$ cd json-spirit/

	; Now install the above library according to the description provided
	; in the cloned folder

	; Install ROFL-common  (library to parse OpenFlow messages)
	$ git clone https://github.com/bisdn/rofl-common  
	$ cd rofl-core/

	; Now install the above library according to the description provided
	; in the cloned folder


### Install the proper virtual switch

The current un-orchestrator supports different types of virtual switches.
You have to install the one that you want to use, choosing from the
possibilities listed in this section.


#### xDPd

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


#### Open vSwitch (OVS)

The list of OF-CONFIG dependencies:

- libnetconf 0.9.x, not higher

- compiler (gcc, clang,...) and standard headers
- pkg-config
- libpthreads
- libxml2 (including headers from the devel package)
- libssh >= 0.6.4 (including headers)
	- Download it from https://red.libssh.org/projects/libssh/files and for install it, follow the instructions in the INSTALL file present in a root directory
 - can be skipped if using --disable-ssh
 
- pyang >= 1.5.0
- python 2.6 or higher with the following modules:
 - os, copy, string, re, argparse, subprocess, inspect, curses, xml, libxml2
 - only with TLS enabled: M2Crypto

- only with TLS enabled by using the --enable-tls option
 - OpenSSL (libssl, libcrypto, including headers from the devel package)

- roff2html
 - optional, used for building HTML version of man pages (make doc)
- rpmbuild
 - optional, used for building RPM package (make rpm).

Compile and install libnetconf as described here, including headers from the devel package:

	; Clone the libnetconf repository
	$ git clone https://github.com/cesnet/libnetconf
    $ cd libnetconf/
    $ git checkout -b 0.9.x origin/0.9.x

Install the libnetconf library by following the instructions in the
INSTALL file contained in the root folder of this library.

You can now install OpenvSwitch:

	; Clone the openvswitch repository
	$ git clone https://github.com/openvswitch/of-config    

Follow the instructions as described in the file INSTALL.md provided
in the root folder of that repository.


### Virtual Execution Environment for network functions

The current un-orchestrator supports different types of execution environments.
You have to install the ones that you want to use, selecting one or more
possibilities from the ones listed in this section.

#### Docker

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

#### Libvirt (and KVM)

	; Clone the libvirt repository
	git clone git://libvirt.org/libvirt.git  

To compile and install libvirt, follow the instructions in the
INSTALL.md file present in the libvirt root folder.

If you run Libvirt for OVS or OVSDB should be use a template presents in a folder "compute_controller/nf_repository/kvm"

### Compile the un-orchestrator

We are now ready to compile the un-orchestrator.

	$ cd orchestrator

	; Choose among possible compilation options
	$ ccmake .  

The previous command allows you to select some configuration parameters for the
un-orchestrator, such as the virtual switch used, the version of Openflow to
use, which kind of execution environment you want to enable, etc.
When you're finished, exit from the 'ccmake' interface by *generating the
configuration files* and type the following commands:

	; Create makefile scripts based on the previously selected options
	$ cmake .

	; Compile and create the executables
	$ make
