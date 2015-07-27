#include "ovs_manager.h"

char *host = NULL, *user = NULL;	

commands *c = NULL;

//this is a message printing function
void clb_print(NC_VERB_LEVEL level, const char* msg) 
{
	switch (level) 
	{
		case NC_VERB_ERROR:
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "libnetconf: \"%s\"",msg);
			break;
		case NC_VERB_WARNING:
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "libnetconf: \"%s\"",msg);
			break;
		case NC_VERB_VERBOSE:
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "libnetconf: \"%s\"",msg);
			break;
		case NC_VERB_DEBUG:
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "libnetconf: \"%s\"",msg);
			break;
	}
}

//Constructor
OVSManager::OVSManager()
{

	char user[64] = "";

	c = new commands();/*create a new object commands*/

	/* initiate libnetconf - all subsystems */
	nc_init(NC_INIT_ALL);
		
	/* set verbosity and function to print libnetconf's messages */
	nc_verbosity(NC_VERB_WARNING);//print error and warning messages
	nc_callback_print(clb_print);//set a specific message printing function via callback

	/* set authentication method preferred*/
	/*nc_ssh_pref(NC_SSH_AUTH_PASSWORD, 3);
	nc_ssh_pref(NC_SSH_AUTH_PUBLIC_KEYS, 1);
	nc_ssh_pref(NC_SSH_AUTH_INTERACTIVE, 2);*/

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Insert the user name for the OFConfig server:",MODULE_NAME);
	scanf("%s", user);

	/*connect to a of-config server*/
	c->cmd_connect(user);

}

//Destroyer
OVSManager::~OVSManager()
{	
	/*disconnect to a of-config server*/
	c->cmd_disconnect();
}

//implementation of createLsi
CreateLsiOut *OVSManager::createLsi(CreateLsiIn cli){
	//printf("Enter in a createLsi!\n");
	return c->cmd_editconfig_lsi(cli);
}

//implementation of destroyLsi
void OVSManager::destroyLsi(uint64_t dpid){
	c->cmd_editconfig_lsi_delete(dpid);
}

//implementation of addNFPorts
AddNFportsOut *OVSManager::addNFPorts(AddNFportsIn anpi){
	return c->cmd_editconfig_NFPorts(anpi);
}

//implementation of destroyNFPorts
void OVSManager::destroyNFPorts(DestroyNFportsIn dnpi){
	c->cmd_editconfig_NFPorts_delete(dnpi);
}

//implementation of addVirtualLink
AddVirtualLinkOut *addVirtualLink(AddVirtualLinkIn avli){
	return c->cmd_addVirtualLink(avli);
}

//implementation of destroyVirtualLink
void OVSManager::destroyVirtualLink(DestroyVirtualLinkIn dvli){
	c->cmd_destroyVirtualLink(dvli);
}

//implementation of addVirtualLink
AddVirtualLinkOut *OVSManager::addVirtualLink(AddVirtualLinkIn avli){
	return c->cmd_addVirtualLink(avli);
}

//implementation of checkPhysicalInterfaces
void OVSManager::checkPhysicalInterfaces(set<CheckPhysicalPortsIn> cppi){
	/*TODO*/
}
 
