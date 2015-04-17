#include  "high_level_match.h"

namespace highlevel
{

Match::Match() :
	graph::Match(),input(NULL), nf_port(0), type(MATCH_GENERIC)
{

}

bool Match::setInputPort(string input_port)
{
	if(type != MATCH_GENERIC)
		return false;
		
	input = (char*)malloc(sizeof(char)*(input_port.length()+1));
	strcpy(input,input_port.c_str());
	type = MATCH_PORT;
	
	return true;
}

bool Match::setNFport(string network_function, int port)
{
	if(type != MATCH_GENERIC)
		return false;

	input = (char*)malloc(sizeof(char)*(network_function.length()+1));
	strcpy(input,network_function.c_str());
	this->nf_port = port;
	type = MATCH_NF;
	
	return true;
}

bool Match::setEndPoint(string graphID, unsigned int endpoint)
{
	if(type != MATCH_GENERIC)
		return false;

	input = (char*)malloc(sizeof(char)*(graphID.length()+1));
	strcpy(input,graphID.c_str());
	this->endpoint = endpoint;
	type = MATCH_ENDPOINT;
	
	return true;
}

bool Match::matchOnPort()
{
	if(type == MATCH_PORT)
		return true;
	return false;
}

bool Match::matchOnNF()
{
	if(type == MATCH_NF)
		return true;
	return false;
}

bool Match::matchOnEndPoint()
{
	if(type == MATCH_ENDPOINT)
		return true;
	return false;
}

string Match::getPhysicalPort()
{
	assert(type == MATCH_PORT);
	return input;
}

string Match::getNF()
{
	assert(type == MATCH_NF);
	return input;
}

int Match::getPortOfNF()
{
	assert(type == MATCH_NF);
	return nf_port;
}

string Match::getGraphID()
{
	assert(type == MATCH_ENDPOINT);
	return input;
}

unsigned int Match::getEndPoint()
{
	assert(type == MATCH_ENDPOINT);
		
	return endpoint;
}

void Match::print()
{
	if(LOGGING_LEVEL <= ORCH_DEBUG_INFO)
	{
		cout << "\t\tmatch:" << endl << "\t\t{" << endl;
	
		if(type == MATCH_NF)
		{
			cout << "\t\t\tNF port: "<< input << ":" << nf_port << endl;
		}
		else if(type == MATCH_PORT)
			cout << "\t\t\tport: " << input << endl;
		else
		{
			assert(type == MATCH_ENDPOINT);
			cout << "\t\t\tendpoint: " << input << ":" << endpoint << endl;
		}
		
		graph::Match::print();
	
		cout << "\t\t}" << endl;
	}
}

Object Match::toJSON()
{
	Object match;	
	
	if(type == MATCH_PORT)
		match[PORT]  = input;
	else if(type == MATCH_NF)
	{
		stringstream nf;
		nf << input << ":" << nf_port;
		match[VNF_ID] = nf.str().c_str();
	}
	else
	{
		assert(type == MATCH_ENDPOINT);
		stringstream ep;
		ep << input << ":" << endpoint;
		match[ENDPOINT_ID] = ep.str().c_str();
	}
	
	graph::Match::toJSON(match);
	
	return match;
}

}
