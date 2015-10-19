#ifndef COMPUTE_CONTROLLER_H_
#define COMPUTE_CONTROLLER_H_ 1

#pragma once

#include <list>

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <pthread.h>
#include <inttypes.h>

#include "../utils/logger.h"
#include "../utils/constants.h"
#include "../utils/sockutils.h"
#include "nf.h"

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#ifdef UNIFY_NFFG
	#include "../node_resource_manager/virtualizer/virtualizer.h"
#endif

#include "nfs_manager.h"

#include "plugins/dpdk/dpdk.h"
#ifdef ENABLE_DOCKER
	#include "plugins/docker/docker.h"
#endif
#ifdef ENABLE_KVM
	#include "plugins/kvm-libvirt/libvirt.h"
#endif
//[+] Add here other implementations for the execution environment

using namespace std;
using namespace json_spirit;

#define DATABASE_ADDRESS	"localhost"
#define DATABASE_PORT		"2828"
#define DATABASE_BASE_URL	"/nfs/"
#define DATABASE_DIGEST_URL	"digest/"

#define CODE_POSITION				9
#define CODE_METHOD_NOT_ALLLOWED	"405"
#define CODE_OK						"200"

class Implementation;

typedef enum{NFManager_OK,NFManager_SERVER_ERROR, NFManager_NO_NF}nf_manager_ret_t;

class ComputeController
{
private:
	
	/**
	*	@brief: mutex used to select the core to be allocated to a (DPDK) NF
	*/
	static pthread_mutex_t nfs_manager_mutex;
	
	/**
	*	@brief: the CPU cores that can be allocated to DPDK NFs
	*/
	static map<int,uint64_t> cores;
	
	/**
	*	@brief: position, in the vector above, of the next CPU core to be
	*	allocated to a DPDK NF
	*/
	static int nextCore;

	/**
	* 	@brief: the pair is <network function name, network function>
	**/
	map<string, NF*> nfs;
	
	/**
	*	@brief: identifier of the LSI attached to the NFs
	**/
	uint64_t lsiID;

	/**
	*	@brief: parse the JSON answer received from the name translator database
	*
	*	@param:	answer	Answer to be parsed
	*	@param:	nf		Name of the network function whose description must be in the asnwer
	*/
	bool parseAnswer(string answer, string nf);
	
	/**
	*	@brief: calculate the core mask for a DPDK NF
	*
	*	@param:	coresRequired	Number of cores required
	*/
	uint64_t calculateCoreMask(string coresRequired);
	
	/**
	*	@brief: For all the NF without an alredy selected implementation, select 
	*	an implementation of the desired type, if at least one implementation
	*	of such a type is availabled
	*
	*	@param:	desiredType	Type of the implementation to be selected
	*/
	void selectImplementation(nf_t desiredType);
	
	/**
	*	@brief: Check if an implementation for all the NFs has been selected. In this
	*	case the return value is true, otherwise it is false
	*/
	bool allSelected();

public:
	ComputeController();
	~ComputeController();

	/**
	*	@brief: Retrieve, from the name resolver, the information for a specific NF
	*
	*	@param:	nf	Name of a network function
	*/
	nf_manager_ret_t retrieveDescription(string nf);
	
	/**
	*	@brief: For each NF, select an implementation. Currently, if a Docker implementation 
	*	is available and Docker is running with the LXC engine, Docker is selected.
	*	Otherwise, a DPDK implementation is selected. Only in case Docker and DPDK
	*	implementations are not available, it selects a KVM implementation (if KVM is
	*	running and a KVM implementation is available).
	*	Summarizing, the priority of the implementations is the following 
	*		- Docker
	*		- DPDK
	*		- KVM
	*/
	bool selectImplementation();
	
#ifdef UNIFY_NFFG
	/**
	*	@brief: Return the number of ports of a specific NF
	*
	*	@param:	name	Name of a network function
	*/
	unsigned int getNumPorts(string name);
#endif
	
	/**
	*	@brief: Return the type selected for a specific NF
	*
	*	@param:	name	Name of a network function
	*/
	nf_t getNFType(string name);
	
	/**
	*	@brief: Set the identifier of the identifier of the LSI attached to the NFs 
	*
	*	@param:	lsiID	Identifier of an LSI
	*/
	void setLsiID(uint64_t lsiID);
	
	/**
	*	@brief: Start the NF with a specific name, with a proper number of ports. The name of these ports
	*	is calculated by this function starting from the LSI identifier and the name of the NF to be started.
	*
	*
	*	@param:	nf_name					Name of the network function to be started
	*	@param: number_of_ports			Number of ports of the network function
	*	@param: ipv4PortsRequirements	IPv4 requirements (address and netmask) of the ports of the network function
	*	@param: ethPortsRequirements	Ethernet requirements (address) of the ports of the network function
	*	FIXME: the requirements are ignored in case of DPDK and KVM network function. Check this earlier in the code
	*/
	bool startNF(string nf_name, unsigned int number_of_ports, map<unsigned int,pair<string,string> > ipv4PortsRequirements,map<unsigned int,string> ethPortsRequirements);
	
	/**
	*	@brief: Stop all the running NFs
	*/
	void stopAll();
	
	/**
	*	@brief: Stop a specific NF
	*
	*	@param:	nf_name	Name of the network function to be stopped
	*/
	bool stopNF(string nf_name);
	
	/**
	*	@brief: Set the core mask representing the cores to be used for DPDK processes. The available cores will
	*	be allocated to DPDK NFs in a round robin fashion, and each DPDK network functions will have just one
	*	core
	*
	*	@param:	core_mask	Mask representing the cores to be allocated to DPDK network functions
	*/
	static void setCoreMask(uint64_t core_mask);
	
	/**
	*	@brief: prints information on the VNFs deployed
	*/
	void printInfo(int graph_id);
	
#ifdef UNIFY_NFFG	
	/**
	*	@brief:	Retrieves information about all the available VNFs, in order to set the virtualizer
	*/
	static nf_manager_ret_t retrieveAllAvailableNFs();
#endif
};

#endif //COMPUTE_CONTROLLER_H_
