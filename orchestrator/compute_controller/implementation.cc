#include "implementation.h"

Implementation::Implementation(nf_t type, string uri, string cores, string location) :
	type(type), uri(uri), cores(cores), location(location)
{
}

Implementation::Implementation(string type, string uri, string cores, string location) :
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

nf_t Implementation::getType()
{
	return type;
}

string Implementation::getURI()
{
	return uri;
}

string Implementation::getCores()
{
	return cores;
}

string Implementation::getLocation()
{
	assert(type == DPDK 
#ifdef ENABLE_KVM
	|| type == KVM
#endif
	);

	return location;
}
