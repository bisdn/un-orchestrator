#include "vlan_action.h"

VlanAction::VlanAction(vlan_action_t type, uint16_t label):
	GenericAction(), type(type), label(label)
{

}

VlanAction::~VlanAction() 
{

}
