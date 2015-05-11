#include "ovs_manager.h"

char *host = NULL, *user = NULL;

//this is a message printing function
void clb_print(NC_VERB_LEVEL level, const char* msg) {
	switch (level) {
	case NC_VERB_ERROR:
		fprintf(stderr, "libnetconf ERROR: %s\n", msg);
		break;
	case NC_VERB_WARNING:
		fprintf(stderr, "libnetconf WARNING: %s\n", msg);
		break;
	case NC_VERB_VERBOSE:
		fprintf(stderr, "libnetconf VERBOSE: %s\n", msg);
		break;
	case NC_VERB_DEBUG:
		fprintf(stderr, "libnetconf DEBUG: %s\n", msg);
		break;
	}
}

//Constructor
OVSManager::OVSManager()
{
	/* initiate libnetconf - all subsystems */
	nc_init(NC_INIT_ALL);
		
	/* set verbosity and function to print libnetconf's messages */
	nc_verbosity(NC_VERB_WARNING);//print error and warning messages
	nc_callback_print(clb_print);//set a specific message printing function via callback

	/* set authentication method preferred*/
	//nc_ssh_pref(NC_SSH_AUTH_PASSWORD, 3);
	//nc_ssh_pref(NC_SSH_AUTH_PUBLIC_KEYS, -1);
	//nc_ssh_pref(NC_SSH_AUTH_INTERACTIVE, -2);

	/*connect to a of-config server*/
	cmd_connect();

}

//Destroyer
OVSManager::~OVSManager()
{	
	/*disconnect to a of-config server*/
	cmd_disconnect();
}

//implementation of createLsi
CreateLsiOut *OVSManager::createLsi(CreateLsiIn cli){
	//printf("Enter in a createLsi!\n");
	return cmd_editconfig_lsi(cli);
}

//implementation of destroyLsi
void OVSManager::destroyLsi(uint64_t dpid){
	cmd_editconfig_lsi_delete(dpid);
}

//implementation of addNFPorts
AddNFportsOut *OVSManager::addNFPorts(AddNFportsIn anpi){
	return cmd_editconfig_NFPorts(anpi);
}

//implementation of destroyNFPorts
void OVSManager::destroyNFPorts(DestroyNFportsIn dnpi){
	cmd_editconfig_NFPorts_delete(dnpi);
}

//implementation of addVirtualLink
AddVirtualLinkOut *addVirtualLink(AddVirtualLinkIn avli){
	return cmd_addVirtualLink(avli);
}

//implementation of destroyVirtualLink
void OVSManager::destroyVirtualLink(DestroyVirtualLinkIn dvli){
	cmd_destroyVirtualLink(dvli);
}

//implementation of addVirtualLink
AddVirtualLinkOut *OVSManager::addVirtualLink(AddVirtualLinkIn avli){
	return cmd_addVirtualLink(avli);
}

//implementation of checkPhysicalInterfaces
void OVSManager::checkPhysicalInterfaces(set<CheckPhysicalPortsIn> cppi){
	/*TODO*/
}
 
