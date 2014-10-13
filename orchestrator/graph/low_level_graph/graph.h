#ifndef GRAPH_H_
#define GRAPH_H_ 1

#pragma once

#include <list>
#include <iostream>
#include "rule.h"
#include "../../utils/logger.h"

using namespace std;

namespace lowlevel
{

class Graph
{
private:
	/**
	*	List of rules to be translated into flow mod messages
	*/
	list<Rule> rules;
	
public:
	/**
	*	Add a rule to the graph
	*/
	void addRule(Rule rule);
	
	/**
	*	Return the rule with a specific ID
	*/
	Rule getRule(string ID);
	
	/**
	*	Remove a specific rule from the graph
	*/
	void removeRule(Rule rule);
	
	/**
	*	Remove  a rule starting from its ID. 
	*	Returns true if another rule with the same match and action
	*	exists in the graph
	*/
	//TODO: questo è inutile, a patto che il removeRule faccia i controlli fatti qui
	bool removeRuleFromID(string ID);
	
	/**
	*	Returns the rules in the graph
	*/
	list<Rule> getRules();
	
	/**
	*	Print the graph
	*/
	void print();
};

class GraphException: public exception
{
public:
	virtual const char* what() const throw()
	{
		return "GraphException";
	}
};

}

#endif	//GRAPH_H_

