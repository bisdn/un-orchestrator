#ifndef HIGH_LEVEL_MATCH_H_
#define HIGH_LEVEL_MATCH_H_ 1

#pragma once

#include "../../utils/logger.h"
#include "../match.h"
#include <string>
#include <iostream>

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

using namespace std;
using namespace json_spirit;

namespace highlevel
{

class Match :  public graph::Match
{

private:

	/**
	*	@brief: PORT		The match is expressed on a port (and potentially
	*						on other parameters)
	*	@brief: NF			The match is expressed on a NF (and potentially
	*						on other parameters)
	*	@brief: ENDPOINT	The match is expressed on a graph end point, that is
	*						a sort of virtual ports to be used to connect many
	*						garphs together (and potentially on other parameters)
	*	@brief: GENERIC		The match is neither expressed on a port, nor on
	*						a network function (but potentially on other 
	*						parameters)
	*/
	typedef enum{MATCH_GENERIC,MATCH_PORT,MATCH_NF, MATCH_ENDPOINT}match_t;

	/**
	*	@brief: this attribute can either represent
	*		a physical port (e.g. eth0), or a port of
	*		a NF. In the second case, only the NF is
	*		specified (e.g., NF).
	*		It can also represent a graph end point, in
	*		particular it indicates the graph ID.
	*/
	char *input;
	
	/**
	*	@brief: in case the previous attribute is a NF, this 
	*		variable represents a port of that NF (e.g., 1).
	*		The combination of this variable and the previous
	*		one identifies a NF port (e.g., NF:1, NF:2)
	*/
	int nf_port;
	
	/**
	*	@brief: in case the input attribute is a graph ID, this
	*		variable represents the endpoint idenfier within the
	*		graph.
	*/
	unsigned int endpoint;
	
	/**
	*	@brief: type of the match
	*/
	match_t type;

public:
	Match();
	
	/**
	*	@brief: set a physical port
	*
	*	@param: input_port	physical port (e.g., eth0)
	*/
	bool setInputPort(string input_port);
	
	/**
	*	@brief: set a NF port
	*
	*	@param: network_function	the name of a NF (e.g., firewall)
	*	@param: port				the port of the NF (e.g., 1)
	*/
	bool setNFport(string network_function, int port);
	
	/**
	*	@brief: set a graph endpoint
	*
	*	@param: graphID		identifier of the graph defining the endpoint
	*	@param: endpoint	identifier of the endpoint within the graph
	*/
	bool setEndPoint(string graphID, unsigned int endpoint);
	
	/**
	*	@brief: return true if the match is on a physical port
	*/
	bool matchOnPort();
	
	/**
	*	@brief: return true if the match is on a NF port 
	*/
	bool matchOnNF();
	
	/**
	*	@brief: return true if the match is on a graph endpoint 
	*/
	bool matchOnEndPoint();
	
	/**
	*	@brief: return the physical port (the match must be on a physical port)
	*/
	string getPhysicalPort();
	
	/**
	*	@brief: get the NF name (the match must be on a NF port)
	*/
	string getNF();
	
	/**
	*	@brief: get the port of a NF (the match must be on a NF port)
	*/
	int getPortOfNF();
	
	/**
	*	@brief: get the graph ID of the graph defining the endpoint (the match must be on a graph endpoint)
	*/
	string getGraphID();
	
	/**
	*	@brief: get the identifier of the endpoint within the graph defining it (the match must be on a graph endpoint)
	*/
	unsigned int getEndPoint();
	
	/**
	*	@brief: print the match in a json like style
	*/
	void print();
	
	/**
	*	@brief: transform the match into a json
	*/
	Object toJSON();
};

}
#endif //HIGH_LEVEL_MATCH_NF_H_
