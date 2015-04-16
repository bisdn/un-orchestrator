#ifndef XDPD_LSI_H_
#define XDPD_LSI_H_ 1

#pragma once

#include "../lsi.h"

#include "xdpd_manager.h"
#include "virtual_link.h"
#include "../../nfs_manager/nf_type.h"

#include <map>
#include <set>
#include <list>
#include <string>
#include <vector>

using namespace std;

class VLink;

class XDPD_LSI : public LSI
{
//XXX: this class is a mess!

friend class XDPDManager;

	
public:
	XDPD_LSI(string controllerAddress, string controllerPort, map<string,string> ports, map<string, list <unsigned int> > network_functions,vector<VLink> virtual_links,map<string,nf_t>  nf_types, string wirelessPort = "");
	
	string getControllerAddress();
	string getControllerPort();
	
	list<string> getEthPortsName();
	bool hasWireless();
	string getWirelessPortName();
	
	set<string> getNetworkFunctionsName();
	list<string> getNetworkFunctionsPortNames(string nf);
	
	list<uint64_t> getVirtualLinksRemoteLSI();
	
	uint64_t getDpid();

	
	map<string,unsigned int> getEthPorts();
	pair<string,unsigned int> getWirelessPort();
	map<string,string> getPortsType();
	
	map<string,unsigned int> getNetworkFunctionsPorts(string nf);
	
	map<string,nf_t> getNetworkFunctionsType();

	vector<VLink> getVirtualLinks();
	VLink getVirtualLink(uint64_t ID);	
	map<string, uint64_t> getNFsVlinks();
	map<string, uint64_t> getPortsVlinks();
	map<string, uint64_t> getEndPointsVlinks();
	
	//FIXME: public is not a good choice
	void setNFsVLinks(map<string, uint64_t> nfs_vlinks);
	void addNFvlink(string NF, uint64_t vlinkID);
	void removeNFvlink(string nf_port);
	
	void setPortsVLinks(map<string, uint64_t> ports_vlinks);
	void addPortvlink(string port, uint64_t vlinkID);
	void removePortvlink(string port);
	
	void setEndPointsVLinks(map<string, uint64_t> endpoints_vlinks);
	void addEndpointvlink(string endpoint, uint64_t vlinkID);
	void removeEndPointvlink(string endpoint);
	
protected:
	void setDpid(uint64_t dpid);
	bool setEthPortID(string port, uint64_t id);
	bool setWirelessPortID(uint64_t id);
	bool setNfPortsID(string nf, map<string, unsigned int>);
	void setVLinkIDs(unsigned int position, unsigned int localID, unsigned int remoteID);

	int addVlink(VLink vlink);
	void removeVlink(uint64_t ID);
	
	void addNF(string name, list< unsigned int> ports);
	void removeNF(string nf);
};

#endif //XDPD_LSI_H_
