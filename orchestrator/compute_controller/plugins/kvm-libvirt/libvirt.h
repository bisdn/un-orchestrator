#ifndef LIBVIRT_H_
#define LIBVIRT_H_ 1

#pragma once

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
#include "../../../utils/logger.h"
#include "../../../utils/constants.h"

#include "../../nfs_manager.h"
#include "../../startNF_in.h"

#include "libvirt_constants.h"

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

using namespace std;

class Libvirt : public NFsManager
{
private:
	static virConnectPtr connection;
	
	/**
	*	@brief:	Open a connection with QEMU/KVM
	*/
	void connect();
	
	/**
	*	@brief: Disconnect from QEMU/KVM
	*/
	void disconnect();
	
	/**
	*	@brief: Custom error handler
	*/
	static void customErrorFunc(void *userdata, virErrorPtr err);

public:

	Libvirt();
	~Libvirt();
	
	bool isSupported();
	
	bool startNF(StartNFIn sni);
	bool stopNF(StopNFIn sni);
};

#endif //LIBVIRT_H_
