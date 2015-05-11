#ifndef OVSManager_H_
#define OVSManager_H_

#include "../../switch_manager.h"

#include "../../../../utils/logger.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <string>
#include <list>
#include <sstream>
#include <getopt.h>

#include <libnetconf.h>
#include <libnetconf_ssh.h>

#include "commands.h"
#include "../../../../utils/logger.h"
#include "../../../../utils/constants.h"

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>

using namespace std;

class LSI;

class OVSManager : public SwitchManager
{
private:

public:
	OVSManager();

	~OVSManager();

	CreateLsiOut *createLsi(CreateLsiIn cli);

	AddNFportsOut *addNFPorts(AddNFportsIn anpi);

	AddVirtualLinkOut *addVirtualLink(AddVirtualLinkIn avli);

	void destroyLsi(uint64_t dpid);

	void destroyNFPorts(DestroyNFportsIn dnpi);

	void destroyVirtualLink(DestroyVirtualLinkIn dvli); 

	void checkPhysicalInterfaces(set<CheckPhysicalPortsIn> cppi);

};

class OVSManagerException: public SwitchManagerException
{
public:
	virtual const char* what() const throw()
	{
		return "OVSManagerException";
	}
};

#endif
