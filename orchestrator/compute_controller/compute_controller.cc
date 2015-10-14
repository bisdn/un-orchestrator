#include "compute_controller.h"

pthread_mutex_t ComputeController::nfs_manager_mutex = PTHREAD_MUTEX_INITIALIZER;

map<int,uint64_t> ComputeController::cores;
int ComputeController::nextCore = 0;

ComputeController::ComputeController()
{
}

ComputeController::~ComputeController()
{
}

void ComputeController::setCoreMask(uint64_t core_mask)
{
	uint64_t mask = 1;
	for(uint64_t i = 0; i < 64; i++)
	{
		if(core_mask & mask)
		{
			cores[nextCore] = mask;
			nextCore++;
		}
			
		mask = mask << 1;
	}
	nextCore = 0;
	
	for(unsigned int i = 0; i < cores.size(); i++)
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Mask of an available core: \"%d\"",cores[i]);
}

nf_manager_ret_t ComputeController::retrieveDescription(string nf)
{
	try
 	{
 		string translation;

 		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Considering the NF \"%s\"",nf.c_str());

		char ErrBuf[BUFFER_SIZE];
		struct addrinfo Hints;
		struct addrinfo *AddrInfo;
		int socket;							// keeps the socket ID for this connection
		int WrittenBytes;					// Number of bytes written on the socket
		int ReadBytes;						// Number of bytes received from the socket
		char DataBuffer[DATA_BUFFER_SIZE];	// Buffer containing data received from the socket

		memset(&Hints, 0, sizeof(struct addrinfo));

		Hints.ai_family= AF_INET;
		Hints.ai_socktype= SOCK_STREAM;

		if (sock_initaddress (DATABASE_ADDRESS, DATABASE_PORT, &Hints, &AddrInfo, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error resolving given address/port (%s/%s): %s",  DATABASE_ADDRESS, DATABASE_PORT, ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		stringstream tmp;
		tmp << "GET " << DATABASE_BASE_URL << nf << " HTTP/1.1\r\n";
		tmp << "Host: :" << DATABASE_ADDRESS << ":" << DATABASE_PORT << "\r\n";
		tmp << "Connection: close\r\n";
		tmp << "Accept: */*\r\n\r\n";
		string message = tmp.str();

		char command[message.size()+1];
		command[message.size()]=0;
		memcpy(command,message.c_str(),message.size());

		if ( (socket= sock_open(AddrInfo, 0, 0,  ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
		{
			// AddrInfo is no longer required
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot contact the name resolver: %s", ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		WrittenBytes = sock_send(socket, command, strlen(command), ErrBuf, sizeof(ErrBuf));
		if (WrittenBytes == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
			return NFManager_SERVER_ERROR;

		}

		ReadBytes= sock_recv(socket, DataBuffer, sizeof(DataBuffer), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
		if (ReadBytes == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		// Terminate buffer, just for printing purposes
		// Warning: this can originate a buffer overflow
		DataBuffer[ReadBytes]= 0;

		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Data received: ");
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",DataBuffer);

		shutdown(socket,SHUT_WR);
		sock_close(socket,ErrBuf,sizeof(ErrBuf));

		if(strncmp(&DataBuffer[CODE_POSITION],CODE_METHOD_NOT_ALLLOWED,3) == 0)
			return NFManager_NO_NF;

		if(strncmp(&DataBuffer[CODE_POSITION],CODE_OK,3) != 0)
			return NFManager_SERVER_ERROR;

		//the HTTP headers must be removed
		int i = 0;
		for(; i < ReadBytes; i++)
		{
			if((i+4) <= ReadBytes)
			{
				if((DataBuffer[i] == '\r') && (DataBuffer[i+1] == '\n') && (DataBuffer[i+2] == '\r') && (DataBuffer[i+3] == '\n'))
				{
					i += 4;
					break;
				}
			}
		}

		translation.assign(&DataBuffer[i]);

		if(!parseAnswer(translation,nf))
		{
			//ERROR IN THE SERVER
			return NFManager_SERVER_ERROR;
		}
 	}
	catch (std::exception& e)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Exception: %s",e.what());
		return NFManager_SERVER_ERROR;
	}

	return NFManager_OK;
}

bool ComputeController::parseAnswer(string answer, string nf)
{
	try
	{
		Value value;
		read(answer, value);
		Object obj = value.getObject();

		bool foundName = false;
		bool foundImplementations = false;

		list<Implementation*> possibleImplementations;
		string nf_name;
#ifdef UNIFY_NFFG
		unsigned int numports = 0;
		string description;
		
		bool foundNports = false, foundDescription = false;
#endif

		for( Object::const_iterator i = obj.begin(); i != obj.end(); ++i )
		{
	 	    const string& name  = i->first;
		    const Value&  value = i->second;

		    if(name == "name")
		    {
		    	foundName = true;
		    	nf_name = value.getString();
		    	if(nf_name != nf)
		    	{
			    	logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Required NF \"%s\", received info for NF \"%s\"",nf.c_str(),nf_name.c_str());
					return false;
		    	}
		    }
		    else if(name == "nports")
		    {
#ifdef UNIFY_NFFG
				foundNports = true;
		    	numports = value.getInt();
#endif
		    }
		    else if(name == "description")
		    {
#ifdef UNIFY_NFFG
				foundDescription = true;
		    	description = value.getString();
#endif
		    }
		    else if(name == "implementations")
		    {
		    	foundImplementations = true;
		    	const Array& impl_array = value.getArray();
		    	if(impl_array.size() == 0)
		    	{
			    	logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Key \"implementations\" without descriptions");
					return false;
		    	}

		    	//Itearate on the implementations
		    	for( unsigned int impl = 0; impl < impl_array.size(); ++impl)
				{
					//This is an implementation, with a type and an URI
					Object implementation = impl_array[impl].getObject();
					bool foundURI = false;
					bool foundType = false;
					bool foundCores = false;
					bool foundLocation = false;

					string type;
			    	string uri;
					string cores;
					string location;

					for( Object::const_iterator im = implementation.begin(); im != implementation.end(); ++im )
					{
				 	    const string& impl_name  = im->first;
						const Value&  impl_value = im->second;

						if(impl_name == "type")
						{
							foundType = true;
							type = impl_value.getString();
							if(!NFType::isValid(type))
							{
								logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Invalid implementation type \"%s\"",type.c_str());
								return false;
							}
						}
						else if(impl_name == "uri")
						{
							foundURI = true;
							uri = impl_value.getString();
						}
						else if(impl_name == "cores")
						{
							foundCores = true;
							cores = impl_value.getString();
						}
						else if(impl_name == "location")
						{
							foundLocation = true;
							location = impl_value.getString();
						}
						else
						{
							logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\" within an implementation",impl_name.c_str());
							return false;
						}
					}
					if(!foundURI || !foundType)
					{
						logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Key \"uri\", key \"type\", or both are not found into an implementation description");
						return false;
					}
					if(type == "dpdk")
					{
						if(!foundCores || !foundLocation)
						{
							logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Description of a NF of type \"%s\" received without the \"cores\" attribute, \"location\" attribute, or both",type.c_str());
							return false;
						}
					}
					else if(foundCores || foundLocation)
					{
						logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Description of a NF of type \"%s\" received with a wrong attribute (\"cores\", \"location\", or both)",type.c_str());
						return false;
					}

					possibleImplementations.push_back(new Implementation(type,uri,cores,location));
				}
		    } //end if(name == "implementations")
		    else
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\"",name.c_str());
				return false;
			}
		}//end iteration on the answer

		if(!foundName || !foundImplementations
#ifdef UNIFY_NFFG
			|| !foundNports || !foundDescription
#endif		
		)
		{
#ifdef UNIFY_NFFG
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Key \"name\", and/or key \"implementations\", and/or key \"num-ports\", and/or key \"description\" not found in the answer");
#else
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Key \"name\", or key \"implementations\", or both not found in the answer");
#endif
			return false;
		}

		NF *new_nf = new NF(nf_name
#ifdef UNIFY_NFFG		
			,numports, description
#endif	
		);
		assert(possibleImplementations.size() != 0);
		for(list<Implementation*>::iterator impl = possibleImplementations.begin(); impl != possibleImplementations.end(); impl++)
			new_nf->addImplementation(*impl);

		nfs[nf_name] = new_nf;

	}catch(...)
	{
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The content does not respect the JSON syntax");
		return false;
	}

	return true;
}

bool ComputeController::selectImplementation()
{
	NFsManager *manager = NULL;

#ifdef ENABLE_DOCKER
	//Manage Docker execution environment

	manager = new Docker();
	if(docker->isSupported())
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Select a Docker implementation if exists.");
		selectImplementation(DOCKER);
		
		if(allSelected(false))
			return true;
	}
	else
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Docker deamon is not running (at least, it is not running with the LXC implementation).");
#endif

	//Manage DPDK execution environment

	manager = new Dpdk();
	if(manager->isSupported())
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Select DPDK implementation if exists.");
	
		selectImplementation(DPDK);
		if(allSelected(false))
			return true;
	}
	else
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "DPDK is not supported.");

#ifdef ENABLE_KVM
	////Manage QEMU/KVM execution environment through libvirt
	
	manager = new Libvirt();
	
	if(libvirt->isSupported())
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Select QEMU/KVM implementation if exists.");
		selectImplementation(KVM);
		return allSelected(true);
	}
	else
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "QEMU/KVM is not supported.");

#endif

	//[+] Add here other implementations for the execution environment

	return false;
}

void ComputeController::selectImplementation(nf_t desiredType)
{
	//Select an implmentation of the NF
	for(map<string, NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
	{
		NF *current = nf->second;
		
		//An implementation is selected only for those functions that do not have an implementation yet
		if(current->getSelectedImplementation() == NULL)
		{
			list<Implementation*> implementations = current->getAvailableImplementations();

			list<Implementation*>::iterator impl;
			for(impl = implementations.begin(); impl != implementations.end(); impl++)
			{
				if((*impl)->getType() == desiredType)
				{
					NFsManager *manager = NULL;

					//Create the proper NFsManager according to the selected type
					switch(desiredType)
					{
						case DPDK:
							manager = new Dpdk();
							break;
#ifdef ENABLE_DOCKER
						case DOCKER:
							manager = new Docker();
							break;
#endif
#ifdef ENABLE_KVM
						case KVM:
							manager = new Libvirt();
							break;
#endif
						//[+] Add here other implementations for the execution environment
						default:
							assert(0);
					}
					
					manager->setImplementation(*impl);					
					current->setSelectedImplementation(manager);
										
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s implementation has been selected for NF \"%s\".",NFType::toString(desiredType).c_str(),nf->first.c_str());
					break;
				}
			}
		}
	}
}

bool ComputeController::allSelected(bool lastCall)
{
	bool retVal = true;

	for(map<string, NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
	{
		NF *current = nf->second;
		if(current->getSelectedImplementation() == NULL)
		{
			if(lastCall)
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The NF \"%s\" cannot be run.",nf->first.c_str());
			retVal = false;
		}
	}
	return retVal;
}

#ifdef UNIFY_NFFG
unsigned int ComputeController::getNumPorts(string name)
{
	assert(nfs.count(name) != 0);

	NF *nf = nfs[name];
	unsigned int np = nf->getNumPorts();
	
	assert(np != 0);
	
	return np;
}
#endif

nf_t ComputeController::getNFType(string name)
{
	assert(nfs.count(name) != 0);

	NF *nf = nfs[name];
	NFsManager *impl = nf->getSelectedImplementation();

	assert(impl != NULL);

	return impl->getNFType();
}

void ComputeController::setLsiID(uint64_t lsiID)
{
	this->lsiID = lsiID;
}

//TODO: the number of ports is bad. I should pass the name of the ports!
//Now I'm assuming that, if the functions NF requires two ports, they are identified with
//NF:1 and NF:2, but this could not be true.
//Moreover, I'm creating the name in this way: lsiID:nfName:x, where x goes from 0 to
//the number of ports.. But xDPd could give different names to the ports!
bool ComputeController::startNF(string nf_name, unsigned int number_of_ports, map<unsigned int,pair<string,string> > ipv4PortsRequirements,map<unsigned int,string> ethPortsRequirements)
{
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Starting the NF \"%s\"",nf_name.c_str());

	if(nfs.count(nf_name) == 0)
	{
		assert(0);
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Unknown NF with name \"%s\"",nf_name.c_str());
		return false;
	}

	NF *nf = nfs[nf_name];
	
	StartNFIn sni(lsiID, nf_name, number_of_ports, ipv4PortsRequirements, ethPortsRequirements);
	
	NFsManager *nfsManager = nf->getSelectedImplementation();
	if(!nfsManager->startNF(sni))
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while starting the NF \"%s\"",nf_name.c_str());
		return false;
	}
	
	nf->setRunning(true);

	return true;
}

void ComputeController::stopAll()
{
	for(map<string, NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
		stopNF(nf->first);
}

bool ComputeController::stopNF(string nf_name)
{
	//FIXME: remove the NF from the map?
	//FIXME: if not, remove at least the selected implementation?

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Stopping the NF \"%s\"",nf_name.c_str());

	if(nfs.count(nf_name) == 0)
	{
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Unknown NF with name \"%s\"",nf_name.c_str());
		return false;
	}

	NF *nf = nfs[nf_name];
	NFsManager *nfsManager = nf->getSelectedImplementation();
	
	StopNFIn sni(lsiID,nf_name);
	
	if(!nfsManager->stopNF(sni))
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while stopping the NF \"%s\"",nf_name.c_str());
		return false;
	}
	
	nf->setRunning(false);

	return true;
}

uint64_t ComputeController::calculateCoreMask(string coresRequried)
{
	int requiredCores;
	sscanf(coresRequried.c_str(),"%d",&requiredCores);

	pthread_mutex_lock(&nfs_manager_mutex);
	uint64_t mask = 0;
	for(int i = 0; i < requiredCores; i++)
	{
		mask |= cores[nextCore];
		nextCore = (nextCore+1)%cores.size();
	}
	pthread_mutex_unlock(&nfs_manager_mutex);

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "The NF requires %d cores. Its core mask is  \"%x\"",requiredCores,mask);

	return mask;
}

unsigned int ComputeController::convertNetmask(string netmask)
{
	unsigned int slash = 0;
	unsigned int mask;
	
	int first, second, third, fourth;
	sscanf(netmask.c_str(),"%d.%d.%d.%d",&first,&second,&third,&fourth);
	mask = (first << 24) + (second << 16) + (third << 8) + fourth;
	
	for(int i = 0; i < 32; i++)
	{
		if((mask & 0x1) == 1)
			slash++;
		mask = mask >> 1;
	}
	
	return slash;
}

void ComputeController::printInfo(int graph_id)
{
	for(map<string, NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
	{
		nf_t type = nf->second->getSelectedImplementation()->getNFType();
		string str = NFType::toString(type);
		if(graph_id == 2)
			coloredLogger(ANSI_COLOR_BLUE,ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\t\tName: '%s'%s\t-\tType: %s\t-\tStatus: %s",nf->first.c_str(),(nf->first.length()<=7)? "\t" : "", str.c_str(),(nf->second->getRunning())?"running":"stopped");
		else if(graph_id == 3)
			coloredLogger(ANSI_COLOR_RED,ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\t\tName: '%s'%s\t-\tType: %s\t-\tStatus: %s",nf->first.c_str(),(nf->first.length()<=7)? "\t" : "", str.c_str(),(nf->second->getRunning())?"running":"stopped");
		else
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\t\tName: '%s'%s\t-\tType: %s\t-\tStatus: %s",nf->first.c_str(),(nf->first.length()<=7)? "\t" : "", str.c_str(),(nf->second->getRunning())?"running":"stopped");
	}
}

#ifdef UNIFY_NFFG	
nf_manager_ret_t ComputeController::retrieveAllAvailableNFs()
{
	set<string> NFsNames;

	try
 	{
 		string translation;

 		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Retrieving information on all the available VNFs");

		char ErrBuf[BUFFER_SIZE];
		struct addrinfo Hints;
		struct addrinfo *AddrInfo;
		int socket;				// keeps the socket ID for this connection
		int WrittenBytes;			// Number of bytes written on the socket
		int ReadBytes;				// Number of bytes received from the socket
		char DataBuffer[DATA_BUFFER_SIZE];	// Buffer containing data received from the socket

		memset(&Hints, 0, sizeof(struct addrinfo));

		Hints.ai_family= AF_INET;
		Hints.ai_socktype= SOCK_STREAM;

		if (sock_initaddress (DATABASE_ADDRESS, DATABASE_PORT, &Hints, &AddrInfo, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error resolving given address/port (%s/%s): %s",  DATABASE_ADDRESS, DATABASE_PORT, ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		stringstream tmp;
		tmp << "GET " << DATABASE_BASE_URL << DATABASE_DIGEST_URL << " HTTP/1.1\r\n";
		tmp << "Host: :" << DATABASE_ADDRESS << ":" << DATABASE_PORT << "\r\n";
		tmp << "Connection: close\r\n";
		tmp << "Accept: */*\r\n\r\n";
		string message = tmp.str();

		char command[message.size()+1];
		command[message.size()]=0;
		memcpy(command,message.c_str(),message.size());

		if ( (socket= sock_open(AddrInfo, 0, 0,  ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
		{
			// AddrInfo is no longer required
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot contact the name resolver: %s", ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		WrittenBytes = sock_send(socket, command, strlen(command), ErrBuf, sizeof(ErrBuf));
		if (WrittenBytes == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
			return NFManager_SERVER_ERROR;

		}

		ReadBytes= sock_recv(socket, DataBuffer, sizeof(DataBuffer), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
		if (ReadBytes == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		// Terminate buffer, just for printing purposes
		// Warning: this can originate a buffer overflow
		DataBuffer[ReadBytes]= 0;

		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Data received: ");
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",DataBuffer);

		shutdown(socket,SHUT_WR);
		sock_close(socket,ErrBuf,sizeof(ErrBuf));

		if(strncmp(&DataBuffer[CODE_POSITION],CODE_METHOD_NOT_ALLLOWED,3) == 0)
			return NFManager_NO_NF;

		if(strncmp(&DataBuffer[CODE_POSITION],CODE_OK,3) != 0)
			return NFManager_SERVER_ERROR;

		//the HTTP headers must be removed
		int i = 0;
		for(; i < ReadBytes; i++)
		{
			if((i+4) <= ReadBytes)
			{
				if((DataBuffer[i] == '\r') && (DataBuffer[i+1] == '\n') && (DataBuffer[i+2] == '\r') && (DataBuffer[i+3] == '\n'))
				{
					i += 4;
					break;
				}
			}
		}

		translation.assign(&DataBuffer[i]);
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Information on NFs:");
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",translation.c_str());

		//Parse the answer		
		try
		{
			Value value;
			read(translation, value);
			Object obj = value.getObject();

			bool foundNFs = false;

			for( Object::const_iterator i = obj.begin(); i != obj.end(); ++i )
			{
		 	    const string& name  = i->first;
				const Value&  value = i->second;

				if(name == "network-functions")
				{
					foundNFs = true;
					
					const Array& names_array = value.getArray();
					if(names_array.size() == 0)
					{
						logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Key \"network-functions\" without descriptions");
						return NFManager_NO_NF;
					}

					//Itearate on the VNFs
					for( unsigned int i = 0; i < names_array.size(); ++i)
					{
						//This is an implementation, with a type and an URI
						Object nf_name = names_array[i].getObject();
						
						for( Object::const_iterator nfn = nf_name.begin(); nfn != nf_name.end(); ++nfn )
						{
							const string& the_name  = nfn->first;
							const Value&  the_value = nfn->second;
												
							if(the_name == "name")
							{
								string vnfName = the_value.getString();
								NFsNames.insert(vnfName);
							}
							else
							{
								logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\"",the_name.c_str());
								return NFManager_SERVER_ERROR;
							}
						}
					}

				}//end if(name == "network-functions")
				else
				{
					logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\"",name.c_str());
					return NFManager_SERVER_ERROR;
				}
			}
			if(!foundNFs)
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Key \"network-functions\" not found in the answer");
				return NFManager_SERVER_ERROR;
			}
		}catch(...)
		{
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The content does not respect the JSON syntax");
			return NFManager_SERVER_ERROR;
		}
 	}
	catch (std::exception& e)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Exception: %s",e.what());
		return NFManager_SERVER_ERROR;
	}

	//For each VNF, download its description
	ComputeController tmpManager;
	for(set<string>::iterator name = NFsNames.begin(); name != NFsNames.end(); name++)
	{
		nf_manager_ret_t retval = tmpManager.retrieveDescription(*name);
		if(retval !=NFManager_OK)
			return retval;
	}
	//Provide the dfescriptions to the virtualizer
	set<NF*> vnfs;
	for(map<string, NF*>::iterator it = tmpManager.nfs.begin(); it != tmpManager.nfs.end(); it++)
		vnfs.insert(it->second);

	Virtualizer::addSupportedVNFs(vnfs);

	return NFManager_OK;
}
#endif
