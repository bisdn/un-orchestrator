#include "dpdk.h"

bool Dpdk::isSupported()
{
	return true;
}

bool Dpdk::startNF(StartNFIn sni)
{
	uint64_t lsiID = sni.getLsiID();
	string nf_name = sni.getNfName();
	unsigned int n_ports = sni.getNumberOfPorts();
	uint64_t coreMask = sni.getCoreMask();
		
	string uri_image = implementation->getURI();	
		
	stringstream uri;
	if(implementation->getLocation() == "local")
		uri << "file://";
	uri << uri_image;

	stringstream command;
	command << PULL_AND_RUN_DPDK_NF << " " << lsiID << " " << nf_name << " " << uri.str() << " " << coreMask <<  " " << NUM_MEMORY_CHANNELS << " " << n_ports;

	for(unsigned int i = 1; i <= n_ports; i++)
		command << " " << lsiID << "_" << nf_name << "_" << i;

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"",command.str().c_str());

	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;

	if(retVal == 0)
		return false;
		
	return true;
}

bool Dpdk::stopNF(StopNFIn sni)
{
	uint64_t lsiID = sni.getLsiID();
	string nf_name = sni.getNfName();
	
	stringstream command;
		
	command << STOP_DPDK_NF << " " << lsiID << " " << nf_name;

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"",command.str().c_str());
	
	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;

	if(retVal == 0)
		return false;

	return true;

}
