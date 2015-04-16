#ifndef SwitchManager_H_
#define SwitchManager_ 1

#pragma once

#include "lsi.h"
#include "xdpd/virtual_link.h"	//IVANO: link the abstract virtual_link

#include "../nfs_manager/nf_type.h"

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include <string>
#include <list>
#include <sstream>

using namespace std;
using namespace json_spirit;

class SwitchManager
{
protected:

	SwitchManager() {}

public:

	virtual ~SwitchManager() {}

	/**
	*	@brief: Cretes a new LSI
	*
	*	@param: lsi		Description of the LSI
	*					to be created
	*/
	virtual void createLsi(LSI &lsi) = 0;
	
	/**
	*	@brief: Create NF ports of a specific NF on an LSI
	*
	*	@brief: lsi		Description of the LSI containing the
	*					NF ports to be created
	*	@brief: nf		Name and port idendifiers of the NF whose ports must be created
	*	@brief: type	Type of the NF associated with the ports to be created
	*/
	virtual void addNFPorts(LSI &lsi,pair<string, list<unsigned int> > nf, nf_t type) = 0;
	
	/**
	*	@brief: Destroy add a virtual link to an LSI
	*
	*	@param: lsi		Description of the LSI containing the vlink
	*					to be added
	*	@param: vlink	Structure representing the virtual link to
	*					to be added to the LSI
	*/
	virtual uint64_t addVirtualLink(LSI &lsi, VLink vlink) = 0;
	
	/**
	*	@brief: Destroy an existing LSI
	*
	*	@param: lsi		Description of the LSI
	*					to be destroyed
	*/
	virtual void destroyLsi(LSI &lis) = 0;
	
	/**
	*	@brief: Destroy all the NF ports of a specific NF
	*
	*	@brief: lsi		Description of the LSI containing the
	*					NF ports to be removed
	*	@brief: nf		Name of the NF whose ports must be removed
	*/
	virtual void destroyNFPorts(LSI &lsi,string nf) = 0;
	
	/**
	*	@brief: Destroy an virtual link from an LSI
	*
	*	@param: lsi		Description of the LSI containing the vlink
	*					to be removed
	*	@param: vlinkID	Identifier of the vlink to be removed
	*/
	virtual void destroyVirtualLink(LSI &lsi, uint64_t vlinkID) = 0; 
	
	/**
	*	@brief: Discover the physical interfaces
	*/
	virtual map<string,string> discoverPhyPorts() = 0;
};

class XDPDManagerException: public exception
{
public:
	virtual const char* what() const throw()
	{
		return "xDPDManagerException";
	}
};

#endif //SwitchManager_H_
