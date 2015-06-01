Start OVS:

	sudo /usr/share/openvswitch/scripts/ovs-ctl start

===============================================================================
    
OF-CONFIG server can be started by

	sudo ofc-server

By default, ofc-server starts in daemon mode. To avoid daemon mode, pass -f parameter.

ofc-server supports some parameters that can be found in help: ofc-server -h

Useful parameter is -v<level> that specifies level of verbose output.

    sudo ofc-server -v 3 -f

