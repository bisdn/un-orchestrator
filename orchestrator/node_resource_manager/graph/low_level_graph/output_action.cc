#include "output_action.h"

namespace lowlevel
{

Action::Action(uint32_t port_id)
	: type(openflow::OFPAT_OUTPUT), port_id(port_id)
{

}

bool Action::operator==(const Action &other) const
{
	if((type == other.type) && (port_id == other.port_id))
		return true;
		
	return false;
}
	
openflow::ofp_action_type Action::getActionType()
{
	return type;
}

void Action::fillFlowmodMessage(rofl::openflow::cofflowmod &message)
{
	switch(OFP_VERSION)
	{
		case OFP_10:
			message.set_actions().add_action_output(cindex(0)).set_port_no(port_id);
			break;
		case OFP_12:
		case OFP_13:
			message.set_instructions().set_inst_apply_actions().set_actions().add_action_output(cindex(0)).set_port_no(port_id);
			break;	
	}
	
	unsigned int position = 1;
	for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
		(*ga)->fillFlowmodMessage(message,&position);
}

void Action::print()
{
	if(LOGGING_LEVEL <= ORCH_DEBUG_INFO)
	{
		cout << "\t\tAction:" << endl << "\t\t{" << endl;
		cout << "\t\t\tOUTPUT: " << port_id << endl;
		for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
			(*ga)->print();		
		cout << "\t\t}" << endl;
	}
}

string Action::prettyPrint(LSI *lsi0,map<string,LSI *> lsis)
{
	stringstream ss;

	ss << "output to ";

	map<string,unsigned int> pysicalPorts = lsi0->getPhysicalPorts();
	for(map<string,unsigned int>::iterator it = pysicalPorts.begin(); it != pysicalPorts.end(); it++)
	{
		if(it->second == port_id)
		{
			ss << it->first;
			return ss.str();
		}		
	}
	
	//The port corresponds to a virtual link... we search the corresponding graph
	
	for(map<string,LSI *>::iterator it = lsis.begin(); it != lsis.end(); it++)
	{
		vector<VLink> vlinks = it->second->getVirtualLinks();
		for(vector<VLink>::iterator vl = vlinks.begin(); vl != vlinks.end(); vl++)
		{
			if(vl->getRemoteID() == port_id)
			{
				ss << port_id << " (graph: " << it->first << ")";
				goto conclude;
			}
		}
	}
	
	//The code could be here only when a SIGINT is received and all the graph are going to be removed
	ss << port_id << " (unknown graph)";

conclude:
	
	for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
		ss << (*ga)->prettyPrint();
	
	return ss.str();
}

void Action::addGenericAction(GenericAction *ga)
{
	genericActions.push_back(ga);
}

}
