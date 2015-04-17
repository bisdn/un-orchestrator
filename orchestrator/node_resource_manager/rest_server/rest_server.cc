#include "rest_server.h"

GraphManager *RestServer::gm = NULL;

#ifdef READ_JSON_FROM_FILE
	bool RestServer::init(char *filename, int core_mask, bool wireless, char *wirelessName)
#else
	bool RestServer::init(int core_mask, bool wireless, char *wirelessName)
#endif
{	
	try
	{
		gm = new GraphManager(core_mask, wireless, wirelessName);
		
	}catch (...)
	{
		return false;		
	}
	
#ifdef READ_JSON_FROM_FILE

	sleep(2); //This give time to the controller to be initialized

	std::ifstream file;
	file.open(filename);
	if(file.fail())
    {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot open the file %s",filename);
		return false;
	}
	
	stringstream stream;
	string str; 
    while (std::getline(file, str))
        stream << str << endl;
            
    if(doPut(stream.str()) == 0)
		return false;
#endif
		
	return true;
}

void RestServer::terminate()
{
	delete(gm);
}

#ifndef READ_JSON_FROM_FILE
void RestServer::request_completed (void *cls, struct MHD_Connection *connection,
						void **con_cls, enum MHD_RequestTerminationCode toe)
{
	struct connection_info_struct *con_info = (struct connection_info_struct *)(*con_cls);

	if (NULL == con_info) 
		return;

	if(con_info->length != 0)
	{
		free(con_info->message);
		con_info->message = NULL;
	}
	
	free (con_info);
	*con_cls = NULL;
}

int RestServer::answer_to_connection (void *cls, struct MHD_Connection *connection,
			const char *url, const char *method, const char *version,
			const char *upload_data,
			size_t *upload_data_size, void **con_cls)
{
	
	if(NULL == *con_cls)
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "New %s request for %s using version %s", method, url, version);
		if(LOGGING_LEVEL <= ORCH_DEBUG)
			MHD_get_connection_values (connection, MHD_HEADER_KIND, &print_out_key, NULL);
	
		struct connection_info_struct *con_info;
		con_info = (struct connection_info_struct*)malloc (sizeof (struct connection_info_struct));
		
		assert(con_info != NULL);		
		if (NULL == con_info) 
			return MHD_NO;
		
		if ((0 == strcmp (method, PUT)) || (0 == strcmp (method, DELETE)) )
		{
			con_info->message = (char*)malloc(REQ_SIZE * sizeof(char));
			con_info->length = 0;
		}
		else if (0 == strcmp (method, GET))
			con_info->length = 0;
		else
		{
			con_info->message = (char*)malloc(REQ_SIZE * sizeof(char));
			con_info->length = 0;
		}
	
		*con_cls = (void*) con_info;
		return MHD_YES;
	}

	if (0 == strcmp (method, GET))
		return doGet(connection,url);
	else if( (0 == strcmp (method, PUT)) || (0 == strcmp (method, DELETE)) )
	{	
		struct connection_info_struct *con_info = (struct connection_info_struct *)(*con_cls);
		assert(con_info != NULL);
		if (*upload_data_size != 0)
		{
			strcpy(&con_info->message[con_info->length],upload_data);
			con_info->length += *upload_data_size;
			*upload_data_size = 0;
			return MHD_YES;
		}
		else if (NULL != con_info->message)
		{
			con_info->message[con_info->length] = '\0';
			return (0 == strcmp (method, PUT))? doPut(connection,url,con_cls) : doDelete(connection,url,con_cls);
		}
	}
	else
	{
		//Methods not implemented
		struct connection_info_struct *con_info = (struct connection_info_struct *)(*con_cls);
		assert(con_info != NULL);
		if (*upload_data_size != 0)
		{
			strcpy(&con_info->message[con_info->length],upload_data);
			con_info->length += *upload_data_size;
			*upload_data_size = 0;
			return MHD_YES;
		}
		else
		{
			con_info->message[con_info->length] = '\0';
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Method \"%s\" not implemented",method);
			struct MHD_Response *response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
			int ret = MHD_queue_response (connection, MHD_HTTP_NOT_IMPLEMENTED, response);
			MHD_destroy_response (response);
			return ret;
		}
	}
	
	//Just to remove a warning in the compiler
	return MHD_YES;
}


int RestServer::print_out_key (void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "%s: %s", key, value);
	return MHD_YES;
}
#endif

#ifndef READ_JSON_FROM_FILE
int RestServer::doPut(struct MHD_Connection *connection, const char *url, void **con_cls)
#else
int RestServer::doPut(string toBeCreated)
#endif
{
#ifndef READ_JSON_FROM_FILE
	struct MHD_Response *response;
	
	struct connection_info_struct *con_info = (struct connection_info_struct *)(*con_cls);
	assert(con_info != NULL);

	//Check the URL
	char delimiter[] = "/";
 	char * pnt;

	char graphID[BUFFER_SIZE];
	
	char tmp[BUFFER_SIZE];
	strcpy(tmp,url);
	pnt=strtok(tmp, delimiter);
	int i = 0;
	while( pnt!= NULL ) 
	{
		switch(i)
		{
			case 0:
				if(strcmp(pnt,BASE_URL_GRAPH) != 0)
				{
put_malformed_url:
					logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource \"%s\" does not exist", url);
					response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
					int ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
					MHD_destroy_response (response);
					return ret;
				}
				break;
			case 1:
				strcpy(graphID,pnt);
		}
		
		pnt = strtok( NULL, delimiter );
		i++;
	}
	if(i != 2)
	{
		//the URL is malformed
		goto put_malformed_url;
	}
	
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource to be created/updated: %s",graphID);
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Content:");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",con_info->message);
	
	if(MHD_lookup_connection_value (connection,MHD_HEADER_KIND, "Host") == NULL)
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\"Host\" header not present in the request");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
		MHD_destroy_response (response);
		return ret;
	}
	
/*	const char *c_type = MHD_lookup_connection_value (connection,MHD_HEADER_KIND, "Content-Type");
	if(strcmp(c_type,JSON_C_TYPE) != 0)
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Content-Type must be: "JSON_C_TYPE);
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_UNSUPPORTED_MEDIA_TYPE, response);
		MHD_destroy_response (response);
		return ret;
	}*/
#else
	char graphID[BUFFER_SIZE];
	strcpy(graphID,"NF-FG");
	
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource to be created/updated: %s",graphID);
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Content:");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",toBeCreated.c_str());
#endif

	bool newGraph = !(gm->graphExists(graphID));
	
	string gID(graphID);
	highlevel::Graph *graph = new highlevel::Graph(gID);

#ifndef READ_JSON_FROM_FILE
	if(!parsePutBody(*con_info,*graph,newGraph))
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Malformed content");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
		MHD_destroy_response (response);
		return ret;
	}
#else
	if(!parsePutBody(toBeCreated,*graph,newGraph))
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Malformed content");
		return 0;
	}
#endif

	graph->print();
	try
	{
#ifndef READ_JSON_FROM_FILE
		if(newGraph)
#endif
		{
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "A new graph must be created");		
			if(!gm->newGraph(graph))
			{
				logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph description is not valid!");
#ifndef READ_JSON_FROM_FILE
				response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
				int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
				MHD_destroy_response (response);
				return ret;
#else
				return 0;
#endif
			}
		}
#ifndef READ_JSON_FROM_FILE
		else
		{
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "An existing graph must be updated");
			if(!gm->updateGraph(graphID,graph))
			{
				delete(graph);
				logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph description is not valid!");
				response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
				int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
				MHD_destroy_response (response);
				return ret;		
			}	
		}
#endif
	}catch (...)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred during the %s of the graph!",(newGraph)? "creation" : "update");
#ifndef READ_JSON_FROM_FILE
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
		MHD_destroy_response (response);
		return ret;
#else
		return 0;
#endif
	}

#ifndef READ_JSON_FROM_FILE
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph has been properly %s!",(newGraph)? "created" : "updated");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "");
	
	//TODO: put the proper content in the answer
	response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
	stringstream absolute_url;
	absolute_url << REST_URL << ":" << REST_PORT << url;
	MHD_add_response_header (response, "Location", absolute_url.str().c_str());
	int ret = MHD_queue_response (connection, MHD_HTTP_CREATED, response);

	MHD_destroy_response (response);
	return ret;	
#else
	return 1;
#endif
}

#ifndef READ_JSON_FROM_FILE
bool RestServer::parsePutBody(struct connection_info_struct &con_info,highlevel::Graph &graph, bool newGraph)
#else
bool RestServer::parsePutBody(string toBeCreated,highlevel::Graph &graph, bool newGraph)
#endif
{
	//for each NF, contains the set of ports it requires
	map<string,set<unsigned int> > nfs_ports_found;

	//for each NF, contains a list of ports - mac addresses
	map<string,list<pair<unsigned int,string> > > portsMacAddress;
	//for each NF, contains a list of ports - ip addresses
	map<string,list<pair<unsigned int,string> > > portsAddress;
	//for each NF, contains a list of ports - netmask
	map<string,list<pair<unsigned int,string > > > portsMask;

	try
	{
		Value value;
#ifndef READ_JSON_FROM_FILE
		read(con_info.message, value );
#else
		read(toBeCreated, value );		
#endif
		Object obj = value.getObject();
		
	  	bool foundFlowGraph = false;
		
		//Identify the flow rules
		for( Object::const_iterator i = obj.begin(); i != obj.end(); ++i )
		{
	 	    const string& name  = i->first;
		    const Value&  value = i->second;
		    
		    if(name == FLOW_GRAPH)
		    {
		    	foundFlowGraph = true;
		    	
		    	bool foundVNFs = false;
		    	bool foundFlowRules = false;
		    	
		  		Object flow_graph = value.getObject();
		    	for(Object::const_iterator fg = flow_graph.begin(); fg != flow_graph.end(); fg++)
		    	{
		    		const string& fg_name  = fg->first;
				    const Value&  fg_value = fg->second;
				    if(fg_name == VNFS)
				    {
				    	foundVNFs = true;
				    	const Array& vnfs_array = fg_value.getArray();
				    	if(vnfs_array.size() == 0)
				    	{
					    	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" without rules",VNFS);
							return false;
				    	}
				    	
				    	//Itearate on the VNFs
				    	for( unsigned int vnf = 0; vnf < vnfs_array.size(); ++vnf )
						{
							//This is a VNF, with an ID and a template
							Object network_function = vnfs_array[vnf].getObject();
#ifdef POLITO_MESSAGE							
							bool foundTemplate = false;
#endif					
							bool foundID = false;
							
							map<string,string> ipv4_addresses; 	//port name,ipv4 address
							map<string,string> ipv4_masks;				//port name, ipv4 address

							//Parse the rule
							for(Object::const_iterator nf = network_function.begin(); nf != network_function.end(); nf++)
							{
								const string& nf_name  = nf->first;
								const Value&  nf_value = nf->second;
					
								if(nf_name == _ID)
								{
									logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",VNFS,_ID,nf_value.getString().c_str());
									foundID = true;
									if(!graph.addNetworkFunction(nf_value.getString()))
									{
										logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Two VNFs with the same ID \"%s\" in \"%s\"",nf_value.getString().c_str(),VNFS);
										return false;
									}
								}
								else if(nf_name == TEMPLATE)
								{
									logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",VNFS,TEMPLATE,nf_value.getString().c_str());
#ifdef POLITO_MESSAGE
									foundTemplate = true;
									//XXX: currently, this information is ignored
#endif
								}
								else if(nf_name == PORTS_WITH_REQ)
								{
									const Array& ports_requirements_array = nf_value.getArray();
									if(ports_requirements_array.size() == 0)
									{
										logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Wrong value for key \"%s\"",PORTS_WITH_REQ);
										return false;
									}
									//Itearate on the ports with requirements
							    	for(unsigned int port = 0; port < ports_requirements_array.size(); ++port )
									{
										//This is a port with requirements, with a name and IPv4 information
										Object requirements = ports_requirements_array[port].getObject();
										bool foundName = false;
										bool foundIPv4 = false;
										bool foundEthernet = false;
										
										string portName, mac, address, netmask;
										
										for(Object::const_iterator rq = requirements.begin(); rq != requirements.end(); rq++)
										{
											const string& rq_name  = rq->first;
											const Value&  rq_value = rq->second;
					
											if(rq_name == PORT_NAME)
											{
												foundName = true;
												logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",VNFS,PORTS_WITH_REQ,rq_value.getString().c_str());
												portName = rq_value.getString();
											}
											else if(rq_name == ETHERNET)
											{
												foundEthernet = true;
												Object eth_parameters = rq_value.getObject();
												bool foundAddress = false;
												for(Object::const_iterator eth = eth_parameters.begin(); eth != eth_parameters.end(); eth++)
												{
													const string& eth_name  = eth->first;
													const Value&  eth_value = eth->second;
																				
													if(eth_name == ADDRESS)
													{
														foundAddress = true;
														logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\"->\"%s\"->\"%s\": \"%s\"",VNFS,PORTS_WITH_REQ,ETHERNET,ADDRESS,eth_value.getString().c_str());
														if(!MatchParser::validateMac(eth_value.getString().c_str()))
														{
															logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ADDRESS,eth_value.getString().c_str());
															return false;
														}
														mac = eth_value.getString();
													}
													else
													{
														logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\" in a port of \"%s\"",eth_name.c_str(),ETHERNET);
														return false;
													}
												}
												if(!foundAddress)
												{
													logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" not found in an elmenet of \"%s\"",ADDRESS,MASK,ETHERNET);
													return false;
												}
											}
											else if(rq_name == IP4)
											{
												foundIPv4 = true;
												Object ipv4_parameters = rq_value.getObject();
												bool foundAddress = false;
												bool foundMask = false;
												for(Object::const_iterator ip4 = ipv4_parameters.begin(); ip4 != ipv4_parameters.end(); ip4++)
												{
													const string& ip4_name  = ip4->first;
													const Value&  ip4_value = ip4->second;
																				
													if(ip4_name == ADDRESS)
													{
														foundAddress = true;
														logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\"->\"%s\"->\"%s\": \"%s\"",VNFS,PORTS_WITH_REQ,IP4,ADDRESS,ip4_value.getString().c_str());
														if(!MatchParser::validateIpv4(ip4_value.getString()))
														{
															logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ADDRESS,ip4_value.getString().c_str());
															return false;
														}
														address = ip4_value.getString();
													}
													else if(ip4_name == MASK)
													{
														foundMask = true;
														foundAddress = true;
														logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\"->\"%s\"->\"%s\": \"%s\"",VNFS,PORTS_WITH_REQ,IP4,MASK,ip4_value.getString().c_str());
														if(!MatchParser::validateIpv4Netmask(ip4_value.getString()))
														{
															logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",MASK,ip4_value.getString().c_str());
															return false;
														}
														netmask = ip4_value.getString();
													}
													else
													{
														logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\" in a port of \"%s\"",ip4_name.c_str(),IP4);
														return false;
													}
												}
												if(!foundAddress || !foundMask)
												{
													logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\", or key \"%s\", or both not found in an elmenet of \"%s\"",ADDRESS,MASK,IP4);
													return false;
												}
											}
											else
											{
												logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\" in a port of \"%s\"",rq_name.c_str(),PORTS_WITH_REQ);
												return false;
											}
										}
										if(!foundName || !(foundIPv4 || foundEthernet))
										{
											logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\", or key \"%s\", or key \"%s\" or all of them not found in an elmenet of \"%s\"",PORT_NAME,ETHERNET,IP4,PORTS_WITH_REQ);
											return false;
										}
										logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Port \"%s\" configuration - ipv4: %s - netmask: %s",portName.c_str(), address.c_str(), netmask.c_str());
										
										string theName = MatchParser::nfName(portName);
										unsigned int nf_port = MatchParser::nfPort(portName);
										
										//TODO: controllare che theName sia uguale al nome della funzione che si sta descrivendo
										
										
										if(foundEthernet)
										{
											//Save the MAC address
											list<pair<unsigned int,string> > list_mac_addresses;
											if(portsMacAddress.count(theName) != 0)
													list_mac_addresses = portsMacAddress[theName];
											list_mac_addresses.push_back(make_pair<unsigned int,string>(nf_port,mac));
											portsMacAddress[theName] = list_mac_addresses;
										}
										
										if(foundIPv4)
										{
											//Save the IP address
											list<pair<unsigned int,string> > list_addresses;
											if(portsAddress.count(theName) != 0)
													list_addresses = portsAddress[theName];
											list_addresses.push_back(make_pair<unsigned int,string>(nf_port,address));
											portsAddress[theName] = list_addresses;
										
											list<pair<unsigned int,string> > list_masks;
											if(portsMask.count(theName) != 0)
													list_masks = portsMask[theName];
											list_masks.push_back(make_pair<unsigned int,string>(nf_port,netmask));
											portsMask[theName] = list_masks;
										}
									}
								}
								else
								{
									logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\" in a VNF of \"%s\"",nf_name.c_str(),VNFS);
									return false;
								}
							}
							if(
#ifdef POLITO_MESSAGE							
							!foundTemplate || 
#endif							
							!foundID)
							{
								logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\", or key \"%s\", or both not found in an elmenet of \"%s\"",_ID,TEMPLATE,VNFS);
								return false;
							}
						}					

				    }//end if(fg_name == VNFS)
				    else if (fg_name == FLOW_RULES)
				    {
				    	foundFlowRules = true;
				    	const Array& flow_rules_array = fg_value.getArray();
				    	
				    	if(flow_rules_array.size() == 0)
				    	{
					    	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" without rules",FLOW_RULES);
							return false;
				    	}
				    	
				    	//Itearate on the flow rules
				    	for( unsigned int fr = 0; fr < flow_rules_array.size(); ++fr )
						{	
							//This is a rule, with a match, an action, and an ID
							Object flow_rule = flow_rules_array[fr].getObject();
							highlevel::Action *action = NULL;
							highlevel::Match match;
							string ruleID;
							uint64_t priority = 0;
					
							bool foundAction = false;
							bool foundMatch = false;
							bool foundID = false;
						
							//Parse the rule
							for(Object::const_iterator afr = flow_rule.begin(); afr != flow_rule.end(); afr++)
							{
								const string& fr_name  = afr->first;
								const Value&  fr_value = afr->second;
					
								if(fr_name == _ID)
								{
									foundID = true;
									ruleID = fr_value.getString();
								}
								else if(fr_name == PRIORITY)
								{
									if(sscanf(fr_value.getString().c_str(),"%"SCNd64,&priority) != 1)
									{
										logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",PRIORITY,value.getString().c_str());
										return false;
									}
								}
								else if(fr_name == MATCH)
								{
									foundMatch = true;
									if(!MatchParser::parseMatch(fr_value.getObject(), match,nfs_ports_found,graph))
									{
										return false;
									}
								}
								else if(fr_name == ACTION)
								{
									foundAction = true;
									Object theAction = fr_value.getObject();

									bool foundOne = false;

									for(Object::const_iterator a = theAction.begin(); a != theAction.end(); a++)
									{
										const string& a_name  = a->first;
										const Value&  a_value = a->second;
	
										if(a_name == PORT)
										{
											if(foundOne)
											{
												logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Only one between keys \"%s\", \"%s\" and \"%s\" are allowed in \"%s\"",PORT,VNF_ID,ENDPOINT_ID,ACTION);
												return false;
											}
											foundOne = true;
											action = new highlevel::ActionPort(a_value.getString());
											graph.addPort(a_value.getString());
										}
										else if(a_name == VNF_ID)
										{
											if(foundOne)
											{
												logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Only one between keys \"%s\", \"%s\" and \"%s\" are allowed in \"%s\"",PORT,VNF_ID,ENDPOINT_ID,ACTION);
												return false;
											}
											foundOne = true;
										
											logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",ACTION,VNF_ID,a_value.getString().c_str());
										
											string name = MatchParser::nfName(a_value.getString());
											unsigned int port = MatchParser::nfPort(a_value.getString());
											if(name == "" || port == 0)
											{
												logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Network function \"%s\" is not valid. It must be in the form \"name:port\"",a_value.getString().c_str());
												return false;	
											}
											action = new highlevel::ActionNetworkFunction(name, port);
										
											set<unsigned int> ports_found;
											if(nfs_ports_found.count(name) != 0)
												ports_found = nfs_ports_found[name];
											ports_found.insert(port);
											nfs_ports_found[name] = ports_found;
										}
#ifndef READ_JSON_FROM_FILE
										else if(a_name == ENDPOINT_ID)
										{
											if(foundOne)
											{
												logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Only one between keys \"%s\", \"%s\" and \"%s\" are allowed in \"%s\"",PORT,VNF_ID,ENDPOINT_ID,ACTION);
												return false;
											}
											foundOne = true;
										
											logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",ACTION,ENDPOINT_ID,a_value.getString().c_str());
										
											string graph_id = MatchParser::graphID(a_value.getString());
											unsigned int endPoint = MatchParser::graphEndPoint(a_value.getString());
											if(graph_id == "" || endPoint == 0)
											{
												logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Graph end point \"%s\" is not valid. It must be in the form \"graphID:endpoint\"",value.getString().c_str());
												return false;	
											}
											action = new highlevel::ActionEndPoint(graph_id, endPoint);
											graph.addEndPoint(graph_id,action->toString());
										}
#endif
										else
										{
											logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\" in \"%s\"",a_name.c_str(),ACTION);
											return false;
										}
									}
	
									if(!foundOne)
									{
										logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Neither Key \"%s\", nor key \"%s\" found in \"%s\"",PORT,VNF_ID,ACTION);
										return false;
									}
							
								}//end if(fr_name == ACTION)
								else
								{
									logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\" in a rule of \"%s\"",name.c_str(),FLOW_RULES);
									return false;
								}
							}
						
							if(!foundAction || !foundMatch || !foundID)
							{
								logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\", or key \"%s\", or key \"%s\", or all of them not found in an elmenet of \"%s\"",_ID,MATCH,ACTION,FLOW_RULES);
								return false;
							}
						
							highlevel::Rule rule(match,action,ruleID,priority);
							
							if(!graph.addRule(rule))
							{
								logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph has at least two rules with the same ID: %s",ruleID.c_str());
								return false;
							}
					
						}//for( unsigned int fr = 0; fr < flow_rules_array.size(); ++fr )
				    }// end  if (fg_name == FLOW_RULES)
				    else
					{
					    logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\" in \"%s\"",fg_name.c_str(),FLOW_GRAPH);
						return false;
					}
		    	}
		    	if(!foundFlowRules)
				{
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" not found in \"%s\"",FLOW_RULES,FLOW_GRAPH);
					return false;
				}
				if(!foundVNFs)
					logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" not found in \"%s\"",VNFS,FLOW_GRAPH);
		    }
		    else
		    {
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Invalid key: %s",name.c_str());
				return false;
		    }
		}
		if(!foundFlowGraph)
		{
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" not found",FLOW_GRAPH);
			return false;
		}
	}catch(exception& e)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "The content does not respect the JSON syntax: ",e.what());
		return false;
	}
    
    for(map<string,set<unsigned int> >::iterator it = nfs_ports_found.begin(); it != nfs_ports_found.end(); it++)
	{
		set<unsigned int> ports = it->second;
		assert(ports.size() != 0);
		
		for(set<unsigned int>::iterator p = ports.begin(); p != ports.end(); p++) 
		{
			if(!graph.updateNetworkFunction(it->first,*p))
			{
				if(newGraph)
					return false;
				else
				{
					//The message does not describe the current NF into the section "VNFs". However, this NF could be 
					//already part of the graph, and in this case the match/action using it is valid. On the contrary,
					//if the NF is no longer part of the graph, there is an error, and the graph cannot be updated.
					if(gm->graphContainsNF(graph.getID(),it->first))
					{
						graph.addNetworkFunction(it->first);
						graph.updateNetworkFunction(it->first,*p);
					}
					else
						return false;
				}
			}
		}
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "NF \"%s\" requires ports:",it->first.c_str());
		for(set<unsigned int>::iterator p = ports.begin(); p != ports.end(); p++)
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "\t%d",*p);
	}
	
	
	//Save the mac addresses
	map<string,list<pair<unsigned int,string> > >::iterator macAddr = portsMacAddress.begin();
	 
	for(; macAddr != portsMacAddress.end(); macAddr++)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "NF \"%s\" requires the following parameters:",macAddr->first.c_str());
	
		list<pair<unsigned int,string> > addresses = macAddr->second;

		list<pair<unsigned int,string> >::iterator a = addresses.begin();
		for(; a != addresses.end(); a++) 
		{
			
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "\tPort %d - mac address: %s",a->first,a->second.c_str());
		
			if(!graph.updateNetworkFunctionEthernetPortsRequirements(macAddr->first,a->first,a->second))
				return false;
		}
	}
	
	//Save the IPv4 addresses
	assert(portsAddress.size() == portsMask.size());
	
	map<string,list<pair<unsigned int,string> > >::iterator addr = portsAddress.begin();
	map<string,list<pair<unsigned int,string > > >::iterator mask = portsMask.begin();
	 
	for(; addr != portsAddress.end(); addr++, mask++)
	{
		assert(addr->first == mask->first);
	
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "NF \"%s\" requires the following parameters for ports:",addr->first.c_str());
	
		list<pair<unsigned int,string> > addresses = addr->second;
		list<pair<unsigned int,string> > netmasks = mask->second;

		assert((addresses.size() != 0) && (addresses.size() == netmasks.size()));
		
		list<pair<unsigned int,string> >::iterator a = addresses.begin();
		list<pair<unsigned int,string> >::iterator m = netmasks.begin();

		for(; a != addresses.end(); a++, m++) 
		{
			assert(a->first == m->first);
			
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "\tPort %d - address: %s - netmask: %s",a->first,a->second.c_str(),m->second.c_str());
		
			if(!graph.updateNetworkFunctionIPv4PortsRequirements(addr->first,a->first,a->second,m->second))
				return false;
		}
	}
	
    return true;
}

#ifndef READ_JSON_FROM_FILE
int RestServer::doGet(struct MHD_Connection *connection, const char *url)
{
	struct MHD_Response *response;
	int ret;
	
	bool request = false; //false->graph - true->interfaces
	
	//Check the URL
	char delimiter[] = "/";
 	char * pnt;

	char graphID[BUFFER_SIZE];
	char tmp[BUFFER_SIZE];
	strcpy(tmp,url);
	pnt=strtok(tmp, delimiter);
	int i = 0;
	while( pnt!= NULL ) 
	{
		switch(i)
		{
			case 0:
				if(strcmp(pnt,BASE_URL_GRAPH) == 0)
					request = false;
				else if(strcmp(pnt,BASE_URL_IFACES) == 0)
					request = true;
				else
				{
get_malformed_url:
					logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource \"%s\" does not exist", url);
					response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
					ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
					MHD_destroy_response (response);
					return ret;
				}
				break;
			case 1:
				strcpy(graphID,pnt);
		}
		
		pnt = strtok( NULL, delimiter );
		i++;
	}
	if( (!request && i != 2) || (request && i != 1) )
	{
		//the URL is malformed
		goto get_malformed_url; 
	}
	
	if(MHD_lookup_connection_value (connection,MHD_HEADER_KIND, "Host") == NULL)
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\"Host\" header not present in the request");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
		MHD_destroy_response (response);
		return ret;
	}
	
	if(!request)
		//request for a graph description
		return doGetGraph(connection,graphID);
	else
		//request for interfaces description
		return doGetInterfaces(connection);
}

int RestServer::doGetGraph(struct MHD_Connection *connection,char *graphID)
{
	struct MHD_Response *response;
	int ret;
	
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Required resource: %s",graphID);
	
	if(!gm->graphExists(graphID))
	{	
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Method GET is not supported for this resource");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header (response, "Allow", PUT);
		ret = MHD_queue_response (connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
		MHD_destroy_response (response);
		return ret;
	}
	
	try
	{
		Object json = gm->toJSON(graphID);
		stringstream ssj;
 		write_formatted(json, ssj );
 		string sssj = ssj.str();
 		char *aux = (char*)malloc(sizeof(char) * (sssj.length()+1));
 		strcpy(aux,sssj.c_str());
		response = MHD_create_response_from_buffer (strlen(aux),(void*) aux, MHD_RESPMEM_PERSISTENT);		
		MHD_add_response_header (response, "Content-Type",JSON_C_TYPE);
		MHD_add_response_header (response, "Cache-Control",NO_CACHE);
		ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
		MHD_destroy_response (response);
		return ret;
	}catch(...)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while retrieving the graph description!");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
		MHD_destroy_response (response);
		return ret;
	}
}

int RestServer::doGetInterfaces(struct MHD_Connection *connection)
{
	struct MHD_Response *response;
	int ret;
	
	try
	{
		Object json = gm->toJSONPhysicalInterfaces();
		stringstream ssj;
 		write_formatted(json, ssj );
 		string sssj = ssj.str();
 		char *aux = (char*)malloc(sizeof(char) * (sssj.length()+1));
 		strcpy(aux,sssj.c_str());
		response = MHD_create_response_from_buffer (strlen(aux),(void*) aux, MHD_RESPMEM_PERSISTENT);		
		MHD_add_response_header (response, "Content-Type",JSON_C_TYPE);
		MHD_add_response_header (response, "Cache-Control",NO_CACHE);
		ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
		MHD_destroy_response (response);
		return ret;
	}catch(...)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while retrieving the description of the physical interfaces!");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
		MHD_destroy_response (response);
		return ret;
	}
}

int RestServer::doDelete(struct MHD_Connection *connection, const char *url, void **con_cls)
{
	struct MHD_Response *response;
	int ret;
	
	//Check the URL
	char delimiter[] = "/";
 	char * pnt;

	char graphID[BUFFER_SIZE];
	char flowID[BUFFER_SIZE];
	bool specificFlow = false;
	
	char tmp[BUFFER_SIZE];
	strcpy(tmp,url);
	pnt=strtok(tmp, delimiter);
	int i = 0;
	while( pnt!= NULL ) 
	{
		switch(i)
		{
			case 0:
				if(strcmp(pnt,BASE_URL_GRAPH) != 0)
				{
delete_malformed_url:
					logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource \"%s\" does not exist", url);
					response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
					ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
					MHD_destroy_response (response);
					return ret;
				}
				break;
			case 1:
				strcpy(graphID,pnt);
				break;
			case 2:
				strcpy(flowID,pnt);
				specificFlow = true;
		}
		
		pnt = strtok( NULL, delimiter );
		i++;
	}
	if((i != 2) && (i != 3))
	{
		//the URL is malformed
		goto delete_malformed_url;
	}
	
	if(MHD_lookup_connection_value (connection,MHD_HEADER_KIND, "Host") == NULL)
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\"Host\" header not present in the request");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
		MHD_destroy_response (response);
		return ret;
	}
	
	struct connection_info_struct *con_info = (struct connection_info_struct *)(*con_cls);
	assert(con_info != NULL);
	if(con_info->length != 0)
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "DELETE with body is not allowed");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
		MHD_destroy_response (response);
		return ret;
	}
	
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Deleting resource: %s/%s",graphID,(specificFlow)?flowID:"");

	if(!gm->graphExists(graphID) || (specificFlow && !gm->flowExists(graphID,flowID)))
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Method DELETE is not supported for this resource");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header (response, "Allow", PUT);
		ret = MHD_queue_response (connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
		MHD_destroy_response (response);
		return ret;
	}
	
	try
	{	
		if(!specificFlow)
		{
			//The entire graph must be deleted
			if(!gm->deleteGraph(graphID))
			{
				response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
				int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
				MHD_destroy_response (response);
				return ret;
			}
			else 
				logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph has been properly deleted!");
				logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "");
		}
		else
		{
			//A specific flow must be deleted
			if(!gm->deleteFlow(graphID,flowID))
			{
				response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
				int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
				MHD_destroy_response (response);
				return ret;
			}
			else
				logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The flow has been properly deleted!");
		}
		
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);		
		ret = MHD_queue_response (connection, MHD_HTTP_NO_CONTENT, response);
		MHD_destroy_response (response);
		return ret;
	}catch(...)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred during the destruction of the graph!");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
		MHD_destroy_response (response);
		return ret;
	}	
}
#endif

