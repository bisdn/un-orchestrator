#include "high_level_action_port.h"

namespace highlevel
{

ActionPort::ActionPort(string port) :
	Action(ACTION_ON_PORT), port(port)
{

}

ActionPort::~ActionPort() 
{

}

bool ActionPort::operator==(const ActionPort &other) const
{
	if(port == other.port)
		return true;
		
	return false;
}

string ActionPort::getInfo()
{
	return port;
}

string ActionPort::toString()
{
	return port;
}

void ActionPort::print()
{
	if(LOGGING_LEVEL <= ORCH_DEBUG_INFO)
	{
		cout << "\t\tAction:" << endl << "\t\t{" << endl;
		cout << "\t\t\tport: " << port << endl;
		for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
			(*ga)->print();
		cout << "\t\t}" << endl;
	}
}

Object ActionPort::toJSON()
{
	Object action;
	action[PORT] = port.c_str();
	
	for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
		(*ga)->toJSON(action);
	
	return action;
}

}
