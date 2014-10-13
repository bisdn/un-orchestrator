#include "nf.h"


NF::NF(string name) :
	name(name), selectedImplementation(NULL), isRunning(false)
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
