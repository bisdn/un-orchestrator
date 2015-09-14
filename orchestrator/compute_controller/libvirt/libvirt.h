#ifndef LIBVIRT_COMMANDS_H_
#define LIBVIRT_COMMANDS_H_ 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string>
#include <list>
#include <map>
#include <string.h>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include "../../utils/logger.h"
#include "../../utils/constants.h"

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>

using namespace std;

class Libvirt
{
private:
	virConnectPtr conn;

public:

	int cmd_connect();
	
	void cmd_close();
	
	int cmd_startNF(uint64_t lsiID, string nf_name, string uri_image, unsigned int n_ports, map<unsigned int,pair<string,string> > ipv4PortsRequirements,map<unsigned int,string> ethPortsRequirements);
	
	int cmd_destroy(uint64_t lsiID, string nf_name);
};

#endif //LIBVIRT_COMMANDS_H_
