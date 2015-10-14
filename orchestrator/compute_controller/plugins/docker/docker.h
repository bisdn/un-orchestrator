#ifndef DOCKER_H_
#define DOCKER_H_ 1

#pragma once

#include "../../nfs_manager.h"


using namespace std;

class Docker : public NFsManager
{
public:
	bool isSupported();
	
	bool startNF(StartNFIn sni);
	bool stopNF(StopNFIn sni);
};

#endif //DOCKER_H_
