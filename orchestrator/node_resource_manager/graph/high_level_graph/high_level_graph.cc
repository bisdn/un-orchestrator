#include "high_level_graph.h"

namespace highlevel
{

Graph::Graph(string ID) :
	ID(ID)
{
}

string Graph::getID()
{
	return ID;
}

set<string> Graph::getPorts()
{
	return ports;
}

map<string, list<unsigned int> > Graph::getNetworkFunctions()
{
	return networkFunctions;
}

map<unsigned int,string> Graph::getNetworkFunctionEthernetPortsRequirements(string nf)
{
	if(networkFunctionsEthernetPortRequirements.count(nf) != 0)
		return (networkFunctionsEthernetPortRequirements.find(nf))->second;
		
	//No requirements are specified for this NF
	map<unsigned int,string> empty;
	return empty;
}

map<unsigned int,pair<string,string> > Graph::getNetworkFunctionIPv4PortsRequirements(string nf)
{
	if(networkFunctionsIPv4PortRequirements.count(nf) != 0)
		return (networkFunctionsIPv4PortRequirements.find(nf))->second;
		
	//No requirements are specified for this NF
	map<unsigned int,pair<string,string> > empty;
	return empty;
}

list<Rule> Graph::getRules()
{
	return rules;
}
	
bool Graph::addPort(string port)
{
	if(ports.count(port) != 0)
		return false;

	ports.insert(port);
	
	return true;
}

bool Graph::addNetworkFunction(string nf)
{
	if(networkFunctions.count(nf) != 0)
		return false;

	list<unsigned int> ports;
	networkFunctions[nf] = ports;
	
	return true;
}

bool Graph::updateNetworkFunction(string nf, unsigned int port)
{
	if(networkFunctions.count(nf) == 0)
	{
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "NF \"%s\" does not exist",nf.c_str());
		return false;
	}
	list<unsigned int> ports = networkFunctions.find(nf)->second;
	
	ports.push_back(port);
	
	networkFunctions[nf] = ports;
	
	return true;
}

bool Graph::updateNetworkFunctionIPv4PortsRequirements(string nf, unsigned int port, string address, string netmask)
{
	if(networkFunctions.count(nf) == 0)
	{
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "NF \"%s\" does not exist",nf.c_str());
		return false;
	}
	
	map<unsigned int,pair<string,string> > ports;
	if(networkFunctionsIPv4PortRequirements.count(nf) != 0)
	{
		ports = networkFunctionsIPv4PortRequirements[nf];	

		if(ports.count(port) != 0)
		{
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The configuration for the port \"%s:%d\" has already been specified",nf.c_str(),port);
			return false;
		}
	}
	ports[port] = make_pair<string,string>(address,netmask);
										
	networkFunctionsIPv4PortRequirements[nf] = ports;
	
	return true;
}

bool Graph::updateNetworkFunctionEthernetPortsRequirements(string nf, unsigned int port, string address)
{
	if(networkFunctions.count(nf) == 0)
	{
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "NF \"%s\" does not exist",nf.c_str());
		return false;
	}
	
	map<unsigned int,string> ports;
	if(networkFunctionsEthernetPortRequirements.count(nf) != 0)
	{
		ports = networkFunctionsEthernetPortRequirements[nf];	

		if(ports.count(port) != 0)
		{
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The configuration for the port \"%s:%d\" has already been specified",nf.c_str(),port);
			return false;
		}
	}
	ports[port] = address;
										
	networkFunctionsEthernetPortRequirements[nf] = ports;
	
	return true;
}

bool Graph::addRule(Rule rule)
{
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		if(*r == rule)
			return false;
	}

	rules.push_back(rule);
	
	return true;
}

bool Graph::ruleExists(string ID)
{
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		if(r->getFlowID() == ID)
			return true;
	}
	return false;
}

Rule Graph::getRuleFromID(string ID)
{
	list<Rule>::iterator r = rules.begin();
	
	for(; r != rules.end(); r++)
	{
		if(r->getFlowID() == ID)
			return *r;
	}
	
	assert(0);
	
	//This cannot happen; it is just for the compiler.
	return *r;
}

RuleRemovedInfo Graph::removeRuleFromID(string ID)
{
	RuleRemovedInfo rri;
	
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		if(r->getFlowID() == ID)
		{
			Match match = r->getMatch();
			Action *action = r->getAction();
			
			action_t actionType = action->getType();
			bool matchOnPort = match.matchOnPort();
			bool matchOnNF = match.matchOnNF();

			if(actionType == ACTION_ON_PORT)
			{
				//Removed an action on a port. It is possible that a vlink must be removed
				rri.port = ((ActionPort*)action)->getInfo();
				rri.isNFport = false;
				rri.isPort = true;
				rri.isEndpoint = false;
				
				rri.ports.push_back(rri.port);
			}
			else if(actionType == ACTION_ON_NETWORK_FUNCTION)
			{
				//Removed an action on a NF. It is possible that a vlink must be removed
				stringstream nf_port;
				nf_port << ((ActionNetworkFunction*)action)->getInfo() << "_" << ((ActionNetworkFunction*)action)->getPort();
				rri.nf_port = nf_port.str();
				
				//Potentially, the NF is useless in the graph
				rri.nfs.push_back(((ActionNetworkFunction*)action)->getInfo());
				rri.isNFport = true;
				rri.isPort = false;
				rri.isEndpoint = false;
			}
			else
			{
				//Removed an action on an endpoint
				assert(actionType == ACTION_ON_ENDPOINT);
				rri.endpoint = action->toString();
				
				rri.isNFport = false;
				rri.isPort = false;
				rri.isEndpoint = true;
			}
			
			if(matchOnNF)
				//Potentially, the NF is useless in the graph
				rri.nfs.push_back(match.getNF());
			else if(matchOnPort)
				//Potentially, the port is useless in the graph
				rri.ports.push_back(match.getPhysicalPort());
			else
			{
				stringstream ss;
				ss << match.getGraphID() << ":" << match.getEndPoint();
				rri.endpoint = ss.str();
			}

			//finally, remove the rule!
			rules.erase(r);
		
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "The graph still contains the rules: ");
			for(list<Rule>::iterator print = rules.begin(); print != rules.end(); print++)
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\t%s",print->getFlowID().c_str());
		
			return rri;
		}//end if(r->getFlowID() == ID)	
	}
	
	assert(0);
	
	//Just for the compiler
	return rri;
}

int Graph::getNumberOfRules()
{
	return rules.size();
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

Object Graph::toJSON()
{
	Object flow_graph;
	
	Array flow_rules;
	for(list<Rule>::iterator r = rules.begin(); r != rules.end();r++)
	{
		flow_rules.push_back(r->toJSON());
	}
	
	flow_graph[FLOW_RULES] = flow_rules;
	
	return flow_graph;
}

bool Graph::stillExistNF(string nf)
{
	if(networkFunctions.count(nf) == 0)
		return false;

	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		Match match = r->getMatch();
		Action *action = r->getAction();
		
		action_t actionType = action->getType();
		bool matchOnNF = match.matchOnNF();
		
		if(matchOnNF)
		{
			if(match.getNF() == nf)
				//The NF still exists into the graph
				return true;
		}
		
		if(actionType == ACTION_ON_NETWORK_FUNCTION)
		{
			if(((ActionNetworkFunction*)action)->getInfo() == nf)
				//The NF still exist into the graph
				return true;
		}
	}
	
	networkFunctions.erase(nf);
	return false;
}

bool Graph::stillExistEndpoint(string endpoint)
{
	if(endpoints.count(endpoint) == 0)
		return false;

	if(endpointIsUsedInMatch(endpoint))
		return true;
		
	if(endpointIsUsedInAction(endpoint))
		return true;
			
	endpoints.erase(endpoint);
	return false;
}

bool Graph::stillExistPort(string port)
{
	if(ports.count(port) == 0)
		return false;

	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		Match match = r->getMatch();
		Action *action = r->getAction();
		
		action_t actionType = action->getType();
		bool matchOnPort = match.matchOnPort();
		
		if(matchOnPort)
		{
			if(match.getPhysicalPort() == port)
				//The port still exists into the graph
				return true;
		}
		
		if(actionType == ACTION_ON_PORT)
		{
			if(((ActionPort*)action)->getInfo() == port)
				//The port still exist into the graph
				return true;
		}
	}

	ports.erase(port);
	
	return false;
}

bool Graph::addEndPoint(string graphID, string endpoint)
{	
	if(graphID == ID)
		endpoints[endpoint] = true;
	else
		endpoints[endpoint] = false;
		
	return endpoints[endpoint];
}

set<string> Graph::getEndPoints()
{
	set<string> endPoints;
	
	for(map<string,bool>::iterator ep = endpoints.begin(); ep != endpoints.end(); ep++)
		endPoints.insert(ep->first);

	return endPoints;
}

bool Graph::isDefinedHere(string endpoint)
{
	assert(endpoints.count(endpoint) != 0);
	
	return endpoints[endpoint];
}

string Graph::getEndpointInvolved(string flowID)
{
	highlevel::Rule r = getRuleFromID(flowID);
	highlevel::Match m = r.getMatch();
	highlevel::Action *a = r.getAction();
	
	if(a->getType() == highlevel::ACTION_ON_ENDPOINT)
		return a->toString();

	if(m.matchOnEndPoint())
	{
		stringstream ss;
		ss << m.getGraphID() << ":" << m.getEndPoint();
		return ss.str();
	}
	
	return "";
}

bool Graph::endpointIsUsedInMatch(string endpoint)
{
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		Match match = r->getMatch();		
		if(match.matchOnEndPoint())
		{
			stringstream ss;
			ss << match.getGraphID() << ":" << match.getEndPoint();
			if(ss.str() == endpoint)
				//The endpoint is used in a match
				return true;
		}		
	}
	return false;
}

bool Graph::endpointIsUsedInAction(string endpoint)
{
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		Action *action = r->getAction();		
		action_t actionType = action->getType();
		
		if(actionType == ACTION_ON_ENDPOINT)
		{
			if(action->toString() == endpoint)
				//The port is used in an action
				return true;
		}
	}
	return false;
}

}
