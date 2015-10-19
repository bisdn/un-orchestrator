#ifndef NF_H_
#define NF_H_ 1

#pragma once

#include <string>
#include <assert.h>
#include <list>

#include "../utils/constants.h"
#include "description.h"
#include "nfs_manager.h"

using namespace std;

class NF
{
private:
	/**
	*	@brief: name of the NF. This should be unique
	*/
	string name;
	
#ifdef UNIFY_NFFG
	/**
	*	@brief: number of ports of a NF
	*/
	unsigned int numPorts;
	
	/**
	*	@brief: text describing the NF
	*/
	string text_description;
#endif
	
	/**
	*	@brief: available descriptions of the NF
	*/
	list<Description*> descriptions;
	
	/**
	*	@brief: manager associated with the selected description for this NF
	*/
	NFsManager *selectedDescription;	
		
	/**
	*	@brief: true if the network function is running, false otherwise
	*/
	bool isRunning;
	
public:
#ifdef UNIFY_NFFG
	NF(string name, unsigned int numPorts, string text_description);
#else
	NF(string name);
#endif
	
	void addDescription(Description *description);
	list<Description*> getAvailableDescriptions();
	
	void setSelectedDescription(NFsManager *impl);
	NFsManager *getSelectedDescription();
	
	void setRunning(bool val);
	bool getRunning();
	
	string getName();
	
#ifdef UNIFY_NFFG
	unsigned int getNumPorts();
	string getTextDescription();
#endif	
};

#endif //NF_H_ 1
