#ifndef StopNFIn_H_
#define StopNFIn_ 1

#pragma once

#include <string>

/**
* @file StopNFIn_in.h
*
* @brief Parameters to be used to stop the network function.
*/

using namespace std;

class StopNFIn
{

friend class ComputeController;

private:

	/**
	*	@brief: identifier of the LSI to which the VNF is connected
	*/
	uint64_t lsiID;
	
	/**
	*	@brief: name of the network function
	*/
	string nf_name;

protected:
	StopNFIn(uint64_t lsiID, string nf_name) 
		: lsiID(lsiID), nf_name(nf_name)
	{
	}
	
public:

	uint64_t getLsiID()
	{
		return lsiID;
	}
	
	string getNfName()
	{
		return nf_name;
	}
};


#endif //StopNFIn_H_
