#ifndef CheckPhysicalPorts_in_H_
#define CheckPhysicalPorts_in_ 1

#pragma once

#include <string>
#include <list>
#include <inttypes.h>
#include <assert.h>

/**
* @file checkPhysicalPorts_in.h
*
* @brief Description of a physical port to be handled by the node orchestrator through the
*		 virtual switch.
*/

using namespace std;

/**
*	@brief: different virtual switches may handle differently ethernet and wifi ports
*/
enum physicalPortType_t {ETHERNET_PORT,WIFI_PORT};

/**
*	@brief: different virtual switches may handle differently physical ports according to
*			the fact that they are connected to the edge or to the core of the network
*/
//FIXME: hide this information to the switch manager? How could be useful?
enum physicalPortSide_t {NONE,EDGE,CORE};

class CheckPhysicalPortsIn
{

friend class FileParser;

private:

	/**
	*	@brief: name of the physical port
	*/
	string name;
	
	/**
	*	@brief: type of the physical port
	*/
	physicalPortType_t type;
	
	/**
	*	@brief: side of the physical port, with respect to the node
	*/
	physicalPortSide_t side;

protected:
	CheckPhysicalPortsIn(string name, physicalPortType_t type, physicalPortSide_t side) 
		: name(name), type(type), side(side)
	{
	}
	
public:
	
	string getPortName() const
	{
		return name;
	}
		
	physicalPortType_t getPortType() const
	{
		return type;
	}
	
	physicalPortSide_t getPortSide()
	{
		return side;
	}
	
	string getPortTypeToString()
	{
		switch(type)
		{
			case ETHERNET_PORT:
				return string("ethernet");
				break;
			case WIFI_PORT:
				return string("wifi");
				break;
			default:
				assert(0);
				return "";
		}
	}
	
	string getPortSideToString() const
	{
		switch(side)
		{
			case EDGE:
				return string("edge");
				break;
			case CORE:
				return string("core");
				break;
			case NONE:
				return string("none");
				break;
			default:
				assert(0);
				return "";
		}
	}
	
	//XXX this operator is required to put an object of this class into a set
	bool operator< (const CheckPhysicalPortsIn& lhs) const
	{
		return true;
	}
};

#endif //CheckPhysicalPorts_in_
