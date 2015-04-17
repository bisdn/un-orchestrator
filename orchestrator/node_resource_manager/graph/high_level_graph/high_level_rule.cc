#include "high_level_rule.h"

namespace highlevel
{

Rule::Rule(Match match, Action *action,string flowID, uint64_t priority) :
	match(match), action(action), priority(priority), flowID(flowID)
{
}

bool Rule::operator==(const Rule &other) const 
{	
	if(flowID == other.flowID)
		//The two rules have the same ID
		return true;	
	
	return false;
}

string Rule::getFlowID()
{
	return flowID;
}

uint64_t Rule::getPriority()
{
	return priority;
}

Match Rule::getMatch()
{
	return match;
}

Action *Rule::getAction()
{
	return action;
}

void Rule::print()
{
	if(LOGGING_LEVEL <= ORCH_DEBUG_INFO)
	{
		cout << "\trule " << flowID << ": " << endl << "\t{" << endl;
		cout << "\t\tpriority : " << priority << endl;
		match.print();
		action->print();
		cout << "\t}" << endl;
	}
}

Object Rule::toJSON()
{
	Object rule;
	
	rule[_ID] = flowID.c_str();
	rule[PRIORITY] = priority;
	rule[MATCH] = match.toJSON();
	rule[ACTION] = action->toJSON();
		
	return rule;
}

}
