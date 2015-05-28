#ifndef COMMANDS_H_
#define COMMANDS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string>
#include <list>
#include <map>
#include <string.h>
#include <libvirt/libvirt.h>
#include "../../utils/logger.h"
#include "../../utils/constants.h"

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>

using namespace std;

class Libvirt
{
private:

public:

	int cmd_connect();
	
	void cmd_close();
	
	int cmd_startNF(uint64_t lsiID, string nf_name, string uri_image, unsigned int n_ports);
	
	int cmd_shutdown(uint64_t lsiID, string nf_name);
};

#endif
