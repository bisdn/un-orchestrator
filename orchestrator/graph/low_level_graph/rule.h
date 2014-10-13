#ifndef RULE_H_
#define RULE_H_ 1

#pragma once

#include "low_level_match.h"
#include "action.h"
#include "../../utils/logger.h"

#include <ostream>

//FIXME: is it correct to include this?
#include <rofl/datapath/pipeline/openflow/openflow1x/pipeline/of1x_group_table.h>

using namespace std;

namespace lowlevel
{

typedef enum{ADD_RULE,RM_RULE}commad_t;

class Rule
{
private:
	uint64_t priority;
	Match match;
	Action action;
	
	string flowID;
	
public:
	Rule(Match match, Action action, string flowID, uint64_t priority);
	
	bool operator==(const Rule &other) const;
	
	/**
	*	@brief: translate the rule into a flowmod message
	*
	*	@param: message		flowmod message
	*	@param: of_version	openflow version of the flowmod message
	*/
	void fillFlowmodMessage(rofl::openflow::cofflowmod &message, uint8_t of_version, commad_t command);

	/**
	*	@brief: return the identifier of this rule
	*/
	string getID();

	void print();
};

}

#endif //RULE_H_
