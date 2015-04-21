#include "graph_translator.h"

lowlevel::Graph GraphTranslator::lowerGraphToLSI0(highlevel::Graph *graph, LSI *tenantLSI, LSI *lsi0, map<string, unsigned int> endPointsDefinedInMatches, map<string, unsigned int> endPointsDefinedInActions,map<string, unsigned int > &availableEndPoints, bool creating)
{
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Creating rules for LSI-0");
	
	map<string,unsigned int> ports_lsi0 = lsi0->getEthPorts();
	
	vector<VLink> tenantVirtualLinks = tenantLSI->getVirtualLinks();//FIXME: a map <emulated port name, vlink> would be better
	
	lowlevel::Graph lsi0Graph;
	
	list<highlevel::Rule> highLevelRules = graph->getRules();
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "The high level graph contains %d rules",highLevelRules.size());
	for(list<highlevel::Rule>::iterator hlr = highLevelRules.begin(); hlr != highLevelRules.end(); hlr++)
	{
		logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Considering a rule");
	
		highlevel::Match match = hlr->getMatch();
		highlevel::Action *action = hlr->getAction();
		uint64_t priority = hlr->getPriority();
		
		if( (match.matchOnNF()) && (action->getType() == highlevel::ACTION_ON_NETWORK_FUNCTION) )
		{
			//NF -> NF : rule not included in LSI-0
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\tIt matches a NF, and the action is a NF. Not inserted in LSI-0");
			continue;
		}
		
		if( (match.matchOnNF()) && (action->getType() == highlevel::ACTION_ON_ENDPOINT) )
		{
			/**
			*	NF -> endpoint
			*/
			if(graph->isDefinedHere(action->toString()))
			{
				/**
				*	the rule is not included in case the endpoint is defined by the graph itself
				*/
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\tRule with action expressed on end point \"%s\" defined in this graph. The rule is not inserted in the LSI-0",action->toString().c_str());
			}
			else
			{
				assert(endPointsDefinedInMatches.count(action->toString()) != 0);
		
				if(creating)
				{
					availableEndPoints[action->toString()]++;
					logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "endpoint \"%s\" used %d times",action->toString().c_str(), availableEndPoints[action->toString()]);
				}
				else
				{
					availableEndPoints[action->toString()]--;
					logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "endpoint \"%s\" still used %d times",action->toString().c_str(), availableEndPoints[action->toString()]);
				}

				/**				
				*	The entire match must be replaced with the virtual link associated with the endpoint
				*	expressed in the action.
				*	The endpoint in the action must be replaced with the port identifier defined into the graph defining
				*	the endpoint itself (hence expressed in endPointsDefinedInMatches)
				*/
				
				string action_info = action->getInfo();
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Match on NF \"%s\", action is on endpoint \"%s\"",match.getNF().c_str(),action->toString().c_str());
		
				//Translate the match
				lowlevel::Match lsi0Match;
				
				map<string, uint64_t> endpoints_vlinks = tenantLSI->getEndPointsVlinks();
				if(endpoints_vlinks.count(action->toString()) == 0)
				{
					logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses an action on endpoint \"%s\", which has not been translated into a virtual link",action->toString().c_str());
				}
				uint64_t vlink_id = endpoints_vlinks.find(action->toString())->second;
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\t\tThe virtual link related to endpoint \"%s\" has ID: %x",action->toString().c_str(),vlink_id);
				vector<VLink>::iterator vlink = tenantVirtualLinks.begin();
				for(;vlink != tenantVirtualLinks.end(); vlink++)
				{
					if(vlink->getID() == vlink_id)
						break;
				}
				assert(vlink != tenantVirtualLinks.end());
				lsi0Match.setInputPort(vlink->getRemoteID());

				//Translate the action
				unsigned int portForAction = endPointsDefinedInMatches[action->toString()];
				lowlevel::Action lsi0Action(portForAction);			

				//Create the rule and add it to the graph
				//The rule ID is created as follows  highlevelGraphID_hlrID
				stringstream newRuleID;
				newRuleID << graph->getID() << "_" << hlr->getFlowID();
				lowlevel::Rule lsi0Rule(lsi0Match,lsi0Action,newRuleID.str(),priority);
				lsi0Graph.addRule(lsi0Rule);
			}
			continue;
		}
		
		if(match.matchOnPort())
		{
			//The port name must be replaced with the port identifier
		
			string port = match.getPhysicalPort();
			if(ports_lsi0.count(port) == 0)
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses a match on port \"%s\", which is not attached to LSI-0",port.c_str());
				throw GraphManagerException();
			}
		
			//Translate the match
			lowlevel::Match lsi0Match;
			lsi0Match.setAllCommonFields(match);		
			map<string,unsigned int>::iterator translation = ports_lsi0.find(port);
			lsi0Match.setInputPort(translation->second);
			
			//Translate the action
			string action_info = action->getInfo();
			if(action->getType() == highlevel::ACTION_ON_PORT)
			{
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\tIt matches the port \"%s\", and the action is output to port %s",port.c_str(),action_info.c_str());
			
				//The port name must be replaced with the port identifier
				if(ports_lsi0.count(action_info) == 0)
				{
					logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses an action on port \"%s\", which is not attached to LSI-0",port.c_str());
					throw GraphManagerException();
				}
				
				map<string,unsigned int>::iterator translation = ports_lsi0.find(action_info);
				unsigned int portForAction = translation->second;
					
				lowlevel::Action lsi0Action(portForAction);	
	
				//Create the rule and add it to the graph
				//The rule ID is created as follows  highlevelGraphID_hlrID
				stringstream newRuleID;
				newRuleID << graph->getID() << "_" << hlr->getFlowID();
				lowlevel::Rule lsi0Rule(lsi0Match,lsi0Action,newRuleID.str(),priority);
				lsi0Graph.addRule(lsi0Rule);
			}
			else //XXX: for sure the action is a NF. Currently, other actions are not supported
			{
				assert(action->getType() == highlevel::ACTION_ON_NETWORK_FUNCTION);
			
				highlevel::ActionNetworkFunction *action_nf = (highlevel::ActionNetworkFunction*)action;
			
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\tIt matches the port \"%s\", and the action is \"%s:%d\"",port.c_str(),action_info.c_str(),action_nf->getPort());
				
				//Al the traffic for a NF is sent on the same virtual link
				
				stringstream action_port;
				action_port << action_info << "_" << action_nf->getPort();
				
				map<string, uint64_t> nfs_vlinks = tenantLSI->getNFsVlinks();
				if(nfs_vlinks.count(action_port.str()) == 0)
				{
					logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses a NF action \"%s:%d\" which has not been translated into a virtual link",action_info.c_str(),action_nf->getPort());
				}
				uint64_t vlink_id = nfs_vlinks.find(action_port.str())->second;
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\t\tThe virtual link related to NF \"%s\" has ID: %x",action_port.str().c_str(),vlink_id);
				vector<VLink>::iterator vlink = tenantVirtualLinks.begin();
				for(;vlink != tenantVirtualLinks.end(); vlink++)
				{
					if(vlink->getID() == vlink_id)
						break;
				}
				assert(vlink != tenantVirtualLinks.end());
				lowlevel::Action lsi0Action(vlink->getRemoteID());
				
				//Create the rule and add it to the graph
				//The rule ID is created as follows  highlevelGraphID_hlrID
				stringstream newRuleID;
				newRuleID << graph->getID() << "_" << hlr->getFlowID();
				lowlevel::Rule lsi0Rule(lsi0Match,lsi0Action,newRuleID.str(),priority);
				lsi0Graph.addRule(lsi0Rule);
			}
	
		 } //end of match.matchOnPort()
		 else if(match.matchOnEndPoint())
		 {
		 	assert(action->getType() == highlevel::ACTION_ON_NETWORK_FUNCTION);
		 
		 	stringstream ss;
		 	ss << match.getGraphID() << ":" << match.getEndPoint();
		 
		 	if(graph->isDefinedHere(ss.str()))
			{
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\tRule with match expressed on end point \"%s\" defined in this graph. The rule is not inserted in the LSI-0",ss.str().c_str());
			}
			else
			{
				if(creating)
				{
					availableEndPoints[ss.str()]++;
					logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "endpoint \"%s\" used %d times",ss.str().c_str(), availableEndPoints[ss.str()]);
				}
				else
				{
					availableEndPoints[ss.str()]--;
					logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "endpoint \"%s\" still used %d times",ss.str().c_str(), availableEndPoints[ss.str()]);
				}
			
				highlevel::ActionNetworkFunction *action_nf = (highlevel::ActionNetworkFunction*)action;
			
				//Al the traffic for a NF is sent on the same virtual link
				stringstream action_port;
				string action_info = action->getInfo();
				action_port << action_info << "_" << action_nf->getPort();
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\tIt matches the endpoint \"%s\", and the action is \"%s:%d\"",ss.str().c_str(),action_info.c_str(),action_nf->getPort());
				
				
				assert(endPointsDefinedInActions.count(ss.str()) != 0);
				
				//Translate the match
				lowlevel::Match lsi0Match;
				lsi0Match.setAllCommonFields(match);		
				lsi0Match.setInputPort(endPointsDefinedInActions[ss.str()]);

				
				//Translate the action
				map<string, uint64_t> nfs_vlinks = tenantLSI->getNFsVlinks();
				if(nfs_vlinks.count(action_port.str()) == 0)
				{
					logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses a NF action \"%s:%d\" which has not been translated into a virtual link",action_info.c_str(),action_nf->getPort());
				}
				uint64_t vlink_id = nfs_vlinks.find(action_port.str())->second;
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\t\tThe virtual link related to NF \"%s\" has ID: %x",action_port.str().c_str(),vlink_id);
				vector<VLink>::iterator vlink = tenantVirtualLinks.begin();
				for(;vlink != tenantVirtualLinks.end(); vlink++)
				{
					if(vlink->getID() == vlink_id)
						break;
				}
				assert(vlink != tenantVirtualLinks.end());
				lowlevel::Action lsi0Action(vlink->getRemoteID());
				
				//Create the rule and add it to the graph
				//The rule ID is created as follows  highlevelGraphID_hlrID
				stringstream newRuleID;
				newRuleID << graph->getID() << "_" << hlr->getFlowID();
				lowlevel::Rule lsi0Rule(lsi0Match,lsi0Action,newRuleID.str(),priority);
				lsi0Graph.addRule(lsi0Rule);
			}
			continue;
		 } //end of match.matchOnEndPoint()
		 else
		 {
		 	assert(match.matchOnNF());
		 	assert(action->getType() == highlevel::ACTION_ON_PORT);
		 	 
			//The entire match must be replaced with the virtual link associated with the port
			//expressed in the OUTPUT action.
			//The port in the OUTPUT action must be replaced with its port identifier
		
			string action_info = action->getInfo();
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Match on NF \"%s\", action is on port \"%s\"",match.getNF().c_str(),action_info.c_str());
			if(ports_lsi0.count(action_info) == 0)
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses an action on port \"%s\", which is not attached to LSI-0",action_info.c_str());
				throw GraphManagerException();
			}
		
			//Translate the match
			lowlevel::Match lsi0Match;
				
			map<string, uint64_t> ports_vlinks = tenantLSI->getPortsVlinks();
			if(ports_vlinks.count(action_info) == 0)
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses an action on port \"%s\", which has not been translated into a virtual link",action_info.c_str());
			}
			uint64_t vlink_id = ports_vlinks.find(action_info)->second;
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\t\tThe virtual link related to port \"%s\" has ID: %x",action_info.c_str(),vlink_id);
			vector<VLink>::iterator vlink = tenantVirtualLinks.begin();
			for(;vlink != tenantVirtualLinks.end(); vlink++)
			{
				if(vlink->getID() == vlink_id)
					break;
			}
			assert(vlink != tenantVirtualLinks.end());
			lsi0Match.setInputPort(vlink->getRemoteID());

			//Translate the action
			map<string,unsigned int>::iterator translation = ports_lsi0.find(action_info);
			unsigned int portForAction = translation->second;
			
			lowlevel::Action lsi0Action(portForAction);			

			//Create the rule and add it to the graph
			//The rule ID is created as follows  highlevelGraphID_hlrID
			stringstream newRuleID;
			newRuleID << graph->getID() << "_" << hlr->getFlowID();
			lowlevel::Rule lsi0Rule(lsi0Match,lsi0Action,newRuleID.str(),priority);
			lsi0Graph.addRule(lsi0Rule);
		 }//end of match.matchOnNF()
	}
	
	return lsi0Graph;	
}

lowlevel::Graph GraphTranslator::lowerGraphToTenantLSI(highlevel::Graph *graph, LSI *tenantLSI, LSI *lsi0)
{
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Creating rules for the tenant LSI");
	
	map<string,unsigned int> ports_lsi0 = lsi0->getEthPorts();
	
	vector<VLink> tenantVirtualLinks = tenantLSI->getVirtualLinks();//FIXME: a map <emulated port name, vlink> would be better
	set<string> tenantNetworkFunctions = tenantLSI->getNetworkFunctionsName();
	
	lowlevel::Graph tenantGraph;
	
	list<highlevel::Rule> highLevelRules = graph->getRules();

	for(list<highlevel::Rule>::iterator hlr = highLevelRules.begin(); hlr != highLevelRules.end(); hlr++)
	{
		logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Considering a rule");
	
		highlevel::Match match = hlr->getMatch();
		highlevel::Action *action = hlr->getAction();
		uint64_t priority = hlr->getPriority();
		
		if( (match.matchOnPort()) && (action->getType() == highlevel::ACTION_ON_PORT) )
		{
			/**
			*	physical port -> physical port : rule not included in LSI-0
			*/
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\tIt matches a port, and the action is OUTPUT to a port. Not inserted in the tenant LSI");
			continue;
		}
		
		if(match.matchOnPort() || match.matchOnEndPoint())
		{
		 	assert(action->getType() == highlevel::ACTION_ON_NETWORK_FUNCTION);
		 	
		 	/** 
			*	The entire match must be replaced with the virtual link associated with the action.
			*	The action is translated into an action to the port identifier of the NF
			*	representing the action itself
			*/
		
			string action_info = action->getInfo();

			highlevel::ActionNetworkFunction *action_nf = (highlevel::ActionNetworkFunction*)action;
			unsigned int inputPort = action_nf->getPort();
			
			if(tenantNetworkFunctions.count(action_info) == 0)
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses an action \"%s\", which is not a NF attacched to the tenant LSI",action_info.c_str());
				throw GraphManagerException();
			}
			
			map<string,unsigned int> tenantNetworkFunctionsPorts = tenantLSI->getNetworkFunctionsPorts(action_info);
			
			stringstream nf_port;
			nf_port << action_info << "_" << inputPort;
			
			if(match.matchOnPort())
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Match on port \"%s\", action is \"%s:%d\"",match.getPhysicalPort().c_str(),action_info.c_str(),inputPort);
			else
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Match on endpoint \"%s:%d\", action is \"%s:%d\"",match.getGraphID().c_str(),match.getEndPoint(),action_info.c_str(),inputPort);
	
			if(tenantNetworkFunctionsPorts.count(nf_port.str()) == 0)
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses an action \"%s:%d\", which is not a NF attacched to the tenant LSI",action_info.c_str(),inputPort);
				throw GraphManagerException();
			}
		
			//Translate the match
			lowlevel::Match tenantMatch;
				
			map<string, uint64_t> nfs_vlinks = tenantLSI->getNFsVlinks();
			if(nfs_vlinks.count(nf_port.str()) == 0)
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses the action \"%s:%d\", which has not been translated into a virtual link",action_info.c_str(),inputPort);
			}
			uint64_t vlink_id = nfs_vlinks.find(nf_port.str())->second;
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\t\tThe virtual link related to action \"%s\" has ID: %x",nf_port.str().c_str(),vlink_id);
			vector<VLink>::iterator vlink = tenantVirtualLinks.begin();
			for(;vlink != tenantVirtualLinks.end(); vlink++)
			{
				if(vlink->getID() == vlink_id)
					break;
			}
			assert(vlink != tenantVirtualLinks.end());
			tenantMatch.setInputPort(vlink->getLocalID());

			//Translate the action
			map<string,unsigned int>::iterator translation = tenantNetworkFunctionsPorts.find(nf_port.str());
			lowlevel::Action tenantAction(translation->second);

			//Create the rule and add it to the graph
			lowlevel::Rule tenantRule(tenantMatch,tenantAction,hlr->getFlowID(),priority);
			tenantGraph.addRule(tenantRule);		
		}//end match.matchOnPort
		else //match.matchOnNF()
		{
			assert(match.matchOnNF());
			
			/**
			*	Each NF is translated into its port ID on tenant-LSI.
			*	The other parameters expressed in the match are not
			*	changed.
			*/
			
			string nf = match.getNF();
			int nfPort = match.getPortOfNF();
			
			if(tenantNetworkFunctions.count(nf) == 0)
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses a match \"%s\", which is not a NF attacched to the tenant LSI",nf.c_str());
				throw GraphManagerException();
			}
			
			map<string,unsigned int> tenantNetworkFunctionsPorts = tenantLSI->getNetworkFunctionsPorts(nf);
			
			stringstream nf_output;
			nf_output << nf << "_" << nfPort;
						
			if(tenantNetworkFunctionsPorts.count(nf_output.str()) == 0)
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses (at rule %s) a match on \"%s:%d\", which is not attached to the tenant LSI",(hlr->getFlowID()).c_str(),nf.c_str(),nfPort);
				throw GraphManagerException();
			}
		
			//Translate the match
			lowlevel::Match tenantMatch;
			tenantMatch.setAllCommonFields(match);		
	
			map<string,unsigned int>::iterator translation = tenantNetworkFunctionsPorts.find(nf_output.str());
			tenantMatch.setInputPort(translation->second);
			
			//Translate the action
			string action_info = action->getInfo(); //e.g., "firewall"
			if(action->getType() == highlevel::ACTION_ON_NETWORK_FUNCTION)
			{
				highlevel::ActionNetworkFunction *action_nf = (highlevel::ActionNetworkFunction*)action;
				unsigned int inputPort = action_nf->getPort();//e.g., "1"
				stringstream nf_port;
				nf_port << action_info << "_" << inputPort;//e.g., "firewall_1"

				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\tIt matches the \"%s:%d\", and the action is \"%s:%d\"",nf.c_str(),nfPort,action_info.c_str(),inputPort);
			
				if(tenantNetworkFunctions.count(action_info) == 0)
				{
					logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses the  \"%s\", which is not a NF attacched to the tenant LSI",nf.c_str());
					throw GraphManagerException();
				}

				map<string,unsigned int> tenantNetworkFunctionsPortsAction = tenantLSI->getNetworkFunctionsPorts(action_info);
					
				//The NF must be replaced with the port identifier
				if(tenantNetworkFunctionsPortsAction.count(nf_port.str()) == 0)
				{
					logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses an action (at rule %s) on NF \"%s:%d\", which is not attached to LSI-0",(hlr->getFlowID()).c_str(),action_info.c_str(),inputPort);
					throw GraphManagerException();
				}
				map<string,unsigned int>::iterator translation = tenantNetworkFunctionsPortsAction.find(nf_port.str());
				lowlevel::Action tenantAction(translation->second);
	
				//Create the rule and add it to the graph
				lowlevel::Rule tenantRule(tenantMatch,tenantAction,hlr->getFlowID(),priority);
				tenantGraph.addRule(tenantRule);
			}
			else if(action->getType() == highlevel::ACTION_ON_PORT)
			{
				/**
				*	The phyPort is translated into the tenant side virtual link that 
				*	"represents the phyPort" in the tenant LSI. The other parameters 
				*	expressed in the match are not changed.
				*/
			
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\tIt matches the \"%s:%d\", and the action is  OUTPUT to port \"%s\"",nf.c_str(),nfPort,action_info.c_str());
				
				//Al the traffic for a physical is sent on the same virtual link
				
				map<string, uint64_t> ports_vlinks = tenantLSI->getPortsVlinks();
				if(ports_vlinks.count(action_info) == 0)
				{
					logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses an OUTPUT action on port \"%s\" which has not been translated into a virtual link",action_info.c_str());
				}
				uint64_t vlink_id = ports_vlinks.find(action_info)->second;
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\t\tThe virtual link related to the physical port \"%s\" has ID: %x",action_info.c_str(),vlink_id);
				vector<VLink>::iterator vlink = tenantVirtualLinks.begin();
				for(;vlink != tenantVirtualLinks.end(); vlink++)
				{
					if(vlink->getID() == vlink_id)
						break;
				}
				assert(vlink != tenantVirtualLinks.end());
				lowlevel::Action tenantAction(vlink->getLocalID());
				
				//Create the rule and add it to the graph
				lowlevel::Rule tenantRule(tenantMatch,tenantAction,hlr->getFlowID(),priority);
				tenantGraph.addRule(tenantRule);
			}
			else
			{
				assert(action->getType() == highlevel::ACTION_ON_ENDPOINT);
			
				/**
				*	the endpoint is translated into the tenant side virtual link that 
				*	"represents the endpoint" in the tenant LSI. The other parameters 
				*	expressed in the match are not changed.
				*/
			
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\tIt matches the \"%s:%d\", and the action is an output on endpoint \"%s\"",nf.c_str(),nfPort,action->toString().c_str());
				
				//Al the traffic for an endpoint is sent on the same virtual link
				
				map<string, uint64_t> endpoints_vlinks = tenantLSI->getEndPointsVlinks();
				
				if(endpoints_vlinks.count(action->toString()) == 0)
				{
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "The tenant graph expresses an action on endpoint \"%s\" which has not been translated into a virtual link",action->toString().c_str());
					assert(0);
				}
				uint64_t vlink_id = endpoints_vlinks.find(action->toString())->second;
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "\t\tThe virtual link related to the endpoint \"%s\" has ID: %x",action->toString().c_str(),vlink_id);
				vector<VLink>::iterator vlink = tenantVirtualLinks.begin();
				for(;vlink != tenantVirtualLinks.end(); vlink++)
				{
					if(vlink->getID() == vlink_id)
						break;
				}
				assert(vlink != tenantVirtualLinks.end());
				lowlevel::Action tenantAction(vlink->getLocalID());
				
				//Create the rule and add it to the graph
				lowlevel::Rule tenantRule(tenantMatch,tenantAction,hlr->getFlowID(),priority);
				tenantGraph.addRule(tenantRule);
			}
		} //end match.matchOnNF

	}

	return tenantGraph;	
}
