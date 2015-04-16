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
	"Turn on to enable the debug of Openflow flowmod messages"
	OFF
)
IF(DEBUG_OPENFLOW)
	ADD_DEFINITIONS(-DDEBUG_OPENFLOW)
ENDIF(DEBUG_OPENFLOW)


# Set source files
SET(SOURCES
	node_orchestrator.cc
	
	graph/match.h
	graph/match.cc
	
	graph/low_level_graph/action.h
	graph/low_level_graph/action.cc
	graph/low_level_graph/graph.h
	graph/low_level_graph/graph.cc
	graph/low_level_graph/low_level_match.h
	graph/low_level_graph/low_level_match.cc
	graph/low_level_graph/rule.h
	graph/low_level_graph/rule.cc
	
	graph/high_level_graph/high_level_action.h
	graph/high_level_graph/high_level_action.cc
	graph/high_level_graph/high_level_action_nf.h
	graph/high_level_graph/high_level_action_nf.cc
	graph/high_level_graph/high_level_action_port.h
	graph/high_level_graph/high_level_action_port.cc
	graph/high_level_graph/high_level_action_endpoint.h
	graph/high_level_graph/high_level_action_endpoint.cc
	graph/high_level_graph/high_level_rule.h
	graph/high_level_graph/high_level_rule.cc
	graph/high_level_graph/high_level_graph.h
	graph/high_level_graph/high_level_graph.cc
	graph/high_level_graph/high_level_match.h
	graph/high_level_graph/high_level_match.cc
	
	switch_manager/switch_manager.h
	switch_manager/lsi.h
	
	switch_manager/xdpd/xdpd_lsi.h
	switch_manager/xdpd/xdpd_lsi.cc
	switch_manager/xdpd/xdpd_manager.h
	switch_manager/xdpd/xdpd_manager.cc
	switch_manager/xdpd/virtual_link.h
	switch_manager/xdpd/virtual_link.cc

	graph_manager/graph_manager.h
	graph_manager/graph_manager.cc
	graph_manager/graph_info.h
	graph_manager/graph_info.cc
	graph_manager/graph_translator.h
	graph_manager/graph_translator.cc
	graph_manager/rule_removed_info.h
	
	controller/controller.h
	controller/controller.cc
	
	nfs_manager/nfs_manager.h
	nfs_manager/nfs_manager.cc
	nfs_manager/nf.h
	nfs_manager/nf.cc
	nfs_manager/nf_type.h
	nfs_manager/implementation.h
	nfs_manager/implementation.cc
	
	rest_server/rest_server.h
	rest_server/rest_server.cc
	rest_server/match_parser.h
	rest_server/match_parser.cc

	utils/logger.h
	utils/logger.c
	utils/constants.h
	utils/sockutils.h
	utils/sockutils.c
)

INCLUDE_DIRECTORIES (
	/usr/include/boost
)


# Create the executable
ADD_EXECUTABLE(
	node-orchestrator
	${SOURCES}
)



LINK_DIRECTORIES(
	/usr/lib/
	/usr/local/lib/
	/usr/x86_64-linux-gnu/
	/usr/lib/x86_64-linux-gnu/
)


TARGET_LINK_LIBRARIES( node-orchestrator
	libpthread.so
	librofl.so
	libjson_spirit.so
	libmicrohttpd.so
	libboost_system.so
	-lrt
)

