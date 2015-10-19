#include "nf.h"

#ifdef UNIFY_NFFG
NF::NF(string name, unsigned int numPorts, string text_description) :
	name(name), numPorts(numPorts), text_description(text_description), selectedDescription(NULL), isRunning(false)
#else
NF::NF(string name) :
	name(name), selectedDescription(NULL), isRunning(false)
#endif
{

}

void NF::addDescription(Description *description)
{
	descriptions.push_back(description);
}

list<Description*> NF::getAvailableDescriptions()
{
	return descriptions;
}

void NF::setSelectedDescription(NFsManager *impl)
{
	selectedDescription = impl;
}

NFsManager *NF::getSelectedDescription()
{
	return selectedDescription;
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
	
	string NF::getTextDescription()
	{
		return text_description;
	}
#endif	

