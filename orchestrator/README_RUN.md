Command lines parameters for the un-orchestrator can be retrieved thorugh the
command:
  sudo ./node-orchestrator --h

The output will change according to the fact that you compiled the un-orchestrator
with the flag READ_JSON_FROM_FILE enabled or not.

===============================================================================

READ_JSON_FROM_FILE *disabled* (default configuration)

Usage: 
  sudo ./name-orchestrator --f file_name
                                                                                         
Parameters:
  --f file_name
        Name of the file containing the physical ports to be handled by the node         
        orchestrator                                                                     
                                                                                         
Options:
  --p tcp_port
        TCP port used by the REST server to receive commands (default is 8080)           
  --c core_mask
        Mask that specifies which cores must be used for DPDK network functions. These   
        cores will be allocated to the DPDK network functions in a round robin fashion   
        (default is 0x2)                                                                 
  --h
        Print this help.                                                                 
                                                                                         
Example:
  sudo ./node-orchestrator --f config/example.xml
  
===============================================================================

READ_JSON_FROM_FILE *enabled*

Usage:
  sudo ./name-orchestrator --p file_name --f file_name
                                                                                         
Parameters:
  --p file_name
        Name of the file containing the physical ports to be handled by the node         
        orchestrator                                                                     
  --f file_name
        Name of the file describing the NF-FG to be deployed on the node                 
                                                                                         
Options:
  --c core_mask
        Mask that specifies which cores must be used for DPDK network functions. These   
        cores will be allocated to the DPDK network functions in a round robin fashion   
        (default is 0x2)                                                                 
  --h
        Print this help.                                                                 
                                                                                         
Example:
  sudo ./node-orchestrator --p example/config.xml --f example.json
    
===============================================================================

Please check config/example.xml to understand the configuration file descrfibing
the physical ports to be handeld by the  un-orchestrator.

===============================================================================

Please check README_COMMANDS to know the commands to be used to interact with
the un-orchestrator.

