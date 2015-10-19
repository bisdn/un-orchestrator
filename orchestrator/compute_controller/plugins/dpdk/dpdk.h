#ifndef DPDK_H_
#define DPDK_H_ 1

#pragma once

#include "../../nfs_manager.h"
#include "dpdk_constants.h"

#include <string>
#include <sstream>
#include <stdlib.h>

using namespace std;

class Dpdk : public NFsManager
{
public:
	
	//TODO: currently we are assuming that DPDK is alway supported. Not
	//sure that this assumption is correct.
	bool isSupported();
	
	bool startNF(StartNFIn sni);
	bool stopNF(StopNFIn sni);
};

#endif //DPDK_H_
