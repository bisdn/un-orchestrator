#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "libvirt.h"

/* TODO - These should come from an orchestrator config file (curently, there is only one for the UN ports) */
static const char* QEMU_BIN_PATH = NULL; /* Can point to qemu bin or a wrapper script that tweaks the command line. If NULL, Libvirt default or path found in XML is used */
static const char* OVS_BASE_SOCK_PATH = "/usr/local/var/run/openvswitch/";

/*error handler libvirt*/
static void customErrorFunc(void *userdata, virErrorPtr err)
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

/*connect to qemu with root privileges*/
int Libvirt::cmd_connect(){

	virSetErrorFunc(NULL, customErrorFunc);

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Connecting to Libvirt ...\n");
	conn = virConnectOpen("qemu:///system");
	if (conn == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Failed to open connection to qemu:///system\n");
		return 0;
	}
	
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Open connection to qemu:///system successfull\n");
	
	return 1;
}

/*close connection*/
void Libvirt::cmd_close(){
	virConnectClose(conn);
	conn = NULL;
}

/*retrieve and start NF*/
int Libvirt::cmd_startNF(uint64_t lsiID, string nf_name, string uri_image, unsigned int n_ports)
{
	virDomainPtr dom = NULL;
	char domain_name[64], bridge_name[64], port_name[64];
	const char *xmlconfig = NULL;
	
	int rc;
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;

	/* Domain name */
	sprintf(domain_name, "%" PRIu64 "_%s", lsiID, nf_name.c_str());
	
	bool use_template = false;
	const string xml_end = ".xml";
	if (uri_image.length() >= xml_end.length() &&
		(0 == uri_image.compare(uri_image.length() - xml_end.length(), xml_end.length(), xml_end))) {
		use_template = true;
	}

	
	/* Connect to qemu with root privileges*/
	if(!cmd_connect())
		return 0;
	
	/* Create XML for VM */
	if (use_template) {
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
	    				// Currently we just remove any net interface device present in the temple and re-create our own
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

		/* Create NICs */
		for(unsigned int i=1;i<=n_ports;i++) {
			// TODO: This is OVS vhostuser specific - Should be extracted to network plugin part...
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

    	    xmlNodePtr drvn = xmlNewChild(ifn, NULL, BAD_CAST "driver", NULL);
    	    xmlNodePtr drv_hostn = xmlNewChild(drvn, NULL, BAD_CAST "host", NULL);
    	    xmlNewProp(drv_hostn, BAD_CAST "csum", BAD_CAST "off");
    	    xmlNewProp(drv_hostn, BAD_CAST "gso", BAD_CAST "off");
    	    xmlNodePtr drv_guestn = xmlNewChild(drvn, NULL, BAD_CAST "guest", NULL);
    	    xmlNewProp(drv_guestn, BAD_CAST "tso4", BAD_CAST "off");
    	    xmlNewProp(drv_guestn, BAD_CAST "tso6", BAD_CAST "off");
    	    xmlNewProp(drv_guestn, BAD_CAST "ecn", BAD_CAST "off");
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
	}
	else {  /* TODO: This provides a largely hard-coded Libvirt domain definition. It can always be done by referring to an XML template. Hence this path could be subsumed in above one */
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Creating Libvirt XML template for image %s\n", uri_image.c_str());

		buf = xmlBufferCreate();
		if(buf == NULL){
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlWriterMemory: Error creating the xml buffer\n.");
			return 0;
		}

		/* Create a new XmlWriter for memory, with no compression.*/
		writer = xmlNewTextWriterMemory(buf, 0);
		if (writer == NULL) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error creating the xml writer\n");
			return 0;
		}
	
		/*Root element "domain"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "domain");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			return 0;
		}

		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "kvm");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add element "name"*/
		rc = xmlTextWriterWriteElement(writer, BAD_CAST "name", BAD_CAST domain_name);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add element "memory"*/
		rc = xmlTextWriterWriteElement(writer, BAD_CAST "memory", BAD_CAST /*"4194304"*/MEMORY_SIZE);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add element "vcpu"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "vcpu");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}
	
		/*add attribute "placement"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "placement", BAD_CAST "static");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}
	
		/*add content "vcpu"*/
		rc = xmlTextWriterWriteRaw(writer, BAD_CAST /*"4"*/NUMBER_OF_CORES);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}
	
		/*close element "vcpu"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*add element "os"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "os");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add element "type"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "type");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}
	
		/*add attribute "arch"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "arch", BAD_CAST "x86_64");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}
	
		/*add attribute "machine"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "machine", BAD_CAST "pc");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}
	
		/*add content "type"*/
		rc = xmlTextWriterWriteRaw(writer, BAD_CAST "hvm");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}
	
		/*close element "type"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*add element "boot"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "boot");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}
	
		/*add attribute "dev"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "dev", BAD_CAST "hd");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*close element "boot"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

			/*add element "boot"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "boot");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}
	
		/*add attribute "dev"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "dev", BAD_CAST "cdrom");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*close element "boot"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*close element "os"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*add element features*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "features");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			return 0;
		}

		/*add element acpi*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "acpi");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			return 0;
		}

		/*close element "acpi"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*add element apic*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "apic");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			return 0;
		}

		/*close element "apic"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

			/*add element pae*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "pae");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			return 0;
		}

		/*close element "pae"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*close element "features"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*add element "devices"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "devices");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			return 0;
		}

		/*add element "disk"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "disk");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			return 0;
		}

		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "file");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "device"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "device", BAD_CAST "disk");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add element "source"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "source");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add attribute "file"*/
		rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "file", "%s", uri_image.c_str());
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*close element "source"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*add element "driver"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "driver");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add attribute "name"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name", BAD_CAST "qemu");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "qcow2");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*close element "driver"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*add element "target"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "target");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add attribute "dev"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "dev", BAD_CAST "vda");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "bus"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "bus", BAD_CAST "virtio");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*close element "target"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*close element "disk"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

			/*add element "disk"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "disk");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			return 0;
		}

		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "block");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "device"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "device", BAD_CAST "cdrom");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add element "driver"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "driver");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add attribute "name"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name", BAD_CAST "qemu");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "raw" /*"qcow2"*/);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*close element "driver"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*add element "target"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "target");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add attribute "dev"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "dev", BAD_CAST "hdc");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "bus"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "bus", BAD_CAST "ide");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*close element "target"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*close element "disk"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		sprintf(bridge_name, "Bridge%" PRIu64, lsiID);

		/* Create NICs */
		for(unsigned int i=1;i<=n_ports;i++){
			/*add element "interface"*/
			rc = xmlTextWriterStartElement(writer, BAD_CAST "interface");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				return 0;
			}

			/*add attribute "type"*/
			rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "direct");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
				return 0;
			}

			/*add element "source"*/
			rc = xmlTextWriterStartElement(writer, BAD_CAST "source");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				return 0;
			}

			/*add ports*/
			/*lsi_example_i*/
			sprintf(port_name, "%" PRIu64 "_%s_%u", lsiID, nf_name.c_str(), i);

			/*add attribute "dev"*/
			rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "dev", "%s", port_name);
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
				return 0;
			}

			/*add attribute "mode"*/
			//XXX I use this mode because all other modes drop the unicast traffic - https://seravo.fi/2012/virtualized-bridged-networking-with-macvtap
			rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "mode", "passthrough");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
				return 0;
			}

			/*close element "source"*/
			rc = xmlTextWriterEndElement(writer);
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
				return 0;
			}

			/*add element "model"*/
			rc = xmlTextWriterStartElement(writer, BAD_CAST "model");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				return 0;
			}

			/*add attribute "type"*/
			rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "type", "virtio");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
				return 0;
			}

				/*close element "model"*/
			rc = xmlTextWriterEndElement(writer);
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
				return 0;
			}

				/*add element "virtualport"*/
			rc = xmlTextWriterStartElement(writer, BAD_CAST "virtualport");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				return 0;
			}

			/*add attribute "type"*/
			rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "openvswitch");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
				return 0;
			}

			/*close element "virtualport"*/
			rc = xmlTextWriterEndElement(writer);
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
				return 0;
			}

			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%u\n", n_ports);

			/*close element "interface"*/
			rc = xmlTextWriterEndElement(writer);
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
				return 0;
			}
		}

		/*add element "serial"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "serial");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "pty");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add element "target"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "target");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add attribute "port"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "port", BAD_CAST "0");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*close element "target"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*close element "serial"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*add element "console"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "console");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "pty");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}
		
		/*add element "target"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "target");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}
		
		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "serial");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "port"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "port", BAD_CAST "0");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*close element "target"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*close element "console"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*add element "input"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "input");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "mouse");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "bus"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "bus", BAD_CAST "ps2");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*close element "input"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

			/*add element "input"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "input");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "keyboard");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "bus"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "bus", BAD_CAST "ps2");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*close element "input"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*add element "graphics"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "graphics");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "vnc");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "port"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "port", BAD_CAST "-1");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}	

		/*add attribute "autoport"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "autoport", BAD_CAST "yes");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*close element "graphics"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*close element "devices"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		/*add element "sound"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "sound");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add attribute "model"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "model", BAD_CAST "ich6");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add element "address"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "address");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "pci");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "domain"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "domain", BAD_CAST "0x0000");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "bus"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "bus", BAD_CAST "0x00");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "slot"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "slot", BAD_CAST "0x04");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "function"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "function", BAD_CAST "0x0");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*close element "address"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}
		
		/*close element "sound"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}
		
		/*add element "video"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "sound");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add element "model"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "model");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}

		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "cirrus");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "vram"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "vram", BAD_CAST "16384");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*add attribute "heads"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "heads", BAD_CAST "1");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}

		/*close element "model"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}
		
		/*add element "address"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "address");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}
		
		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "pci");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}
		
		/*add attribute "domain"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "domain", BAD_CAST "0x0000");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}
		
		/*add attribute "bus"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "bus", BAD_CAST "0x00");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}
		
		/*add attribute "slot"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "slot", BAD_CAST "0x03");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}
		
		/*add attribute "function"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "function", BAD_CAST "0x0");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			return 0;
		}
		
		/*close element "address"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}
		
		/*close element "video"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}
		
		/*close element "domain"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			return 0;
		}

		xmlFreeTextWriter(writer);
		
		xmlconfig = (const char *)buf->content;
	}
	
#if 1  /* Debug */
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Dumping XML to %s\n", domain_name);
	FILE* fp = fopen(domain_name, "w");
	if (fp) {
		fwrite(xmlconfig, 1, strlen(xmlconfig), fp);
		fclose(fp);
	}
#endif

	dom = virDomainCreateXML(conn, xmlconfig, 0);
	if (!dom) {
		virDomainFree(dom);
    		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Domain definition failed");
    		return 0;
	}
	
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Boot guest");
	
	virDomainFree(dom);
	
	return 1;
}

/*stop NF*/
int Libvirt::cmd_destroy(uint64_t lsiID, string nf_name){
	char *tmmp_file = new char[64];
	
	/*image_name*/
	sprintf(tmmp_file, "%" PRIu64, lsiID);
	strcat(tmmp_file, "_");
	strcat(tmmp_file, (char *)nf_name.c_str());

	/*destroy the VM*/
	if(virDomainDestroy(virDomainLookupByName(conn, tmmp_file)) != 0){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "failed to stop (destroy) VM.\n");
		return 0;
	}
	
	cmd_close();
	
	return 1;
}
