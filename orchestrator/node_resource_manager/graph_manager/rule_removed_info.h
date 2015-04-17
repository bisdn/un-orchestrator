#ifndef RULE_REMOVED_INFO_H_
#define RULE_REMOVED_INFO_H_ 1

#pragma once

/**
*	@brief: class containing information on NFs and virtual links to be removed from the graph
*/

class RuleRemovedInfo
{
public:	
	/**
	*	This list contains no more than two NFs.
	*/
	list<string> nfs;
	
	/**
	*	This list contains no more than two ports.
	*/
	list<string> ports;
	
	/**
	* If the action to be removed contained a NF port, it is inserted here
	*/
	string nf_port;
	bool isNFport;
	
	/**
	*	If the action to be removed contained a port, it is inserted here
	*/
	string port;
	bool isPort;
	
	/**
	*	If the action to be removed contained an endpoint, it is inserted here
	*/
	string endpoint;
	bool isEndpoint;
};

#endif //RULE_REMOVED_INFO
