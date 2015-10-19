#ifndef HIGH_LEVEL_ACTION_ENDPOINT_H_
#define HIGH_LEVEL_ACTION_ENDPOINT_H_ 1

#include "high_level_output_action.h"
#include "../../../utils/logger.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace highlevel
{

class ActionEndPoint : public Action
{
private:
	/**
	*	@brief: ID of the graph defining this endpoint
	*/
	string graphID;
	
	/**
	*	@brief: endpoint identifier
	*/
	unsigned int endpoint;
	
public:

	ActionEndPoint(string graphID, unsigned int endpoint);
	string getInfo();
	unsigned int getPort();
	string toString();
	
	bool operator==(const ActionEndPoint &other) const;
	
	void print();
	Object toJSON();
};

}

#endif //HIGH_LEVEL_ACTION_ENDPOINT_H_
