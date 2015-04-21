#ifndef AddNFportsOut_H_
#define AddNFportsOut_ 1

#pragma once

#include <string>

/**
* @file addNFports_out.h
*
* @brief Description of network function ports destroyed.
*/

using namespace std;

class AddNFportsOut
{

friend class GraphManager;

private:

	/**
	*	@brief: name of the network functions whose ports have been connected to the lsi
	*/
	string nf_name;

	/**
	*	@brief: map of ports name, identifier within the lsi
	*/
	map<string, unsigned int> ports;
	
protected:

	string getNFname()
	{
		return nf_name;
	}

	map<string, unsigned int> getPorts()
	{
		return ports;
	}

public:
	AddNFportsOut(string nf_name,map<string, unsigned int> ports) 
		: nf_name(nf_name), ports(ports.begin(),ports.end())
	{
	}
	
};


#endif //AddNFportsOut_H_
