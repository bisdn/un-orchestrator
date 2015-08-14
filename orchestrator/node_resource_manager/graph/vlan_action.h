#ifndef VLAN_ACTION_H_
#define VLAN_ACTION_H_ 1

#include "generic_action.h"

#include <inttypes.h>
#include <iostream>
#include <sstream>

#include <rofl/common/protocols/fvlanframe.h>

enum vlan_action_t {ACTION_VLAN_PUSH,ACTION_VLAN_POP};

class VlanAction : public GenericAction
{
private:
	vlan_action_t type;
	uint16_t label;
	
public:
	VlanAction(vlan_action_t type, uint16_t label = 0);
	~VlanAction();
	
	void print();
	void toJSON(Object &json);
	
	void fillFlowmodMessage(rofl::openflow::cofflowmod &message, unsigned int *position);
	
	string prettyPrint();
};


#endif //VLAN_ACTION_H_
