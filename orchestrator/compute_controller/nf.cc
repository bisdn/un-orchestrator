#include "nf.h"

#ifdef UNIFY_NFFG
NF::NF(string name, unsigned int numPorts, string description) :
	name(name), numPorts(numPorts), description(description), selectedImplementation(NULL), isRunning(false)
#else
NF::NF(string name) :
	name(name), selectedImplementation(NULL), isRunning(false)
#endif
{

}

void NF::addImplementation(Implementation *implementation)
{
	implementations.push_back(implementation);
}

list<Implementation*> NF::getAvailableImplementations()
{
	return implementations;
}

void NF::setSelectedImplementation(Implementation *impl)
{
	selectedImplementation = impl;
	
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Selected a \%s\" implementation for NF \"%s\"", NFType::toString(selectedImplementation->getType()).c_str() ,name.c_str());
}

Implementation *NF::getSelectedImplementation()
{
	return selectedImplementation;
}

bool NF::getRunning()
{
	return isRunning;
}

void NF::setRunning(bool val)
{
	isRunning = val;
}

string NF::getName()
{
	return name;
}

#ifdef UNIFY_NFFG
	unsigned int NF::getNumPorts()
	{
		return numPorts;
	}
	
	string NF::getDescription()
	{
		return description;
	}
#endif	

