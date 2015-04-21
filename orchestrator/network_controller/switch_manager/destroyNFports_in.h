#ifndef DestroyNFportsIn_H_
#define DestroyNFportsIn_ 1

#pragma once

#include <string>
#include <inttypes.h>

/**
* @file destroyNFports_in.h
*
* @brief Description of network function ports destroyed.
*/

using namespace std;

class DestroyNFportsIn
{

friend class GraphManager;

private:

	/**
	*	@brief: identifier of the lsi to which the network function ports must be removed
	*/
	uint64_t dpid;
	
	/**
	*	@brief: name of the network functions whose ports must be destroyed
	*/
	string nf_name;
	
	/**
	*	@brief: network function ports to be destroyed
	*/
	set<string> nf_ports;

protected:
	DestroyNFportsIn(uint64_t dpid, string nf_name, set<string> nf_ports)
		: dpid(dpid), nf_name(nf_name), nf_ports(nf_ports)
	{
	}
	
public:

	uint64_t getDpid()
	{
		return dpid;
	}
	
	string getNFname()
	{
		return nf_name;
	}
	
	set<string> getNFports()
	{
		return nf_ports;
	}
	
};


#endif //DestroyNFportsIn_H_
