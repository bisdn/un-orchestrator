#ifndef Virtualizer_H_
#define Virtualizer_H_ 1

#pragma once

#include <Python.h>
#include "../../utils/constants.h"
#include "../../utils/logger.h"
#include "../../compute_controller/nf.h"
#include "../../compute_controller/implementation.h"

#include <string>
#include <map>
#include <set>
#include <iostream>
#include <sstream>

using namespace std;

class Virtualizer
{
private:

	/**
	*	@brief: maps the physical name of a port on the name exported by the virtualizer
	*/
	static map<string,string> nameVirtualization;

	/**
	*	@brief: maps the ID of a VNF, with a specific type of a specific VNF.
	*		E.e.,	VNF1 -> NAT,KVM, VNF2 -> FIREWALL,DPDK
	*/
	static map<string,pair<string,string> > NFVirtualization;
	
	/**
	*	@brief: number used to generate the ID of a VNF
	*/
	static unsigned int currentID;

public:

	/**
	*	@brief: Initializes the virtualizer
	*/
	static bool init();
	
	/**
	*	@brief: Terminates the virtualizer
	*/
	static void terminate();
	
	/**
	*	@brief: Sets the resources of the node in the virtualizer
	*
	*	@param:	cpu				Number of CPUs of the node
	*	@param:	memory			Size of the memory of the node
	*	@param:	meemory_unit	Indicate the measurement unit for the memory
	*	@param:	storage			Size of the storage of the node
	*	@param:	storage_unit	Indicate the measurement unit for the storage
	*/
	static bool addResources(int cpu, int memory, char *memory_unit, int storage, char *storage_unit);
	
	/**
	*	@brief: Sets a port of the node in the virtualizer
	*
	*	@param:	physicalName	Real name of the port
	*	@param:	name			Name of the port as to be exported by the virtualizer
	*	@param:	memory			Type of the port
	*/
	static bool addPort(char *physicalName, char *name, char *type);
	
	/**
	*	@brief: Edit the port ID of a specific port in the virtualizer
	*
	*	@param: portName	Physical name of the port whose ID has to be replaced (the virtualizer
	*						know the mapping between this name and the virtualized one)
	*	@param: ID			New ID to be associated with the port
	*/
	static bool EditPortID(string portName, unsigned int ID);
	
	/**
	*	@brief: Adds to the virtualizer the VNFs supported by the Universal node
	*
	*	@param: nfs			Set of the VNFs supported by the Universal node
	*/
	static bool addSupportedVNFs(set<NF*> nfs);

};

#endif //Virtualizer_H_
