#ifndef NF_H_
#define NF_H_ 1

#pragma once

#include <string>
#include <assert.h>
#include <list>

#include "../utils/constants.h"
#include "implementation.h"

using namespace std;

class NF
{
private:
	/**
	*	@brief: name of the NF
	*/
	string name;
	
#ifdef UNIFY_NFFG
	/**
	*	@brief: number of ports of a VNF
	*/
	unsigned int numPorts;
#endif
	
	/**
	*	@brief: available implementations of the NF
	*/
	list<Implementation*> implementations;
		
	Implementation *selectedImplementation;	
		
	/**
	*	@brief: true if the network function is running, false otherwise
	*/
	bool isRunning;
	
public:
#ifdef UNIFY_NFFG
	NF(string name, unsigned int numPorts); //FIXME: togliere da opzionale
#else
	NF(string name);
#endif
	
	void addImplementation(Implementation *implementation);
	list<Implementation*> getAvailableImplementations();
	
	void setSelectedImplementation(Implementation *impl);
	Implementation *getSelectedImplementation();
	
	void setRunning(bool val);
	bool getRunning();
	
	string getName();
	
#ifdef UNIFY_NFFG
	unsigned int getNumPorts();
#endif	
};

#endif //NF_H_ 1
