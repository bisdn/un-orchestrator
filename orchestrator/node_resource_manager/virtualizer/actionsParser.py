#!/usr/bin/env python
__author__ = 'Ivano Cerrato'

#Constants used by the parser
import constants

def handleSpecificAction(action,node):
	'''
	Given an action, invokes the proper handler for such an action
	'''	
	
	if action == "vlan":
		return handleVlanAction(node)
	
	#XXX add here code to handle further actions
	
	#Cannot be here
	
	return ""
	
def handleVlanAction(node):
	'''
	Parses the content of an action and translates it in the internal JSON representation.
	Such a representation is then returned.
	'''
	
	action = {}
	
	for operation in node:	
		operations = constants.supported_actions["vlan"]

		#TODO: check if operation is defined
		
		action["operation"] = operation.tag
		if operation.tag == "push":
			action["vlan_id"] = operation.text
		
	return action
