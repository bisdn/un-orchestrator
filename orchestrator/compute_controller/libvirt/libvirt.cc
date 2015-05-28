#ifndef COMMANDS
#define COMMANDS

#include "libvirt.h"

#endif

/*map [image_name, domain] */
map<char *, virDomainPtr> img_name;

virConnectPtr conn;

/*connect to qemu with root privileges*/
int Libvirt::cmd_connect(){
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
}

/*retrieve and start NF*/
int Libvirt::cmd_startNF(uint64_t lsiID, string nf_name, string uri_image, unsigned int n_ports){
	virDomainPtr dom;
	char* bridge_name = new char[64], *temp = new char[64], *tmmp_file = new char[64], *sw = new char[64], *port_name = new char[64], *tmp = new char[64];
	strcpy(sw, "Bridge");
	const char *xmlconfig = "";
	
	int rc;
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	
	/*image_name*/
	sprintf(tmmp_file, "%" PRIu64, lsiID);
	strcat(tmmp_file, "_");
	strcat(tmmp_file, (char *)nf_name.c_str());
	
	/*connect to qemu with root privileges*/
	if(!cmd_connect())
		return 0;
	
	/*create XML for VM*/
	/*Create a new XML buffer, to which the XML document will be written*/
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
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "name", BAD_CAST tmmp_file);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		return 0;
	}
	
	/*add element "memory"*/
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "memory", BAD_CAST "1500000");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
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
	
	/*add element "emulator"*/
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "emulator", BAD_CAST "/usr/bin/qemu-kvm");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
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
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "raw");
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
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "raw");
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
    	
	/*create the bridge_name*/
	sprintf(temp, "%" PRIu64, lsiID);
	strcat(sw, temp);
	strcpy(bridge_name, sw);
		
	for(unsigned int i=1;i<=n_ports;i++){
		
		/*add element "interface"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "interface");
		if (rc < 0) {
	       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}
	
		/*add attribute "type"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST "bridge");
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
	
		/*add attribute "bridge"*/
		rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "bridge", "%s", bridge_name);
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
	
		/*add ports*/	
		/*lsi_example_i*/
		strcpy(tmp, (char *)nf_name.c_str());
		sprintf(temp, "%" PRIu64, lsiID);
		strcpy(port_name, temp);
		strcat(port_name, "_");
		strcat(port_name, tmp);
		strcat(port_name, "_");
		sprintf(temp, "%u", i);
		strcat(port_name, temp);
	
		/*add element "target"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "target");
		if (rc < 0) {
		       	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			return 0;
		}
	
		/*add attribute "dev"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "dev", BAD_CAST port_name);
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
	dom = virDomainCreateXML(conn, xmlconfig, 0);
	if (!dom) {
    		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Domain definition failed");
    		return 0;
	}
	
	/*run the VM*/
	if (virDomainCreate(dom) < 0) {
    		virDomainFree(dom);
   		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot boot guest");
    		return 0;
	}
	
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Boot guest");
	
	virDomainFree(dom);
	
	/*save value of domain*/
	img_name[tmmp_file] = dom;
	
	return 1;
}

/*stop NF*/
int Libvirt::cmd_shutdown(uint64_t lsiID, string nf_name){
	char *tmmp_file = new char[64];	
	
	/*image_name*/
	sprintf(tmmp_file, "%" PRIu64, lsiID);
	strcat(tmmp_file, "_");
	strcat(tmmp_file, (char *)nf_name.c_str());

	virDomainPtr dom = img_name[tmmp_file];
	
	if(virDomainShutdown(dom) != 0){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "failed to stop (shutdown) VM.\n");
		return 0;
	}
	
	if(virDomainDestroy(dom) != 0){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "failed to stop (destroy) VM.\n");
		return 0;
	}
	
	cmd_close();
	
	return 1;
}
