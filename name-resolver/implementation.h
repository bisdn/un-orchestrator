#ifndef IMPLEMENTATION_H_
#define IMPLEMENTATION_H_ 1

#pragma once

#include <string>
#include <assert.h>

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include "constants.h"
#include "nf.h"

using namespace std;
using namespace json_spirit;

typedef enum{DPDK,DOCKER,KVM}nf_t;

class Implementation
{
private:
	nf_t type;
	string uri;
	
	//These are used only in case of DPDK
	string cores;
	string location;
	
public:
	Implementation(nf_t type, string uri, string cores, string location);
	
	Object toJSON();
};

#endif //IMPLEMENTATION_H_
