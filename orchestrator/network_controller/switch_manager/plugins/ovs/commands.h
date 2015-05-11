
#include <stdlib.h>
#include <stdio.h>
#include "../../createLSIin.h"
#include "../../createLSIout.h"
#include "../../addNFports_in.h"
#include "../../addNFports_out.h"
#include "../../addVirtualLink_in.h"
#include "../../addVirtualLink_out.h"
#include "../../checkPhysicalPorts_in.h"
#include "../../destroyNFports_in.h"

#ifndef COMMANDS_H_
#define COMMANDS_H_

int cmd_connect();
int cmd_disconnect();
CreateLsiOut* cmd_editconfig_lsi (CreateLsiIn cli);
AddNFportsOut *cmd_editconfig_NFPorts(AddNFportsIn anpi);
AddVirtualLinkOut *cmd_addVirtualLink(AddVirtualLinkIn avli);
void cmd_destroyVirtualLink(DestroyVirtualLinkIn dvli);
void cmd_editconfig_lsi_delete(uint64_t dpid);
void cmd_editconfig_NFPorts_delete(DestroyNFportsIn dnpi);
int cmd_editconfig_port (NC_EDIT_TESTOPT_TYPE testopt, int operation_type, AddNFportsIn anpi);

#endif
