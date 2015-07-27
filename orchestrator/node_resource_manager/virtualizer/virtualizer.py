#!/usr/bin/env python
__author__ = 'Ivano Cerrato'

#NF-FG library
from virtualizer3 import *
#Constants used by the parser
import constants

import json
import logging
import os

import xml.etree.ElementTree as ET
import re
import xml.dom.minidom


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
	
	v = Virtualizer(id=constants.INFRASTRUCTURE_ID, name=constants.INFRASTRUCTURE_NAME)
		
	v.nodes.add(
		Infra_node(
			id=constants.NODE_ID,
			name=constants.NODE_NAME,
			type=constants.NODE_TYPE,
			resources=NodeResources(
				cpu='0',
				mem='0',
				storage='0'
			)
		)
	)

	try:
		tmpFile = open(constants.TMP_FILE, "w")
		tmpFile.write(v.xml())
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return 0
			
	''' Initizialize the file describing the deployed graph as a json'''
	rules = []
	vnfs = []
	return flowRulesToFile(rules,vnfs,constants.GRAPH_FILE)
		
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
		
def addResources(cpu, memory, memory_unit, storage, storage_unit):
	'''
	Adds the description of the resources of the node to the 
	tmp file
	'''
	
	LOG.debug("Reading tmp file '%s'...",constants.TMP_FILE)
	try:
		tree = ET.parse(constants.TMP_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		return 0
	LOG.debug("File correctly read")
	
	infrastructure = Virtualizer.parse(root=tree.getroot())
	
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	
	resources = universal_node.resources
	resources.cpu.setValue(str(cpu) + " VCPU")
	resources.mem.setValue(str(memory) + " " + memory_unit)
	resources.storage.setValue(str(storage) + " " + storage_unit)
	
	try:
		tmpFile = open(constants.TMP_FILE, "w")
		tmpFile.write(infrastructure.xml())
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
	
	#TODO: support the port-abstract
	#TODO: The port-abstract should have the capabilities (that are optional)
	#FIXME: Are the current info of port-sap correct?

	LOG.debug("Reading tmp file '%s'...",constants.TMP_FILE)
	try:
		tree = ET.parse(constants.TMP_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		return 0
	LOG.debug("File correctly read")

	LOG.debug("Adding port '%s'",name)
	
	infrastructure = Virtualizer.parse(root=tree.getroot())
		
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	
	port = Port(id=name, name=name, port_type=ptype)
	universal_node.ports.add(port)

	if(ptype == 'port-abstract'):
		LOG.error("port-sap is not supported!")
		return 0
	
	try:
		tmpFile = open(constants.TMP_FILE, "w")
		tmpFile.write(infrastructure.xml())
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
	
	LOG.debug("Reading tmp file '%s'...",constants.TMP_FILE)
	try:
		tree = ET.parse(constants.TMP_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		return 0
	LOG.debug("File correctly read")
	
	LOG.debug("I'm going to set the ID of port '%s' to '%d'",portName,portID)
	
	infrastructure = Virtualizer.parse(root=tree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	ports = universal_node.ports
			
	for port in ports:
		LOG.debug("Considering port: '%s'",port.name.getValue())
		if port.name.getValue() == portName:
			port.id.setValue(str(portID))
			found = True
				
	if not found:
		'''There has been some internal error'''
		LOG.debug("Port %s not found!",portName)
		return 0
	
	try:
		tmpFile = open(constants.TMP_FILE, "w")
		tmpFile.write(infrastructure.xml())
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
		tree = ET.parse(constants.TMP_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		return 0
	LOG.debug("File correctly read")
	
	LOG.debug("Inserting VNF %s, ID %s, type %s, num ports %d...",ID,name,vnftype,numports)
	
	infrastructure = Virtualizer.parse(root=tree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	capabilities = universal_node.capabilities
	supportedNF = capabilities.supported_NFs
	
	vnf = Infra_node(id=ID,name=name,type=vnftype)
	
	i = 1
	for x in range(0, numports):
		port = Port(id=str(i), name='VNF port ' + str(i), port_type='port-abstract')
		vnf.ports.add(port)
		i = i+1
	
	supportedNF.add(vnf)
	
	try:
		tmpFile = open(constants.TMP_FILE, "w")
		tmpFile.write(infrastructure.xml())
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
			return get_config()
			
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

def get_config():
	'''
	Return the current configuration of the node.
	'''
	
	LOG.debug("Executing the get-config command")
	
	LOG.debug("Reading file: %s",constants.TMP_FILE)
	try:
		tree = ET.parse(constants.TMP_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		return 0
	LOG.debug("File correctly read")
	
	infrastructure = Virtualizer.parse(root=tree.getroot())
	
	LOG.debug("%s",infrastructure.xml())
	return infrastructure.xml()

def edit_config(content):

	'''
	Executes the command 'edit_config'
	Content is the body of the edit_config request
	'''

	LOG.debug("Executing the edit-config command")
	
	#All'update bisogna dare solo le regole nuove.. Altrimenti fallisce! :S
	#Mi tengo il grafo sotto forma di json in un file (sono obbligato ad averlo in un file, siccome non si mantiene stato tra diverse chiamate
	#C-python. 
	#Quando arriva una richiesta, mi creo il nuovo grafo e mi carico quelli sul file.. Li confronto, e vedo quali sono le regole nuove. Ritorno le regole nuove al C,
	#ed aggiorno il json da mettere sul file
	
	try:
		#Extract the needed information from the message received from the network
		vnfs = extractVNFsInstantiated(content)	#VNF deployed on the universal node
		theJson = extractRules(content)			#Flowrules installed on the universal ndoe
		
		#Extract the rules to be removed from the universal node
		rulesToBeRemoved = extractToBeRemovedRules(content)
		#Selects, among the rules listed in the received configuration, those that are not 
		#installed yet in the universal node
		rulesToBeAdded = readGraphFromFileAndCompare(theJson,vnfs)

		#FIXME: provide only the VNFs to be actually added, not all the ones described in the received configuration
		#Prepare a file containing, in the json internal format, the new rules that have to be 
		#installed in the universal node
		flowRulesToFile(rulesToBeAdded,vnfs,constants.NEW_GRAPH_FILE)
		#Prepare a file containing a list of IDs of the rules to be removed from the universal node
		toBeRemovedToFile(rulesToBeRemoved)
		
		#Updates the file containing the current configuration of the universal node, by editing the
		#<flowtable> and the <NF_instances>
		updateUniversalNodeConfig(content)
	except:
		Log.error("A fatal error occurred while handligng the configuration received")
		Log.warning("The internal status of the universal node could not be coherent. Please reboot the universal node.")
		return False
		
	LOG.debug("Configuration updated!")
		
	return True
	
def extractVNFsInstantiated(content):
	'''
	Parses the message and extracts the type of the deployed network functions.
	'''
	
	LOG.debug("Extracting the network functions (to be) deployed on the universal node")

	try:
		tree = ET.ElementTree(ET.fromstring(content))
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		raise VirtualizerError
	 
	infrastructure = Virtualizer.parse(root=tree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	instances = universal_node.NF_instances	
	
	nfinstances = []
	for instance in instances:
		vnf = {}
		vnf['id'] = instance.type.getValue()
		nfinstances.append(vnf)
		LOG.debug("Required VNF: '%s'",instance.type.getValue())
		
	#TODO: check if the required VNFs are supported by the node!!!!!
		
	return nfinstances
	
def extractRules(content):
	'''
	Parses the message and translates the flowrules in the internal JSON representation
	Returns a json representing the rules in the internal format of the universal node
	'''
	
	LOG.debug("Extracting the flowrules to be installed in the universal node")
	
	try:
		tree = ET.ElementTree(ET.fromstring(content))
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		raise VirtualizerError
			
	infrastructure = Virtualizer.parse(root=tree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	flowtable = universal_node.flowtable
	
	rules = []
	for flowentry in flowtable:
		rule = {}
		
		f_id = flowentry.id.getValue()
		priority = flowentry.priority.getValue()
	
		
		
		#out = flowentry.out.getTarget()
	
#		print port
#		print out	
		
		
		
		#Iterate on the match in order to translate it into the json syntax
		#supported internally by the universal node
		match = {}
		if flowentry.match is not None:
			if type(flowentry.match.data) is str:
				#FIXME: I guess this cannot happen! But never say never...
				#Then I let it here
				#print "match: " + flowentry.match
				raise VirtualizerError
			elif type(flowentry.match.data) is ET.Element:
				for node in flowentry.match.data:
					if not 	supportedMatch(node.tag):
						raise VirtualizerError
					match[node.tag] = node.text
					
		#TODO: Add the port to the match!
		port = flowentry.port.getTarget()
		print flowentry.port.getValue()
		print port
		
		action = {}
		#The universal node supports only the output actions! But this is 
		#expressed in the <out> element. Hence, the <action> element must
		#be empty!!
		if type(flowentry.action.data) is str or type(flowentry.action.data) is ET.Element:
			raise VirtualizerError
		
		#TODO: Add the port to the action!

		#Prepare the rule
		rule['id'] = f_id
		rule['priority'] = priority
		rule['match'] = match
		rule['action'] = action
				
		rules.append(rule)
#		
		
#		try:
#			tree = ET.ElementTree(ET.fromstring(mat))
#	except ET.ParseError as e:
#		print('ParseError: %s' % e.message)
#		return 0
		
#		stringmatch = match.getAsText()
#		print "---- " + stringmatch
#		if stringmatch != "None" :
#			print "THERE IS MATCH"
#		LOG.debug("++ %s",match.getAsText())
		
#		try:
#			tree = ET.ElementTree(ET.fromstring(match.getAsText()))
#		except ET.ParseError as e:
#			print('ParseError: %s' % e.message)
#			return 0
			
#		LOG.debug("--------------")
		
#		print "Port: " + port.getValue
#		LOG.debug("++++++++++++++++")
#		print "Match: " + match.getValue()
#		print "Action: " + action.getAsText()
#		print "Out: " + out.getValue
		
#		newRule = []
	
	LOG.debug("Rules extracted:")
	LOG.debug(json.dumps(rules))
	
	return rules
	
def extractToBeRemovedRules(content):
	'''
	Parses the message and identifies those flowrules to be removed
	'''

	LOG.debug("Identifying the flowrules to be removed from the universal node")
	
	try:
		tree = ET.ElementTree(ET.fromstring(content))
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		raise VirtualizerError
			
	infrastructure = Virtualizer.parse(root=tree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	flowtable = universal_node.flowtable
	
	ids = []
	for flowentry in flowtable:
		#TODO: wating for an updated version of the library to extract the operation 
		#associated with a flowentry
		f_id = flowentry.id.getValue()		
		ids.append(f_id)

	return ids
	
def supportedMatch(tag):
	'''
	Given an element within match, this function checks whether such an element is supported or node
	'''
	
	if tag in constants.supported_matches:
		LOG.debug("'%s' is supported!",tag)
		return True
	else:
		LOG.debug("'%s' is not supported!",tag)
		return False
	
def flowRulesToFile(flowRules,vnfs,fileName):
	'''
	Given a set (potentially empty) of flow rules, write it in a file respecting the syntax expected by the Univeral Node
	'''
	
	LOG.debug("Writing rules on file '%s'",fileName)
	
	myjson = {}
	graph = {}
		
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
	
def toBeRemovedToFile(rulesID):
	'''
	Given a list (potentially empty) of rule IDs, write them in a file 
	'''
	
	LOG.debug("Writing rules to be removed on file '%s'",constants.REMOVE_GRAPH_FILE)
	
	myjson = {}
	remove = {}
	vnfs = []
	
	for rule in rulesID:
		current = {}
		current['id'] = rule
		#vnfs.append(current)
		
	myjson['remove'] = vnfs
	
	try:
		tmpFile = open(constants.REMOVE_GRAPH_FILE, "w")
		tmpFile.write(json.dumps(myjson))
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return False
		
	return True
	
def readGraphFromFileAndCompare(newRules,newVNFs):
	'''
	Read the graph currently deployed. It is stored in a tmp file, in a json format.
	Then, compare it with the new request, in order to identify the new rules to be
	deployed.
	
	This function is useless in case the config coming from the network is a diff wrt
	the current configuration of the universal node.
	However, I let it here just in case sometimes the configuration received is not
	a diff.
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
	theVNFs = flowgraph['VNFs']	
	
	rulesToBeAdded = []
	
	for newRule in newRules:
		#For each new rule, compare it with the ones already part of the graph
		newMatch = newRule['match']
		newAction = newRule['action']
		newPriority = newRule['priority']
		newId = newRule['id']
		
		LOG.debug("New match: %s",json.dumps(newMatch))
		LOG.debug("New action: %s",json.dumps(newAction))
		
		equal = False
		for rule in flowrules:
			match = rule['match']
			action = rule['action']
			priority = rule['priority']
			theId = rule['id']
			
			if match == newMatch and action == newAction and priority == newPriority and theId == newId:
				equal = True
				break
		
		if not equal:
			#The new rule is not yet part of the graph
			LOG.debug("The rule must be inserted!")
			LOG.debug("%s",json.dumps(newRule))
			rulesToBeAdded.append(newRule)
	
	#Update the current graph deployed with...
	
	#... the new flowrules
	for tba in rulesToBeAdded:
		flowrules.append(tba)
	
	#... and the new VNFs
	for vnf in newVNFs:
		LOG.debug("New VNF: %s!",vnf)
		if vnf not in theVNFs:
			LOG.debug("The VNF must be inserted!")
			theVNFs.append(vnf)
	
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
	
def updateUniversalNodeConfig(newContent):

	LOG.debug("Reading tmp file '%s', which contains the current configuration of the universal node...",constants.TMP_FILE)
	try:
		oldTree = ET.parse(constants.TMP_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		return 0
	LOG.debug("File correctly read")
	
	infrastructure = Virtualizer.parse(root=oldTree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	flowtable = universal_node.flowtable
	nfInstances = universal_node.NF_instances
	
	LOG.debug("Getting the new flowrules to be installed on the universal node")
	try:
		newTree = ET.ElementTree(ET.fromstring(newContent))
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		raise VirtualizerError
			
	newInfrastructure = Virtualizer.parse(root=newTree.getroot())
	newFlowtable = newInfrastructure.nodes.node[constants.NODE_ID].flowtable
	newNfInstances = newInfrastructure.nodes.node[constants.NODE_ID].NF_instances
	
	#Update the NF instances
	for instance in newNfInstances:
		nfInstances.add(instance)
	
	#Update the flowtable
	for flowentry in newFlowtable:
		flowtable.add(flowentry)
	#It is not necessary to remove conflicts, since they are already handled by the library,
	#i.e., it does not insert two identical rules
	
	try:
		tmpFile = open(constants.TMP_FILE, "w")
		tmpFile.write(infrastructure.xml())
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		raise VirtualizerError
	
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


