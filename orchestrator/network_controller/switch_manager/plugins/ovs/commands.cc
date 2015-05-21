#ifndef COMMANDS
#define COMMANDS

#include "ovs_manager.h"

#endif

struct nc_session* session = NULL;

int rnumber = 1;
uint64_t dnumber = 1;
int pnumber = 0, nfnumber = 0;

/*map use to obtain name of switch from id*/
map<uint64_t, string> switch_id;
/*map use to obtain virtual link id from bridge name*/
map<string, list<uint64_t> > virtual_link_id;
/*map use to obtain name of port from port_id*/
map<uint64_t, string> port_id;
map<pair<uint64_t, string>, string> nf_id;
map<uint64_t, uint64_t> vl_id;

//return the password related by username and hostname
char *password(const char *username, const char *hostname){
	
	char *psw = new char[64];	

	printf("Insert password: ");

	scanf("%s", psw);

	return psw;
}

/*split*/
vector<string> &split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

/*split string*/
vector<string> split(const string &s, char delim) {
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}

/*convert uint64_t into char* */
char *num_to_string(uint64_t num) {

	char *mac = new char[64], *pmac = (char *)&num;

	sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", pmac[7], pmac[6], pmac[5], pmac[4], pmac[3], pmac[2], pmac[1], pmac[0]);

	return mac;
  
}

/* rpc parameter is freed after the function call */
static int send_recv_process(const char* operation, nc_rpc* rpc, const char* output_file, FILE* output) {
	nc_reply *reply = NULL;
	char *data = NULL;
	FILE* out_stream;
	int ret = EXIT_SUCCESS;

	/* send the request and get the reply */
	switch (nc_session_send_recv(session, rpc, &reply)) {
	case NC_MSG_UNKNOWN:
		if (nc_session_get_status(session) != NC_SESSION_STATUS_WORKING) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "receiving rpc-reply failed.");
			cmd_disconnect();
			ret = EXIT_FAILURE;
			break;
		}
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Unknown error occurred.");
		ret = EXIT_FAILURE;
		break;
	case NC_MSG_NONE:
		/* error occurred, but processed by callback */
		break;
	case NC_MSG_REPLY:
		switch (nc_reply_get_type(reply)) {
		case NC_REPLY_OK:
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Result OK\n");
			break;
		case NC_REPLY_DATA:
			if (output_file != NULL) {
				out_stream = fopen(output_file, "w");
				if (out_stream == NULL) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Could not open the output file \"%s\"", output_file);
					ret = EXIT_FAILURE;
					break;
				}
				fprintf(out_stream, "%s", data = nc_reply_get_data(reply));
				fclose(out_stream);
			}
			free(data);
			break;
		case NC_REPLY_ERROR:
			/* wtf, you shouldn't be here !?!? */
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "operation failed, but rpc-error was not processed.");
			ret = EXIT_FAILURE;
			break;
		default:
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "unexpected operation result.");
			ret = EXIT_FAILURE;
			break;
		}
		break;
	default:
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Unknown error occurred.");
		ret = EXIT_FAILURE;
		break;
	}
	nc_rpc_free(rpc);
	nc_reply_free(reply);

	return ret;
}

/*connect to a of-config server with default username, host and port*/
int cmd_connect(char *user) {
	/*@FIXME*/

	//this is a password callback
	nc_callback_sshauth_password(password);

	/* set default transport protocol */
	nc_session_transport(NC_TRANSPORT_SSH);

	/* create the session */
	session = nc_session_connect(NULL, 0, user, NULL);/*host = default 'localhost', port = default '830'*/
	/* fail to create a session*/	
	if(session == NULL)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Connecting to the localhost:830 failed.");
		throw OVSManagerException();
	}

	return EXIT_SUCCESS;
}

int cmd_disconnect(){
	/*not connected to any NETCONF server*/
	if (session == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Not connected to any NETCONF server.");
		throw OVSManagerException();
	} else {
		nc_session_free(session);
		session = NULL;
	}

	return EXIT_SUCCESS;
}

/*
*	@cli = struct CreateLsiIn
*	@FIXME = without virtual Link
*/
CreateLsiOut* cmd_editconfig_lsi (CreateLsiIn cli){

	/*information for CreateLsiOut to return*/
	CreateLsiOut *clo = NULL;
	map<string,unsigned int> physical_ports;
	map<string,map<string, unsigned int> >  network_functions_ports;
	list<pair<unsigned int, unsigned int> > virtual_links;
	map<string,unsigned int> n_ports;

	int rnumber_old = 0, dnumber_new = 0, nfnumber_old = 0, pnumber_old = 0;

	NC_EDIT_TESTOPT_TYPE testopt = NC_EDIT_TESTOPT_SET;
	NC_DATASTORE target = NC_DATASTORE_RUNNING /*by default is running*/;
	nc_rpc *rpc = NULL;
	
	//variable to write .xml file
	int rc;
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	FILE *fp;

	//list of ports
	list<string> ports = cli.getPhysicalPortsName();

	//list of nf	
	set<string> nfs = cli.getNetworkFunctionsName();

	//list of nft	
	map<string,nf_t> nf_type = cli.getNetworkFunctionsType(); 	

	list<uint64_t> vport = cli.getVirtualLinksRemoteLSI();

	const char *config = "File.xml";
	FILE *output = NULL;

	const char *bridge_name = "";
	const char *peer_name = "";

	//uint64_t port_id_1 = 0, port_id_2 = 0;

	char sw[64] = "Bridge", ctr[64] = "Controller", vrt[64] = "VirtualPort", trv[64] = "VPort";

	char temp[64] = "", tmp[64] = "", switch_name[64] = "", of_version[64] = "";

	char cmdLine[4096];

	/*set the OpenFlow version*/
	switch(OFP_VERSION)
	{
		case OFP_10:
			strcpy(of_version, "OpenFlow10");
			break;
		case OFP_12:
			strcpy(of_version, "OpenFlow12");
			break;
		case OFP_13:
			strcpy(of_version, "OpenFlow13");
			break;
	}


	/*Create a new XML buffer, to which the XML document will be written*/
	buf = xmlBufferCreate();
	if(buf == NULL){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlWriterMemory: Error creating the xml buffer\n.");
		throw OVSManagerException();		
	}
	
	/* Create a new XmlWriter for memory, with no compression.*/
    	writer = xmlNewTextWriterMemory(buf, 0);
    	if (writer == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error creating the xml writer\n");
		throw OVSManagerException();
    	}

	/*Root element "capable-switch"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "capable-switch");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}
			
	/*add attribute "xmlns"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns", BAD_CAST "urn:onf:config:yang");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}
			
	/*add attribute "xmlns:nc"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:nc",
	        BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}
	
	/*check if physical ports is 0*/
	if(ports.size() !=0 || nfs.size() != 0){
		/*create an element name "resources" as child of "capable-switch"*/		
		rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			throw OVSManagerException();
		}
	}

	if(ports.size() !=0){
		/*save the current rnumber*/
		rnumber_old = rnumber;

		/*for each port in the list of the ports*/
		for(list<string>::iterator p = ports.begin(); p != ports.end(); p++){
			/*create an element name "port as child of "resources""*/			
			rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
				throw OVSManagerException();
			}
					
			/*add attribute "nc:operation"*/
			rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation",
	                       	BAD_CAST "create");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
				throw OVSManagerException();
			}

			/*add element "name"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "name",
	               		BAD_CAST (*p).c_str());
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}
			
			/*add element "requested-number"*/
			rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "requested-number", "%d", rnumber);
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}

			/*add element "configuration"*/
			rc = xmlTextWriterStartElement(writer, BAD_CAST "configuration");
    			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
				throw OVSManagerException();
    			}
					
			/*add element "admin-state"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "admin-state", BAD_CAST "up");
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}	

			/*add element "no-receive"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-receive",BAD_CAST "false");
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}				
		
			/*add element "no-forward"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-forward",BAD_CAST "false");
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}				
				
			/*add element "no-packet-in"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-packet-in",BAD_CAST "false");
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}
				
			/*close element "configuration"*/
			rc = xmlTextWriterEndElement(writer);
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
				throw OVSManagerException();
    			}

			/*close element "port"*/
			rc = xmlTextWriterEndElement(writer);
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
				throw OVSManagerException();
    			}

			/*increment the value of request-number*/
			rnumber++;
		}
		
	}

	if(nfs.size() != 0){	

		nfnumber_old = rnumber;
	
		/*for each network function name in the list of nfs*/
		for(set<string>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
		{
			list<string> nfs_ports = cli.getNetworkFunctionsPortNames(*nf);
			/*for each network function port in the list of nfs_ports*/
			for(list<string>::iterator nfp = nfs_ports.begin(); nfp != nfs_ports.end(); nfp++){
				/*create an element name "port as child of "resources""*/			
				rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
					throw OVSManagerException();
				}
					
				/*add attribute "nc:operation"*/
				rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation", BAD_CAST "create");
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
					throw OVSManagerException();
				}

				strcpy(tmp, (char *)(*nfp).c_str());
				sprintf(temp, "%" PRIu64, dnumber);
				strcat(temp, "_");
				strcat(temp, tmp);

				/*add element "name"*/
				rc = xmlTextWriterWriteElement(writer, BAD_CAST "name",
	                       		BAD_CAST temp);
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
					throw OVSManagerException();
				}
				
				/*add element "requested-number"*/
				rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "requested-number", "%d", rnumber);
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
					throw OVSManagerException();
				}

				/*add element "configuration"*/
				rc = xmlTextWriterStartElement(writer, BAD_CAST "configuration");
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
					throw OVSManagerException();
				}
					
				/*add element "admin-state"*/
				rc = xmlTextWriterWriteElement(writer, BAD_CAST "admin-state", BAD_CAST "up");
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
					throw OVSManagerException();
				}	

				/*add element "no-receive"*/
				rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-receive",BAD_CAST "false");
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
					throw OVSManagerException();
				}				
		
				/*add element "no-forward"*/
				rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-forward",BAD_CAST "false");
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
					throw OVSManagerException();
				}				
				
				/*add element "no-packet-in"*/
				rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-packet-in",BAD_CAST "false");
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
					throw OVSManagerException();
				}
			
				/*close element "configuration"*/
				rc = xmlTextWriterEndElement(writer);
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
					throw OVSManagerException();
				}

				/*close element "port"*/
				rc = xmlTextWriterEndElement(writer);
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
					throw OVSManagerException();
				}

				/*increment the value of request-number*/
				rnumber++;
			}
		}
	}

	if(ports.size() != 0 || nfs.size() != 0){
		/*close element "resources"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			throw OVSManagerException();
		}
	}

	/*create an element name "logical-switches" as child of "capable-switch"*/		
	rc = xmlTextWriterStartElement(writer, BAD_CAST "logical-switches");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}

	/*add element "switch"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "switch");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*add attribute "nc:operation"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation",
              	BAD_CAST "create");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}

	/*create current name of a bridge*/
	sprintf(temp, "%" PRIu64, dnumber);
	strcat(sw, temp);
	strcpy(temp, sw);

	dnumber_new = dnumber;

	/*create an element name "id" as child of "switch"*/		
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "id", BAD_CAST temp);
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}		

	/*fill the map switch_id*/
	switch_id[dnumber] = string(temp);

	/*copy the current name of a switch*/
	strcpy(switch_name, temp);

	/*create an element name "datapath-id" as child of "switch"*/		
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "datapath-id", BAD_CAST num_to_string(dnumber));
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}

	if(ports.size() != 0 || nfs.size() != 0){
		/*add element "resources"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			throw OVSManagerException();
		}
	}

	/*check if physical ports is 0*/
	if(ports.size() != 0){

		/*store the old rnumber to the current rnumber*/
		rnumber = rnumber_old;

		/*for each port in the list of the ports*/
		for(list<string>::iterator p = ports.begin(); p != ports.end(); p++){

			/*add element "port"*/
			rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}

			/*add attribute "nc:operation"*/
			rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation", BAD_CAST "create");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
				throw OVSManagerException();
			}

			/*add content "port"*/
			rc = xmlTextWriterWriteRaw(writer, BAD_CAST (*p).c_str());
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}

			/*close element "port"*/
			rc = xmlTextWriterEndElement(writer);
	    		if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
				throw OVSManagerException();
	    		}

			/*add current port to physical_ports map*/
			physical_ports[(*p)] = rnumber;

			/*increment request-number value*/
			rnumber++;
		}
		
	}	

	if(nfs.size() != 0){

		rnumber = nfnumber_old;
	
		/*for each network function name in the list of nfp*/
		for(set<string>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
		{
			list<string> nfs_ports = cli.getNetworkFunctionsPortNames(*nf);

			map<string,unsigned int> n_ports_1;

			/*for each network function port in the list of nfs_ports*/				
			for(list<string>::iterator nfp = nfs_ports.begin(); nfp != nfs_ports.end(); nfp++){

				/*add element "port"*/
				rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
				if (rc < 0) {
		       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
					throw OVSManagerException();
				}

				/*add attribute "nc:operation"*/
				rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation", BAD_CAST "create");
		    		if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
					throw OVSManagerException();
				}

				strcpy(tmp, (char *)(*nfp).c_str());
				sprintf(temp, "%" PRIu64, dnumber);
				strcat(temp, "_");
				strcat(temp, tmp);

				/*add content "port"*/
				rc = xmlTextWriterWriteRaw(writer, BAD_CAST temp);
				if (rc < 0) {
		       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
					throw OVSManagerException();
				}

				/*close element "port"*/
				rc = xmlTextWriterEndElement(writer);
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
					throw OVSManagerException();
		    		}

				//vector<string> x = split((*nfp).c_str(), '_');

				/*fill the map ports*/
				n_ports_1[(*nfp)/*x[0]*/] = rnumber/*i*/;

				nf_id[make_pair(dnumber, (*nfp))] = string(temp);

				/*increment request-number value*/
				rnumber++;
			}

			/*fill the network_functions_ports*/
			network_functions_ports[(*nf)] = n_ports_1;

		}
	}

	if(ports.size() != 0 || nfs.size() != 0){
		/*close element "resources"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
	       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			throw OVSManagerException();
		}	
	}

	/*add element "controllers"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "controllers");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*add element "controller"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "controller");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*add attribute "nc:operation"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation",
	      	BAD_CAST "create");
    	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}

	/*create the current name of a controller*/
	sprintf(temp, "%" PRIu64, dnumber);
	strcat(ctr, temp);
	strcpy(temp, ctr);

	/*increment the value of datapath-id*/
	dnumber++;

	/*add element "id"*/
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "id", 
		BAD_CAST ctr);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*add element "ip-address"*/
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "ip-address", 
		BAD_CAST cli.getControllerAddress().c_str());
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*add element "port"*/
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "port", 
		BAD_CAST cli.getControllerPort().c_str());
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*add element local-ip-address*/
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "local-ip-address", 
		BAD_CAST "127.0.0.1");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*add element "protocol"*/
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "protocol", 
		BAD_CAST "tcp");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*close element "controller"*/
	rc = xmlTextWriterEndElement(writer);
    	if (rc < 0) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
    	}

	/*close element "controllers"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*close element "switch"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*close element "logical-switches"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*close element "capable-switch"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	xmlFreeTextWriter(writer);

	fp = fopen(config, "w");
    	if (fp == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__,"testXmlwriterMemory: Error at fopen\n");
        	throw OVSManagerException();
    	}

	fprintf(fp, "%s", (const char *) buf->content);

	fclose(fp);

	xmlBufferFree(buf);	

	/* read the configuration */
	doc = xmlReadFile("File.xml", NULL, XML_PARSE_NOBLANKS|XML_PARSE_NSCLEAN|XML_PARSE_NOERROR|XML_PARSE_NOWARNING);
	if (doc == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "failed to parse the file.");
        	throw OVSManagerException();
	}

	/* remove the <config> root if included */
	root = xmlDocGetRootElement(doc);
	if (root != NULL) {
		if (xmlStrEqual(root->name, BAD_CAST "config") && xmlStrEqual(root->ns->href, BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0")) {
			xmlDocSetRootElement(doc, root->children);
			xmlUnlinkNode(root);
			xmlFree(root);
		}
	}

	/* dump the content */
	xmlDocDumpMemory(doc, (xmlChar**)&config, NULL);

	xmlFreeDoc(doc);

	/* create requests */
	rpc = nc_rpc_editconfig(target, NC_DATASTORE_CONFIG, NC_EDIT_DEFOP_MERGE, NC_EDIT_ERROPT_STOP, testopt, config);
	//free(config);

	if (rpc == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "edit-config", "creating rpc request failed.");
        	throw OVSManagerException();
	}

	/* send the request and get the reply */
	send_recv_process("edit-config", rpc, NULL, output);

	/*remove file File.xml*/
	if( remove( "File.xml" ) != 0 )
    		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error deleting file" );

	/*for each bridge id in the list of remote lsi*/
	if(cli.getVirtualLinksRemoteLSI().size() != 0)
	{
		for(list<uint64_t>::iterator nf = vport.begin(); nf != vport.end(); nf++)
		{
			strcpy(vrt, "VPort");
			strcpy(trv, "VPort");

			/*create virtual link*/
			sprintf(temp, "%d", pnumber);
			strcat(vrt, temp);

			pnumber_old = pnumber;

			pnumber++;

			sprintf(temp, "%d", pnumber);
			strcat(trv, temp);

			/*store the value of bridge_name and peer_name*/
			bridge_name = switch_id[dnumber_new].c_str();
			peer_name = switch_id[(*nf)].c_str();

			/*bridge_name virtual link*/
			sprintf(cmdLine, "./VirtualLink.sh %s %s %s %s", bridge_name, peer_name, vrt, trv);

			/*execute VirtualLink.sh*/
			system(cmdLine);

			/*store the information [bridge name, list of peer port]*/
			virtual_link_id[/*port_id_1*/bridge_name].push_back(pnumber);

			/*store the information [port_id, port_name]*/
			port_id[/*port_id_1*/pnumber_old] = vrt;

			/*store the information [switch_id, port_id]*/
			vl_id[pnumber_old] = dnumber_new;
			vl_id[pnumber] = (*nf);

			/*peer_name virtual link*/
			//sprintf(cmdLine, "./VirtualLink.sh %s %s %s", peer_name, trv, vrt);

			/*execute VirtualLink.sh*/
			//system(cmdLine);

			/*store the information [port_id, port_name]*/
			port_id[/*port_id_2*/pnumber] = trv;

			/*store the information [bridge name, list of peer port]*/
			virtual_link_id[/*port_id_2*/peer_name].push_back(pnumber_old);

			//sprintf(cmdLine, "./IdPort.sh %s %s", bridge_name, vrt);

			/*execute IdPort.sh*/
			//port_id_1 = system(cmdLine);

			//port_id_1 = port_id_1/256;

			//sprintf(cmdLine, "./IdPort.sh %s %s", peer_name, trv);

			/*execute IdPort.sh*/
			//port_id_2 = system(cmdLine);

			//port_id_2 = port_id_2/256;

			/*store the information of virtual link*/
			virtual_links.push_back(make_pair(/*port_id_1, port_id_2*/pnumber_old, pnumber));

			pnumber++;
		}
	}	

	/*set string bash*/
	sprintf(cmdLine, "./OFVersion.sh %s %s", switch_name, of_version);

	/*execute OFVersion.sh*/
	system(cmdLine);

	clo = new CreateLsiOut(dnumber_new, physical_ports, network_functions_ports, virtual_links);

	return clo;
}

void cmd_delete_virtual_link(uint64_t dpid, uint64_t id){
	NC_EDIT_TESTOPT_TYPE testopt = NC_EDIT_TESTOPT_SET;
	NC_DATASTORE target = NC_DATASTORE_RUNNING /*by default is running*/;
	nc_rpc *rpc = NULL;
	
	//variable to write .xml file
	int rc;
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	FILE *fp;

	char *config = "File.xml";
	FILE *output = NULL;

	map<string, unsigned int> ports;

	virtual_link_id[switch_id[dpid]].clear();

	/*Create a new XML buffer, to which the XML document will be written*/
	buf = xmlBufferCreate();
	if(buf == NULL){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlWriterMemory: Error creating the xml buffer\n.");
		throw OVSManagerException();		
	}
	
	/* Create a new XmlWriter for memory, with no compression.*/
    	writer = xmlNewTextWriterMemory(buf, 0);
    	if (writer == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error creating the xml writer\n");
		throw OVSManagerException();
    	}

	/*Root element "capable-switch"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "capable-switch");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}
			
	/*add attribute "xmlns"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns", BAD_CAST "urn:onf:config:yang");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}
			
	/*add attribute "xmlns:nc"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:nc",
        	BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}

	/*create an element name "resources" as child of "capable-switch"*/		
	rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}

	/*create an element name "port as child of "resources""*/			
	rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}
					
	/*add attribute "nc:operation"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation", BAD_CAST "remove");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}

	/*add element "name"*/
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "name",
		BAD_CAST port_id[id].c_str());
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*close element "port"*/
	rc = xmlTextWriterEndElement(writer);
    	if (rc < 0) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
    	}

	/*close element "resources"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*create an element name "logical-switches" as child of "capable-switch"*/		
	rc = xmlTextWriterStartElement(writer, BAD_CAST "logical-switches");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}

	/*add element "switch"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "switch");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*create an element name "id" as child of "switch"*/		
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "id", BAD_CAST switch_id[dpid].c_str());
    	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
    	}

	/*add element "resources"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*add element "port"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*add attribute "nc:operation"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation", BAD_CAST "remove");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}

	/*add content "port"*/
	rc = xmlTextWriterWriteRaw(writer, BAD_CAST (port_id[id]).c_str());
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*close element "port"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*close element "resources"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*close element "switch"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*close element "logical-switches"*/
	rc = xmlTextWriterEndElement(writer);
    	if (rc < 0) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
    	}

	/*close element "capable-switch"*/
	rc = xmlTextWriterEndElement(writer);
    	if (rc < 0) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
    	}

	xmlFreeTextWriter(writer);

	fp = fopen(config, "w");
    	if (fp == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__,"testXmlwriterMemory: Error at fopen\n");
        	throw OVSManagerException();
    	}

	fprintf(fp, "%s", (const char *) buf->content);

	fclose(fp);

	xmlBufferFree(buf);	

	/* read the configuration */
	doc = xmlReadFile("File.xml", NULL, XML_PARSE_NOBLANKS|XML_PARSE_NSCLEAN|XML_PARSE_NOERROR|XML_PARSE_NOWARNING);
	if (doc == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "failed to parse the file.");
        	throw OVSManagerException();
	}

	/* remove the <config> root if included */
	root = xmlDocGetRootElement(doc);
	if (root != NULL) {
		if (xmlStrEqual(root->name, BAD_CAST "config") && xmlStrEqual(root->ns->href, BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0")) {
			xmlDocSetRootElement(doc, root->children);
			xmlUnlinkNode(root);
			xmlFree(root);
		}
	}

	/* dump the content */
	xmlDocDumpMemory(doc, (xmlChar**)&config, NULL);

	xmlFreeDoc(doc);

	/* create requests */
	rpc = nc_rpc_editconfig(target, NC_DATASTORE_CONFIG, NC_EDIT_DEFOP_MERGE, NC_EDIT_ERROPT_STOP, testopt, config);
	free(config);
	if (rpc == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "edit-config", "creating rpc request failed.");
        	throw OVSManagerException();
	}

	/* send the request and get the reply */
	send_recv_process("edit-config", rpc, NULL, output);

	/*remove file File.xml*/
	if( remove( "File.xml" ) != 0 )
    		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error deleting file" );
}

void cmd_editconfig_lsi_delete(uint64_t dpid){

	NC_EDIT_TESTOPT_TYPE testopt = NC_EDIT_TESTOPT_SET;
	NC_DATASTORE target = NC_DATASTORE_RUNNING /*by default is running*/;
	nc_rpc *rpc = NULL;
	
	//variable to write .xml file
	int rc;
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	FILE *fp;

	char *config = "File.xml";
	FILE *output = NULL;
	
	list<uint64_t> ii;

	/*for all virtual link ports*/
	for(list<uint64_t>::iterator i = virtual_link_id[switch_id[dpid]].begin(); i != virtual_link_id[switch_id[dpid]].end(); i++){
		cmd_delete_virtual_link(vl_id[(*i)], (*i));
	}

	/*Create a new XML buffer, to which the XML document will be written*/
	buf = xmlBufferCreate();
	if(buf == NULL){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlWriterMemory: Error creating the xml buffer\n.");
		throw OVSManagerException();		
	}
	
	/* Create a new XmlWriter for memory, with no compression.*/
    	writer = xmlNewTextWriterMemory(buf, 0);
    	if (writer == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error creating the xml writer\n");
		throw OVSManagerException();
    	}

	/*Root element "capable-switch"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "capable-switch");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}
			
	/*add attribute "xmlns"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns", BAD_CAST "urn:onf:config:yang");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}
			
	/*add attribute "xmlns:nc"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:nc",
	        BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}

	/*create an element name "logical-switches" as child of "capable-switch"*/		
	rc = xmlTextWriterStartElement(writer, BAD_CAST "logical-switches");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}

	/*add element "switch"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "switch");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*add attribute "nc:operation"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation",
	       	BAD_CAST "remove");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}

	/*create an element name "id" as child of "switch"*/		
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "id", BAD_CAST switch_id[dpid].c_str());
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}
	
	/*close element "switch"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*close element "logical-switches"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*close element "capable-switch"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}
		
	xmlFreeTextWriter(writer);

	fp = fopen(config, "w");
    	if (fp == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__,"testXmlwriterMemory: Error at fopen\n");
        	throw OVSManagerException();
    	}

	fprintf(fp, "%s", (const char *) buf->content);

	fclose(fp);

	xmlBufferFree(buf);	

	/* read the configuration */
	doc = xmlReadFile("File.xml", NULL, XML_PARSE_NOBLANKS|XML_PARSE_NSCLEAN|XML_PARSE_NOERROR|XML_PARSE_NOWARNING);
	if (doc == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "failed to parse the file.");
        	throw OVSManagerException();
	}

	/* remove the <config> root if included */
	root = xmlDocGetRootElement(doc);
	if (root != NULL) {
		if (xmlStrEqual(root->name, BAD_CAST "config") && xmlStrEqual(root->ns->href, BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0")) {
			xmlDocSetRootElement(doc, root->children);
			xmlUnlinkNode(root);
			xmlFree(root);
		}
	}

	/* dump the content */
	xmlDocDumpMemory(doc, (xmlChar**)&config, NULL);

	xmlFreeDoc(doc);

	/* create requests */
	rpc = nc_rpc_editconfig(target, NC_DATASTORE_CONFIG, NC_EDIT_DEFOP_MERGE, NC_EDIT_ERROPT_STOP, testopt, config);
	free(config);
	if (rpc == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "edit-config", "creating rpc request failed.");
        	throw OVSManagerException();
	}

	/* send the request and get the reply */
	send_recv_process("edit-config", rpc, NULL, output);	

	/*remove file File.xml*/
	if( remove( "File.xml" ) != 0 )
    		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error deleting file" );

}

AddNFportsOut *cmd_editconfig_NFPorts(AddNFportsIn anpi){

	NC_EDIT_TESTOPT_TYPE testopt = NC_EDIT_TESTOPT_SET;
	NC_DATASTORE target = NC_DATASTORE_RUNNING /*by default is running*/;
	nc_rpc *rpc = NULL;
	
	//variable to write .xml file
	int rc;
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	FILE *fp;

	char *config = "File.xml";
	FILE *output = NULL;

	char temp[64] = "", tmp[64] = "";

	int nfnumber_old = 0;

	AddNFportsOut *anf = NULL;

	list<string> nfp = anpi.getNetworkFunctionsPorts();

	map<string, unsigned int> ports;

	/*Create a new XML buffer, to which the XML document will be written*/
	buf = xmlBufferCreate();
	if(buf == NULL){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlWriterMemory: Error creating the xml buffer\n.");
		throw OVSManagerException();		
	}
	
	/* Create a new XmlWriter for memory, with no compression.*/
    	writer = xmlNewTextWriterMemory(buf, 0);
    	if (writer == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error creating the xml writer\n");
		throw OVSManagerException();
    	}

	/*Root element "capable-switch"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "capable-switch");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}
			
	/*add attribute "xmlns"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns", BAD_CAST "urn:onf:config:yang");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}
			
	/*add attribute "xmlns:nc"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:nc",
        	BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}

	if(nfp.size() !=0){
		/*create an element name "resources" as child of "capable-switch"*/		
		rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			throw OVSManagerException();
		}

		nfnumber_old = rnumber;

		/*for each port in the list of the getNetworkFunctionsPorts*/
		for(list<string>::iterator p = nfp.begin(); p != nfp.end(); p++){

			strcpy(tmp, (char *)(*p).c_str());
			sprintf(temp, "%" PRIu64, dnumber);
			strcat(temp, "_");
			strcat(temp, tmp);

			/*create an element name "port as child of "resources""*/			
			rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
				throw OVSManagerException();
			}
					
			/*add attribute "nc:operation"*/
			rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation", BAD_CAST "create");
			if (rc < 0) {
       				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
				throw OVSManagerException();
			}

			/*add element "name"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "name", BAD_CAST temp);
			if (rc < 0) {
       				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}
				
			/*add element "requested-number"*/
			rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "requested-number", "%d", rnumber);
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}

			/*add element "configuration"*/
			rc = xmlTextWriterStartElement(writer, BAD_CAST "configuration");
    			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
				throw OVSManagerException();
    			}
					
			/*add element "admin-state"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "admin-state", BAD_CAST "up");
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}	

			/*add element "no-receive"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-receive",BAD_CAST "false");
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}				
		
			/*add element "no-forward"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-forward",BAD_CAST "false");
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}				
				
			/*add element "no-packet-in"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-packet-in",BAD_CAST "false");
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}
				
			/*close element "configuration"*/
			rc = xmlTextWriterEndElement(writer);
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
				throw OVSManagerException();
    			}

			/*close element "port"*/
			rc = xmlTextWriterEndElement(writer);
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
				throw OVSManagerException();
    			}

			/*fill map ports to return*/
			ports[(*p)] = rnumber;

			nf_id[make_pair(dnumber, (*p))] = string(temp);

			/*increment the value of request-number*/
			rnumber++;
		}

		/*close element "resources"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
	       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			throw OVSManagerException();
		}	
	}

	/*create an element name "logical-switches" as child of "capable-switch"*/		
	rc = xmlTextWriterStartElement(writer, BAD_CAST "logical-switches");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}

	/*add element "switch"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "switch");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*create an element name "id" as child of "switch"*/		
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "id", BAD_CAST switch_id[anpi.getDpid()].c_str());
    	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
    	}

	if(nfp.size() != 0){

		/*add element "resources"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
		if (rc < 0) {
	       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			throw OVSManagerException();
		}

		rnumber = nfnumber_old;

		/*for each port in the list of the ports*/
		for(list<string>::iterator p = nfp.begin(); p != nfp.end(); p++){

			strcpy(tmp, (char *)(*p).c_str());
			sprintf(temp, "%" PRIu64, dnumber);
			strcat(temp, "_");
			strcat(temp, tmp);

			/*add element "port"*/
			rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}

			/*add attribute "nc:operation"*/
			rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation", BAD_CAST "create");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
				throw OVSManagerException();
			}

			/*add content "port"*/
			rc = xmlTextWriterWriteRaw(writer, BAD_CAST temp);
			if (rc < 0) {
	       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}

			/*close element "port"*/
			rc = xmlTextWriterEndElement(writer);
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
				throw OVSManagerException();
			}

			rnumber++;
		}

		/*close element "resources"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
	       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			throw OVSManagerException();
		}
	}

	/*close element "switch"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*close element "logical-switches"*/
	rc = xmlTextWriterEndElement(writer);
    	if (rc < 0) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
    	}

	/*close element "capable-switch"*/
	rc = xmlTextWriterEndElement(writer);
    	if (rc < 0) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
    	}
	
	xmlFreeTextWriter(writer);

	fp = fopen(config, "w");
    	if (fp == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__,"testXmlwriterMemory: Error at fopen\n");
        	throw OVSManagerException();
    	}

	fprintf(fp, "%s", (const char *) buf->content);

	fclose(fp);

	xmlBufferFree(buf);	

	/* read the configuration */
	doc = xmlReadFile("File.xml", NULL, XML_PARSE_NOBLANKS|XML_PARSE_NSCLEAN|XML_PARSE_NOERROR|XML_PARSE_NOWARNING);
	if (doc == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "failed to parse the file.");
        	throw OVSManagerException();
	}

	/* remove the <config> root if included */
	root = xmlDocGetRootElement(doc);
	if (root != NULL) {
		if (xmlStrEqual(root->name, BAD_CAST "config") && xmlStrEqual(root->ns->href, BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0")) {
			xmlDocSetRootElement(doc, root->children);
			xmlUnlinkNode(root);
			xmlFree(root);
		}
	}

	/* dump the content */
	xmlDocDumpMemory(doc, (xmlChar**)&config, NULL);

	xmlFreeDoc(doc);

	/* create requests */
	rpc = nc_rpc_editconfig(target, NC_DATASTORE_CONFIG, NC_EDIT_DEFOP_MERGE, NC_EDIT_ERROPT_STOP, testopt, config);
	free(config);
	if (rpc == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "edit-config", "creating rpc request failed.");
        	throw OVSManagerException();
	}

	/* send the request and get the reply */
	send_recv_process("edit-config", rpc, NULL, output);

	/*remove file File.xml*/
	if( remove( "File.xml" ) != 0 )
    		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error deleting file" );

	anf = new AddNFportsOut(anpi.getNFname(), ports);

	return anf;
}

void cmd_editconfig_NFPorts_delete(DestroyNFportsIn dnpi){
	NC_EDIT_TESTOPT_TYPE testopt = NC_EDIT_TESTOPT_SET;
	NC_DATASTORE target = NC_DATASTORE_RUNNING /*by default is running*/;
	nc_rpc *rpc = NULL;
	
	//variable to write .xml file
	int rc;
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	FILE *fp;

	char *config = "File.xml";
	FILE *output = NULL;

	map<string, unsigned int> ports;

	set<string> nfp = dnpi.getNFports();

	/*Create a new XML buffer, to which the XML document will be written*/
	buf = xmlBufferCreate();
	if(buf == NULL){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlWriterMemory: Error creating the xml buffer\n.");
		throw OVSManagerException();		
	}
	
	/* Create a new XmlWriter for memory, with no compression.*/
    	writer = xmlNewTextWriterMemory(buf, 0);
    	if (writer == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error creating the xml writer\n");
		throw OVSManagerException();
    	}

	/*Root element "capable-switch"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "capable-switch");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}
			
	/*add attribute "xmlns"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns", BAD_CAST "urn:onf:config:yang");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}
			
	/*add attribute "xmlns:nc"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:nc",
	        BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}

	if(nfp.size() !=0){
		/*create an element name "resources" as child of "capable-switch"*/		
		rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			throw OVSManagerException();
		}

		/*for each port in the list of the getNetworkFunctionsPorts*/
		for(set<string>::iterator p = nfp.begin(); p != nfp.end(); p++){
			/*create an element name "port as child of "resources""*/			
			rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
				throw OVSManagerException();
			}
					
			/*add attribute "nc:operation"*/
			rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation",
	                       	BAD_CAST "remove");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
				throw OVSManagerException();
			}

			/*add element "name"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "name",
	               		BAD_CAST nf_id[make_pair(dnpi.getDpid(), (*p).c_str())].c_str());
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}

			/*close element "port"*/
			rc = xmlTextWriterEndElement(writer);
			if (rc < 0) {
       				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
				throw OVSManagerException();
			}
		}

		/*close element "resources"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			throw OVSManagerException();
		}
	}

	/*create an element name "logical-switches" as child of "capable-switch"*/		
	rc = xmlTextWriterStartElement(writer, BAD_CAST "logical-switches");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}
	
	/*add element "switch"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "switch");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*create an element name "id" as child of "switch"*/		
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "id", BAD_CAST switch_id[dnpi.getDpid()].c_str());
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}

	/*add element "resources"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	if(nfp.size() != 0){
		/*for each port in the list of the ports*/
		for(set<string>::iterator p = nfp.begin(); p != nfp.end(); p++){

			/*add element "port"*/
			rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}

			/*add attribute "nc:operation"*/
			rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation", BAD_CAST "remove");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
				throw OVSManagerException();
			}

			/*add content "port"*/
			rc = xmlTextWriterWriteRaw(writer, BAD_CAST nf_id[make_pair(dnpi.getDpid(), (*p).c_str())].c_str());
			if (rc < 0) {
	       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}

			/*close element "port"*/
			rc = xmlTextWriterEndElement(writer);
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
				throw OVSManagerException();
			}
		}

		/*close element "resources"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			throw OVSManagerException();
		}
	}

	/*close element "switch"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*close element "logical-switches"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*close element "capable-switch"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	xmlFreeTextWriter(writer);

	fp = fopen(config, "w");
    	if (fp == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__,"testXmlwriterMemory: Error at fopen\n");
        	throw OVSManagerException();
    	}

	fprintf(fp, "%s", (const char *) buf->content);

	fclose(fp);

	xmlBufferFree(buf);	

	/* read the configuration */
	doc = xmlReadFile("File.xml", NULL, XML_PARSE_NOBLANKS|XML_PARSE_NSCLEAN|XML_PARSE_NOERROR|XML_PARSE_NOWARNING);
	if (doc == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "failed to parse the file.");
        	throw OVSManagerException();
	}

	/* remove the <config> root if included */
	root = xmlDocGetRootElement(doc);
	if (root != NULL) {
		if (xmlStrEqual(root->name, BAD_CAST "config") && xmlStrEqual(root->ns->href, BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0")) {
			xmlDocSetRootElement(doc, root->children);
			xmlUnlinkNode(root);
			xmlFree(root);
		}
	}

	/* dump the content */
	xmlDocDumpMemory(doc, (xmlChar**)&config, NULL);

	xmlFreeDoc(doc);

	/* create requests */
	rpc = nc_rpc_editconfig(target, NC_DATASTORE_CONFIG, NC_EDIT_DEFOP_MERGE, NC_EDIT_ERROPT_STOP, testopt, config);
	free(config);
	if (rpc == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "edit-config", "creating rpc request failed.");
        	throw OVSManagerException();
	}

	/* send the request and get the reply */
	send_recv_process("edit-config", rpc, NULL, output);

	/*remove file File.xml*/
	if( remove( "File.xml" ) != 0 )
    		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error deleting file" );

}

AddVirtualLinkOut *cmd_addVirtualLink(AddVirtualLinkIn avli){

	/*struct to return*/
	AddVirtualLinkOut *avlo = NULL;

	const char *bridge_name = switch_id[avli.getDpidA()].c_str();
	char vrt[64] = "VPort", trv[64] = "VPort", temp[64] = ""; 
	const char *peer_name = switch_id[avli.getDpidB()].c_str();

	list<uint64_t> po;
	list<uint64_t> po1;

	//uint64_t port_id_1 = 0, port_id_2 = 0;

	char cmdLine[4096];

	int pnumber_old = 0;

	/*create name virtual port*/
	sprintf(temp, "%d", pnumber);
	strcat(vrt, temp);

	pnumber_old = pnumber;

	pnumber++;

	sprintf(temp, "%d", pnumber);
	strcat(trv, temp);

        sprintf(cmdLine, "./VirtualLink.sh %s %s %s", bridge_name, vrt, trv);

	/*execute VirtualLink.sh*/
	system(cmdLine);

	sprintf(cmdLine, "./VirtualLink.sh %s %s %s", bridge_name, trv, vrt);

	/*execute VirtualLink.sh*/
	system(cmdLine);
	
	//sprintf(cmdLine, "./IdPort.sh %s %s", bridge_name, vrt);

	/*execute IdPort.sh*/
	//port_id_1 = system(cmdLine);

	//port_id_1 = port_id_1/256;

	/*store the information [port_id, bridge_name]*/
	virtual_link_id[/*port_id_1*/bridge_name].push_back(pnumber);

	/*store the information [port_id, port_name]*/
	port_id[/*port_id_1*/pnumber_old] = vrt;

	//sprintf(cmdLine, "./IdPort.sh %s %s", peer_name, trv);

	/*execute IdPort.sh*/
	//port_id_2 = system(cmdLine);

	//port_id_2 = port_id_2/256;

	/*store the information [port_id, bridge_name]*/
	virtual_link_id[/*port_id_2*/peer_name].push_back(pnumber_old);

	/*store the information [port_id, port_name]*/
	port_id[/*port_id_2*/pnumber] = trv;

	pnumber++;

	avlo = new AddVirtualLinkOut(/*port_id_1, port_id_2*/pnumber_old, pnumber);

	return avlo;

}

void cmd_destroyVirtualLink(DestroyVirtualLinkIn dvli){
	NC_EDIT_TESTOPT_TYPE testopt = NC_EDIT_TESTOPT_SET;
	NC_DATASTORE target = NC_DATASTORE_RUNNING /*by default is running*/;
	nc_rpc *rpc = NULL;
	
	//variable to write .xml file
	int rc;
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	FILE *fp;

	char *config = "File.xml";
	FILE *output = NULL;

	map<string, unsigned int> ports;

	virtual_link_id[switch_id[dvli.getDpidA()]].remove(dvli.getIdB());
	virtual_link_id[switch_id[dvli.getDpidB()]].remove(dvli.getIdA());

	/*Create a new XML buffer, to which the XML document will be written*/
	buf = xmlBufferCreate();
	if(buf == NULL){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlWriterMemory: Error creating the xml buffer\n.");
		throw OVSManagerException();		
	}
	
	/* Create a new XmlWriter for memory, with no compression.*/
    	writer = xmlNewTextWriterMemory(buf, 0);
    	if (writer == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error creating the xml writer\n");
		throw OVSManagerException();
    	}

	/*Root element "capable-switch"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "capable-switch");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}
			
	/*add attribute "xmlns"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns", BAD_CAST "urn:onf:config:yang");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}
			
	/*add attribute "xmlns:nc"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:nc",
        	BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}

	/*create an element name "resources" as child of "capable-switch"*/		
	rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}

	/*create an element name "port as child of "resources""*/			
	rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}
					
	/*add attribute "nc:operation"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation", BAD_CAST "remove");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}

	/*add element "name"*/
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "name",
		BAD_CAST port_id[dvli.getIdA()].c_str());
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*close element "port"*/
	rc = xmlTextWriterEndElement(writer);
    	if (rc < 0) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
    	}
	
	if(dvli.getIdB() != 0){	
		/*create an element name "port as child of "resources""*/			
		rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			throw OVSManagerException();
		}
					
		/*add attribute "nc:operation"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation", BAD_CAST "remove");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			throw OVSManagerException();
		}

		/*add element "name"*/
		rc = xmlTextWriterWriteElement(writer, BAD_CAST "name",
			BAD_CAST port_id[dvli.getIdB()].c_str());
		if (rc < 0) {
	       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			throw OVSManagerException();
		}

		/*close element "port"*/
		rc = xmlTextWriterEndElement(writer);
	    	if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			throw OVSManagerException();
	    	}
	}

	/*close element "resources"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*create an element name "logical-switches" as child of "capable-switch"*/		
	rc = xmlTextWriterStartElement(writer, BAD_CAST "logical-switches");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
	}

	/*add element "switch"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "switch");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*create an element name "id" as child of "switch"*/		
	rc = xmlTextWriterWriteElement(writer, BAD_CAST "id", BAD_CAST switch_id[dvli.getDpidA()].c_str());
    	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
		throw OVSManagerException();
    	}

	/*add element "resources"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*add element "port"*/
	rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*add attribute "nc:operation"*/
	rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation", BAD_CAST "remove");
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
		throw OVSManagerException();
	}

	/*add content "port"*/
	rc = xmlTextWriterWriteRaw(writer, BAD_CAST (port_id[dvli.getIdA()]).c_str());
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
		throw OVSManagerException();
	}

	/*close element "port"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*close element "resources"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	/*close element "switch"*/
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0) {
       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
	}

	if(dvli.getDpidB() != 0){
		/*add element "switch"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "switch");
		if (rc < 0) {
	       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			throw OVSManagerException();
		}
	
		/*create an element name "id" as child of "switch"*/		
		rc = xmlTextWriterWriteElement(writer, BAD_CAST "id", BAD_CAST switch_id[dvli.getDpidB()].c_str());
	    	if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			throw OVSManagerException();
	    	}

		/*add element "resources"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
		if (rc < 0) {
	       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			throw OVSManagerException();
		}

		/*add element "port"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			throw OVSManagerException();
		}

		/*add attribute "nc:operation"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation", BAD_CAST "remove");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
			throw OVSManagerException();
		}

		/*add content "port"*/
		rc = xmlTextWriterWriteRaw(writer, BAD_CAST (port_id[dvli.getIdB()]).c_str());
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			throw OVSManagerException();
		}

		/*close element "port"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			throw OVSManagerException();
		}

		/*close element "resources"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
	       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			throw OVSManagerException();
		}

		/*close element "switch"*/
		rc = xmlTextWriterEndElement(writer);
		if (rc < 0) {
	       		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
			throw OVSManagerException();
		}
	}

	/*close element "logical-switches"*/
	rc = xmlTextWriterEndElement(writer);
    	if (rc < 0) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
    	}

	/*close element "capable-switch"*/
	rc = xmlTextWriterEndElement(writer);
    	if (rc < 0) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
		throw OVSManagerException();
    	}

	xmlFreeTextWriter(writer);

	fp = fopen(config, "w");
    	if (fp == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__,"testXmlwriterMemory: Error at fopen\n");
        	throw OVSManagerException();
    	}

	fprintf(fp, "%s", (const char *) buf->content);

	fclose(fp);

	xmlBufferFree(buf);	

	/* read the configuration */
	doc = xmlReadFile("File.xml", NULL, XML_PARSE_NOBLANKS|XML_PARSE_NSCLEAN|XML_PARSE_NOERROR|XML_PARSE_NOWARNING);
	if (doc == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "failed to parse the file.");
        	throw OVSManagerException();
	}

	/* remove the <config> root if included */
	root = xmlDocGetRootElement(doc);
	if (root != NULL) {
		if (xmlStrEqual(root->name, BAD_CAST "config") && xmlStrEqual(root->ns->href, BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0")) {
			xmlDocSetRootElement(doc, root->children);
			xmlUnlinkNode(root);
			xmlFree(root);
		}
	}

	/* dump the content */
	xmlDocDumpMemory(doc, (xmlChar**)&config, NULL);

	xmlFreeDoc(doc);

	/* create requests */
	rpc = nc_rpc_editconfig(target, NC_DATASTORE_CONFIG, NC_EDIT_DEFOP_MERGE, NC_EDIT_ERROPT_STOP, testopt, config);
	free(config);
	if (rpc == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "edit-config", "creating rpc request failed.");
        	throw OVSManagerException();
	}

	/* send the request and get the reply */
	send_recv_process("edit-config", rpc, NULL, output);

	/*remove file File.xml*/
	if( remove( "File.xml" ) != 0 )
    		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error deleting file" );

}
