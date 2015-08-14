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

void Action::addGenericAction(GenericAction *ga)
{
	genericActions.push_back(ga);
}

list<GenericAction*> Action::getGenericActions()
{
	return genericActions;
}

}
