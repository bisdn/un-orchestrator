#include "libvirt.h"
#include "libvirt_constants.h"

void Libvirt::customErrorFunc(void *userdata, virErrorPtr err)
{
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Failure of libvirt library call:\n");
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, " Code: %d\n", err->code);
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, " Domain: %d\n", err->domain);
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, " Message: %s\n", err->message);
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, " Level: %d\n", err->level);
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, " str1: %s\n", err->str1);
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, " str2: %s\n", err->str2);
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, " str3: %s\n", err->str3);
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, " int1: %d\n", err->int1);
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, " int2: %d\n", err->int2);
}

virConnectPtr Libvirt::connection = NULL;

Libvirt::Libvirt()
{
	virSetErrorFunc(NULL, customErrorFunc);
}

Libvirt::~Libvirt()
{
	if(connection != NULL)
		disconnect();
}

bool Libvirt::isSupported()
{
	connect();
	
	if(connection == NULL)
		return false;
	return true;
}

void Libvirt::connect()
{
	if(connection != NULL)
		//The connection is already open
		return;

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Connecting to Libvirt ...\n");
	connection = virConnectOpen("qemu:///system");
	if (connection == NULL) 
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Failed to open connection to qemu:///system\n");
	else
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Open connection to qemu:///system successfull\n");
}

void Libvirt::disconnect()
{
	virConnectClose(connection);
	connection = NULL;
}

bool Libvirt::startNF(StartNFIn sni)
{
	virDomainPtr dom = NULL;
	char domain_name[64], port_name[64];
	const char *xmlconfig = NULL;
	
	string nf_name = sni.getNfName();
	unsigned int n_ports = sni.getNumberOfPorts();
	map<unsigned int,string> ethPortsRequirements = sni.getEthPortsRequirements();
	string uri_image = description->getURI();

	/* Domain name */
	sprintf(domain_name, "%" PRIu64 "_%s", sni.getLsiID(), sni.getNfName().c_str());
	
	bool ovsdpdk = false;
	if (uri_image.compare(0, ovs.size(), ovs.c_str(), ovs.size()) != 0) {
		ovsdpdk = true;
	}
	
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Using Libvirt XML template %s\n", uri_image.c_str());
	xmlInitParser();

	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;

	/* Load XML document */
	doc = xmlParseFile(uri_image.c_str());
	if (doc == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Unable to parse file \"%s\"\n", uri_image.c_str());
		return 0;
	}

	/* xpath evaluation for Libvirt various elements we may want to update */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Unable to create new XPath context\n");
		xmlFreeDoc(doc);
		return 0;
	}
	const xmlChar* xpathExpr = BAD_CAST "/domain/devices/interface|/domain/name|/domain/devices/emulator";
	xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
	if(xpathObj == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error: unable to evaluate xpath expression \"%s\"\n", xpathExpr);
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		return 0;
	}

	enum E_updates {
		EMULATOR_UPDATED = 0x01,
		DOMAIN_NAME_UPDATED = 0x02,
	};
	uint32_t update_flags = 0;

	xmlNodeSetPtr nodes = xpathObj->nodesetval;
	int size = (nodes) ? nodes->nodeNr : 0;
    logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "xpath return size: %d\n", size);
	int i;
	for(i = size - 1; i >= 0; i--) {
	  	xmlNodePtr node = nodes->nodeTab[i];

		if (node != NULL) {
			switch (node->type) {
		   		case XML_ELEMENT_NODE:
		   			if (xmlStrcmp(node->name, (xmlChar*)"name") == 0) {
		   				xmlNodeSetContent(node, BAD_CAST domain_name);
		   				update_flags |= DOMAIN_NAME_UPDATED;
		   			}
		   			else if (xmlStrcmp(node->name, (xmlChar*)"emulator") == 0) {
		   				if (QEMU_BIN_PATH) {
		   					xmlNodeSetContent(node, (xmlChar*)QEMU_BIN_PATH);
							update_flags |= EMULATOR_UPDATED;
		   				}
		   			}
		   			else if (xmlStrcmp(node->name, (xmlChar*)"interface") == 0) {
		   				// Currently we just remove any net interface device present in the template and re-create our own
		   				// with the exception of bridged interfaces which are handy for managing the VM.
		   				xmlChar* type = xmlGetProp(node, (xmlChar*)"type");
		   				if (xmlStrcmp(type, (xmlChar*)"bridge") == 0) {
							xmlFree(type);
		   					continue;
		   				}
		   				xmlUnlinkNode(node);
		   				xmlFreeNode(node);
		   			}
		   			break;
		   		case XML_ATTRIBUTE_NODE:
		   			fprintf(stdout, "Error, ATTRIBUTE found here\n");
		   			break;
		   		default:
		   			fprintf(stdout, "Other type\n");
		   			break;
		   	}
		}

		/*
		 * All the elements returned by an XPath query are pointers to
		 * elements from the tree *except* namespace nodes where the XPath
		 * semantic is different from the implementation in libxml2 tree.
		 * As a result when a returned node set is freed when
		 * xmlXPathFreeObject() is called, that routine must check the
		 * element type. But node from the returned set may have been removed
		 * by xmlNodeSetContent() resulting in access to freed data.
		 * This can be exercised by running
		 *       valgrind xpath2 test3.xml '//discarded' discarded
		 * There is 2 ways around it:
		 *   - make a copy of the pointers to the nodes from the result set
		 *     then call xmlXPathFreeObject() and then modify the nodes
		 * or
		 *   - remove the reference to the modified nodes from the node set
		 *     as they are processed, if they are not namespace nodes.
		 */
		 if (nodes->nodeTab[i]->type != XML_NAMESPACE_DECL) {
		      nodes->nodeTab[i] = NULL;
		 }
	}

	/* Cleanup of XPath data */
	xmlXPathFreeObject(xpathObj);

    /* Add domain name if not present */
    if (0 == (update_flags & DOMAIN_NAME_UPDATED)) {
		xmlNewTextChild(xmlDocGetRootElement(doc), NULL, BAD_CAST "name", BAD_CAST domain_name);
    }

	/* Create xpath evaluation context for Libvirt domain/devices */
	/* Evaluate xpath expression */
	const xmlChar* xpathExpr_devs = BAD_CAST "/domain/devices";
	xpathObj = xmlXPathEvalExpression(xpathExpr_devs, xpathCtx);
	if(xpathObj == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error: unable to evaluate xpath expression \"%s\"\n", xpathExpr);
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		return 0;
	}
	nodes = xpathObj->nodesetval;
	if (!nodes || (nodes->nodeNr != 1)) {
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "xpath(devices) failed accessing <devices> node\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		return 0;
	}

    xmlNodePtr devices = nodes->nodeTab[0];

    /* Add emulator if not present and must be modified */
    if ((0 == (update_flags & EMULATOR_UPDATED)) && (QEMU_BIN_PATH != NULL)) {
    	xmlNewTextChild(devices, NULL, BAD_CAST "emulator", BAD_CAST QEMU_BIN_PATH);
    }
	
	/* Create XML for VM */
	if (ovsdpdk) {
		/* Create NICs */
		for(unsigned int i=1;i<=n_ports;i++) {
			// TODO: This is OVS vhostuser specific - Should be coordinated with network plugin part...
			char sock_path[255];
			sprintf(sock_path, "%s/%s_%u", OVS_BASE_SOCK_PATH, domain_name, i);
			xmlNodePtr ifn = xmlNewChild(devices, NULL, BAD_CAST "interface", NULL);
    	    xmlNewProp(ifn, BAD_CAST "type", BAD_CAST "vhostuser");

    	    xmlNodePtr srcn = xmlNewChild(ifn, NULL, BAD_CAST "source", NULL);
    	    xmlNewProp(srcn, BAD_CAST "type", BAD_CAST "unix");
    	    xmlNewProp(srcn, BAD_CAST "path", BAD_CAST sock_path);
    	    xmlNewProp(srcn, BAD_CAST "mode", BAD_CAST "client");

    	    xmlNodePtr modeln = xmlNewChild(ifn, NULL, BAD_CAST "model", NULL);
    	    xmlNewProp(modeln, BAD_CAST "type", BAD_CAST "virtio");

			if(ethPortsRequirements.count(i) > 0) {
	    	    xmlNodePtr macn = xmlNewChild(ifn, NULL, BAD_CAST "mac", NULL);
	    	    xmlNewProp(macn, BAD_CAST "address", BAD_CAST (ethPortsRequirements.find(i)->second.c_str()));
			}

    	    xmlNodePtr drvn = xmlNewChild(ifn, NULL, BAD_CAST "driver", NULL);
    	    xmlNodePtr drv_hostn = xmlNewChild(drvn, NULL, BAD_CAST "host", NULL);
    	    xmlNewProp(drv_hostn, BAD_CAST "csum", BAD_CAST "off");
    	    xmlNewProp(drv_hostn, BAD_CAST "gso", BAD_CAST "off");
    	    xmlNodePtr drv_guestn = xmlNewChild(drvn, NULL, BAD_CAST "guest", NULL);
    	    xmlNewProp(drv_guestn, BAD_CAST "tso4", BAD_CAST "off");
    	    xmlNewProp(drv_guestn, BAD_CAST "tso6", BAD_CAST "off");
    	    xmlNewProp(drv_guestn, BAD_CAST "ecn", BAD_CAST "off");
		}
	}else{
		/* Create NICs */
		for(unsigned int i=1;i<=n_ports;i++) {
			// TODO: This is OVS vhostuser specific - Should be extracted to network plugin part...
			/*add ports*/			
			//Create name of port --> nf_name+b+lsiID+p+i
			locale loc;	
				
			for (string::size_type j=0; j<nf_name.length(); ++j)
    			nf_name[j] = tolower(nf_name[j], loc);
    				
			sprintf(port_name, "%sp%ub" "%" PRIu64, nf_name.c_str(), i, sni.getLsiID());
			
			
			xmlNodePtr ifn = xmlNewChild(devices, NULL, BAD_CAST "interface", NULL);
    	    xmlNewProp(ifn, BAD_CAST "type", BAD_CAST "direct");

    	    xmlNodePtr srcn = xmlNewChild(ifn, NULL, BAD_CAST "source", NULL);
    	    xmlNewProp(srcn, BAD_CAST "dev", BAD_CAST port_name);
    	    xmlNewProp(srcn, BAD_CAST "mode", BAD_CAST "passthrough");

    	    xmlNodePtr modeln = xmlNewChild(ifn, NULL, BAD_CAST "model", NULL);
    	    xmlNewProp(modeln, BAD_CAST "type", BAD_CAST "virtio");
    	    
    	    xmlNodePtr virt = xmlNewChild(ifn, NULL, BAD_CAST "virtualport", NULL);
    	    xmlNewProp(virt, BAD_CAST "type", BAD_CAST "openvswitch");
		}
	}

	/* Cleanup of XPath data */
	xmlXPathFreeContext(xpathCtx);

	/* Get resulting document */
	xmlChar* xml_buf; int xml_bufsz;
	xmlDocDumpMemory(doc, &xml_buf, &xml_bufsz);
	xmlconfig = (const char *)xml_buf;

	/* Final XML Cleanup */
	xmlFreeDoc(doc);
	xmlCleanupParser();
	
#if 1  /* Debug */
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Dumping XML to %s\n", domain_name);
	FILE* fp = fopen(domain_name, "w");
	if (fp) {
		fwrite(xmlconfig, 1, strlen(xmlconfig), fp);
		fclose(fp);
	}
#endif

	dom = virDomainCreateXML(connection, xmlconfig, 0);
	if (!dom) {
		virDomainFree(dom);
    		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Domain definition failed");
    		return false;
	}
	
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Boot guest");
	
	virDomainFree(dom);
	
	return true;
}

bool Libvirt::stopNF(StopNFIn sni)
{
	char *vm_name = new char[64];
	
	/*image_name*/
	sprintf(vm_name, "%" PRIu64 "_%s", sni.getLsiID(), sni.getNfName().c_str());

	assert(connection != NULL);

	/*destroy the VM*/
	if(virDomainDestroy(virDomainLookupByName(connection, vm_name)) != 0){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "failed to stop (destroy) VM. %s\n", vm_name);
		return false;
	}
	
	return true;
}
