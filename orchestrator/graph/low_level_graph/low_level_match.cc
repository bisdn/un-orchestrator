#include "low_level_match.h"

namespace lowlevel
{

Match::Match() :
	graph::Match(),isInput_port(false)
{

}

bool Match::operator==(const Match &other) const
{
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Matches to be compared:");
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\tisInputPort %s vs %s ",(isInput_port)?"yes":"no",(other.isInput_port)?"yes":"no");
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\tinput port: %d vs %d",input_port,other.input_port);

	if((isInput_port && !other.isInput_port) ||
		(!isInput_port && other.isInput_port) )
		return false;
		
	if(input_port != other.input_port)
		return false;

	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Comparing other fields...");

	return this->isEqual(other);
}

void Match::fillFlowmodMessage(rofl::openflow::cofflowmod &message)
{
	if(isInput_port)
		message.set_match().set_in_port(input_port);
		
	if(eth_src != NULL)
	{
		if(eth_src_mask)
			message.set_match().set_eth_src(cmacaddr(eth_src), cmacaddr(eth_src_mask));		
		else
			message.set_match().set_eth_src(cmacaddr(eth_src));
	}
		
	if(eth_dst != NULL)
	{
		if(eth_dst_mask)
			message.set_match().set_eth_dst(cmacaddr(eth_dst), cmacaddr(eth_dst_mask));		
		else
			message.set_match().set_eth_dst(cmacaddr(eth_dst));
	}
	if(isEthType)
		message.set_match().set_eth_type(ethType);
	if(isVlanID)
		message.set_match().set_vlan_vid(vlanID);
	else if(isAnyVlan)
		message.set_match().set_vlan_present();
	else if(isNoVlan)
		message.set_match().set_vlan_untagged();
	if(isVlanPCP)
		message.set_match().set_vlan_pcp(vlanPCP);
	if(isIpDSCP)
		message.set_match().set_ip_dscp(ipDSCP);
	if(isIpECN)
		message.set_match().set_ip_ecn(ipECN);
	if(isIpProto)
		message.set_match().set_ip_proto(ipProto);
	if(ipv4_src)
	{
		if(ipv4_src_mask)
			message.set_match().set_ipv4_src(caddress_in4(ipv4_src),caddress_in4(ipv4_src_mask));
		else
			message.set_match().set_ipv4_src(caddress_in4(ipv4_src));
	}
	if(ipv4_dst)
	{
		if(ipv4_dst_mask)
			message.set_match().set_ipv4_dst(caddress_in4(ipv4_dst),caddress_in4(ipv4_dst_mask));
		else
			message.set_match().set_ipv4_dst(caddress_in4(ipv4_dst));
	}
	if(isTcpSrc)
		message.set_match().set_tcp_src(tcp_src);
	if(isTcpDst)
		message.set_match().set_tcp_dst(tcp_dst);
	if(isUdpSrc)
		message.set_match().set_udp_src(udp_src);
	if(isUdpDst)
		message.set_match().set_udp_dst(udp_dst);	
	if(isSctpSrc)
		message.set_match().set_sctp_src(sctp_src);
	if(isSctpDst)
		message.set_match().set_sctp_dst(sctp_dst);		
	if(isIcmpv4Type)
		message.set_match().set_icmpv4_type(icmpv4Type);
	if(isIcmpv4Code)
		message.set_match().set_icmpv4_code(icmpv4Code);	
	if(isArpOpcode)
		message.set_match().set_arp_opcode(arpOpcode);
	if(arp_spa)	
	{
		if(arp_spa_mask)
			message.set_match().set_arp_spa(caddress_in4(arp_spa),caddress_in4(arp_spa_mask));
		else
			message.set_match().set_arp_spa(caddress_in4(arp_spa));
	}
	if(arp_tpa)	
	{
		if(arp_tpa_mask)
			message.set_match().set_arp_tpa(caddress_in4(arp_tpa),caddress_in4(arp_tpa_mask));
		else
			message.set_match().set_arp_tpa(caddress_in4(arp_tpa));
	}
	if(arp_sha)
		message.set_match().set_arp_sha(cmacaddr(arp_sha));
	if(arp_tha)
		message.set_match().set_arp_tha(cmacaddr(arp_tha));
	if(ipv6_src)
	{
		if(ipv6_src_mask)
			message.set_match().set_ipv6_src(caddress_in6(ipv6_src),caddress_in6(ipv6_src_mask));
		else
			message.set_match().set_ipv6_src(caddress_in6(ipv6_src));
	}
	if(ipv6_dst)
	{
		if(ipv6_dst_mask)
			message.set_match().set_ipv6_dst(caddress_in6(ipv6_dst),caddress_in6(ipv6_dst_mask));
		else
			message.set_match().set_ipv6_dst(caddress_in6(ipv6_dst));
	}
	if(isIpv6Flabel)
		message.set_match().set_ipv6_flabel(ipv6_flabel);
	if(isIcmpv6Type)
		message.set_match().set_icmpv6_type(icmpv6Type);
	if(isIcmpv6Code)
		message.set_match().set_icmpv6_code(isIcmpv6Code);
	if(ipv6_nd_target)	
		message.set_match().set_ipv6_nd_target(caddress_in6(ipv6_nd_target));
	if(ipv6_nd_sll)	
		message.set_match().set_ipv6_nd_sll(cmacaddr(ipv6_nd_sll));
	if(ipv6_nd_tll)	
		message.set_match().set_ipv6_nd_tll(cmacaddr(ipv6_nd_tll));
	if(isMplsLabel)
		message.set_match().set_mpls_label(mplsLabel);
	if(isMplsTC)
		message.set_match().set_mpls_tc(mplsTC);
}

void Match::setInputPort(unsigned int input_port)
{
	this->input_port = input_port;
	isInput_port = true;

}

void Match::setAllCommonFields(graph::Match match)
{
	graph::Match::setAllCommonFields(match);
}

void Match::print()
{
	if(LOGGING_LEVEL <= ORCH_DEBUG_INFO)
	{
		cout << "\t\tmatch:" << endl << "\t\t{" << endl;
		cout << "\t\t\tport: "<< input_port << endl;
		graph::Match::print();
		cout << "\t\t}" << endl;
	}
}


}
