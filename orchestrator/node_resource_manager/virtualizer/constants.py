'''
	File used by the orchestrator to maintain the state (i.e., rules deployed,
	VNF instantiated
'''

#TMP file use by the orchestrator to maintain the current configuration of the node
CONFIGURATION_FILE = './node_resource_manager/virtualizer/.universalnode.xml'

#TMP file used by the orchestrator and representing the deployed graph,
#in the JSON syntax internally used by the orchestrator itself
GRAPH_FILE = './node_resource_manager/virtualizer/.graph.json'

#File containing the new piece of graph to be deployed, in the JSON
#syntax internally used by the orchestrator
NEW_GRAPH_FILE = './node_resource_manager/virtualizer/.new_graph.json'
#File containing the IDs of the rules to be removed from the graph
REMOVE_GRAPH_FILE = './node_resource_manager/virtualizer/.remove_graph.json'

'''
	Information to be exported
'''
INFRASTRUCTURE_NAME = 'Single node'
INFRASTRUCTURE_ID = 'UUID001'
NODE_NAME = 'Universal Node'
NODE_ID = 'UUID11'
NODE_TYPE = 'BisBis'

'''
	Supported matches
'''
#XXX: this list must be aligned with that in 
# [orchestrator]/utils/constants.h
supported_matches = [
	"eth_src",
	"eth_src_mask",
	"eth_dst",
	"eth_dst_mask",
	"ethertype",
	"vlan_id",
	"vlan_pcp",
	"ip_dscp",
	"ip_ecn",
	"ip_proto",
	"ipv4_src",
	"ipv4_src_mask",
	"ipv4_dst",
	"ipv4_dst_mask",
	"tcp_src",
	"tcp_dst",
	"udp_src",
	"udp_dst",
	"sctp_src",
	"sctp_dst",
	"icmpv4_type",
	"icmpv4_code",
	"arp_opcode",
	"arp_spa",
	"arp_spa_mask",
	"arp_tpa",
	"arp_tpa_mask",
	"arp_sha",
	"arp_tha",
	"ipv6_src",
	"ipv6_src_mask",
	"ipv6_dst",
	"ipv6_dst_mask",
	"ipv6_flabel",
	"ipv6_nd_target",
	"ipv6_nd_sll",
	"ipv6_nd_tll",
	"icmpv6_type",
	"icmpv6_code",
	"mpls_label",
	"mpls_tc"
]

