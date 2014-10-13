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
	message.set_instructions().set_inst_apply_actions().set_actions().add_action_output(cindex(0)).set_port_no(port_id);
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

}
