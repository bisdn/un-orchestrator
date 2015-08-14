#include "action.h"

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
	
			//The next row adds a vlan pop		
//			message.set_instructions().set_inst_apply_actions().set_actions().add_action_pop_vlan(rofl::cindex(1));
			
			//The next rows add a vlan push
//			message.set_instructions().set_inst_apply_actions().set_actions().add_action_push_vlan(rofl::cindex(1)).set_eth_type(rofl::fvlanframe::VLAN_CTAG_ETHER);
//			message.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(rofl::cindex(2)).set_oxm(rofl::openflow::coxmatch_ofb_vlan_vid(25 | rofl::openflow::OFPVID_PRESENT));
			break;
	}
}

void Action::print()
{
	if(LOGGING_LEVEL <= ORCH_DEBUG_INFO)
	{
		cout << "\t\tAction:" << endl << "\t\t{" << endl;
		cout << "\t\t\tOUTPUT: " << port_id << endl;
		cout << "\t\t}" << endl;
	}
}

string Action::prettyPrint(LSI *lsi0,map<string,LSI *> lsis)
{
	stringstream ss;

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
				return ss.str();	
			}
		}
	}
	
	//The code could be here only when a SIGINT is received and all the graph are going to be removed
	ss << port_id << " (unknown graph)";
	return ss.str();
}

}
