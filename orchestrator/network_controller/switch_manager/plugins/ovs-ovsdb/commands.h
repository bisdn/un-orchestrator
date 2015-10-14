#ifndef OVS_COMMANDS_H_
#define OVS_COMMANDS_H_ 1

#include <inttypes.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include "Jzon.h"

#include <iostream>
#include <fstream>

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <locale>
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
#include <string.h>

#define INTERRUPTED_BY_SIGNAL (errno == EINTR || errno == ECHILD)

using namespace std;

class commands
{
private:

public:
	commands();

	~commands();
	
	int cmd_connect();
	
	int cmd_disconnect(int s);
	
	CreateLsiOut* cmd_editconfig_lsi (CreateLsiIn cli, int s);
	
	void add_ports(int rnumber, string p, uint64_t dnumber, int nf, int s);
	
	AddNFportsOut *cmd_editconfig_NFPorts(AddNFportsIn anpi, int s);
	
	AddVirtualLinkOut *cmd_addVirtualLink(AddVirtualLinkIn avli, int s);
	
	void cmd_add_virtual_link(string vrt, string trv, char ifac[64], uint64_t dpi, int s);
	
	void cmd_destroyVirtualLink(DestroyVirtualLinkIn dvli, int s);
	
	void cmd_editconfig_lsi_delete(uint64_t dpid, int s);
	
	void cmd_editconfig_NFPorts_delete(DestroyNFportsIn dnpi, int s);
	
	void cmd_delete_virtual_link(uint64_t dpid, uint64_t id, int s);

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
