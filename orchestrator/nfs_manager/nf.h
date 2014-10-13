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
	NF(string name);
	
	void addImplementation(Implementation *implementation);
	list<Implementation*> getAvailableImplementations();
	
	void setSelectedImplementation(Implementation *impl);
	Implementation *getSelectedImplementation();
};

#endif //NF_H_ 1
