#ifndef MATCH_PARSER_H_
#define MATCH_PARSER_H_ 1

#pragma once

#include "rest_server.h"
#include "../virtualizer/virtualizer.h"

class MatchParser
{

friend class RestServer;

protected:

	static string nfName(string name_port);
	static unsigned int nfPort(string name_port);
	
	static unsigned int graphEndPoint(string name_port);
	
	static bool parseMatch(Object object, highlevel::Match &match, map<string,set<unsigned int> > &nfs, highlevel::Graph &graph);
	
private:
	static bool validateMac(const char* mac);
	static bool validateIpv4(const string &ipAddress);
	static bool validateIpv6(const string &ipAddress);
	static bool validateIpv4Netmask(const string &netmask);
	
public:
	static string graphID(string name_port);
};

#endif //MATCH_PARSER_H_
