#ifndef NFsManager_H_
#define NFsManager_H_ 1

#pragma once

#include "startNF_in.h"
#include "stopNF_in.h"
#include "implementation.h"

/**
* @file nfs_manager.h
*
* @brief Network functions manager interface. It must be properly implemented for each vSwitch supported by the node.
*/

using namespace std;

class Implementation;

class NFsManager
{
protected:
	/**
	*	@brief: Description of the implementation of the network function
	*/
	Implementation *implementation;

public:

	virtual ~NFsManager() {}

	/**
	*	@brief: check wheter the execution environment is supported or not
	*/
	virtual bool isSupported() = 0;

	/**
	*	@brief:	Retrieve and start the network function
	*/
	virtual bool startNF(StartNFIn sni) = 0;
	
	/**
	*	@brief: stop the network function
	*/
	virtual bool stopNF(StopNFIn sni) = 0;
	
	/**
	*	@brief: set the description of the network function to be handled by the manager
	*/
	void setImplementation(Implementation *implementation);
	
	/**
	*	@brief: provide the type of the network function handled by the manager
	*/
	nf_t getNFType();
	
	/**
	*	@brief: returns the number of cores to be associated with the network function
	*			handled by the manager. "" means that no core has to be bound to the
	*			network function.
	*/
	string getCores();
};

class NFsManagerException: public exception
{
public:
	virtual const char* what() const throw()
	{
		return "NFsManagerException";
	}
};

#endif //NFsManager_H_
