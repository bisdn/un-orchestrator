#include "vlan_action.h"

VlanAction::VlanAction(vlan_action_t type, uint16_t label):
	GenericAction(), type(type), label(label)
{

}

VlanAction::~VlanAction() 
{

}

void VlanAction::print()
{
	stringstream ss;
	ss << ((type == ACTION_VLAN_PUSH)? "push " : "pop");
	if(type == ACTION_VLAN_PUSH)
		ss << " " << label;
	ss << endl;

	cout << "\t\t\tvlan: " << ss.str();
}

void VlanAction::toJSON(Object &action)
{
	Object vlanAction;
	vlanAction[VLAN_OP] = ((type == ACTION_VLAN_PUSH)? "push" : "pop");
	if(type == ACTION_VLAN_PUSH)
	{
		stringstream s_label;
		s_label << label;
		vlanAction[VLAN_ID] = s_label.str();
	}
	
	action[VLAN] = vlanAction;
}

void  VlanAction::fillFlowmodMessage(rofl::openflow::cofflowmod &message, unsigned int *position)
{
	switch(OFP_VERSION)
	{
		case OFP_10:
			assert(0 && "TODO");
			//TODO
			exit(0);
			break;
		case OFP_12:
			if(type == ACTION_VLAN_PUSH)
			{
				message.set_instructions().set_inst_apply_actions().set_actions().add_action_push_vlan(rofl::cindex(*position)).set_eth_type(rofl::fvlanframe::VLAN_CTAG_ETHER);
				(*position)++;
				message.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(rofl::cindex(*position)).set_oxm(rofl::openflow::coxmatch_ofb_vlan_vid(label | rofl::openflow::OFPVID_PRESENT));
				(*position)++;
			}
			else
			{
				message.set_instructions().set_inst_apply_actions().set_actions().add_action_pop_vlan(rofl::cindex(*position));
				(*position)++;
			}
			break;
		case OFP_13:
			assert(0 && "TODO");
			//TODO
			exit(0);
			break;
	}
}

string VlanAction::prettyPrint()
{
	stringstream ss;
	ss << " # vlan: " << ((type == ACTION_VLAN_PUSH)? "push " : "pop");
	if(type == ACTION_VLAN_PUSH)
		ss << " " << label;
	return ss.str();
}

