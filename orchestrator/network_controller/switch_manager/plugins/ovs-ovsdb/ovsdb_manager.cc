#include "ovsdb_manager.h"

char *host = NULL, *user = NULL;	
int s = 0;

commands *c = NULL;

//Constructor
OVSDBManager::OVSDBManager()
{
	c = new commands();/*create a new object commands*/

	//s = c->cmd_connect();
}

//Destroyer
OVSDBManager::~OVSDBManager()
{	
	//c->cmd_disconnect(s);
}

//implementation of createLsi
CreateLsiOut *OVSDBManager::createLsi(CreateLsiIn cli){
	return c->cmd_editconfig_lsi(cli, s);
}

//implementation of destroyLsi
void OVSDBManager::destroyLsi(uint64_t dpid){
	c->cmd_editconfig_lsi_delete(dpid, s);
}

//implementation of addNFPorts
AddNFportsOut *OVSDBManager::addNFPorts(AddNFportsIn anpi){
	return c->cmd_editconfig_NFPorts(anpi, s);
}

//implementation of destroyNFPorts
void OVSDBManager::destroyNFPorts(DestroyNFportsIn dnpi){
	c->cmd_editconfig_NFPorts_delete(dnpi, s);
}

//implementation of destroyVirtualLink
void OVSDBManager::destroyVirtualLink(DestroyVirtualLinkIn dvli){
	c->cmd_destroyVirtualLink(dvli, s);
}

//implementation of addVirtualLink
AddVirtualLinkOut *OVSDBManager::addVirtualLink(AddVirtualLinkIn avli){
	return c->cmd_addVirtualLink(avli, s);
}

//implementation of checkPhysicalInterfaces
void OVSDBManager::checkPhysicalInterfaces(set<CheckPhysicalPortsIn> cppi){
	/*TODO*/
}
 
