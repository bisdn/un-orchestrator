#include "docker.h"

bool Docker::isSupported()
{
	int retVal;

	retVal = system(CHECK_DOCKER);
	retVal = retVal >> 8;

	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Script returned: %d\n",retVal);

	if(retVal > 0)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Docker deamon is running.");
		return true;
	}

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Docker deamon is not running (at least, it is not running with the LXC implementation).");
	return false;
}

bool Docker::startNF(StartNFIn sni)
{
	uint64_t lsiID = sni.getLsiID();
	string nf_name = sni.getNfName();
	unsigned int n_ports = sni.getNumberOfPorts();
	pair<string,string> > ipv4PortsRequirements = sni.getIpv4PortsRequirements();
	map<unsigned int,string> ethPortsRequirements = sni.getEthPortsRequirements();
	
	string uri_image = implementation->getURI();
	
	stringstream command;
	command << PULL_AND_RUN_DOCKER_NF << " " << lsiID << " " << nf_name << " " << impl->getURI() << " " << number_of_ports;
	
	//create the names of the ports
	for(unsigned int i = 1; i <= number_of_ports; i++)
		command << " " << lsiID << "_" << nf_name << "_" << i;
		
	//specify the IPv4 requirements for the ports
	for(unsigned int i = 1; i <= number_of_ports; i++)
	{
		if(ipv4PortsRequirements.count(i) == 0)
			command << " " << 0;
		else
		{
			pair<string, string> req = (ipv4PortsRequirements.find(i))->second;
			command << " " << req.first <<"/" << convertNetmask(req.second);
		}
	}
	//specify the ethernet requirements for the ports
	for(unsigned int i = 1; i <= number_of_ports; i++)
	{
		if(ethPortsRequirements.count(i) == 0)
			command << " " << 0;
		else
			command << " " << (ethPortsRequirements.find(i))->second;
	}

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"",command.str().c_str());

	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;
	
	if(retVal == 0)
		return false;
		
	return true;
}

bool Docker::stopNF(StopNFIn sni)
{
	uint64_t lsiID = sni.getLsiID();
	string nf_name = sni.getNfName();

	command << STOP_DOCKER_NF << " " << lsiID << " " << nf_name;
	
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"",command.str().c_str());
	retVal = system(command.str().c_str());
	retVal = retVal >> 8;

	if(retVal == 0)
		return false;

	return true;
}

