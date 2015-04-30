The Universal Node orchestrator (un-orchestrator) is the master of the universal
node. It receives commands through a REST API, and takes care of implementing 
these commands.

More in detail, when it receives a command to deploy a new NF-FG, it does all
the operations required to actually implement the graph: 

* retrive the network function images;
* configure the virtual switch (vSwitch) to create a new LSI and the ports required 
  to connect it to the network functions to be deployed;
* deploy and starts the network functions;
* translates the rules to steer the traffic in openflow flowmod messages to be 
  sent to the vSwitch (some flowmod are sent to the new LSI, others to the LSI-0, 
  i.e. an LSI which steer the traffic into the proper graph.).

Similarly, the node-orchestrator takes care of updating or destroying a graph,
when the proper messages are received.

===============================================================================

The network controller

The network controller is the module exploited by the un-orchestrator to interact
with the vSwitch. It consists of two parts:

* the Openflow controller: a new Openflow controller is created for each new LSI,
and it is used to steer the traffic among the ports of the LSI itself;
* the switch manager: it is used to create/destroy LSI, create/destroy virtual
  ports, and so on. In practice, it allows the un-orchestrator to interact with 
  the vSwitch in order to perform management operations. Each virtual switch
  implementation (e.g., xDPd, OvS) may require a diffrerent implementation for
  the switch manager, according to the commands supported by the vSwitch itself.

If you are interested to add the support for a new virtual switch, please check
the file network_controller/switch_manager/README.

===============================================================================

The compute controller

The compute controller is the module exploited by the un-orchestrator to interact
with the hypervisor.

WARNING: this component is currently no modular, hence it is not possible to
add the support for new hypervisors.

===============================================================================

Please check README_COMPILE to compile the un-orchestrator.

===============================================================================

Please check README_RUN to startup the un-orchestrator.

===============================================================================

Please check README_COMMANDS to know the commands to be used to interact with
the un-orchestrator.
