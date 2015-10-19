The network functions manager allows the un-orchestrator to interact with the 
execution environment for management purposes, such as the startup and the
shutdown of network functions.

Thanks to the proper plugin, the un-orchestrator can interact with a specific 
execution environment. New plugins (and then the support for a new execution
environment) can be added in the folder "compute_controller/plugins".
Each plugin is required to define a class implementing the interface 
"compute_controller/nfs_manager.h". Moreover, the files:
* orchestrator/CMakeLists.txt
* orchestrator/compute_controller/compute_controller.h
* orchestrator/compute_controller/compute_controller.cc
* orchestrator/compute_controller/implementation.cc
* orchestrator/compute_controller/nf_type.h
must be properly updated so that the new plugin is recognized by system.
	
