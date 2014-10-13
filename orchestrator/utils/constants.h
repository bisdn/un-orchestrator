#ifndef CONSTANTS_H_
#define CONSTANTS_H_ 1

#define MODULE_NAME 			"node-orchestrator"

/*
*	Commands to xDPD - Answers from xDPD
*/
#define DISCOVER_PHY_PORTS		"discover-physical-ports"
#define CREATE_LSI				"create-lsi"
#define CREATE_NF_PORTS			"create-nfs-ports"
#define CREATE_VLINKS			"create-virtual-links"
#define DESTROY_LSI				"destroy-lsi"
#define DESTROY_NF_PORTS		"destroy-nfs-ports"
#define DESTROY_VLINKS			"destroy-virtual-links"
#define OK						"ok"
#define XDPD_ERROR				"error"

/*
*	Connections
*/
#define	XDPD_ADDRESS				"127.0.0.1"
#define OF_CONTROLLER_ADDRESS 		"127.0.0.1"
#define FIRTS_OF_CONTROLLER_PORT	6653

#define REST_PORT 				8080
#define BASE_URL_GRAPH			"graph"
#define BASE_URL_IFACES			"interfaces"
#define REST_URL 				"http://localhost"
#define REQ_SIZE 				2*1024*1024

/*
*	Rest methods
*/
#define PUT						"PUT"
#define GET						"GET"
#define DELETE					"DELETE"

/*
*	HTTP headers
*/
#define JSON_C_TYPE				"application/json"
#define NO_CACHE				"no-cache"

/*
*	Costants in the JSON describing the graph
*/

#define FLOW_GRAPH		"flow-graph"
	#define VNFS 			"VNFs"
		//#define _ID			"id"
		#define	TEMPLATE		"template"
		#define PORTS_WITH_REQ	"ports_with_requirements"
			#define PORT_NAME	"name"
			#define ETHERNET	"ethernet"
				#define ADDRESS		"address"
			#define IP4			"ipv4"
				//#define ADDRESS		"address"
				#define MASK		"netmask"		
	#define FLOW_RULES		"flow-rules"
		#define _ID				"id"
		#define PRIORITY		"priority"
		#define MATCH			"match"
			//#define	PORT		"port"
			//#define	VNF_ID		"VNF_id"
			//#define ENDPOINT_ID	"endpoint_id"
			#define ETH_SRC			"eth_src"
			#define ETH_SRC_MASK	"eth_src_mask"
			#define ETH_DST			"eth_dst"
			#define ETH_DST_MASK 	"eth_dst_mask"
			#define ETH_TYPE		"ethertype"
			#define VLAN_ID			"vlan_id"
				#define ANY_VLAN		"ANY"
				#define NO_VLAN			"NO_VLAN"
			#define VLAN_PCP		"vlan_pcp"
			#define IP_DSCP			"ip_dscp"
			#define IP_ECN			"ip_ecn"
			#define IP_PROTO		"ip_proto"
			#define IPv4_SRC		"ipv4_src"
			#define IPv4_SRC_MASK	"ipv4_src_mask"
			#define IPv4_DST		"ipv4_dst"
			#define IPv4_DST_MASK	"ipv4_dst_mask"
			#define TCP_SRC			"tcp_src"
			#define TCP_DST			"tcp_dst"
			#define UDP_SRC			"udp_src"
			#define UDP_DST			"udp_dst"
			#define SCTP_SRC		"sctp_src"
			#define SCTP_DST		"sctp_dst"
			#define ICMPv4_TYPE		"icmpv4_type"
			#define ICMPv4_CODE		"icmpv4_code"
			#define ARP_OPCODE		"arp_opcode"
			#define ARP_SPA			"arp_spa"
			#define ARP_SPA_MASK	"arp_spa_mask"
			#define ARP_TPA			"arp_tpa"
			#define ARP_TPA_MASK	"arp_tpa_mask"
			#define ARP_SHA			"arp_sha"
			#define ARP_THA			"arp_tha"
			#define IPv6_SRC		"ipv6_src"
			#define IPv6_SRC_MASK	"ipv6_src_mask"
			#define IPv6_DST		"ipv6_dst"
			#define IPv6_DST_MASK	"ipv6_dst_mask"
			#define IPv6_FLABEL		"ipv6_flabel"
			#define IPv6_ND_TARGET	"ipv6_nd_target"
			#define IPv6_ND_SLL		"ipv6_nd_sll"
			#define IPv6_ND_TLL		"ipv6_nd_tll"
			#define ICMPv6_TYPE		"icmpv6_type"
			#define ICMPv6_CODE		"icmpv6_code"
			#define MPLS_LABEL		"mpls_label"
			#define MPLS_TC			"mpls_tc"
		#define ACTION			"action"
			#define	PORT			"port"
			#define	VNF_ID			"VNF_id"
			#define ENDPOINT_ID		"endpoint_id"
			
/*
*	Misc
*/
#define BUFFER_SIZE				20480
#define DATA_BUFFER_SIZE		20480

/*
*	Network functions
*/
#define CORE_MASK				0x2

#endif //CONSTANTS_H_
