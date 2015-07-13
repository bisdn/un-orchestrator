'''
	File used by the orchestrator to maintain the state (i.e., rules deployed,
	VNF instantiated
'''

#TMP file use by the orchestrator to maintain the state of the node
TMP_FILE = './node_resource_manager/virtualizer/.universalnode.xml'

#TMP file used by the orchestrator and representing the deployed graph,
#in the JSON syntax internally used by the orchestrator itself
GRAPH_FILE = './node_resource_manager/virtualizer/.graph.json'

#File containing the new piece of graph to be deployed, in the JSON
#syntax internally used by the orchestrator
NEW_GRAPH_FILE = './node_resource_manager/virtualizer/.new_graph.json'
#File containing the IDs of the rules to be removed from the graph
REMOVE_GRAPH_FILE = './node_resource_manager/virtualizer/.remove_graph.json'

#File containing the ID of the next rule of the graph
RULE_ID_FILE = './node_resource_manager/virtualizer/.ruleid'

'''
	Information to be exported
'''
INFRASTRUCTURE_NAME = 'Single node'
INFRASTRUCTURE_ID = 'UUID001'
NODE_NAME = 'Universal Node'
NODE_ID = 'UUID11'


