#include "high_level_action_nf.h"

namespace highlevel
{

ActionNetworkFunction::ActionNetworkFunction(string nf, unsigned int port) :
	Action(ACTION_ON_NETWORK_FUNCTION), nf(nf), port(port)
{

}	

bool ActionNetworkFunction::operator==(const ActionNetworkFunction &other) const
{
	if((nf == other.nf) && (port == other.port))
		return true;
		
	return false;
}

string ActionNetworkFunction::getInfo()
{
	return nf;
}

unsigned int ActionNetworkFunction::getPort()
{
	return port;
}

string ActionNetworkFunction::toString()
{
	stringstream ss;
	ss << nf << ":" << port;
	
	return ss.str();
}

void ActionNetworkFunction::print()
{
	if(LOGGING_LEVEL <= ORCH_DEBUG_INFO)
	{
		cout << "\t\tAction:" << endl << "\t\t{" << endl;
		cout << "\t\t\tNF port: " <<nf << ":" << port << endl;
		cout << "\t\t}" << endl;
	}
}

Object ActionNetworkFunction::toJSON()
{
	Object action;
	stringstream network_function;
	network_function << nf << ":" << port;
	action[VNF_ID] = network_function.str().c_str();
	
	return action;
}

}
