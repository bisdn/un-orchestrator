#ifndef OVS_COMMANDS_H_
#define OVS_COMMANDS_H_ 1

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>
#include "../../createLSIin.h"
#include "../../createLSIout.h"
#include "../../addNFports_in.h"
#include "../../addNFports_out.h"
#include "../../addVirtualLink_in.h"
#include "../../addVirtualLink_out.h"
#include "../../checkPhysicalPorts_in.h"
#include "../../destroyNFports_in.h"

#include "inttypes.h"

using namespace std;

class commands
{
private:

public:
	commands();

	~commands();

	int cmd_connect(char *user);
	
	int cmd_disconnect();
	
	CreateLsiOut* cmd_editconfig_lsi (CreateLsiIn cli);
	
	AddNFportsOut *cmd_editconfig_NFPorts(AddNFportsIn anpi);
	
	AddVirtualLinkOut *cmd_addVirtualLink(AddVirtualLinkIn avli);
	
	void cmd_destroyVirtualLink(DestroyVirtualLinkIn dvli);
	
	void cmd_editconfig_lsi_delete(uint64_t dpid);
	
	void cmd_editconfig_NFPorts_delete(DestroyNFportsIn dnpi);
	
	int cmd_editconfig_port (NC_EDIT_TESTOPT_TYPE testopt, int operation_type, AddNFportsIn anpi);
	
	void cmd_delete_virtual_link(uint64_t dpid, uint64_t id);

};

class commandsException: public SwitchManagerException
{
public:
	virtual const char* what() const throw()
	{
		return "commandsException";
	}
};

#endif //OVS_COMMANDS_H_
