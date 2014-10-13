#ifndef GRAPH_TRANSLATOR_H_
#define GRAPH_TRANSLATOR_H_ 1

#pragma once

#include <sstream>

#include "graph_manager.h"
#include "../graph/high_level_graph/high_level_graph.h"

class GraphTranslator
{
friend class GraphManager;

	/**
	*	@Brief: this class applies some (complicated) rules to translate the highlevel description of
	*	the graph into lowlevel descriptions to be provided (as flowmod messages) respectively to the
	*	LSI-0 and to the tenant-LSI
	*/

protected:

	/**
	*	If the match is expressed on an endpoint, the action must be expressed on a NF
	*	If the action is expressed on an endpoint, the match must be expressed on a NF
	*/
	
	/**
	*	@brief: translate an high level graph into a rules to be sent to
	*		the LSI-0
	*
	*	@param: graph						High level graph to be translated
	*	@param: tenantLSI					Information related to the LSI of the tenant
	*	@param: lsi0						Information related to the LSI-0
	*	@param:	endPointsDefinedInMatches	For each endpoint currently defined, contains the port
	*										in the LSI-0 to be used to send packets on that endpoint
	*	@param: endPointsDefinedInActions	For each endpoint currently defined, contains the port
	*										in the LSI-0 to be used to match packets coming from that
	*										endpoint
	*	@param: availableEndPoints			Indicates the number of time each available endpoint is used
	*										by graphs not defining the endpoint itself
	*	@param: creating					Indicates if the translation is needed to create or to detroy
	*										a graph
	*
	*	@Translation rules:
	*		phyPort -> phyPort :
	*			Each phyPort is translated into its port ID on LSI-0.
	*			The other parameters expressed in the match are not
	*			changed.
	*		phyPort -> NF: 
	*			PhyPort is translated into its port ID on LSI-0, while
	*			NF is translated into the LSI-0 side virtual link that
	*			"represents the NF" in LSI-0.
	*			The other parameters expressed in the match are not
	*			changed.
	*		NF -> phyPort:
	*			The entire match is replaced with a match on the LSI-0
	*			side of the virtual link that "represents the port" in the
	*			tenant LSI. 
	*			phyPort is translated into it port ID on LSI-0.
	*		NF -> NF:
	*			This rule does not appear in LSI-0.
	*		NF -> endpoint (the endpoint is in the action part of the rule):
	*			* endpoint defined in the current graph: no rule inserted in LSI-0
	*			* endpoint defined into another graph: the endpoint must be defined 
	*				into a match of the other graph.
	*				The entire match is replated with a match on the LSI-0 side of the 
	*				virtual link that "represents the endpoint" in the LSI-0. The action 
	*				is replaced with the identifier of the port stored in endPointsDefinedInMatches 
	*				and associated with the endpoint itself.
	*				Example: the current graph defines the rule:
	*					match: NFa:2 - action: ep
	*				while another graph defined the rule
	*					match: ep - action NFa:1
	*				The rule in the current graph will has, as a match, the LSI-0 part of 
	*				the virtual link corresponding to ep, while, as an action, the content 
	*				of endPointsDefinedInMatches[ep]
	*		endpoint -> NF (the endpoint is the match part of the rule):
	*			* endpoint defined in the current graph: no rule inserted in LSI-0
	*			* endpoint defined into another graph: the endpoint must be defined into 
	*				an action of the other graph.
	*				The endpoint is translated into the id saved in endPointsDefinedInActions 
	*				and associated with the endpoint itself, while he NF is translated into 
	*				the LSI-0 side virtual link that "represents the NF" in LSI-0.
	*				The other parameters expressed into the match are not changed
	*/
	static lowlevel::Graph lowerGraphToLSI0(highlevel::Graph *graph, LSI *tenantLSI, LSI *lsi0, map<string, unsigned int> endPointsDefinedInMatches, map<string, unsigned int> endPointsDefinedInActions, map<string, unsigned int > &availableEndPoints, bool creating = true);
	
	/**
	*	@brief: translate an high level graph into a rules to be sent to
	*		the Tenant-LSI
	*
	*	@param: graph		High level graph to be translated
	*	@graph: tenantLSI	Information related to the LSI of the tenant
	*	@graph: lsi0		Information related to the LSI-0
	*
	*	@Translation rules:
	*		phyPort -> phyPort :
	*			This rule does not appear in the tenant LSI.
	*		phyPort -> NF : 
	*		endpoint -> NF:
	*			The entire match is replaced with a match on the tenant-LSI
	*			side of the virtual link that represents the NF on LSI-0.
	*			NF is translalted into the port ID on tenant-LSI.
	*		NF -> phyPort:
	*			NF is translated into the port ID on tenant-LSI, while phyPort
	*			is translated into the tenant side virtual link that "represents
	*			the phyPort" in the tenant LSI.
	*			The other parameters expressed in the match are not changed.
	*		NF -> NF:
	*			Each NF is translated into its port ID on tenant-LSI.
	*			The other parameters expressed in the match are not
	*			changed.
	*		NF -> endpoint:
	*			NF is translated into the port ID on tenant-LSI, while endpoint 
	*			is rtanslated into the tenant side virtual link that "represents
	*			the endpoint" in the tenant LSI.
	*/
	static lowlevel::Graph lowerGraphToTenantLSI(highlevel::Graph *graph, LSI *tenantLSI, LSI *lsi0);

};

#endif //GRAPH_TRANSLATOR_H_
