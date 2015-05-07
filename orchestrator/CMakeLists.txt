CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(node-orchestrator)

IF(CMAKE_COMPILER_IS_GNUCXX)
        ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings )
ENDIF(CMAKE_COMPILER_IS_GNUCXX)
IF(CMAKE_COMPILER_IS_GNUCC)
        ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings )
ENDIF(CMAKE_COMPILER_IS_GNUCC)


# Rather complicated CMake code for setting the logging level
SET(LOGGING_LEVEL "ORCH_WARNING" CACHE STRING "Orchestrator logging levels: ORCH_DEBUG, ORCH_DEBUG_INFO, ORCH_WARNING, ORCH_ERROR, ORCH_INFO")
SET(LOGGING_LEVEL_VALUES "ORCH_DEBUG" "ORCH_DEBUG_INFO" "ORCH_WARNING" "ORCH_ERROR" "ORCH_INFO")
SET_PROPERTY(CACHE LOGGING_LEVEL PROPERTY STRINGS ${LOGGING_LEVEL_VALUES})
LIST(FIND LOGGING_LEVEL_VALUES ${LOGGING_LEVEL} LOGGING_LEVEL_INDEX)

IF(${LOGGING_LEVEL_INDEX} EQUAL -1)
	message(FATAL_ERROR "Option ${LOGGING_LEVEL} not supported, valid entries are ${LOGGING_LEVEL_VALUES}")
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 0)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_DEBUG)
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 1)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_DEBUG_INFO)
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 2)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_WARNING)
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 3)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_ERROR)
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 4)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_INFO)
ENDIF()
# End of the rather complicated CMake code for setting the logging level

# Rather complicated CMake code for selecting the Openflow version
SET(OFP_VERSION "OFP_12" CACHE STRING "Openflow version: OFP_10, OFP_12, OFP_13")
SET(OFP_VERSION_VALUES "OFP_10" "OFP_12" "OFP_13")
SET_PROPERTY(CACHE OFP_VERSION PROPERTY STRINGS ${OFP_VERSION_VALUES})
LIST(FIND OFP_VERSION_VALUES ${OFP_VERSION} OFP_VERSION_INDEX)

IF(${OFP_VERSION_INDEX} EQUAL -1)
	message(FATAL_ERROR "Option ${OFP_VERSION} not supported, valid entries are ${OFP_VERSION_VALUES}")
ENDIF()
IF(${OFP_VERSION_INDEX} EQUAL 0)
	ADD_DEFINITIONS(-DOFP_VERSION=OFP_10)
ENDIF()
IF(${OFP_VERSION_INDEX} EQUAL 1)
	ADD_DEFINITIONS(-DOFP_VERSION=OFP_12)
ENDIF()
IF(${OFP_VERSION_INDEX} EQUAL 2)
	ADD_DEFINITIONS(-DOFP_VERSION=OFP_13)
ENDIF()
# End of the rather complicated CMake code for setting the Openflow version


# Rather complicated CMake code for selecting the virtual switch implementation
SET(VSWITCH_IMPLEMENTATION "XDPD" CACHE STRING "vSwitch implementation: XDPD")
SET(VSWITCH_IMPLEMENTATION_VALUES "XDPD") #Add here other implementations
SET_PROPERTY(CACHE VSWITCH_IMPLEMENTATION PROPERTY STRINGS ${VSWITCH_IMPLEMENTATION_VALUES})
LIST(FIND VSWITCH_IMPLEMENTATION_VALUES ${VSWITCH_IMPLEMENTATION} VSWITCH_IMPLEMENTATION_INDEX)

IF(${VSWITCH_IMPLEMENTATION_INDEX} EQUAL -1)
	message(FATAL_ERROR "Option ${VSWITCH_IMPLEMENTATION} not supported, valid entries are ${VSWITCH_IMPLEMENTATION_VALUES}")
ENDIF()
IF(${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 0)
	ADD_DEFINITIONS(-DOFP_VSWITCH_IMPLEMENTATION=XDPD)
ENDIF()
# End of the rather complicated CMake code for setting the Openflow version

OPTION(
	RUN_NFS
	"Turn on the actually run the NFs"
	ON
)
IF(RUN_NFS)
	ADD_DEFINITIONS(-DRUN_NFS)
ENDIF(RUN_NFS)

OPTION(
	READ_JSON_FROM_FILE
	"Turn on to read the json describing a graph from file. In this case the REST server does not start."
	OFF
)
IF(READ_JSON_FROM_FILE)
	ADD_DEFINITIONS(-DREAD_JSON_FROM_FILE)
ENDIF(READ_JSON_FROM_FILE)


OPTION(
	ENABLE_DOCKER
	"Turn on the support of NFs implemented in Docker containers"
	ON
)
IF(ENABLE_DOCKER)
	ADD_DEFINITIONS(-DENABLE_DOCKER)
ENDIF(ENABLE_DOCKER)

OPTION(
	ENABLE_KVM
	"Turn on the support of NFs implemented in KVM virtual machines"
	ON
)
IF(ENABLE_KVM)
	ADD_DEFINITIONS(-DENABLE_KVM)
ENDIF(ENABLE_KVM)

OPTION(
	POLITO_MESSAGE
	"Turn on to support a slightly different JSON message describing the graph"
	OFF
)
IF(POLITO_MESSAGE)
	ADD_DEFINITIONS(-DPOLITO_MESSAGE)
ENDIF(POLITO_MESSAGE)

OPTION(
	LOG_1024X768
	"Turn on to print the log for a monitor of 1024 X 768"
	OFF
)
IF(LOG_1024X768)
	ADD_DEFINITIONS(-DLOG_1024X768)
ENDIF(LOG_1024X768)

OPTION(
	DEBUG_OPENFLOW
	"Turn on to enable the debug of Openflow messages"
	OFF
)
IF(DEBUG_OPENFLOW)
	ADD_DEFINITIONS(-DDEBUG_OPENFLOW)
ENDIF(DEBUG_OPENFLOW)


# Set source files
SET(SOURCES
	node_orchestrator.cc
	
	node_resource_manager/graph_manager/graph_manager.h
	node_resource_manager/graph_manager/graph_manager.cc
	node_resource_manager/graph_manager/graph_info.h
	node_resource_manager/graph_manager/graph_info.cc
	node_resource_manager/graph_manager/graph_translator.h
	node_resource_manager/graph_manager/graph_translator.cc
	node_resource_manager/graph_manager/rule_removed_info.h
	node_resource_manager/graph_manager/lsi.h
	node_resource_manager/graph_manager/lsi.cc
	node_resource_manager/graph_manager/virtual_link.h
	node_resource_manager/graph_manager/virtual_link.cc
	node_resource_manager/graph_manager/fileParser.h
	node_resource_manager/graph_manager/fileParser.cc
	
	node_resource_manager/graph/match.h
	node_resource_manager/graph/match.cc
	
	node_resource_manager/graph/low_level_graph/action.h
	node_resource_manager/graph/low_level_graph/action.cc
	node_resource_manager/graph/low_level_graph/graph.h
	node_resource_manager/graph/low_level_graph/graph.cc
	node_resource_manager/graph/low_level_graph/low_level_match.h
	node_resource_manager/graph/low_level_graph/low_level_match.cc
	node_resource_manager/graph/low_level_graph/rule.h
	node_resource_manager/graph/low_level_graph/rule.cc
	
	node_resource_manager/graph/high_level_graph/high_level_action.h
	node_resource_manager/graph/high_level_graph/high_level_action.cc
	node_resource_manager/graph/high_level_graph/high_level_action_nf.h
	node_resource_manager/graph/high_level_graph/high_level_action_nf.cc
	node_resource_manager/graph/high_level_graph/high_level_action_port.h
	node_resource_manager/graph/high_level_graph/high_level_action_port.cc
	node_resource_manager/graph/high_level_graph/high_level_action_endpoint.h
	node_resource_manager/graph/high_level_graph/high_level_action_endpoint.cc
	node_resource_manager/graph/high_level_graph/high_level_rule.h
	node_resource_manager/graph/high_level_graph/high_level_rule.cc
	node_resource_manager/graph/high_level_graph/high_level_graph.h
	node_resource_manager/graph/high_level_graph/high_level_graph.cc
	node_resource_manager/graph/high_level_graph/high_level_match.h
	node_resource_manager/graph/high_level_graph/high_level_match.cc
	
	network_controller/switch_manager/switch_manager.h		
	network_controller/switch_manager/addNFports_in.h
	network_controller/switch_manager/addNFports_out.h
	network_controller/switch_manager/addVirtualLink_in.h
	network_controller/switch_manager/addVirtualLink_out.h
	network_controller/switch_manager/createLSIin.h
	network_controller/switch_manager/createLSIout.h
	network_controller/switch_manager/destroyNFports_in.h
	network_controller/switch_manager/destroyVirtualLink_in.h 
	network_controller/switch_manager/checkPhysicalPorts_in.h 
	
	network_controller/openflow_controller/controller.h
	network_controller/openflow_controller/controller.cc
	
	compute_controller/nfs_manager.h
	compute_controller/nfs_manager.cc
	compute_controller/nf.h
	compute_controller/nf.cc
	compute_controller/nf_type.h
	compute_controller/implementation.h
	compute_controller/implementation.cc
	
	node_resource_manager/rest_server/rest_server.h
	node_resource_manager/rest_server/rest_server.cc
	node_resource_manager/rest_server/match_parser.h
	node_resource_manager/rest_server/match_parser.cc

	utils/logger.h
	utils/logger.c
	utils/constants.h
	utils/sockutils.h
	utils/sockutils.c
)

IF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 0)
    SET(SOURCES2
    ${SOURCES}
    network_controller/switch_manager/plugins/xdpd/xdpd_manager.h
	network_controller/switch_manager/plugins/xdpd/xdpd_manager.cc
	network_controller/switch_manager/plugins/xdpd/xdpd_constants.h
    )
ENDIF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 0)

INCLUDE_DIRECTORIES (
	/usr/include/boost
	/usr/include/libxml2
)


# Create the executable
ADD_EXECUTABLE(
	node-orchestrator
	${SOURCES2}
)



LINK_DIRECTORIES(
	/usr/lib/
	/usr/local/lib/
	/usr/x86_64-linux-gnu/
	/usr/lib/x86_64-linux-gnu/
)


TARGET_LINK_LIBRARIES( node-orchestrator
	libpthread.so
	librofl_common.so
	libjson_spirit.so
	libmicrohttpd.so
	libboost_system.so
	libxml2.so
	-lrt
)

