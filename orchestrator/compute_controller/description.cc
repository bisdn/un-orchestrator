#include "description.h"

Description::Description(nf_t type, string uri, string cores, string location) :
	type(type), uri(uri), cores(cores), location(location)
{
}

Description::Description(string type, string uri, string cores, string location) :
	 uri(uri), cores(cores), location(location)
{

	if(type == "dpdk")
	{
		this->type = DPDK;
		return;
	}
#ifdef ENABLE_DOCKER
	else if(type == "docker")
	{
		this->type = DOCKER;
		return;
	} 
#endif	
#ifdef ENABLE_KVM
	else if(type == "kvm")
	{
		this->type = KVM;
		return;
	} 
#endif	

	//[+] Add here other implementations for the execution environment

	assert(0);
	return;
}

nf_t Description::getType()
{
	return type;
}

string Description::getURI()
{
	return uri;
}

string Description::getCores()
{
	return cores;
}

string Description::getLocation()
{
	assert(type == DPDK 
#ifdef ENABLE_KVM
	|| type == KVM
#endif
	);

	return location;
}
