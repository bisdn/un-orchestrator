#include "high_level_action.h"

namespace highlevel
{

Action::Action(action_t type) :
	type(type)
{

}

Action::~Action() 
{

}

action_t Action::getType()
{
	return type;
}


}
