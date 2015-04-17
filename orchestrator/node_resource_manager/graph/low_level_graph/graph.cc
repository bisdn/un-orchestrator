#include "graph.h"

namespace lowlevel
{	

void Graph::addRules(list<Rule> rules)
{
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
		addRule(*r);
}
	
void Graph::addRule(Rule rule)
{
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Looking for rule: %s",rule.getID().c_str());

	rules.push_back(rule);

	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "The graph contains the following rules: ");	
	for(list<Rule>::iterator it = rules.begin(); it != rules.end(); it++)
	{
		logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\t%s",it->getID().c_str());
	}
}

Rule Graph::getRule(string ID)
{
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Looking for rule: %s",ID.c_str());
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "The graph contains the following rules: ");
	list<Rule>::iterator it  = rules.begin();
	for(; it != rules.end(); it++)
	{
		logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\t%s",it->getID().c_str());
	}

	for(list<Rule>::iterator it = rules.begin(); it != rules.end(); it++)
	{
		if(it->getID() == ID)
			return *it;
	}
	
	//The rule searched does not exist in this graph
	throw new GraphException();
}

void Graph::removeRules(list<Rule> rules)
{
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
		removeRule(*r);
}

void Graph::removeRule(Rule rule)
{
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Removing rule: %s",rule.getID().c_str());
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "The graph contains the following rules: ");	
	for(list<Rule>::iterator it = rules.begin(); it != rules.end(); it++)
	{
		logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\t%s",it->getID().c_str());
	}

	for(list<Rule>::iterator it = rules.begin(); it != rules.end(); it++)
	{
		if(*it == rule)
		{
			rules.erase(it);
			return;
		}
	}

	assert(0);
}

bool Graph::removeRuleFromID(string ID)
{
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Removing rule: %s",ID.c_str());
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "The graph contains the following rules: ");	
	for(list<Rule>::iterator it = rules.begin(); it != rules.end(); it++)
	{
		logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\t%s",it->getID().c_str());
	}
	
	bool retVal = false;
	for(list<Rule>::iterator it = rules.begin(); it != rules.end(); it++)
	{
		if(it->getID() == ID)
		{
			//the rule to remove has been found
			//check if another identical rule exists
			for(list<Rule>::iterator again = rules.begin(); again != rules.end(); again++)
			{
				if(it->getID() != again->getID())
				{
					if(*it == *again)
					{
						//another rule with the same match and action has been found
						retVal = true;
						break;
					}
				}
			}
			rules.erase(it);			
			goto ok;
		}
	}
	assert(0);

ok:
	return retVal;
}

list<Rule> Graph::getRules()
{
	return rules;
}

void Graph::print()
{
	if(LOGGING_LEVEL <= ORCH_DEBUG_INFO)
	{
		cout << "Graph :" << endl << "{" << endl;
		for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
			r->print();
		cout << "}" << endl;
	}
}

void Graph::prettyPrint(LSI *lsi0,map<string,LSI *> lsis)
{
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
			r->prettyPrint(lsi0,lsis);
}

}
