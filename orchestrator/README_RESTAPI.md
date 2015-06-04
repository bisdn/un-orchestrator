# How to use the un-orchestrator

This file describes, thorugh a set of examples, some of the commands that
can be used to interact with the un-orchestrator.

The un-orchestrator can either accept a NF-FG from a file, or from its 
REST interface (which features an embedded HTTP server).
Some examples are available in the 'config' folder.


### Main REST commands accepted by the un-orchestrator

Retrieve the description of the graph with name "myGraph":

	GET /graph/myGraph HTTP/1.1

Delete the graph with name "myGraph"

	DELETE /graph/myGraph HTTP/1.1

Delete the flow with ID "flow_id" from the graph with name "myGraph":

	DELETE /graph/myGraph/flow_id HTTP/1.1

Retrieve information on the available physical interfaces:

	GET /interfaces HTTP/1.1



### NF-FG examples
This section provides some NF-FG graphs that can be used to configure
the un-orchestrator.

### vSwitch pass-through example

This example is very simple: it configures a graph that receives all the
traffic from interface ge0 and sends it to interface ge1, without any
network function in the middle.

First, create a new file, named *myGraph.json*, that contains the following
graph data:

    {
        "flow-graph":
        {
			"flow-rules": [
			{
				"id": "00000001",  
				"match":  
				{
					"port" : "ge0"  
				},
                "action":  
				{
					"port": "ge1"  
				}  
			}  
			]  
		}
	}


Now, send this JSON graph to the un-orchestrator using your favorite
REST tool (e.g., some nice plugins for Mozilla Firefox). Just in case,
you can also use the cURL command line tool, such as in the following
example:

    curl -i -H "Content-Type: application/json" -d myGraph.json -X PUT  http://un-orchestrator-address:port


* This example is more complex, and it includes a network function called "bridge".
  Packets coming from the interface ge0 are sent to the first port of the network
  function (bridge:1), while packes coming from the second port of the network
  function (bridge:2) are sent on the network interface ge1
  
PUT /graph/myGraph HTTP/1.1   
Content-Type : application/json   
  
{  
	"flow-graph":  
	{  
		"VNFs": [  
			{  
				"id": "bridge"  
			}  
		],  
		"flow-rules": [  
			{  
				"id": "00000001",  
				"match":  
				{  
					"port" : "ge0"  
				},  
				"action":  
				{  
					"VNF_id": "bridge:1"  
				}  
			},  
			{  
				"id": "00000002",  
				"match":  
				{  
					"VNF_id" : "bridge:2"  
				},  
				"action":  
				{  
					"port": "ge1"  
				}  
			}  
		]  
	}  
}  

This example implements the following graph:

    eth0 -> firewall -> if (tcp_dst == 80) -> web cache  -> nat  -> eth1  
                        else \--------------------------/ 

PUT /graph/myGraph HTTP/1.1   
Content-Type : application/json   
               
{  
	"flow-graph":   
	{  
		"VNFs": [  
			{  
				"id": "firewall"  
			},  
			{  
				"id": "NAT"  
			},  
			{  
				"id": "web-cache"  
			}  
		],  
		"flow-rules": [  
			{  
				"id": "00000001",  
				"match":  
				{  
					"port" : "eth0"   	   
				},  
				"action":  
				{  
					"VNF_id": "firewall:1"  
				}  
			},  
			{  
				"id": "00000002",  
				"priority" : "10",  
				"match":  
				{  
					"VNF_id" : "firewall:2",  
					"tcp_dst" : "80"  
				},  
				"action":  
				{  
					"VNF_id": "web-cache:1"  
				}  
			},  
			{  
				"id": "00000003",  
				"priority" : "1",  
				"match":  
				{  
					"VNF_id" : "firewall:2"  
				},  
				"action":  
				{  
					"VNF_id": "NAT:1"  
				}  
			},  
			{  
				"id": "00000004",  
				"match":  
				{  
					"VNF_id" : "web-cache:2"  
				},  
				"action":  
				{  
					"VNF_id": "NAT:1"  
				}  
			},  
			{  
				"id": "00000005",  
				"match":  
				{  
					"VNF_id" : "NAT:2"  
				},  
				"action":  
				{  
					"port": "eth1"  
				}  
			}  
		]  
	}  
}  

* This example implements the following graph:

   eth0 <-> bridge <-> eth1
   
   It is worth noting that only the packets to/from a specific MAC address can
   enter into the bridge, while packets with other MAC addresses are not considered
   by this graph.
   Moreover, the bridge has a third interface (bridge:3) having an IPv4 address,
   and the graph specifies a proper path for the packets to/from this address,
   in order, for instance, to allow the client to contact the network function
   through SSH.

PUT /graph/myGraph HTTP/1.1   
Content-Type : application/json 
               
{   
   "flow-graph":{   
      "VNFs":[   
         {    
            "id":"bridge",  
            "ports_with_requirements":[    
               {    
                  "name":"bridge:3",  
                  "ipv4":{    
                     "address":"130.192.225.176",  
                     "netmask":"255.255.255.128"  
                  }  
               }  
            ]  
         }  
      ],  
      "flow-rules":[   
         {    
            "id":"00000001",  
            "match":{    
               "port":"ge0",  
               "eth_src":"00:e0:ed:22:ee:de"  
            },  
            "action":{    
               "VNF_id":"bridge:1"  
            }  
         },  
         {    
            "id":"00000002",  
            "match":{    
               "VNF_id":"bridge:2"  
            },  
            "action":{    
               "port":"ge3"  
            }  
         },  
        {    
            "id":"00000003",  
            "match":{    
               "port":"ge3",  
               "eth_dst":"00:e0:ed:22:ee:de"  
            },  
            "action":{    
               "VNF_id":"bridge:2"  
            }  
         },  
         {    
            "id":"00000004",  
            "match":{    
               "VNF_id":"bridge:1"  
            },  
            "action":{    
               "port":"ge0"  
            }  
         },  
         {    
            "id":"00000005",  
            "priority":"25",  
            "match":{    
               "port":"ge0",  
               "eth_src":"00:e0:ed:22:ee:de",  
               "ethertype":"0x800",  
               "ipv4_dst":"130.192.225.176"  
            },  
            "action":{    
               "VNF_id":"bridge:3"  
            }  
         },  
         {    
            "id":"00000006",  
            "match":{    
               "VNF_id":"bridge:3"  
            },  
            "action":{    
               "port":"ge0"  
            }  
         },  
         {    
            "id":"00000007",  
            "priority":"26",  
            "match":{    
               "port":"ge0",  
               "eth_src":"00:e0:ed:22:ee:de",  
               "ethertype":"0x806",  
               "arp_tpa":"130.192.225.176"  
            },  
            "action":{    
               "VNF_id":"bridge:3"  
            }  
         }  
      ]  
   }  
}  

* It is possible to connect multiple graphs together by using the so called "endpoints".
  An endpoint is always in the form "graph_id:endpoint_id_in_the_graph", where "graph_id"
  is the graph that defines the endpoint.
  A graph that want to use and endpoint defined by another graph, can do it only if that
  endpoint has been specified by that graph. 
  
  As an example, the following command defines an endpoint "myGraph:1", while the second
  command uses that endpoint.

PUT /graph/myGraph HTTP/1.1   
Content-Type : application/json
               
{   
   "flow-graph":{   
      "VNFs":[   
         {  
            "id":"bridge"  
         }  
      ],  
      "flow-rules":[   
         {    
            "id":"00000001",  
            "match":{    
               "endpoint_id":"myGraph:1"  
            },  
            "action":{    
               "VNF_id":"bridge:1"  
            }  
         },  
         {    
            "id":"00000002",  
            "match":{    
               "VNF_id":"bridge:2"  
            },  
            "action":{    
               "port":"ge1"  
            }  
         }  
      ]  
   }  
}  

PUT /graph/othweGraph HTTP/1.1   
Content-Type : application/json   
               
{   
   "flow-graph":{   
      "VNFs":[   
         {    
            "id":"bridge"  
         }  
      ],  
      "flow-rules":[   
         {    
            "id":"00000001",  
            "priority":"25",  
            "match":{    
               "port":"ge0",  
               "eth_src":"aa:aa:aa:aa:aa:aa"   
            },  
            "action":{    
               "endpoint_id":"myGraph:1"  
            }  
         },  
         {    
            "id":"00000002",  
            "match":{    
               "port":"ge0"  
            },  
            "action":{    
               "port":"ge1"  
            }  
         }  
      ]  
   }  
}  

  Since the endpoint "myGraph:1" is defined in a match of the graph "myGraph",
  other graphs can use it only in an action.
  On the other hand, if an endpoint is defined in an action, other graphs can
  use it in a match.

===============================================================================

Within the "match" element, the following fields are allowed (all the values
must be spicified as strings):

	"port"         //only if "VNF_id" and "endpoint_id" are not specified
	"VNF_id"       //only if "port" and "endpoint_id" are not specified
	"endpoint_id"  //only if "port" and "VNF_id" are not specified
	"eth_src"
	"eth_src_mask"
	"eth_dst"
	"eth_dst_mask"
	"ethertype"
	"vlan_id"       //can be a number, "ANY", or "NO_VLAN"
	"vlan_pcp"
	"ip_dscp"
	"ip_ecn"
	"ip_proto"
	"ipv4_src"
	"ipv4_src_mask"
	"ipv4_dst"
	"ipv4_dst_mask"
	"tcp_src"
	"tcp_dst"
	"udp_src"
	"udp_dst"
	"sctp_src"
	"sctp_dst"
	"icmpv4_type"
	"icmpv4_code"
	"arp_opcode"
	"arp_spa"
	"arp_spa_mask"
	"arp_tpa"
	"arp_tpa_mask"
	"arp_sha"
	"arp_tha"
	"ipv6_src"
	"ipv6_src_mask"
	"ipv6_dst"
	"ipv6_dst_mask"
	"ipv6_flabel"
	"ipv6_nd_target"
	"ipv6_nd_sll"
	"ipv6_nd_tll"
	"icmpv6_type"
	"icmpv6_code"
	"mpls_label"
	"mpls_tc"

===============================================================================

Within the "action" element, the following fields are allowed:

	"port"         //only if "VNF_id" and "endpoint_id" are not specified
	"VNF_id"       //only if "port" and "endpoint_id" are not specified
	"endpoint_id"  //only if "port" and "VNF_id" are not specified

===============================================================================

The same message used to create a new graph can be used to add "parts" (i.e.,
network functions and flows) to an existing graph.
  
