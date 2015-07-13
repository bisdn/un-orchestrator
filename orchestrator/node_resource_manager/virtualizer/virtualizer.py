#!/usr/bin/env python
__author__ = 'Ivano Cerrato'

#NF-FG library
import nffglib
#Constants used by the parser
import constants

import json
import logging
import os

#Set the logger
LOG = logging.getLogger(__name__)
LOG.setLevel(logging.DEBUG)	#Change here the logging level
LOG.propagate = False
sh = logging.StreamHandler()
sh.setLevel(logging.DEBUG)
f = logging.Formatter('[%(asctime)s] [Local-Orchestrator] [%(levelname)s] %(message)s')
sh.setFormatter(f)
LOG.addHandler(sh)

#############################################################
#	Functions related to the boot and stop of the virtualizer
#############################################################

class VirtualizerError(Exception):
   """Virtualizer Error!"""
   pass
	
def init():
	
	'''
	The virtualizer maintains the state of the node in a tmp file.
	This function initializes such a file.
	'''
	
	LOG.debug("Initializing the virtualizer...")

	t = """<?xml version="1.0" ?>
	<virtualizer>
		<id>""" + constants.INFRASTRUCTURE_ID + """</id>
		<name>""" + constants.INFRASTRUCTURE_NAME + """</name>
		<nodes>
			<node>
				<id>""" +  constants.NODE_ID + """</id>
				<name>""" + constants.NODE_NAME + """</name>
				<type>BisBis</type>
				<ports>
				</ports>
				<resources>
					<cpu>0</cpu>
					<mem>0</mem>
					<storage>0</storage>
				</resources>
				<capabilities>
	                <supported_NFs>
	               </supported_NFs>
	            </capabilities>
			</node>
		</nodes>
	</virtualizer>"""

	try:
		tmpFile = open(constants.TMP_FILE, "w")
		tmpFile.write(t)
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return 0
		
	''' Initizialize the file with the next ID to be assigned to the rule 
	received from the network'''
	try:
		tmpFile = open(constants.RULE_ID_FILE, "w")
		tmpFile.write(str(0))
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return 0	
		
	''' Initizialize the file describing the deployed graph as a json'''
	rules = []
	return flowRulesToFile(rules,constants.GRAPH_FILE)
		
def terminate():
	'''
	Removes the tmp files used by the virtualizer to maintain the
	state of the node.
	'''
	
	LOG.debug("Terminating the virtualizer'...")
	try:
		os.remove(constants.TMP_FILE)
	except:
		pass
		
	try:
		os.remove(constants.GRAPH_FILE)
	except:
		pass

	try:
		os.remove(constants.NEW_GRAPH_FILE)
	except:
		pass
	
	try:
		os.remove(constants.REMOVE_GRAPH_FILE)
	except:
		pass
		
	try:
		os.remove(constants.RULE_ID_FILE)
	except:
		pass

def addResources(cpu, memory, memory_unit, storage, storage_unit):
	'''
	Adds the description of the resources of the node to the 
	tmp file
	'''
	
	LOG.debug("Reading tmp file '%s'...",constants.TMP_FILE)
	try:
		tmpFile = open(constants.TMP_FILE,"r")
		infrastructure_xml = tmpFile.read()
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return 0
	LOG.debug("File correctly read")
	
	infrastructure = nffglib.Virtualizer.parse(text=infrastructure_xml)
	universal_node = infrastructure.c_nodes.list_node[0]
	
	resources = universal_node.g_node.c_resources.g_softwareResource
	
	resources.l_cpu = str(cpu) + " VCPU"
	resources.l_mem = str(memory) + " " + memory_unit
	resources.l_storage = str(storage) + " " + storage_unit
	
	new_infrastructure_xml = infrastructure.xml()
	
	try:
		tmpFile = open(constants.TMP_FILE, "w")
		tmpFile.write(new_infrastructure_xml)
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return 0
		
	LOG.debug("Resources added")

	return 1
	
def addNodePort(name, ptype):
	'''
	Adds the description of a port of the node
	'''
	
	#TODO: support the port-sap
	#TODO: The port-abstract should have the capabilities (that are optional)

	LOG.debug("Reading tmp file '%s'...",constants.TMP_FILE)
	try:
		tmpFile = open(constants.TMP_FILE,"r")
		infrastructure_xml = tmpFile.read()
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return 0
	LOG.debug("File correctly read")
	
	infrastructure = nffglib.Virtualizer.parse(text=infrastructure_xml)
	
	universal_node = infrastructure.c_nodes.list_node[0]
	
	ports = universal_node.g_node.c_ports
	
	port = nffglib.PortGroup(ports)
	ports.list_port.append(port)
	
	portid = nffglib.IdNameGroup(port)
	port.g_idName = portid
    
	porttype = nffglib.PortTypeGroup(port)
	port.g_portType = porttype
	
	portid.l_id = str(0)
	portid.l_name = name
	if(ptype == 'port-sap'):
		LOG.error("port-sap is not supported!")
		return 0
	
	porttype.l_portType = ptype

	
	new_infrastructure_xml = infrastructure.xml()
	
	try:
		tmpFile = open(constants.TMP_FILE, "w")
		tmpFile.write(new_infrastructure_xml)
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return 0

	LOG.debug("Port added")

	return 1
	
def editPortID(portName, portID):
	'''
	Set portID as ID of the port named portName
	'''
	
	found = False
	
	LOG.debug("Reading tmp file: %s",constants.TMP_FILE)
	try:
		tmpFile = open(constants.TMP_FILE,"r")
		infrastructure_xml = tmpFile.read()
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return 0
	LOG.debug("File correctly read")
		
	LOG.debug("I'm going to set the ID of port %s to %d",portName,portID)
	infrastructure = nffglib.Virtualizer.parse(text=infrastructure_xml)
	universal_node = infrastructure.c_nodes.list_node[0]
	ports = universal_node.g_node.c_ports.list_port;
	for port in ports:
		if port.g_idName.l_name == portName:
			port.g_idName.l_id = str(portID)
			found = True		
			
	if not found:
		'''There has been some internal error'''
		LOG.debug("Port %s not found!",portName)
		return 0
	
	new_infrastructure_xml = infrastructure.xml()
	
	try:
		tmpFile = open(constants.TMP_FILE, "w")
		tmpFile.write(new_infrastructure_xml)
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return 0
		
	LOG.debug("Port ID changed")

	return 1

def addSupportedVNFs(ID, name, vnftype, numports):
	'''
	Adds the description of a VNF that can be deployed on the node
	'''
		
	LOG.debug("Reading tmp file '%s'...",constants.TMP_FILE)
	try:
		tmpFile = open(constants.TMP_FILE,"r")
		infrastructure_xml = tmpFile.read()
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return 0
	LOG.debug("File correctly read")
	
	LOG.debug("Inserting VNF %s, ID %s, type %s, num ports %d...",ID,name,vnftype,numports)
	
	infrastructure = nffglib.Virtualizer.parse(text=infrastructure_xml)
	universal_node = infrastructure.c_nodes.list_node[0]

	capabilities = universal_node.c_capabilities.g_capabilities
	supportedVNFs = capabilities.c_supportedNFs
	
	#Create here the VNF
	nf = nffglib.NodeGroup(supportedVNFs)
	supportedVNFs.list_node.append(nf)

	nfidt = nffglib.IdNameTypeGroup(nf)
	nf.g_idNameType = nfidt

	nfid = nffglib.IdNameGroup(nfidt)
	nfidt.g_idName = nfid

	nfports = nffglib.Ports(nf)
	nf.c_ports = nfports

	i = 1
	for x in range(0, numports):
		nfport = nffglib.PortGroup(nfports)
		nfports.list_port.append(nfport)
		
		nfportid = nffglib.IdNameGroup(nfport)
		nfport.g_idName = nfportid

		nfporttype = nffglib.PortTypeGroup(nfport)
		nfport.g_portType = nfporttype
		
		nfportid.l_id = str(i)
		
		#FIXME: what is the name?
		nfportid.l_name = 'VNF port ' + str(i)
		
		#FIXME: is it correct to set the port type always to port-abstract?
		nfporttype.l_portType = "port-abstract"
		
		i = i+1

	nfid.l_id = ID
	nfid.l_name = name
	nfidt.l_type = str(vnftype)
		
	new_infrastructure_xml = infrastructure.xml()
	
	try:
		tmpFile = open(constants.TMP_FILE, "w")
		tmpFile.write(new_infrastructure_xml)
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return 0

	LOG.debug("VNF added")
	
	return 1

#############################################################################################
#	Functions to handle commands coming from the network and to the manupulation of the graph
#############################################################################################
	
def handle_request(method, url, content = None):
	'''
	Handles HTTP commands coming from the network
	'''

	#TODO: identify some return values to identify the errors

	LOG.debug("Method: %s",method)
	LOG.debug("Url: %s",url)
	
	if not content:
		LOG.debug("This message has empty body!")
	else:
		LOG.debug("Content: %s",content)

	status = 1

	if method == 'GET':
		if url == '/ping':
			return 'OK'
		else:
			a = 'usage:\n'
			b = 'get http://hostip:tcpport - this help message\n'
			c = 'get http://hostip:tcpport/ping - test webserver aliveness\n'
			d = 'post http://hostip:tcpport/get-config - query NF-FG\n'
			e = 'post http://hostip:tcpport/edit-config - send NF-FG request in the post body'
			
			answer = a + b + c + d + e
			
			LOG.debug("Returning: ")
			LOG.debug("%s",answer)
			
			return answer
			
	elif method == 'POST':
		if url == '/ping':
			return 'OK'
			
		elif url == '/get-config':
			return get_config(constants.TMP_FILE)
			
		elif url == '/edit-config':
			if not edit_config(content):
				return 'ERROR'
			return 'config updated'
		else:
			LOG.error("Resource '%s' does not exist", url)
			return 'ERROR'
	else:
		LOG.error("Method '%s' not implemented", method)
		return 'ERROR'

def get_config(fileName):
	'''
	Return the current configuration of the node
	'''
	
	LOG.debug("Executing the get-config command")
	
	try:
		LOG.debug("Reading file: %s",fileName)
		config = nffglib.Virtualizer.parse(file=fileName)
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return 0
		
	LOG.debug("Parsing the file content")
	config_xml = config.xml()
	LOG.debug("File correctly managed!")
	LOG.debug("%s",config_xml)
	return config_xml

def edit_config(content):

	LOG.debug("Executing the edit-config command")

	#TODO
	
	#All'update bisogna dare solo le regole nuove.. Altrimenti fallisce! :S
	#Mi tengo il grafo sotto forma di json in un file (sono obbligato ad averlo in un file, siccome non si mantiene stato tra diverse chiamate
	#C-python. 
	#Quando arriva una richiesta, mi creo il nuovo grafo e mi carico quelli sul file.. Li confronto, e vedo quali sono le regole nuove. Ritorno le regole nuove al C,
	#ed aggiorno il json da mettere sul file
		
	theJson = extractNewRules(content)
	#TODO: fill the json starting from the complex content. Use the nffg library to do this!
	
	rulesToBeRemoved = extractToBeRemovedRules(content)
	
	try:
		rulesToBeAdded = readGraphFromFileAndCompare(theJson)
		flowRulesToFile(rulesToBeAdded,constants.NEW_GRAPH_FILE)
		toBeRemovedToFile(rulesToBeRemoved.constants.REMOVE_GRAPH_FILE)
	except:
		return False
		
	LOG.debug("Configuration updated!")
		
	return True
	
def extractNewRules(content):
	'''
	Parses the message and translates the flowrules in the internal JSON representation
	'''
	#TODO: wait for some advanced parser to be added in the library, in order to properly parse the match and the action
	
	#TODO: I think that the translation of the required VNFs should be done here
	
	#TODO: for each rule, this function has to insert an ID. The next ID is stored in constants.RULE_ID_FILE 
	
	
	whole = json.loads(content)
	flowgraph = whole['flow-graph']
	flowrules = flowgraph['flow-rules']
	
	return flowrules
	
def flowRulesToFile(flowRules,fileName):
	'''
	Given a set (potentially empty) of flow rules, write it in a file respecting the syntax expected by the Univeral Node
	'''
	
	LOG.debug("Writing rules on file '%s'",fileName)
	
	myjson = {}
	graph = {}
	vnfs = []
	
	graph['VNFs'] = vnfs
	graph['flow-rules'] = flowRules
	myjson['flow-graph'] = graph
	
	try:
		tmpFile = open(fileName, "w")
		tmpFile.write(json.dumps(myjson))
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return False
		
	return True
	
def toBeRemovedToFile(rulesID,filename):
	'''
	Given a list (potentially empty) of rule IDs, write them in a file 
	'''
	
	myjson = {}
	remove = {}
	vnfs = []
	
	for rule in rulesID:
		current = {}
		current['id'] = rule
		vnfs.append(current)
		
	myjson['remove'] = vnfs
	
	
	LOG.debug(json.dumps(myjson))
	
def readGraphFromFileAndCompare(newRules):
	'''
	Read the graph currently deployed. It is stored in a tmp file, in a json format.
	Then, compare it with the new request, in order to identify the new rules to be
	deployed.
	'''
	
	LOG.debug("Compare the new rules received with those already deployed")
	
	try:
		LOG.debug("Reading file: %s",constants.GRAPH_FILE)
		tmpFile = open(constants.GRAPH_FILE,"r")
		json_file = tmpFile.read()
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		raise VirtualizerError
	
	whole = json.loads(json_file)
	
	
	flowgraph = whole['flow-graph']
	flowrules = flowgraph['flow-rules']
	
	rulesToBeAdded = []
	
	for newRule in newRules:
		#For each new rule, compare it with the ones already part of the graph
		newMatch = newRule['match']
		newAction = newRule['action']
		
		LOG.debug("New match: %s",json.dumps(newMatch))
		LOG.debug("New action: %s",json.dumps(newAction))
		
		equal = False
		for rule in flowrules:
			match = rule['match']
			action = rule['action']
			
			if match == newMatch and action == newAction:
				equal = True
				break
		
		if not equal:
			#The new rule is not yet part of the graph
			LOG.debug("The rule must be inserted!")
			LOG.debug("%s",json.dumps(newRule))
			rulesToBeAdded.append(newRule)
	
	#Update the current graph deployed		
	for tba in rulesToBeAdded:
		flowrules.append(tba)
	
	LOG.debug("Updated graph:");	
	LOG.debug("%s",json.dumps(whole));
	
	try:
		tmpFile = open(constants.GRAPH_FILE, "w")
		tmpFile.write(json.dumps(whole))
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		raise VirtualizerError
	
	return rulesToBeAdded
	
################################

def main():
	'''
	Only used for debug purposes
	'''

	ids = []
	ids.append(1)
	ids.append(2)
	ids.append(3)
	ids.append(4)

	toBeRemovedToFile(ids,"sara")

#	tmpFile = open("debug.json","r")
#	json_file = tmpFile.read()
#	tmpFile.close()
	
#	whole = json.loads(json_file)
	
	
#	flowgraph = whole['flow-graph']
#	flowrules = flowgraph['flow-rules']

#	LOG.debug("%s",json.dumps(flowrules));
		
#	edit_config(flowrules)
	

#
#	LOG.info("Initializing the virtualizer...")
#	init()
#	
#	LOG.info("Adding resources...")
#	addResources(10,31,"GB",5,"TB")
#
#	LOG.info("Adding a port...")
#	addNodePort("OVS-north external port","port-abstract")
#	LOG.info("Adding a port...")
#	addNodePort("OVS-south external port","port-abstract")
#
#	LOG.info("Adding a VNF...")	
#	addSupportedVNFs("A", "myVNF", 0, 2)	
#	LOG.info("Adding a VNF...")	
#	addSupportedVNFs("B", "myVNF", 0, 3)
#
#	print "Terminating the virtualizer..."
#	terminate()
	
	LOG.info("Bye :D")
	
	
if __name__ == '__main__':
	'''
	Only used for debug purposes
	'''
	main()


