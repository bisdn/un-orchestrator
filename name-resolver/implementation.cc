#include "implementation.h"

Implementation::Implementation(nf_t type, string uri, string cores, string location) :
	type(type), uri(uri), cores(cores), location(location)
{
}

Object Implementation::toJSON()
{
	Object impl;

	impl["uri"]  = uri;
	impl["type"]  = (type == DPDK)? "dpdk" : ((type == DOCKER)? "docker" : "kvm");

	if(type == DPDK)
	{
		impl["cores"] = cores;
		impl["location"] = location;
	}

	return impl;
}
