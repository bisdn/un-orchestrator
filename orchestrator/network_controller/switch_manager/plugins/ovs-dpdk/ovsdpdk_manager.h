#ifndef OVSDPDKManager_H_
#define OVSDPDKManager_H_ 1

#pragma once

#include "../../switch_manager.h"

#include "../../../../utils/constants.h"
#include "../../../../utils/sockutils.h"
#include "../../../../utils/logger.h"

#include "ovsdpdk_constants.h"

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include <string>
#include <list>
#include <sstream>
#include <getopt.h>

using namespace std;
using namespace json_spirit;

class LSI;

class OVSDPDKManager : public SwitchManager
{
private:
	unsigned int m_NextLsiId;
	unsigned int m_NextPortId;

public:
	OVSDPDKManager();

	~OVSDPDKManager();

	//
	// SwitchManager interface
	//
	
	CreateLsiOut *createLsi(CreateLsiIn cli);

	AddNFportsOut *addNFPorts(AddNFportsIn anpi);

	AddVirtualLinkOut *addVirtualLink(AddVirtualLinkIn avli);

	void destroyLsi(uint64_t dpid);

	void destroyNFPorts(DestroyNFportsIn dnpi);

	void destroyVirtualLink(DestroyVirtualLinkIn dvli); 

	void checkPhysicalInterfaces(set<CheckPhysicalPortsIn> cppi);
};

class OVSDPDKManagerException: public SwitchManagerException
{
public:
	virtual const char* what() const throw()
	{
		return "OVSDPDKManagerException";
	}
};

#endif //OVSDPDKManager_H_

