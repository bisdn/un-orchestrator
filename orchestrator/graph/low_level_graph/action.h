#ifndef ACTION_H_
#define ACTION_H_ 1

#pragma once

#include <rofl/common/crofbase.h>
#include <rofl/common/logging.h>
#include <rofl/common/openflow/openflow_common.h>
#include <rofl/common/caddress.h>

#include <ostream>

#include "../../utils/logger.h"
#include "../../utils/constants.h"
#include "../../xdpd_manager/lsi.h"

using namespace rofl;
using namespace std;

namespace lowlevel
{

class Action
{

//XXX: only the OUTPUT action is supported

private:
	openflow::ofp_action_type type;
	uint32_t port_id;
	
public:
	Action(uint32_t port_id);
	openflow::ofp_action_type getActionType();
	
	bool operator==(const Action &other) const;
	
	/**
	*	@brief: insert the action into a flowmod message
	*
	*	@param: message		flowmod message
	*	@param: of_version	openflow version of the flowmod message
	*/
	void fillFlowmodMessage(rofl::openflow::cofflowmod &message);
	
	void print();
	string prettyPrint(LSI *lsi0,map<string,LSI *> lsis);
};

}

#endif //ACTION_H_

