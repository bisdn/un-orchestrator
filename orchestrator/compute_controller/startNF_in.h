#ifndef StartNFIn_H_
#define StartNFIn_ 1

#pragma once

#include <string>
#include <map>
#include <inttypes.h>

/**
* @file StartNFIn_in.h
*
* @brief Parameters to be used to start the network function.
*/

using namespace std;

class StartNFIn
{

friend class ComputeController;

private:

	/**
	*	@brief: identifier of the LSI to which the VNF is connected
	*/
	uint64_t lsiID;
	
	/**
	*	@brief: name of the network function
	*/
	string nf_name;
	
	/**
	*	@brief: number of ports of the network function
	*/
	unsigned int number_of_ports;
		
	/**
	*	@brief: IPv4 parameters (address and netmask) to be associated with the ports of the network function
	*/
	map<unsigned int,pair<string,string> > ipv4PortsRequirements;
	
	/**
	*	@brief: ethernet parameter (MAC address) to be associated with the ports of the network function
	*/
	map<unsigned int,string> ethPortsRequirements;

protected:
	StartNFIn(uint64_t lsiID, string nf_name, unsigned int number_of_ports, map<unsigned int,pair<string,string> > ipv4PortsRequirements, map<unsigned int,string> ethPortsRequirements) 
		: lsiID(lsiID), nf_name(nf_name), number_of_ports(number_of_ports), ipv4PortsRequirements(ipv4PortsRequirements), ethPortsRequirements(ethPortsRequirements)
	{
	}
	
public:

	uint64_t getLsiID()
	{
		return lsiID;
	}
	
	string getNfName()
	{
		return nf_name;
	}
	
	unsigned int getNumberOfPorts()
	{
		return number_of_ports;
	}
	 
	map<unsigned int,pair<string,string> > getIpv4PortsRequirements()
	{
		return ipv4PortsRequirements;
	}
	
	map<unsigned int,string> getEthPortsRequirements()
	{
		return ethPortsRequirements;
	}
};


#endif //StartNFIn_H_
