#ifndef DESCRIPTION_H_
#define DESCRIPTION_H_ 1

#include "nf_type.h"

#pragma once

#include "../utils/logger.h"
#include "../utils/constants.h"
#include "nfs_manager.h"

#include <string>
#include <assert.h>

using namespace std;

class NFsManager;

class Description
{
friend NFsManager;

private:
	nf_t type;
	string uri;
	//The next attribute are meningful only for DPDK VNFs
	//FIXME: this is bad.. The same description should be valid for all the NFs. Then, it is up to the proper
	//plugin to decide wheter an information has to be used or not.
	string cores;
	string location;
	
public:
	Description(nf_t type, string uri, string cores, string location);
	Description(string type, string uri, string cores, string location);
	
	string getURI();
	string getLocation();
	nf_t getType();
	
protected:
	string getCores();
};

#endif //DESCRIPTION_H_
