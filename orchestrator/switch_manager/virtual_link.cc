#include "virtual_link.h"

uint64_t VLink::nextID = 0;

VLink::VLink(uint64_t remote_dpid) :
	remote_dpid(remote_dpid), local_id(0), remote_id(0) 
{
	ID = nextID;
	nextID++;
}
	
uint64_t VLink::getRemoteDpid()
{
	return remote_dpid;
}

unsigned int VLink::getLocalID()
{
	return local_id;
}
unsigned int VLink::getRemoteID()
{
	return remote_id;
}

uint64_t VLink::getID()
{
	return ID;
}
