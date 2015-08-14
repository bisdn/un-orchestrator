#!/usr/bin/env python
__author__ = 'Ivano Cerrato'

#Constants used by the parser
import constants

def handleSpecificAction(action,node):
	
	if action == "vlan":
		return handleVlanAction(node)
	
	#XXX add here code to handle further actions
	
	#Cannot be here
	
	return ""
	
def handleVlanAction(node):
	
	action = {}
	
	for operation in node:	
		operations = constants.supported_actions["vlan"]

		#TODO: check if operation is defined
	
		print operation.tag
		print operation.text
		
		action["operation"] = operation.tag
		if operation.tag == "push":
			action["vlan_id"] = operation.text
		
	return action
