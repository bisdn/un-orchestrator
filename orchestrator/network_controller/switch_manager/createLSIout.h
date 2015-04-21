#ifndef CreateLsiOut_H_
#define CreateLsiOut_ 1

#pragma once

#include <string>
#include <list>
#include <map>

/**
* @file createLSIout.h
*
* @brief Description of an LSI created.
*/

using namespace std;

class CreateLsiOut
{

friend class GraphManager;

private:
	uint64_t dpid;
	
	/**
	* @brief: list of physical port name, identifier on the lsi
	*/
	map<string,unsigned int> physical_ports;
		
	/**
	*	@brief: map of network functions name, map of network fuction ports name, network function ports identifier on the lsi
	*/
	map<string,map<string, unsigned int> >  network_functions_ports;
	
	/**
	*	@brief: list of virtual link identifier on the new lsi, virtual link identifier on the remote lsi
	*/
	list<pair<unsigned int, unsigned int> > virtual_links;
	
protected:

	uint64_t getDpid()
	{
		return dpid;
	}
	
	map<string,unsigned int> getPhysicalPorts()
	{
		return physical_ports;
	}
	
	map<string,map<string, unsigned int> > getNetworkFunctionsPorts()
	{
		return network_functions_ports;
	}
	
	list<pair<unsigned int, unsigned int> > getVirtualLinks()
	{
		return virtual_links;
	}

public:
	CreateLsiOut(uint64_t dpid, map<string,unsigned int> physical_ports, map<string,map<string, unsigned int> >  network_functions_ports, list<pair<unsigned int, unsigned int> > virtual_links) 
		: dpid(dpid), physical_ports(physical_ports.begin(),physical_ports.end()),network_functions_ports(network_functions_ports.begin(),network_functions_ports.end()),virtual_links(virtual_links.begin(),virtual_links.end())
	{}
	
};

#endif //CreateLsiOut_H_
