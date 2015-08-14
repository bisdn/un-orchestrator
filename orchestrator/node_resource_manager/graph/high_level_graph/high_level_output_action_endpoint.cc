#include "high_level_output_action_endpoint.h"

namespace highlevel
{

ActionEndPoint::ActionEndPoint(string graphID, unsigned int endpoint) :
	Action(ACTION_ON_ENDPOINT), graphID(graphID), endpoint(endpoint)
{
}	

bool ActionEndPoint::operator==(const ActionEndPoint &other) const
{
	if((graphID == other.graphID) && (endpoint == other.endpoint))
		return true;
		
	return false;
}

string ActionEndPoint::getInfo()
{
	return graphID;
}

unsigned int ActionEndPoint::getPort()
{
	return endpoint;
}

string ActionEndPoint::toString()
{
	stringstream ss;
	ss << graphID << ":" << endpoint;
	
	return ss.str();
}

void ActionEndPoint::print()
{
	if(LOGGING_LEVEL <= ORCH_DEBUG_INFO)
	{
		cout << "\t\tAction:" << endl << "\t\t{" << endl;
		cout << "\t\t\tendpoint: " << graphID << ":" << endpoint << endl;
		for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
			(*ga)->print();
		cout << "\t\t}" << endl;
	}
}

Object ActionEndPoint::toJSON()
{
	Object action;
	stringstream ep;
	ep << graphID << ":" << endpoint;
	action[ENDPOINT_ID] = ep.str().c_str();
	
	for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
		(*ga)->toJSON(action);
	
	return action;
}

}
