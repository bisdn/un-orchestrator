#ifndef IMPLEMENTATION_H_
#define IMPLEMENTATION_H_ 1

#include "nf_type.h"

#pragma once

#include "../utils/logger.h"
#include "../utils/constants.h"

#include <string>
#include <assert.h>

using namespace std;


class Implementation
{
private:
	nf_t type;
	string uri;
	//The next attribute are meningful only for DPDK implementations
	string cores;
	string location;
	
public:
	Implementation(nf_t type, string uri, string cores, string location);
	Implementation(string type, string uri, string cores, string location);
	
	nf_t getType();
	string getURI();
	string getCores();
	string getLocation();
};

#endif //IMPLEMENTATION_H_
