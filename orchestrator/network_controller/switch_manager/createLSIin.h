#ifndef CreateLsiIn_H_
#define CreateLsiIn_ 1

#pragma once

#include "../../compute_controller/nf_type.h"

#include <string>
#include <list>
#include <inttypes.h>
#include <map>
#include <set>

/**
* @file createLSIin.h
*
* @brief Description of an LSI to be created.
*/

using namespace std;

class CreateLsiIn
{

friend class GraphManager;

private:

	/**
	*	@brief: IPv4 address of the Openflow controller
	*/
	string controllerAddress;
	
	/**
	*	@brief: TCP port of the Openflow controller
	*/
	string controllerPort;
	
	/**
	*	@brief: list of physical ports to be connected to the lsi
	*/
	list<string> physicalPortsName;
	
	/**
	*	@brief: map of network functions name, network functions type
	*/
	map<string,nf_t>  nf_types;
	
	/**
	*	@brief: set of network functions name
	*/
	//FIXME: useless? This information can be retrieved by the previous map
	set<string> networkFunctionsName;
	
	/**
	*	@brief: map of network functions, list of network function ports
	*/
	map<string,list<string> > netFunctionsPortsName;
	
	/**
	*	@brief: list of lsis with which the new one must be connected
	*/
	list<uint64_t> vlinksRemoteLsi;

protected:
	CreateLsiIn(string controllerAddress, string controllerPort, list<string> physicalPortsName, map<string,nf_t>  nf_types, map<string,list<string> > netFunctionsPortsName, list<uint64_t> vlinksRemoteLsi) 
		: controllerAddress(controllerAddress), controllerPort(controllerPort), 
		physicalPortsName(physicalPortsName.begin(),physicalPortsName.end()),
		nf_types(nf_types.begin(),nf_types.end()),
		netFunctionsPortsName(netFunctionsPortsName.begin(),netFunctionsPortsName.end()),
		vlinksRemoteLsi(vlinksRemoteLsi.begin(),vlinksRemoteLsi.end())
	{
		map<string,nf_t>::iterator it = nf_types.begin();
		for(; it != nf_types.end(); it++)
			networkFunctionsName.insert(it->first);
	}
	
public:
	
	string getControllerAddress()
	{
		return controllerAddress;
	}

	string getControllerPort()
	{
		return controllerPort;
	}
	
	list<string> getPhysicalPortsName()
	{	
		return physicalPortsName;
	}
	
	map<string,nf_t> getNetworkFunctionsType()
	{
		return nf_types;
	}

	set<string> getNetworkFunctionsName()
	{			
		return networkFunctionsName;
	}
	
	list<string> getNetworkFunctionsPortNames(string nf)
	{
		return netFunctionsPortsName[nf];
	}
	
	list<uint64_t> getVirtualLinksRemoteLSI()
	{
		return vlinksRemoteLsi;
	}
};


#endif //CreateLsiOut_H_
