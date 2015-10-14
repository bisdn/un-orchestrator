# How to run the un-orchestrator

The full list of command line parameters for the un-orchestrator can be
retrieved by the following command:
  
    $ sudo ./node-orchestrator --h

Please refer to the help provided by the node-orchestrator itself in order to
understand how to use the different options.

The un-orchestrator requires a virtual switch up and running in the server,
which is completely independent from this software.

Therefore you need to start your preferred softswitch first, before running
the un-orchestrator. Proper instructions for xDPd and OpenvSwich are provided
below.


### Configuration file examples

Folder 'config' contains some configuration file examples that can be used 
to configure/test the un-orchestrator.

  * config/physical\_ports-example.xml: configuration file describing
    the physical ports to be handeld by the un-orchestrator.
  * config/simple\_passthrough\_nffg.json: simple graph that implements
    a simple passthrough function, i.e., traffic is received from a first
    physical port and sent out from a second physical port, after having
    been handled to the vswitch.


### How to start xDPd to work with the un-orchestrator

Start xDPd:

	$ cd [xdpd]/build/src/xdpd
	$ sudo ./xdpd
    
xDPd comes with a command line tool called xcli, that can be used to check 
the  flows installed in the lsis, which are the lsis deployed, see statistics 
on flows matched, and so on. The xcli can be run by just typing:

	$ xcli


### How to start OpenvSwitch to work with the un-orchestrator

Start OVS:

	$ sudo /usr/share/openvswitch/scripts/ovs-ctl start

In addition, you have to start the OF-CONFIG server, which represents the
daemon the implements the protocol used to configure the switch.

OF-CONFIG server can be started by

	$ sudo ofc-server

By default, ofc-server starts in daemon mode. To avoid daemon mode, use the
'-f' parameter.
For a the full list of the supported parameters, type:

    $ ofc-server -h
    

### How to start OVSDB to work with the un-orchestrator
    
Start OVS:

	$ sudo /usr/share/openvswitch/scripts/ovs-ctl start

Start ovsdb-server

	$ sudo ovs-appctl -t ovsdb-server ovsdb-server/add-remote ptcp:6632
