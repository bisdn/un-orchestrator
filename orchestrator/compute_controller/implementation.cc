#include "implementation.h"

Implementation::Implementation(nf_t type, string uri, string cores, string location) :
	type(type), uri(uri), cores(cores), location(location)
{
}

Implementation::Implementation(string type, string uri, string cores, string location) :
	type((type == "dpdk")? DPDK : ((type == "docker")? DOCKER : KVM)), uri(uri), cores(cores), location(location)
{
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
	assert(type == DPDK);
	return cores;
}

string Implementation::getLocation()
{
	assert(type == DPDK || type == KVM);
	return location;
}
