#include "xdpd_manager.h"

XDPDManager::XDPDManager(string xDPDport) :
	xDPDport(xDPDport) 
{
	char ErrBuf[BUFFER_SIZE];
	struct addrinfo Hints;
	
	memset(&Hints, 0, sizeof(struct addrinfo));
	
	Hints.ai_family= AF_INET;
	Hints.ai_socktype= SOCK_STREAM;
	
	if (sock_initaddress (XDPD_ADDRESS, xDPDport.c_str(), &Hints, &AddrInfo, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error resolving given address/port (%s/%s): %s",  XDPD_ADDRESS, xDPDport.c_str(), ErrBuf);
		throw XDPDManagerException();
	}

}

XDPDManager::~XDPDManager()
{
	sock_freeaddrinfo(AddrInfo);
}

string XDPDManager::sendMessage(string message)	
{
	char ErrBuf[BUFFER_SIZE];
	int socket;						// keeps the socket ID for this connection
	int WrittenBytes;				// Number of bytes written on the socket
	int ReadBytes;					// Number of bytes received from the socket
	char DataBuffer[DATA_BUFFER_SIZE];			// Buffer containing data received from the socket

	char *command=new char[message.size()+1];
	command[message.size()]=0;
	memcpy(command,message.c_str(),message.size());

	if ( (socket= sock_open(AddrInfo, 0, 0,  ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
	{
		// AddrInfo is no longer required
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot contact xDPd: %s", ErrBuf);
		throw XDPDManagerException();
	}

	WrittenBytes= sock_send(socket, command, strlen(command), ErrBuf, sizeof(ErrBuf));
	if (WrittenBytes == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw XDPDManagerException();

	}
	
	ReadBytes= sock_recv(socket, DataBuffer, sizeof(DataBuffer), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (ReadBytes == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw XDPDManagerException();
	}


	// Terminate buffer, just for printing purposes
	// Warning: this can originate a buffer overflow
	DataBuffer[ReadBytes]= 0;

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Data received: ");
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",DataBuffer);

	shutdown(socket,SHUT_WR);
	sock_close(socket,ErrBuf,sizeof(ErrBuf));
	
	return string(DataBuffer);
}
	
map<string,string> XDPDManager::discoverPhyPorts()
{
	//Prepare the request
	Object json;	
	json["command"] = DISCOVER_PHY_PORTS;
 	stringstream ss;
 	write_formatted(json, ss );

	string answer;
	try
	{
		answer = sendMessage(ss.str());
	}catch (...)
	{
		new XDPDManagerException();
	}
	
	//Parse the answer
	map<string,string> phyPorts;
	bool foundPorts = false;
	Value value;
    read( answer, value );
    Object obj = value.getObject();
    
    if(!findCommand(obj,string(DISCOVER_PHY_PORTS)))
		throw XDPDManagerException();    
	if(!findStatus(obj))
		throw XDPDManagerException();    

	for( Object::const_iterator i = obj.begin(); i != obj.end(); ++i )
    {
        const string& name  = i->first;
        const Value&  value = i->second;
        if( (name == "command") ||  (name == "status"))
        {
			continue;
        }
		else if(name == "ports")
		{
			const Array& ports_array = value.getArray();
			if(ports_array.size() == 0)
			    logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" with an empty port list.",DISCOVER_PHY_PORTS);

			for( unsigned int i = 0; i < ports_array.size(); ++i )
			{
				Object port = ports_array[i].getObject();
				bool foundName = false, foundType = false;
				string port_name, port_type;
				for( Object::const_iterator p = port.begin(); p != port.end(); ++p )
				{
					const string& p_name  = p->first;
					const Value&  p_value = p->second;
					if(p_name == "name")
					{
						foundName = true;
						port_name = p_value.getString();
					}
					else if(p_name == "type")
					{
						foundType = true;
						port_type = p_value.getString();
					}
					else
					{
						logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" with the unespected parameter \"%s\"",DISCOVER_PHY_PORTS,p_name.c_str());
						assert(0);
						throw XDPDManagerException();
					}
				}
				if(!foundName || !foundType)
				{
					logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" with a \"port\" without \"name\", \"type\" or both",DISCOVER_PHY_PORTS);
					assert(0);
					throw XDPDManagerException();	
				}
				phyPorts[port_name] = port_type;
			}

			foundPorts = true;
		}
		else
		{
			//error
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" with the unespected parameter \"%s\"",DISCOVER_PHY_PORTS,name.c_str());
			throw XDPDManagerException();
		}
	}

	if(!foundPorts)
	{
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" without \"ports\" received",DISCOVER_PHY_PORTS);
		throw XDPDManagerException();
	}
	return phyPorts;
}

void XDPDManager::createLsi(LSI &lsi)
{	
	Value value;

	string message = prepareCreateLSIrequest(lsi);
	string answer = sendMessage(message);
		
	read( answer, value );
	Object obj = value.getObject();

	if(!findCommand(obj,string(CREATE_LSI)) || !findStatus(obj))
		throw XDPDManagerException();
		
	try
	{
		parseCreateLSIresponse(lsi, obj);
	} catch(...) {
		throw;
	}
}

string XDPDManager::prepareCreateLSIrequest(LSI lsi)
{
	Object json;
	json["command"] = CREATE_LSI;
 	
 	Object controller;
   	controller["address"] = lsi.getControllerAddress();
	controller["port"] = lsi.getControllerPort();
	
	json["controller"] = controller;
	
	list<string> ports = lsi.getEthPortsName();
	Array ports_array;
	for(list<string>::iterator p = ports.begin(); p != ports.end(); p++)
		ports_array.push_back(*p);	
	if(ports.size() > 0)
		json["ports"] = ports_array;
		
	if(lsi.hasWireless())
		json["wireless"] = lsi.getWirelessPortName();

	set<string> nfs = lsi.getNetworkFunctionsName();
	map<string,nf_t> nf_type = lsi.getNetworkFunctionsType(); 	
	Array nfs_array;
	for(set<string>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
	{
		Object network_function;
		network_function["name"] = *nf;
		
		assert(nf_type.count(*nf) != 0);
		nf_t nft = nf_type[*nf];

		//XXX: this is a trick since xDPd does not support kvm ports
		if(nft == KVM)
			nft = DOCKER;
			
		network_function["type"] = NFType::toString(nft);
		
		Array nfs_ports_array;
		list<string> nfs_ports = lsi.getNetworkFunctionsPortNames(*nf);
		for(list<string>::iterator nfp = nfs_ports.begin(); nfp != nfs_ports.end(); nfp++)
			nfs_ports_array.push_back(*nfp);
		network_function["ports"] = nfs_ports_array;
		
		nfs_array.push_back(network_function);
	}
	if(nfs.size() > 0)
		json["network-functions"] = nfs_array;
	 
	list<uint64_t> vlinks = lsi.getVirtualLinksRemoteLSI();
	//XXX: it supports only the case in which all the vlinks are shered with the same LSI
	if(vlinks.size() > 0)
	{
		list<uint64_t>::iterator vl = vlinks.begin();
		uint64_t remoteDpid = *vl;
		for(; vl != vlinks.end(); vl++)
		{
			if(*vl != remoteDpid)
				throw XDPDManagerException();
		}
		
		Object vlink;
		vlink["number"] = vlinks.size();
		vlink["remote-lsi"] = remoteDpid;
		json["virtual-links"] = vlink;
	 }
	 
 	stringstream ss;
 	write_formatted(json, ss );
 	
 	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Command to be sent: ");
 	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",ss.str().c_str());

 	return ss.str();
}

void XDPDManager::parseCreateLSIresponse(LSI &lsi, Object message)
{
	bool foundLSIid = false;
	bool foundWireless = false;
	
	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
    {
        const string& name  = i->first;
        const Value&  value = i->second;
        if( (name == "command") ||  (name == "status"))
        {
			continue;
        }
        else if(name == "lsi-id")
        {
        	foundLSIid = true;
        	lsi.setDpid(value.getInt()); //FIXME: it is not an int
        }
        else if(name == "ports")
        {
			const Array& ports_array = value.getArray();
			
			if(ports_array.size() != lsi.getEthPorts().size())
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a wrong number of physical ports",CREATE_LSI);
				throw XDPDManagerException();
			}
			
			for( unsigned int j = 0; j < ports_array.size(); ++j )
			{	
				Object port = ports_array[j].getObject();
				unsigned int id = 0;
				string name;
				bool foundID = false;
				bool foundName = false;
				for( Object::const_iterator p = port.begin(); p != port.end(); ++p )
				{
					const string& p_name  = p->first;
		    		const Value&  p_value = p->second;
		    		
		    		if(p_name == "name")
		    		{
		    			name = p_value.getString();
		    			foundName = true;
		    		}
		    		else if(p_name == "id")
		    		{
		    			id = p_value.getInt(); //FIXME: it isn't an int!
		    			foundID = true;
		    		}
		    	}
		    	if(foundName && foundID)
		    	{
		    		if(!lsi.setEthPortID(name,id))
		    		{
		    			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a non-required port \"%d\"",CREATE_LSI,name.c_str());
						throw XDPDManagerException();
		    		}
		    	}
		    	else
	    		{
	    			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a port without the name, the ID, or both",CREATE_LSI);
					throw XDPDManagerException();
	    		}
			} //end iteration on the array
        } //end name=="ports"
        else if(name == "wireless")
        {
        	foundWireless = true;
        	if(!lsi.setWirelessPortID(value.getInt()))
        	{
        		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a non-required wireless port \"%d\"",CREATE_LSI,name.c_str());
				throw XDPDManagerException();
        	}
        } //end name=="wireless"
        else if(name == "network-functions")
        {
			const Array& nfs_array = value.getArray();
			
			if(nfs_array.size() != lsi.getNetworkFunctionsName().size())
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a wrong number of network functions (expected: %d - received: %d)",CREATE_LSI,lsi.getNetworkFunctionsName().size(),nfs_array.size());
				throw XDPDManagerException();
			}
			
			for( unsigned int j = 0; j < nfs_array.size(); ++j )
			{	
				//Each element of the array is a NF		
				Object nf = nfs_array[j].getObject();
			
				string name;
				map<string,unsigned int> ports;
				
				bool foundName = false;
				bool foundPorts = false;

				for( Object::const_iterator n = nf.begin(); n != nf.end(); ++n )
				{
					const string& n_name  = n->first;
		    		const Value&  n_value = n->second;
		    		
		    		if(n_name == "name")
		    		{
		    			name = n_value.getString();
		    			foundName = true;
		    		}
		    		else if(n_name == "ports")
		    		{
						const Array &ports_array = n_value.getArray(); 
						for( unsigned int p = 0; p < ports_array.size(); ++p )
						{
							Object port = ports_array[p].getObject();
			
							bool foundPortName = false;
							bool foundPortID = false;
							string port_name;
	    					unsigned int port_id = 0;
							
							for( Object::const_iterator a_port = port.begin(); a_port != port.end(); ++a_port )
							{	
								const string& ap_name  = a_port->first;
		    					const Value&  ap_value = a_port->second;
		    					
		    					if(ap_name == "name")
		    					{
		    						foundPortName = true;
		    						port_name = ap_value.getString();
		    					}
		    					else if(ap_name == "id")
		    					{
		    						port_id = ap_value.getInt(); //FIXME: this is not an Int!
		    						foundPortID = true;
		    					} 					
							}
							if(!foundPortName || !foundPortID)
	    					{
	    						logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a network function without the name, the ports, or both",CREATE_LSI);
								throw XDPDManagerException();		    					
	    					}
							ports[port_name] = port_id;
						}
						if(ports_array.size() > 0)
			    			foundPorts = true;
		    		} //end if(n_name == "ports")
		    	}
		    	if(foundName && foundPorts)
		    	{

		    		if(!lsi.setNfPortsID(name,ports))
		    		{
		    			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a non-required network function",CREATE_LSI,name.c_str());
						throw XDPDManagerException();
		    		}
		    	}
		    	else
	    		{
	    			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a network function without the name, the ports, or both",CREATE_LSI);
	    			throw XDPDManagerException();
	    		}
			} //end iteration on the array
        }//end name==network-functions
        else if(name == "virtual-links")
        {
			const Array& vls_array = value.getArray();
			
			if(vls_array.size() != lsi.getVirtualLinks().size())
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains virtual links",CREATE_LSI);
				throw XDPDManagerException();
			}
			
			unsigned int currentTranslation = 0;
			for( unsigned int j = 0; j < vls_array.size(); ++j )
			{			
				Object vl = vls_array[j].getObject();
				unsigned int localID = 0;
				unsigned int remoteID = 0;
				bool foundLocalID = false;
				bool foundRemoteID = false;
				for( Object::const_iterator v = vl.begin(); v != vl.end(); ++v )
				{
					const string& v_name  = v->first;
		    		const Value&  v_value = v->second;
		    		
		    		if(v_name == "local-id")
		    		{
		    			localID = v_value.getInt();
		    			foundLocalID = true;
		    		}
		    		else if(v_name == "remote-id")
		    		{
		    			remoteID = v_value.getInt(); //FIXME: it isn't an int!
		    			foundRemoteID = true;
		    		}
		    	}
		    	if(foundLocalID && foundRemoteID)
		    	{
		    		lsi.setVLinkIDs(currentTranslation,localID,remoteID);
		    		currentTranslation++;
		    	}
		    	else
	    		{
	    			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a virtual link without the local ID, the remote ID, or both",CREATE_LSI);
					throw XDPDManagerException();
	    		}
			} //end iteration on the array
        }//end name=="virtual-links"
        else
        {
        	//error
		    logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" with the unespected parameter \"%s\"",CREATE_LSI,name.c_str());
			throw XDPDManagerException();
        }
	} //end parsing the message
	
	if(!foundLSIid)
	{
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" without \"lsi-id\" received",CREATE_LSI);
		throw XDPDManagerException();
	}
	
	if(lsi.hasWireless() && !foundWireless)
	{
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" without \"wireless\" received, although a wireless interface was required",CREATE_LSI);
		throw XDPDManagerException();
	}
}

bool XDPDManager::findCommand(Object message, string expected)
{
	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
    {
        const string& name  = i->first;
        const Value&  value = i->second;
        if( name == "command" )
        {
			if(value.getString() != expected)
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Expected command \"%s\" - received \"%s\"",expected.c_str(),value.getString().c_str());
				return false;	
			}
			else
				return true;
 		}
 	}
 	
 	logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Command not found in the answer");
 	return false;	
}

bool XDPDManager::findStatus(Object message)
{
	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
    {
        const string& name  = i->first;
        const Value&  value = i->second;
        if( name == "status" )
        {
			if(value.getString() != OK)
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Expected status \"%s\" - received \"%s\"",OK,value.getString().c_str());
				return false;	
			}
			else
				return true;
 		}
 	}
 	
 	logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Status not found in the answer");
 	return false;	
}

void XDPDManager::addNFPorts(LSI &lsi,pair<string, list<unsigned int> > nf, nf_t type)
{
	lsi.addNF(nf.first, nf.second);

	string answer = sendMessage(prepareCreateNFPortsRequest(lsi,type,nf.first));
	
	Value value;
    read( answer, value );
    Object obj = value.getObject();
    if(!findCommand(obj,string(CREATE_NF_PORTS)))
		throw XDPDManagerException();    
	if(!findStatus(obj))
		throw XDPDManagerException();
	try
	{
		parseCreateNFPortsResponse(lsi,obj);
	} catch(...) {
		lsi.removeNF(nf.first);
		throw;
	}
}

string XDPDManager::prepareCreateNFPortsRequest(LSI lsi, nf_t type, string name)
{
	Object json;
	json["command"] = CREATE_NF_PORTS;
 	json["lsi-id"] = lsi.getDpid();
	
	Array nfs_array;
	Object network_function;
	network_function["name"] = name;
	
	//XXX: this is a trick since xDPd does not support kvm ports
	if(type == KVM)
		type = DOCKER;
			
	network_function["type"] = NFType::toString(type);
		
	Array nfs_ports_array;
	list<string> nfs_ports = lsi.getNetworkFunctionsPortNames(name);
	for(list<string>::iterator nfp = nfs_ports.begin(); nfp != nfs_ports.end(); nfp++)
		nfs_ports_array.push_back(*nfp);
			
	network_function["ports"] = nfs_ports_array;	
	nfs_array.push_back(network_function);
	
	json["network-functions"] = nfs_array;
	 
 	stringstream ss;
 	write_formatted(json, ss );
 	
 	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Command to be sent: ");
 	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",ss.str().c_str());

 	return ss.str();
}

void XDPDManager::parseCreateNFPortsResponse(LSI &lsi, Object message)
{
	bool foundNFs = false;
	
	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
    {
        const string& name  = i->first;
        const Value&  value = i->second;
        if( (name == "command") ||  (name == "status"))
        {
			continue;
        }
        else if(name == "network-functions")
        {
        	foundNFs = true;	
        	
			const Array& nfs_array = value.getArray();
			
			if(nfs_array.size() != 1)
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a wrong number of network functions ports",CREATE_LSI);
				throw XDPDManagerException();
			}
			
			for( unsigned int j = 0; j < nfs_array.size(); ++j )
			{	
				//Each element of the array is a NF		
				Object nf = nfs_array[j].getObject();
			
				string name;
				map<string,unsigned int> ports;
				
				bool foundName = false;
				bool foundPorts = false;

				for( Object::const_iterator n = nf.begin(); n != nf.end(); ++n )
				{
					const string& n_name  = n->first;
		    		const Value&  n_value = n->second;
		    		
		    		if(n_name == "name")
		    		{
		    			name = n_value.getString();
		    			foundName = true;
		    		}
		    		else if(n_name == "ports")
		    		{
						const Array &ports_array = n_value.getArray(); 
						for( unsigned int p = 0; p < ports_array.size(); ++p )
						{
							Object port = ports_array[p].getObject();
			
							bool foundPortName = false;
							bool foundPortID = false;
							string port_name;
	    					unsigned int port_id = 0;
							
							for( Object::const_iterator a_port = port.begin(); a_port != port.end(); ++a_port )
							{	
								const string& ap_name  = a_port->first;
		    					const Value&  ap_value = a_port->second;
		    					
		    					if(ap_name == "name")
		    					{
		    						foundPortName = true;
		    						port_name = ap_value.getString();
		    					}
		    					else if(ap_name == "id")
		    					{
		    						port_id = ap_value.getInt(); //FIXME: this is not an Int!
		    						foundPortID = true;
		    					} 					
							}
							if(!foundPortName || !foundPortID)
	    					{
	    						logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a network function without the name, the ports, or both",CREATE_LSI);
								throw XDPDManagerException();		    					
	    					}
							ports[port_name] = port_id;
						}
						if(ports_array.size() > 0)
			    			foundPorts = true;
		    		} //end if(n_name == "ports")
		    	}
		    	if(foundName && foundPorts)
		    	{

		    		if(!lsi.setNfPortsID(name,ports))
		    		{
		    			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a non-required network function",CREATE_LSI,name.c_str());
						throw XDPDManagerException();
		    		}
		    	}
		    	else
	    		{
	    			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a network function without the name, the ports, or both",CREATE_NF_PORTS);
	    			throw XDPDManagerException();
	    		}
			} //end iteration on the array
        }//end name==network-functions
        else
        {
        	//error
		    logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" with the unespected parameter \"%s\"",CREATE_NF_PORTS,name.c_str());
			throw XDPDManagerException();
        }
	} //end parsing the message
	
	if(!foundNFs)
	{
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" without \"network-functions\" received",CREATE_NF_PORTS);
		throw XDPDManagerException();
	}
}

uint64_t XDPDManager::addVirtualLink(LSI &lsi, VLink vlink)
{
	string answer = sendMessage(prepareCreateVirtualLinkRequest(lsi,vlink));
	
	Value value;
    read( answer, value );
    Object obj = value.getObject();
    if(!findCommand(obj,string(CREATE_VLINKS)))
		throw XDPDManagerException();    
	if(!findStatus(obj))
		throw XDPDManagerException();
	
	int vlink_position = lsi.addVlink(vlink);	
	
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Virtual link with ID %d inserted in position %d",vlink.getID(),vlink_position);
	
	try
	{
		parseCreateVirtualLinkResponse(lsi, vlink_position, obj);
	} catch(...) {
		lsi.removeVlink(vlink.getID());
		assert(0);
		throw;
	}
	
	return vlink.getID();
}

string XDPDManager::prepareCreateVirtualLinkRequest(LSI lsi,VLink vlink)
{
	Object json;
	json["command"] = CREATE_VLINKS;
	json["number"] = 1;
	
	json["lsi-a"] = lsi.getDpid();
	json["lsi-b"] = vlink.getRemoteDpid();
	 
 	stringstream ss;
 	write_formatted(json, ss );
 	
 	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Command to be sent: ");
 	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",ss.str().c_str());

 	return ss.str();

}

void XDPDManager::parseCreateVirtualLinkResponse(LSI &lsi, int vlink_position, Object message)
{
	bool foundVlinks = false;
	
	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
    {
        const string& name  = i->first;
        const Value&  value = i->second;
        if( (name == "command") ||  (name == "status"))
        {
			continue;
        }
        else if(name == "virtual-links")
        {
        	foundVlinks = true;
			const Array& vls_array = value.getArray();
			
			if(vls_array.size() != 1)//XXX: we create a vlink at a time, althugh xDPd allows the creation of many vlinks together
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a wrong number of virtual links",CREATE_VLINKS);
				throw XDPDManagerException();
			}
			
			unsigned int currentTranslation = 0;
			for( unsigned int j = 0; j < vls_array.size(); ++j )
			{			
				Object vl = vls_array[j].getObject();
				unsigned int localID = 0;
				unsigned int remoteID = 0;
				bool foundA = false;
				bool foundB = false;
				for( Object::const_iterator v = vl.begin(); v != vl.end(); ++v )
				{
					const string& v_name  = v->first;
		    		const Value&  v_value = v->second;
		    		
		    		if(v_name == "id-a")
		    		{
		    			localID = v_value.getInt(); //FIXME: it isn't an int!
		    			foundA = true;
		    		}
		    		else if(v_name == "id-b")
		    		{
		    			remoteID = v_value.getInt(); //FIXME: it isn't an int!
		    			foundB = true;
		    		}
		    	}
		    	if(foundA && foundB)
		    	{
		    		lsi.setVLinkIDs(vlink_position,localID,remoteID);
		    		currentTranslation++;
		    	}
		    	else
	    		{
	    			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" contains a virtual link without the id-a, the id-b, or both",CREATE_VLINKS);
					throw XDPDManagerException();
	    		}
			} //end iteration on the array
        }//end name=="virtual-links"
        else
        {
        	//error
		    logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" with the unespected parameter \"%s\"",CREATE_VLINKS,name.c_str());
			throw XDPDManagerException();
        }
	} //end parsing the message
	
	if(!foundVlinks)
	{
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" without \"virtual-links\" received",CREATE_VLINKS);
		throw XDPDManagerException();
	}
}

void XDPDManager::destroyLsi(LSI &lsi)
{
	string answer = sendMessage(prepareDestroyLSIrequest(lsi));
	
	Value value;
    read( answer, value );
    Object obj = value.getObject();
    if(!findCommand(obj,string(DESTROY_LSI)))
		throw XDPDManagerException();    
	if(!findStatus(obj))
		throw XDPDManagerException();
		
	try
	{
		parseDestroyLSIresponse(lsi, obj);
	} catch(...) {
		throw;
	}
}

void XDPDManager::destroyVirtualLink(LSI &lsi, uint64_t vlinkID)
{	
	string answer = sendMessage(prepareDestroyVirtualLinkRequest(lsi,vlinkID));
	
	Value value;
    read( answer, value );
    Object obj = value.getObject();
    if(!findCommand(obj,string(DESTROY_VLINKS)))
		throw XDPDManagerException();    
	if(!findStatus(obj))
		throw XDPDManagerException();
		
	try
	{
		parseDestroyVirtualLinkResponse(obj);
		lsi.removeVlink(vlinkID);
	} catch(...) {
		throw;
	}
}

void XDPDManager::destroyNFPorts(LSI &lsi,string nf)
{
	string answer = sendMessage(prepareDestroyNFPortsRequest(lsi,nf));
	
	Value value;
    read( answer, value );
    Object obj = value.getObject();
    if(!findCommand(obj,string(DESTROY_NF_PORTS)))
		throw XDPDManagerException();    
	if(!findStatus(obj))
		throw XDPDManagerException();
		
	try
	{
		parseDestroyNFPortsResponse(lsi, obj);
		lsi.removeNF(nf);
	} catch(...) {
		throw;
	}
}

string XDPDManager::prepareDestroyLSIrequest(LSI lsi)
{
	Object json;
	json["command"] = DESTROY_LSI;
 	
 	Object controller;
   	json["lsi-id"] = lsi.getDpid();
	 
 	stringstream ss;
 	write_formatted(json, ss );
 	
 	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Command to be sent: ");
 	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",ss.str().c_str());

 	return ss.str();
}

void XDPDManager::parseDestroyLSIresponse(LSI &lsi, Object message)
{
	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
    {
        const string& name  = i->first;
        if( (name == "command") ||  (name == "status"))
        {
			continue;
        }
        else
        {
        	//error
		    logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" with the unespected parameter \"%s\"",DESTROY_LSI,name.c_str());
			throw XDPDManagerException();
        }
	} //end parsing the message
}

string XDPDManager::prepareDestroyVirtualLinkRequest(LSI lsi, uint64_t vlinkID)
{
	Object json;
	json["command"] = DESTROY_VLINKS;

	Object vlink;
	vlink["lsi-id"] = lsi.getDpid();

	vector<VLink> virtual_links = lsi.getVirtualLinks();
	vector<VLink>::iterator v = virtual_links.begin();
	for(; v != virtual_links.end(); v++)
	{
		if(v->getID() == vlinkID)
			break;
	}
	if( v == virtual_links.end())
	{
       	//error
	    logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Cannot find virtual link with ID: %d",vlinkID);
		assert(0);
		throw XDPDManagerException();
	}
	vlink["vlink-id"] = v->getLocalID();
	
	Array vlinks_array;
	vlinks_array.push_back(vlink);
	
	json["virtual-links"] = vlinks_array;
	 
 	stringstream ss;
 	write_formatted(json, ss );
 	
 	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Command to be sent: ");
 	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",ss.str().c_str());

 	return ss.str();
}

void XDPDManager::parseDestroyVirtualLinkResponse(Object message)
{
	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
    {
        const string& name  = i->first;
        if( (name == "command") ||  (name == "status"))
        {
			continue;
        }
        else
        {
        	//error
		    logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" with the unespected parameter \"%s\"",DESTROY_VLINKS,name.c_str());
			throw XDPDManagerException();
        }
	} //end parsing the message
}

string XDPDManager::prepareDestroyNFPortsRequest(LSI lsi, string nf)
{
	Object json;
	json["command"] = DESTROY_NF_PORTS;
	json["lsi-id"] = lsi.getDpid();

	Array ports_array;

	map<string,unsigned int> nf_ports = lsi.getNetworkFunctionsPorts(nf);
	map<string,unsigned int>::iterator p = nf_ports.begin();
	for(; p != nf_ports.end(); p++)
	{
		stringstream ss;
		ss << lsi.getDpid() << "_" << p->first;
		ports_array.push_back(ss.str());
	}

	if( ports_array.size() == 0)
	{
       	//error
	    logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "It seems that NF '%s' does not have any port!",nf.c_str());
		assert(0);
		throw XDPDManagerException();
	}
	json["ports"] = ports_array;
	 
 	stringstream ss;
 	write_formatted(json, ss );
 	
 	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Command to be sent: ");
 	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",ss.str().c_str());

 	return ss.str();
}

void XDPDManager::parseDestroyNFPortsResponse(LSI &lsi, Object message)
{
	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
    {
        const string& name  = i->first;
        if( (name == "command") ||  (name == "status"))
        {
			continue;
        }
        else
        {
        	//error
		    logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Answer to command \"%s\" with the unespected parameter \"%s\"",DESTROY_NF_PORTS,name.c_str());
			throw XDPDManagerException();
        }
	} //end parsing the message
}

