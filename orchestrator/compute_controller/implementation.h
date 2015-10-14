#ifndef IMPLEMENTATION_H_
#define IMPLEMENTATION_H_ 1

#include "nf_type.h"

#pragma once

#include "../utils/logger.h"
#include "../utils/constants.h"
#include "nfs_manager.h"

#include <string>
#include <assert.h>

using namespace std;

class NFsManager;

class Implementation
{
friend NFsManager;

private:
	nf_t type;
	string uri;
	//The next attribute are meningful only for DPDK implementations
	string cores;
	string location;
	
public:
	Implementation(nf_t type, string uri, string cores, string location);
	Implementation(string type, string uri, string cores, string location);
	
	string getURI();
	string getLocation();
	nf_t getType();
	
protected:
	string getCores();
};

#endif //IMPLEMENTATION_H_
