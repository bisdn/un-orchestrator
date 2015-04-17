#ifndef HIGH_LEVEL_RULE_H_
#define HIGH_LEVEL_RULE_H_ 1

#pragma once

#include "high_level_action.h"
#include "high_level_match.h"
#include "../../../utils/logger.h"

#include <iostream>

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

using namespace std;
using namespace json_spirit;

namespace highlevel
{

class Rule
{

private:
	/**
	*	@brief: the match of this rule
	*/
	Match match;
	
	/**
	*	@brief: the action of this rule
	*/
	Action *action;
	
	/**
	*	@brief: the priority of this rule
	*/
	uint64_t priority;

	/**
	*	@brief: identifier of the flow. Can be useful in case
	*		a flow must be removed from the graph
	*/
	string flowID;

public:
	Rule(Match match, Action *action,string flowID, uint64_t priority);
	
	void print();
	Object toJSON();

	string getFlowID();
	uint64_t getPriority();
	Match getMatch();
	Action *getAction();
	
	/**
	*	@brief: two rules are equal if they have the same flow ID
	*/
	bool operator==(const Rule &other) const;
};

}

#endif //HIHG_LEVEL_RULE_H_
