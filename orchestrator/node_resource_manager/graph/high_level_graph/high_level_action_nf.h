#ifndef HIGH_LEVEL_ACTION_NF_H_
#define HIGH_LEVEL_ACTION_NF_H_ 1

#include "high_level_action.h"
#include "../../../utils/logger.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace highlevel
{

class ActionNetworkFunction : public Action
{
private:
	/**
	*	@brief: the name of the NF (e.g., firewall)
	*/
	string nf;
	
	/**
	*	@brief: the port of the NF (e.g., 1)
	*/
	unsigned int port;
	
public:

	ActionNetworkFunction(string nf, unsigned int port = 1);
	string getInfo();
	unsigned int getPort();
	string toString();
	
	bool operator==(const ActionNetworkFunction &other) const;
	
	void print();
	Object toJSON();
};

}

#endif //HIGH_LEVEL_ACTION_NF_H_
