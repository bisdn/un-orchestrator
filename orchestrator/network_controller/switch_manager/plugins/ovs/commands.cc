#ifndef COMMANDS
#define COMMANDS

#include "ovs_manager.h"

#endif

#define MY_ENCODING "ISO-8859-1"

struct nc_session* session = NULL;

int rnumber = 0;
uint64_t dnumber = 0;
int pnumber = 0;

/*map use to obtain name of switch from id*/
map<uint64_t, string> switch_id;
/*map use to obtain name of switch from virtual_link_id*/
map<uint64_t, string> virtual_link_id;
/*map use to obtain name of port from port_id*/
map<uint64_t, string> port_id;

/*convert uint64_t into char* */
char *num_to_string(uint64_t num) {

	char *mac = new char[64];
	
	char *pmac = (char *)&num;

	sprintf(mac, "%X:%X:%X:%X:%X:%X:%X:%X", pmac[7], pmac[6], pmac[5], pmac[4], pmac[3], pmac[2], pmac[1], pmac[0]);

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
int cmd_connect() {

	/* set default transport protocol */
	nc_session_transport(NC_TRANSPORT_SSH);

	/* create the session */
	session = nc_session_connect(NULL, 0, "patrick", NULL);/*host = default 'localhost', port = default '830'*/
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
	
	//printf("Enter in a cmd_editconfig_lsi\n");

	/*information for CreateLsiOut to return*/
	CreateLsiOut *clo = NULL;
	map<string,unsigned int> physical_ports;
	map<string,map<string, unsigned int> >  network_functions_ports;
	list<pair<unsigned int, unsigned int> > virtual_links;
	map<string,unsigned int> n_ports;

	int rnumber_old = 0;

	NC_EDIT_TESTOPT_TYPE testopt = NC_EDIT_TESTOPT_SET;
	NC_DATASTORE target = NC_DATASTORE_RUNNING /*by default is running*/;
	nc_rpc *rpc = NULL;
	
	//variable to write .xml file
	int rc;
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	FILE *fp;

	//list of ports
	list<string> ports = cli.getPhysicalPortsName();

	//list of nf	
	set<string> nfs = cli.getNetworkFunctionsName();

	//list of nft	
	map<string,nf_t> nf_type = cli.getNetworkFunctionsType(); 	

	char *config = "File.xml", *temp = NULL, *sw = "Switch";
	FILE *output = NULL;

	int operation_type = 0;

	const char *bridge_name = "";
	const char *port_name = "VirtualPort_" + pnumber; 
	const char *peer_name = "";

	uint64_t port_id_1 = 0, port_id_2 = 0;

	//printf("%s\n", port_name);

	char cmdLine[4096];

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

	/*operation that performs is create*/
	if(!operation_type){
		/*Root element "capable-switch"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "capable-switch");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at capable-switch\n");
			throw OVSManagerException();
		}
			
		/*add attribute "xmls"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmls", BAD_CAST "urn:onf:config:yang");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmls\n");
			throw OVSManagerException();
		}
			
		/*add attribute "xmls:nc"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmls:nc",
                          BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmls:nc\n");
			throw OVSManagerException();
		}
			
		if(ports.size() !=0){
			/*create an element name "resources" as child of "capable-switch"*/		
			rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 					"testXmlwriterMemory: resources\n");
				throw OVSManagerException();
			}

			/*save the current rnumber*/
			rnumber_old = rnumber;

			/*for each port in the list of the ports*/
			for(list<string>::iterator p = ports.begin(); p != ports.end(); p++){
				/*create an element name "port as child of "resources""*/			
				rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at port\n");
					throw OVSManagerException();
				}
					
				/*add attribute "nc:operation"*/
				rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation",
                                     	BAD_CAST "create");
				if (rc < 0) {
       					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: nc:operation\n");
					throw OVSManagerException();
				}

				/*add element "name"*/
				rc = xmlTextWriterWriteElement(writer, BAD_CAST "name",
                               		BAD_CAST (*p).c_str());
				if (rc < 0) {
       					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at name\n");
					throw OVSManagerException();
				}
				
				/*add element "requested-number"*/
				rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "requested-number", "%d", rnumber);
    				if (rc < 0) {
        				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at requested-number\n");
					throw OVSManagerException();
				}

				/*add element "configuration"*/
				rc = xmlTextWriterStartElement(writer, BAD_CAST "configuration");
    				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at configuration\n");
					throw OVSManagerException();
    				}
					
				/*add element "admin-state"*/
				rc = xmlTextWriterWriteElement(writer, BAD_CAST "admin-state", BAD_CAST "up");
    				if (rc < 0) {
        				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at admin-state\n");
					throw OVSManagerException();
				}	

				/*add element "no-receive"*/
				rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-receive",BAD_CAST "false");
    				if (rc < 0) {
        				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at no-receive\n");
					throw OVSManagerException();
				}				
		
				/*add element "no-forward"*/
				rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-forward",BAD_CAST "false");
    				if (rc < 0) {
        				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at no-forward\n");
					throw OVSManagerException();
				}				
				
				/*add element "no-packet-in"*/
				rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-packet-in",BAD_CAST "false");
    				if (rc < 0) {
        				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at no-packet-in\n");
					throw OVSManagerException();
				}
				
				/*close element "configuration"*/
				rc = xmlTextWriterEndElement(writer);
    				if (rc < 0) {
        				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at configuration\n");
					throw OVSManagerException();
    				}

				/*close element "port"*/
				rc = xmlTextWriterEndElement(writer);
    				if (rc < 0) {
        				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at end port\n");
					throw OVSManagerException();
    				}

				/*increment the value of request-number*/
				rnumber++;
			}
				
			/*for each network function name in the list of nfs*/
			for(set<string>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
			{
				list<string> nfs_ports = cli.getNetworkFunctionsPortNames(*nf);
				/*for each network function port in the list of nfs_ports*/
				for(list<string>::iterator nfp = nfs_ports.begin(); nfp != nfs_ports.end(); nfp++){
					/*create an element name "port as child of "resources""*/			
					rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
					if (rc < 0) {
						logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at port\n");
						throw OVSManagerException();
					}
					
					/*add attribute "nc:operation"*/
					rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation", BAD_CAST "create");
					if (rc < 0) {
	       					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at nc:operation\n");
						throw OVSManagerException();
					}

					/*add element "name"*/
					rc = xmlTextWriterWriteElement(writer, BAD_CAST "name",
		                       		BAD_CAST (*nfp).c_str());
					if (rc < 0) {
	       					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at name\n");
						throw OVSManagerException();
					}
				
					/*add element "requested-number"*/
					rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "requested-number", "%d", rnumber);
	    				if (rc < 0) {
						logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at requested-number\n");
						throw OVSManagerException();
					}

					/*add element "configuration"*/
					rc = xmlTextWriterStartElement(writer, BAD_CAST "configuration");
	    				if (rc < 0) {
						logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at configuration\n");
						throw OVSManagerException();
	    				}
					
					/*add element "admin-state"*/
					rc = xmlTextWriterWriteElement(writer, BAD_CAST "admin-state", BAD_CAST "up");
	    				if (rc < 0) {
						logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at admin-state\n");
						throw OVSManagerException();
					}	

					/*add element "no-receive"*/
					rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-receive",BAD_CAST "false");
	    				if (rc < 0) {
						logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at no-receive\n");
						throw OVSManagerException();
					}				
		
					/*add element "no-forward"*/
					rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-forward",BAD_CAST "false");
	    				if (rc < 0) {
						logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at no-forward\n");
						throw OVSManagerException();
					}				
				
					/*add element "no-packet-in"*/
					rc = xmlTextWriterWriteElement(writer, BAD_CAST "no-packet-in",BAD_CAST "false");
	    				if (rc < 0) {
						logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at no-packet-in\n");
						throw OVSManagerException();
					}
				
					/*close element "configuration"*/
					rc = xmlTextWriterEndElement(writer);
	    				if (rc < 0) {
						logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at end configuration\n");
						throw OVSManagerException();
	    				}

					/*close element "port"*/
					rc = xmlTextWriterEndElement(writer);
	    				if (rc < 0) {
						logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at end port\n");
						throw OVSManagerException();
	    				}

					/*increment the value of request-number*/
					rnumber++;
				}
			}

			/*close element "resources"*/
			rc = xmlTextWriterEndElement(writer);
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at end resources\n");
				throw OVSManagerException();
    			}

			/*create an element name "logical-switches" as child of "capable-switch"*/		
			rc = xmlTextWriterStartElement(writer, BAD_CAST "logical-switches");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 					"testXmlwriterMemory: Error at logical-switches\n");
				throw OVSManagerException();
			}

			/*add element "switch"*/
			rc = xmlTextWriterStartElement(writer, BAD_CAST "switch");
			if (rc < 0) {
       				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at switch\n");
				throw OVSManagerException();
			}

			/*add attribute "nc:operation"*/
			rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation",
                              	BAD_CAST "create");
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at nc:operation\n");
				throw OVSManagerException();
			}

			temp = sw + dnumber;

			/*create an element name "id" as child of "switch"*/		
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "id", BAD_CAST temp);
    			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at id\n");
				throw OVSManagerException();
    			}		

			/*fill the map switch_id*/
			switch_id[dnumber] = temp;

			/*increment the value of datapath-id*/
			dnumber++;

			/*create an element name "datapath-id" as child of "switch"*/		
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "datapath-id", BAD_CAST num_to_string(dnumber));
    			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at datapath-id\n");
				throw OVSManagerException();
    			}

			/*add element "resources"*/
			rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
			if (rc < 0) {
       				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at resources\n");
				throw OVSManagerException();
			}

			/*store the old rnumber to the current rnumber*/
			rnumber = rnumber_old;

			/*for each port in the list of the ports*/
			for(list<string>::iterator p = ports.begin(); p != ports.end(); p++){

				/*add element "port"*/
				rc = xmlTextWriterWriteElement(writer, BAD_CAST "port", 
					BAD_CAST (*p).c_str());
				if (rc < 0) {
	       				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at port\n");
					throw OVSManagerException();
				}

				/*add attribute "nc:operation"*/
				rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation",
		                     	BAD_CAST "create");
	    			if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
					throw OVSManagerException();
				}

				/*add current port to physical_ports map*/
				physical_ports[num_to_string(dnumber)] = rnumber;

				/*increment request-number value*/
				rnumber++;
			}
			
			/*for each network function name in the list of nfp*/
			for(set<string>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
			{
				list<string> nfs_ports = cli.getNetworkFunctionsPortNames(*nf);
				/*for each network function port in the list of nfs_ports*/				
				for(list<string>::iterator nfp = nfs_ports.begin(); nfp != nfs_ports.end(); nfp++){
					/*add element "port"*/
					rc = xmlTextWriterWriteElement(writer, BAD_CAST "port", 
						BAD_CAST (*nfp).c_str());
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

					/*fill the map ports*/
					n_ports[(*nfp)] = rnumber;
					
					/*increment request-number value*/
					rnumber++;
				}

				/*fill the network_functions_ports*/
				network_functions_ports[*nf] = n_ports;
			}

			/*close element "resources"*/
			rc = xmlTextWriterEndElement(writer);
    			if (rc < 0) {
        			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
				throw OVSManagerException();
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

			/*add element "id"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "id", 
				BAD_CAST "Controller" + rnumber);
			if (rc < 0) {
       				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
				throw OVSManagerException();
			}

			/*add element "ip-address"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "id-address", 
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
		}		
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

	/* create requests */
	rpc = nc_rpc_editconfig(target, NC_DATASTORE_CONFIG, NC_EDIT_DEFOP_MERGE, NC_EDIT_ERROPT_NOTSET, testopt, "File.xml");
	free(config);
	if (rpc == NULL) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "edit-config", "creating rpc request failed.");
        	throw OVSManagerException();
	}

	/* send the request and get the reply */
	send_recv_process("edit-config", rpc, NULL, output);

	/*remove file File.xml*/
	/*if( remove( "File.xml" ) != 0 )
    		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error deleting file" );*/

	/*create virtual link*/
	/*for each bridge id in the list of remote lsi*/
	for(list<uint64_t>::iterator nf = cli.getVirtualLinksRemoteLSI().begin(); nf != 			cli.getVirtualLinksRemoteLSI().end(); nf++)
	{
		/*store the value of bridge_name and peer_name*/
		bridge_name = switch_id[dnumber].c_str();
		peer_name = switch_id[(*nf)].c_str();

		sprintf(cmdLine, "VirtualLink.sh %s %s %s", bridge_name, port_name, 			peer_name);

		/*execute VirtualLink.sh*/
		system(cmdLine);
	
		sprintf(cmdLine, "IdPort.sh %s", bridge_name);

		/*execute IdPort.sh*/
		port_id_1 = system(cmdLine);

		/*store the information [port_id, bridge_name]*/
		virtual_link_id[port_id_1] = bridge_name;

		/*store the information [port_id, port_name]*/
		port_id[port_id_1] = port_name;

		sprintf(cmdLine, "IdPort.sh %s", peer_name);

		/*execute IdPort.sh*/
		port_id_2 = system(cmdLine);

		/*store the information [port_id, bridge_name]*/
		virtual_link_id[port_id_2] = peer_name;

		pnumber++;
	}	

	clo = new CreateLsiOut(dnumber, physical_ports, network_functions_ports, 		virtual_links);

	return clo;
}

void cmd_editconfig_lsi_delete(uint64_t dpid){
	
	int operation_type = 1;

	NC_EDIT_TESTOPT_TYPE testopt = NC_EDIT_TESTOPT_SET;
	NC_DATASTORE target = NC_DATASTORE_RUNNING /*by default is running*/;
	nc_rpc *rpc = NULL;
	
	//variable to write .xml file
	int rc;
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	FILE *fp;

	char *config = "File.xml";
	FILE *output = NULL;

	/*Create a new XML buffer, to which the XML document will be written*/
	buf = xmlBufferCreate();
	if(buf == NULL){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlWriterMemory: Error creating 			the xml buffer\n.");
		throw OVSManagerException();		
	}
	
	/* Create a new XmlWriter for memory, with no compression.*/
    	writer = xmlNewTextWriterMemory(buf, 0);
    	if (writer == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error creating 			the xml writer\n");
		throw OVSManagerException();
    	}

	/*operation that performs is create*/
	if(operation_type){
		/*Root element "capable-switch"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "capable-switch");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: 					Error at xmlTextWriterStartElement\n");
			throw OVSManagerException();
		}
			
		/*add attribute "xmls"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmls", BAD_CAST "urn:onf:config:yang");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: 					Error at xmlTextWriterWriteAttribute\n");
			throw OVSManagerException();
		}
			
		/*add attribute "xmls:nc"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmls:nc",
                          BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: 					Error at xmlTextWriterWriteAttribute\n");
			throw OVSManagerException();
		}

		/*create an element name "logical-switches" as child of "capable-switch"*/		
		rc = xmlTextWriterStartElement(writer, BAD_CAST "logical-switches");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 				"testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
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
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 					"testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
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

	/* create requests */
	rpc = nc_rpc_editconfig(target, NC_DATASTORE_CONFIG, NC_EDIT_DEFOP_MERGE, NC_EDIT_ERROPT_NOTSET, testopt, "File.xml");
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
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	FILE *fp;

	char *config = "File.xml";
	FILE *output = NULL;

	int operation_type = 0;

	AddNFportsOut *anf = NULL;

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
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error creating 			the xml writer\n");
		throw OVSManagerException();
    	}

	/*operation that performs is create*/
	if(!operation_type){
		/*Root element "capable-switch"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "capable-switch");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: 					Error at xmlTextWriterStartElement\n");
			throw OVSManagerException();
		}
			
		/*add attribute "xmls"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmls", BAD_CAST "urn:onf:config:yang");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: 					Error at xmlTextWriterWriteAttribute\n");
			throw OVSManagerException();
		}
			
		/*add attribute "xmls:nc"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmls:nc",
                          BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: 					Error at xmlTextWriterWriteAttribute\n");
			throw OVSManagerException();
		}

		if(anpi.getNetworkFunctionsPorts().size() !=0){
			/*create an element name "resources" as child of "capable-switch"*/		
			rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 					"testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
				throw OVSManagerException();
			}

			/*for each port in the list of the getNetworkFunctionsPorts*/
			for(list<string>::iterator p = anpi.getNetworkFunctionsPorts().begin(); p != anpi.getNetworkFunctionsPorts().end(); p++){
				/*create an element name "port as child of "resources""*/			
				rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 						"testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
					throw OVSManagerException();
				}
					
				/*add attribute "nc:operation"*/
				rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation",
                                     	BAD_CAST "create");
				if (rc < 0) {
       					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: 				Error at xmlTextWriterWriteAttribute\n");
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
        				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: 				Error at xmlTextWriterWriteElement\n");
					throw OVSManagerException();
				}

				/*add element "configuration"*/
				rc = xmlTextWriterStartElement(writer, BAD_CAST "configuration");
    				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 						"testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
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
				ports[(*p)] = anpi.getDpid();

				/*increment the value of request-number*/
				rnumber++;
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
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 				"testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
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
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 					"testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			throw OVSManagerException();
    		}

		/*add element "resources"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			throw OVSManagerException();
		}

		/*for each port in the list of the ports*/
		for(list<string>::iterator p = anpi.getNetworkFunctionsPorts().begin(); p != anpi.getNetworkFunctionsPorts().end(); p++){

			/*add element "port"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "port", 
				BAD_CAST (*p).c_str());
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

	/* create requests */
	rpc = nc_rpc_editconfig(target, NC_DATASTORE_CONFIG, NC_EDIT_DEFOP_MERGE, NC_EDIT_ERROPT_NOTSET, testopt, "File.xml");
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
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	FILE *fp;

	char *config = "File.xml";
	FILE *output = NULL;

	int operation_type = 0;

	map<string, unsigned int> ports;

	/*Create a new XML buffer, to which the XML document will be written*/
	buf = xmlBufferCreate();
	if(buf == NULL){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlWriterMemory: Error creating 			the xml buffer\n.");
		throw OVSManagerException();		
	}
	
	/* Create a new XmlWriter for memory, with no compression.*/
    	writer = xmlNewTextWriterMemory(buf, 0);
    	if (writer == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error creating 			the xml writer\n");
		throw OVSManagerException();
    	}

	/*operation that performs is remove*/
	if(!operation_type){
		/*Root element "capable-switch"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "capable-switch");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: 					Error at xmlTextWriterStartElement\n");
			throw OVSManagerException();
		}
			
		/*add attribute "xmls"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmls", BAD_CAST "urn:onf:config:yang");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: 					Error at xmlTextWriterWriteAttribute\n");
			throw OVSManagerException();
		}
			
		/*add attribute "xmls:nc"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmls:nc",
                          BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: 					Error at xmlTextWriterWriteAttribute\n");
			throw OVSManagerException();
		}

		if(dnpi.getNFports().size() !=0){
			/*create an element name "resources" as child of "capable-switch"*/		
			rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
			if (rc < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 					"testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
				throw OVSManagerException();
			}

			/*for each port in the list of the getNetworkFunctionsPorts*/
			for(set<string>::iterator p = dnpi.getNFports().begin(); p != dnpi.getNFports().end(); p++){
				/*create an element name "port as child of "resources""*/			
				rc = xmlTextWriterStartElement(writer, BAD_CAST "port");
				if (rc < 0) {
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 						"testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
					throw OVSManagerException();
				}
					
				/*add attribute "nc:operation"*/
				rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "nc:operation",
                                     	BAD_CAST "remove");
				if (rc < 0) {
       					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: 				Error at xmlTextWriterWriteAttribute\n");
					throw OVSManagerException();
				}

				/*add element "name"*/
				rc = xmlTextWriterWriteElement(writer, BAD_CAST "name",
                               		BAD_CAST (*p).c_str());
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
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 				"testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
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
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 					"testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			throw OVSManagerException();
    		}

		/*add element "resources"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			throw OVSManagerException();
		}

		/*for each port in the list of the ports*/
		for(set<string>::iterator p = dnpi.getNFports().begin(); p != dnpi.getNFports().end(); p++){

			/*add element "port"*/
			rc = xmlTextWriterWriteElement(writer, BAD_CAST "port", 
				BAD_CAST (*p).c_str());
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

	/* create requests */
	rpc = nc_rpc_editconfig(target, NC_DATASTORE_CONFIG, NC_EDIT_DEFOP_MERGE, NC_EDIT_ERROPT_NOTSET, testopt, "File.xml");
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
	const char *port_name = "VirtualPort_" + pnumber; 
	const char *peer_name = switch_id[avli.getDpidB()].c_str();

	uint64_t port_id_1 = 0, port_id_2 = 0;

	char cmdLine[4096];

        sprintf(cmdLine, "VirtualLink.sh %s %s %s", bridge_name, port_name, peer_name);

	/*execute VirtualLink.sh*/
	system(cmdLine);
	
	sprintf(cmdLine, "IdPort.sh %s", bridge_name);

	/*execute IdPort.sh*/
	port_id_1 = system(cmdLine);

	/*store the information [port_id, bridge_name]*/
	virtual_link_id[port_id_1] = bridge_name;

	/*store the information [port_id, port_name]*/
	port_id[port_id_1] = port_name;

	sprintf(cmdLine, "IdPort.sh %s", peer_name);

	/*execute IdPort.sh*/
	port_id_2 = system(cmdLine);

	/*store the information [port_id, bridge_name]*/
	virtual_link_id[port_id_2] = peer_name;

	pnumber++;

	avlo = new AddVirtualLinkOut(port_id_1, port_id_2);

	return avlo;

}

void cmd_destroyVirtualLink(DestroyVirtualLinkIn dvli){
	NC_EDIT_TESTOPT_TYPE testopt = NC_EDIT_TESTOPT_SET;
	NC_DATASTORE target = NC_DATASTORE_RUNNING /*by default is running*/;
	nc_rpc *rpc = NULL;
	
	//variable to write .xml file
	int rc;
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	FILE *fp;

	char *config = "File.xml";
	FILE *output = NULL;

	int operation_type = 0;

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

	/*operation that performs is remove*/
	if(!operation_type){
		/*Root element "capable-switch"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "capable-switch");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			throw OVSManagerException();
		}
			
		/*add attribute "xmls"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmls", BAD_CAST "urn:onf:config:yang");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: 					Error at xmlTextWriterWriteAttribute\n");
			throw OVSManagerException();
		}
			
		/*add attribute "xmls:nc"*/
		rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xmls:nc",
                          BAD_CAST "urn:ietf:params:xml:ns:netconf:base:1.0");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: 					Error at xmlTextWriterWriteAttribute\n");
			throw OVSManagerException();
		}

		/*create an element name "resources" as child of "capable-switch"*/		
		rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 				"testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
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
	              		BAD_CAST port_id[dvli.getDpidA()].c_str());
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
	              		BAD_CAST port_id[dvli.getDpidB()].c_str());
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
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 				"testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			throw OVSManagerException();
		}

		/*add element "switch"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "switch");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			throw OVSManagerException();
		}

		/*create an element name "id" as child of "switch"*/		
		rc = xmlTextWriterWriteElement(writer, BAD_CAST "id", BAD_CAST virtual_link_id[dvli.getDpidA()].c_str());
    		if (rc < 0) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, 					"testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
			throw OVSManagerException();
    		}

		/*add element "resources"*/
		rc = xmlTextWriterStartElement(writer, BAD_CAST "resources");
		if (rc < 0) {
       			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
			throw OVSManagerException();
		}

		/*add element "port"*/
		rc = xmlTextWriterWriteElement(writer, BAD_CAST "port", 
			BAD_CAST port_id[dvli.getDpidA()].c_str());
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

		/*add element "port"*/
		rc = xmlTextWriterWriteElement(writer, BAD_CAST "port", 
			BAD_CAST port_id[dvli.getDpidB()].c_str());
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
		
	}

	xmlFreeTextWriter(writer);

	//printf("pippo\n");

	fp = fopen(config, "w");
    	if (fp == NULL) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__,"testXmlwriterMemory: Error at fopen\n");
        	throw OVSManagerException();
    	}

	fprintf(fp, "%s", (const char *) buf->content);

	/*char *temp = "";

	while(fscanf(fp, "%s", temp))
		printf("%s\n", temp);*/

	fclose(fp);

	xmlBufferFree(buf);	

	/* create requests */
	rpc = nc_rpc_editconfig(target, NC_DATASTORE_CONFIG, NC_EDIT_DEFOP_MERGE, NC_EDIT_ERROPT_NOTSET, testopt, "File.xml");
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
