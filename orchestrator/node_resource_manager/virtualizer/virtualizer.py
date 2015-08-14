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

import copy

#Set the logger
LOG = logging.getLogger(__name__)
LOG.setLevel(logging.DEBUG)	#Change here the logging level
LOG.propagate = False
sh = logging.StreamHandler()
sh.setLevel(logging.DEBUG)
f = logging.Formatter('[%(asctime)s] [Local-Orchestrator] [%(levelname)s] %(message)s')
sh.setFormatter(f)
LOG.addHandler(sh)

#XXX I don't know why, but I get a crash each time that I try to raise an exception.
#Then, instead of exceptions, I use this global variable
error = False

#############################################################
#	Functions related to the boot and stop of the virtualizer
#############################################################
	
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
		tmpFile = open(constants.CONFIGURATION_FILE, "w")
		tmpFile.write(v.xml())
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return 0
			
	''' Initizialize the file describing the deployed graph as a json'''
	rules = []
	vnfs = []
	return toBeAddedToFile(rules,vnfs,constants.GRAPH_FILE)
		
def terminate():
	'''
	Removes the tmp files used by the virtualizer to maintain the
	state of the node.
	'''
	
	LOG.debug("Terminating the virtualizer'...")
	try:
		os.remove(constants.CONFIGURATION_FILE)
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
	
	LOG.debug("Reading tmp file '%s'...",constants.CONFIGURATION_FILE)
	try:
		tree = ET.parse(constants.CONFIGURATION_FILE)
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
		tmpFile = open(constants.CONFIGURATION_FILE, "w")
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

	LOG.debug("Reading tmp file '%s'...",constants.CONFIGURATION_FILE)
	try:
		tree = ET.parse(constants.CONFIGURATION_FILE)
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
		tmpFile = open(constants.CONFIGURATION_FILE, "w")
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
	
	LOG.debug("Reading tmp file '%s'...",constants.CONFIGURATION_FILE)
	try:
		tree = ET.parse(constants.CONFIGURATION_FILE)
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
		tmpFile = open(constants.CONFIGURATION_FILE, "w")
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
		
	LOG.debug("Reading tmp file '%s'...",constants.CONFIGURATION_FILE)
	try:
		tree = ET.parse(constants.CONFIGURATION_FILE)
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
		tmpFile = open(constants.CONFIGURATION_FILE, "w")
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
			e = 'post http://hostip:tcpport/edit-config - send NF-FG request in the post body\n'
			f = '\n'
			g = 'limitations:\n'
			h = 'the flowrule ID must be unique on the node.\n'
			i = 'type cannot be repeated in multiple NF instances.\n'
			j = 'capabilities are not supported.\n'
			k = 'actions are not supported.\n'
			
			answer = a + b + c + d + e + f + g + h + i + j + j
			
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
	
	LOG.debug("Reading file: %s",constants.CONFIGURATION_FILE)
	try:
		tree = ET.parse(constants.CONFIGURATION_FILE)
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

	#TODO: check that flows refer to existing (i.e., deployed) network function.
	#TODO: check that flows refer to existing ports 
	
	if not isCorrect(content):
		return False		
	
	#
	#	Extract the needed information from the message received from the network
	#
	
	vnfsToBeAdded = extractVNFsInstantiated(content)	#VNF deployed/to be deployed on the universal node
	if error:
		return False
	
	rules = extractRules(content)						#Flowrules installed/to be installed on the universal node
	if error:
		return False
	
	vnfsToBeRemoved = extractToBeRemovedVNFs(content)	#VNFs to be removed from the universal node
	if error:
		return False

	rulesToBeRemoved = extractToBeRemovedRules(content) #Rules to be removed from the universal node
	if error:
		return False
	
	#Selects, among the rules listed in the received configuration, those that are not 
	#installed yet in the universal node
	rulesToBeAdded = diffRulesToBeAdded(rules)
	if error:
		return False
		
	#XXX The previous operation is not done for VNFs, since the C++ part supports such a case	
		
	#
	#	Prapare files used to provide, to the C++ part, the
	#	*	VNFs and rules to be added
	#	*	VNFs and rules to be removed
	#
	
	if not toBeAddedToFile(rulesToBeAdded,vnfsToBeAdded,constants.NEW_GRAPH_FILE):	#Save on a file the new rules and NFs to be installed in the universal node
		return False
		
	if not toBeRemovedToFile(rulesToBeRemoved,vnfsToBeRemoved): #Save on a file the IDs of the rules and the NFs to be removed from the universal node
		return False
	
	#		
	# 	From here, we may have inconsist state in the universal node in case of errors..		
	# 	In fact, the following functions writes data on files maintaining the current configuration of the universal node
	#
			
	if not addToGraphFile(rulesToBeAdded,vnfsToBeAdded): #Update the json representation of the deployed graph, by inserting the new VNFs/rules
		return inconsistentStateMessage()
	
	if not removeFromGraphFile(vnfsToBeRemoved,rulesToBeRemoved): #Update the json representation of the deployed graph, by inserting the new VNFs/rules
		return inconsistentStateMessage()

	#Updates the file containing the current configuration of the universal node, by editing the #<flowtable> and the <NF_instances>
	if not updateUniversalNodeConfig(content):
		return inconsistentStateMessage()
		
	LOG.debug("Configuration updated in the python part of the universal node)!")
		
	return True
	
def extractVNFsInstantiated(content):
	'''
	Parses the message and extracts the type of the deployed network functions.
	
	As far as I understand, the 'type' if a NF is the linker between <NF_instances>
	and <capabilities><supported_NFs>. Then, this function also checks that the type
	of the NF to be instantiated is among those to be supported by the universal node
	'''
	
	global error

	try:
		tree = ET.parse(constants.CONFIGURATION_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		error = True
		return
	
	tmpInfrastructure = Virtualizer.parse(root=tree.getroot())
	supportedNFs = tmpInfrastructure.nodes.node[constants.NODE_ID].capabilities.supported_NFs
	supportedTypes = []
	for nf in supportedNFs:
		nfType = nf.type.getValue()
		supportedTypes.append(nfType)
	
	LOG.debug("Extracting the network functions (to be) deployed on the universal node")

	try:
		tree = ET.ElementTree(ET.fromstring(content))
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		error = True
		return
	 
	infrastructure = Virtualizer.parse(root=tree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	instances = universal_node.NF_instances	
	
	foundTypes = []
	nfinstances = []
	
	LOG.debug("Considering instances:")
	LOG.debug("'%s'",infrastructure.xml())
	
	for instance in instances:
		vnf = {}
		if instance.operation == 'delete':
			#This network function has to be removed from the universal node
			continue
			
		vnfType = instance.type.getValue()
		if vnfType not in supportedTypes:
			LOG.warning("VNF of type '%s' is not supported by the UN!",vnfType)
			error = True
			return
		
		if vnfType in foundTypes:
			LOG.warning("Found multiple NF instances with the same type '%s'!",vnfType)
			LOG.warning("This is not supported by the universal node!")
			error = True
			return
			
		foundTypes.append(vnfType)
			
		vnf['id'] = vnfType
		nfinstances.append(vnf)
		LOG.debug("Required VNF: '%s'",instance.type.getValue())
		
	return nfinstances
	
def extractRulesToBeAdded(content):
	'''
	Parses the message and translates the flowrules in the internal JSON representation
	Returns a json representing the rules in the internal format of the universal node
	'''
	
	global error
	
	LOG.debug("Extracting the flowrules to be installed in the universal node")
	
	try:
		tree = ET.ElementTree(ET.fromstring(content))
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		error = True
		return
			
	infrastructure = Virtualizer.parse(root=tree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	flowtable = universal_node.flowtable
	
	rules = []
	for flowentry in flowtable:
	
		if flowentry.operation == 'delete':
			#This rule has to be removed from the universal node
			continue
	
		rule = {}
		
		f_id = flowentry.id.getValue()
		priority = flowentry.priority.getValue()	
		
		#Iterate on the match in order to translate it into the json syntax
		#supported internally by the universal node
		match = {}
		if flowentry.match is not None:
			if type(flowentry.match.data) is str:
				#FIXME: I guess this cannot happen! But never say never...
				#Then I let it here
				error = True
				return
			elif type(flowentry.match.data) is ET.Element:
				for node in flowentry.match.data:
					if not 	supportedMatch(node.tag):
						error = True
						return
					match[node.tag] = node.text
					
		#The content of <port> must be added to the match
		#XXX: the following code is quite dirty, but it is a consequence of the nffg library
		portPath = flowentry.port.getTarget().getPath()
		port = flowentry.port.getTarget()	
		tokens = portPath.split('/');
		if len(tokens) is not 5 and len(tokens) is not 7:
			LOG.error("Invalid port '%s' defined in a flowentry",portPath)
			error = True
			return
						
		if tokens[3] == 'ports':
			#This is a port of the universal node. We have to extract the virtualized port name
			match['port'] = port.name.getValue()			
		elif tokens[3] == 'NF_instances':
			#This is a port of the NF. I have to extract the port ID and the type of the NF.
			#XXX I'm using the port ID as name of the port
			vnf = port.getParent().getParent()
			vnfType = vnf.type.getValue()
			portID = port.id.getValue()
			match['VNF_id'] = vnfType + ":" + portID
		else:
			LOG.error("Invalid port '%s' defined in a flowentry",port)
			error = True
			return
    		
		action = {}
		#The universal node supports only the output actions! But this is 
		#expressed in the <out> element. Hence, the <action> element must
		#be empty!!
		if type(flowentry.action.data) is str or type(flowentry.action.data) is ET.Element:
			LOG.error("Invalid flowrule with action!",port)
			error = True
			return
			
		
		#The content of <out> must be added to the action
		#XXX: the following code is quite dirty, but it is a consequence of the nffg library
		portPath = flowentry.out.getTarget().getPath()
		port = flowentry.out.getTarget()	
		tokens = portPath.split('/');
		if len(tokens) is not 5 and len(tokens) is not 7:
			LOG.error("Invalid port '%s' defined in a flowentry",portPath)
			error = True
			return
						
		if tokens[3] == 'ports':
			#This is a port of the universal node. We have to extract the ID
			#Then, I have to retrieve the virtualized port name, and from there
			#the real name of the port on the universal node
			action['port'] = port.name.getValue()			
		elif tokens[3] == 'NF_instances':
			#This is a port of the NF. I have to extract the port ID and the type of the NF.
			#XXX I'm using the port ID as name of the port
			vnf = port.getParent().getParent()
			vnfType = vnf.type.getValue()
			portID = port.id.getValue()
			action['VNF_id'] = vnfType + ":" + portID
		else:
			LOG.error("Invalid port '%s' defined in a flowentry",port)
			error = True
			return

		#Prepare the rule
		rule['id'] = f_id
		if priority is not None:
			rule['priority'] = priority
		rule['match'] = match
		rule['action'] = action
				
		rules.append(rule)
	
	LOG.debug("Rules extracted:")
	LOG.debug(json.dumps(rules))
	
	return rules
	
def extractToBeRemovedRules(content):
	'''
	Parses the message and identifies those flowrules to be removed.
	
	The rules to be removed must be already instantiated on the universal node. The rule ID
	is used as a unique identifier for the rules.
	'''

	global error

	try:
		tree = ET.parse(constants.CONFIGURATION_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		error = True
		return
	
	tmpInfrastructure = Virtualizer.parse(root=tree.getroot())
	flowtable = tmpInfrastructure.nodes.node[constants.NODE_ID].flowtable
	rulesDeployed = []
	for flowrule in flowtable:
		fid = flowrule.id.getValue()
		rulesDeployed.append(fid)

	LOG.debug("Identifying the flowrules to be removed from the universal node")
	
	try:
		tree = ET.ElementTree(ET.fromstring(content))
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		error = True
		return
			
	infrastructure = Virtualizer.parse(root=tree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	flowtable = universal_node.flowtable
	
	ids = []
	for flowentry in flowtable:
		if flowentry.operation == 'delete':
			f_id = flowentry.id.getValue()
			if f_id not in rulesDeployed:
				LOG.warning("Rule with ID '%d' is not deployed in the UN!",int(f_id))
				LOG.warning("The rule cannot be removed!")
				error = True
				return
						
			LOG.debug("Rule with id %d has to be removed",int(f_id))
			ids.append(f_id)

	return ids
	
def	extractToBeRemovedVNFs(content):
	'''
	Parses the message and identifies those network functions to be removed
	
	The network functions to be removed must already be instantiated on the universal node. The
	type is used as a unique identifier for the network function.
	'''
	
	global error
	
	try:
		tree = ET.parse(constants.CONFIGURATION_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		error = True
		return
	
	tmpInfrastructure = Virtualizer.parse(root=tree.getroot())
	nf_instances = tmpInfrastructure.nodes.node[constants.NODE_ID].NF_instances
	
	vnfsDeployed = []
	for vnf in nf_instances:
		ftype = vnf.type.getValue()
		vnfsDeployed.append(ftype)
		
	LOG.debug("Identifying the network functions to be removed from the universal node")
		
	try:
		tree = ET.ElementTree(ET.fromstring(content))
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		error = True
		return
	 
	infrastructure = Virtualizer.parse(root=tree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	instances = universal_node.NF_instances	
	
	nfinstances = []
	for instance in instances:
		if instance.operation == 'delete':
			vnfType = instance.type.getValue()
			if vnfType not in vnfsDeployed:
				LOG.warning("Network function with type '%s' is not deployed in the UN!",vnfType)
				LOG.warning("The network function cannot be removed!")
				error = True
				return
			
			LOG.debug("Network function with type '%s' has to be removed",vnfType)
			nfinstances.append(vnfType)
	
	return nfinstances	
	
def supportedMatch(tag):
	'''
	Given an element within match, this function checks whether such an element is supported or node
	'''
	
	if tag in constants.supported_matches:
		LOG.debug("'%s' is supported!",tag)
		return True
	else:
		LOG.error("'%s' is not a supported match!",tag)
		return False
	
def toBeAddedToFile(flowRules,vnfs,fileName):
	'''
	Given a set (potentially empty) of flow rules and NFs, write it in a file respecting the syntax expected by the Univeral Node
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
	
def toBeRemovedToFile(rulesID,vnfsType):
	'''
	Given a list (potentially empty) of rule IDs and NFs, write them in a file 
	'''
	
	LOG.debug("Writing rules to be removed on file '%s'",constants.REMOVE_GRAPH_FILE)
	
	myjson = {}
	graph = {}
	
	rules = []
	vnfs = []
	
	for rule in rulesID:
		current = {}
		current['id'] = rule
		rules.append(current)
		
	for vnf in vnfsType:
		current = {}
		current['id'] = vnf
		vnfs.append(current)
	
	graph['VNFs'] = vnfs
	graph['flow-rules'] = rules
	myjson['flow-graph'] = graph	

	try:
		tmpFile = open(constants.REMOVE_GRAPH_FILE, "w")
		tmpFile.write(json.dumps(myjson))
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return False
		
	return True

def diffRulesToBeAdded(newRules):
	'''
	Read the graph currently deployed. It is stored in a tmp file, in a json format.
	Then, compare it with the new request, in order to identify the new rules to be
	deployed.
	
	This function is useless in case the config coming from the network is a diff wrt
	the current configuration of the universal node.
	However, I let it here just in case sometimes the configuration received is not
	a diff.
	'''
	
	#FIXME: why don't just compare the IDs?
	
	global error	
		
	LOG.debug("Compare the new rules received with those already deployed")
	
	try:
		LOG.debug("Reading file: %s",constants.GRAPH_FILE)
		tmpFile = open(constants.GRAPH_FILE,"r")
		json_file = tmpFile.read()
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		error = True
		return
	
	whole = json.loads(json_file)
	
	flowgraph = whole['flow-graph']
	flowrules = flowgraph['flow-rules']
	
	rulesToBeAdded = []
	
	for newRule in newRules:
		#For each new rule, compare it with the ones already part of the graph
		newMatch = newRule['match']
		newAction = newRule['action']
		newPriority = ""
		if "priority" in newRule.keys():
			newPriority = newRule['priority']
		newId = newRule['id']
		
		LOG.debug("New match: %s",json.dumps(newMatch))
		LOG.debug("New action: %s",json.dumps(newAction))
		
		equal = False
		for rule in flowrules:
			match = rule['match']
			action = rule['action']
			priority = ""
			if "priority" in newRule.keys():
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
			
	return rulesToBeAdded
	
def isCorrect(newContent):
	'''
	Check if the new configuration of the node (in particular, the flowtable) is correct:
	*	the ports are part of the universal  node
	*	the VNFs referenced in the flows are instantiated
	'''
	
	LOG.debug("Checking the correctness of the new configuration...")

	LOG.debug("Reading file '%s', which contains the current configuration of the universal node...",constants.CONFIGURATION_FILE)
	try:
		oldTree = ET.parse(constants.CONFIGURATION_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		return False
	LOG.debug("File correctly read")
		
	infrastructure = Virtualizer.parse(root=oldTree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	flowtable = universal_node.flowtable
	nfInstances = universal_node.NF_instances
	
	tmpInfra = copy.deepcopy(infrastructure)
	
	LOG.debug("Getting the new flowrules to be installed on the universal node")
	try:
		newTree = ET.ElementTree(ET.fromstring(newContent))
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		return False
			
	newInfrastructure = Virtualizer.parse(root=newTree.getroot())
	newFlowtable = newInfrastructure.nodes.node[constants.NODE_ID].flowtable
	newNfInstances = newInfrastructure.nodes.node[constants.NODE_ID].NF_instances
			
	#Update the NF instances with the new NFs
	for instance in newNfInstances:
		if instance.operation == 'delete':
			nfInstances.delete(instance)
		else:
			nfInstances.add(instance)
	
	#Update the flowtable with the new flowentries
	for flowentry in newFlowtable:
		if flowentry.operation == 'delete':
			flowtable.delete(flowentry)
		else:
			flowtable.add(flowentry)

	#Here, infrastructure contains the new configuration of the node
	#Then, we execute the checks on it!
	
	#TODO
		
	return True

def addToGraphFile(newRules,newVNFs):
	'''
	Read the graph currently deployed. It is stored in a tmp file, in a json format.
	Then, adds to it the new VNFs and the new flowrules to be instantiated.
	'''
	
	LOG.debug("Updating the json representation of the whole graph deployed")

	try:
		LOG.debug("Reading file: %s",constants.GRAPH_FILE)
		tmpFile = open(constants.GRAPH_FILE,"r")
		json_file = tmpFile.read()
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return False
	
	whole = json.loads(json_file)
	
	flowgraph = whole['flow-graph']
	flowrules = flowgraph['flow-rules']
	theVNFs = flowgraph['VNFs']	
			
	#Add the new flowrules
	for nr in newRules:
		flowrules.append(nr)
	
	#Add the new VNFs
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
		error = True
		return False
		
	return True
	
def removeFromGraphFile(vnfsToBeRemoved,rulesToBeRemoved):
	'''
	Read the graph currently deployed. It is stored in a tmp file, in a json format.
	Then, removes from it the VNFs and the flowrules to be removed
	'''
	
	LOG.debug("Removes VNFs and flowrules from the graph containing the json representation of the graph")
	
	try:
		LOG.debug("Reading file: %s",constants.GRAPH_FILE)
		tmpFile = open(constants.GRAPH_FILE,"r")
		json_file = tmpFile.read()
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return False
	
	whole = json.loads(json_file)
	
	flowgraph = whole['flow-graph']
	flowrules = flowgraph['flow-rules']
	theVNFs = flowgraph['VNFs']	
	
	newVNFs = []
	for vnf in theVNFs:
		if vnf['id'] not in vnfsToBeRemoved:
			newVNFs.append(vnf)
	
	flowgraph['VNFs'] = newVNFs
	
	newFlows = []
	for rule in flowrules:
		if rule['id'] not in rulesToBeRemoved:
			newFlows.append(rule)
			
	flowgraph['flow-rules'] = newFlows	
	
	LOG.debug("Updated graph:");	
	LOG.debug("%s",json.dumps(whole));
	
	try:
		tmpFile = open(constants.GRAPH_FILE, "w")
		tmpFile.write(json.dumps(whole))
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return False
		
	return True
			
	
def updateUniversalNodeConfig(newContent):
	'''
	Read the configuration of the universal node, and applies the required modifications to
	the NF instances and to the flowtable
	'''
	
	LOG.debug("Updating the file containing the configuration of the node...")
	
	LOG.debug("Reading file '%s', which contains the current configuration of the universal node...",constants.CONFIGURATION_FILE)
	try:
		oldTree = ET.parse(constants.CONFIGURATION_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		return False
	LOG.debug("File correctly read")
		
	infrastructure = Virtualizer.parse(root=oldTree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	flowtable = universal_node.flowtable
	nfInstances = universal_node.NF_instances
	
	tmpInfra = copy.deepcopy(infrastructure)
	
	LOG.debug("Getting the new flowrules to be installed on the universal node")
	try:
		newTree = ET.ElementTree(ET.fromstring(newContent))
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		return False
			
	newInfrastructure = Virtualizer.parse(root=newTree.getroot())
	newFlowtable = newInfrastructure.nodes.node[constants.NODE_ID].flowtable
	newNfInstances = newInfrastructure.nodes.node[constants.NODE_ID].NF_instances
			
	#Update the NF instances with the new NFs
	for instance in newNfInstances:
		if instance.operation == 'delete':
			nfInstances.delete(instance)
		else:
			nfInstances.add(instance)
	
	#Update the flowtable with the new flowentries
	for flowentry in newFlowtable:
		if flowentry.operation == 'delete':
			flowtable.delete(flowentry)
		else:
			flowtable.add(flowentry)
	#It is not necessary to remove conflicts, since they are already handled by the library,
	#i.e., it does not insert two identical rules
	
	try:
		tmpFile = open(constants.CONFIGURATION_FILE, "w")
		tmpFile.write(infrastructure.xml())
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return False
		
	return True
	
def inconsistentStateMessage():
	'''
	Simple function that just prints error messages
	'''

	Log.error("A fatal error occurred while handligng the configuration received")
	Log.warning("The internal status of the universal node could not be coherent. Please reboot the universal node.")
	return False
	
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


